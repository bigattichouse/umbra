/**
 * @file update_executor.c
 * @brief Executes UPDATE operations
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>  // Add this for va_start, va_end
#include <errno.h>
#include <dirent.h>  // Add this for DIR, opendir, readdir, closedir
#include <time.h>    // Add this for time() function
#include "update_executor.h"
#include "../schema/metadata.h"
#include "../schema/type_system.h"
#include "../loader/page_manager.h"
#include "../loader/record_access.h"
#include "../kernel/kernel_generator.h"
#include "../kernel/kernel_compiler.h"
#include "../kernel/kernel_loader.h"
#include "../pages/page_generator.h"
#include "../pages/page_splitter.h"
#include "../query/query_executor.h" 
#include "../util/debug.h"

/**
 * @brief Create update result
 */
static UpdateResult* create_update_result(void) {
    UpdateResult* result = malloc(sizeof(UpdateResult));
    result->rows_affected = 0;
    result->success = false;
    result->error_message = NULL;
    return result;
}

/**
 * @brief Set error in result
 */
static void set_update_error(UpdateResult* result, const char* format, ...) {
    result->success = false;
    
    va_list args;
    va_start(args, format);
    
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    
    result->error_message = strdup(buffer);
    
    va_end(args);
}

/**
 * @brief Generate update kernel
 */
static GeneratedKernel* generate_update_kernel(const UpdateStatement* stmt, 
                                              const TableSchema* schema,
                                              const char* base_dir) {
    // Create a synthetic SELECT statement to use existing kernel generator
    SelectStatement select_stmt;
    select_stmt.from_table = malloc(sizeof(TableRef));
    select_stmt.from_table->table_name = strdup(stmt->table_name);
    select_stmt.from_table->alias = NULL;
    
    // Use WHERE clause from UPDATE
    select_stmt.where_clause = stmt->where_clause;
    
    // Set up as SELECT * for full row access
    select_stmt.select_list.has_star = true;
    select_stmt.select_list.expressions = NULL;
    select_stmt.select_list.count = 0;
    
    // Generate kernel
    GeneratedKernel* kernel = generate_select_kernel(&select_stmt, schema, base_dir);
    
    // Clean up
    free(select_stmt.from_table->table_name);
    free(select_stmt.from_table);
    
    return kernel;
}

/**
 * @brief Update a single record
 */
static int update_record(void* /*record*/, const UpdateStatement* stmt, 
                        const TableSchema* schema, int /*record_index*/) {
    #ifdef DEBUG
    DEBUG("Updating record at index %d, address %p\n",record_index, record);

    #endif
    
    // Apply SET clauses to the record
    for (int i = 0; i < stmt->set_count; i++) {
        const SetClause* set_clause = &stmt->set_clauses[i];
        
        // Find column index
        int col_idx = -1;
        for (int j = 0; j < schema->column_count; j++) {
            if (strcmp(set_clause->column_name, schema->columns[j].name) == 0) {
                col_idx = j;
                break;
            }
        }
        
        if (col_idx < 0) {
            #ifdef DEBUG
            DEBUG("Column '%s' not found in schema\n",                     set_clause->column_name);

            #endif
            continue;
        }
        
        #ifdef DEBUG
        DEBUG("Updating column %s (index %d) type=%d\n", 
                set_clause->column_name, col_idx, schema->columns[col_idx].type);
        #endif
        
        // For now, just log that we'd update this field but don't actually do it
        // This will help us diagnose where the crash is occurring
        if (set_clause->value->type == AST_LITERAL) {
            const Literal* lit = &set_clause->value->data.literal;
            
            #ifdef DEBUG
            DEBUG("Would update %s to ", set_clause->column_name);
            
            switch (schema->columns[col_idx].type) {
                case TYPE_INT:
                    fprintf(stderr, "int value %d\n", lit->value.int_value);
                    break;
                case TYPE_FLOAT:
                    fprintf(stderr, "float value %f\n", lit->value.float_value);
                    break;
                case TYPE_VARCHAR:
                case TYPE_TEXT:
                    fprintf(stderr, "string value '%s'\n", lit->value.string_value);
                    break;
                case TYPE_BOOLEAN:
                    fprintf(stderr, "boolean value %s\n", lit->value.bool_value ? "true" : "false");
                    break;
                default:
                    fprintf(stderr, "unknown type\n");
                    break;
            }
            #endif
        }
    }
    
    return 0;
}

/**
 * @brief Execute an UPDATE statement
 */
UpdateResult* execute_update(const UpdateStatement* stmt, const char* base_dir) {
    UpdateResult* result = create_update_result();
    
    if (!stmt || !base_dir) {
        set_update_error(result, "Invalid parameters");
        return result;
    }
    
    // Load table schema using the shared function
    TableSchema* schema = load_table_schema(stmt->table_name, base_dir);
    if (!schema) {
        set_update_error(result, "Table '%s' not found", stmt->table_name);
        return result;
    }
    
    // Validate statement
    if (!validate_update_statement(stmt, schema)) {
        set_update_error(result, "UPDATE statement validation failed");
        free_table_schema(schema);
        return result;
    }
    
    // Generate kernel for finding matching records
    GeneratedKernel* kernel = generate_update_kernel(stmt, schema, base_dir);
    if (!kernel) {
        set_update_error(result, "Failed to generate update kernel");
        free_table_schema(schema);
        return result;
    }
    
    // Compile kernel
    char kernel_path[2048];
    if (compile_kernel(kernel, base_dir, stmt->table_name, -1, 
                      kernel_path, sizeof(kernel_path)) != 0) {
        set_update_error(result, "Failed to compile update kernel");
        free_generated_kernel(kernel);
        free_table_schema(schema);
        return result;
    }
    
    // Load kernel
    LoadedKernel loaded_kernel;
    if (load_kernel(kernel_path, kernel->kernel_name, stmt->table_name, -1, 
                   &loaded_kernel) != 0) {
        set_update_error(result, "Failed to load update kernel");
        free_generated_kernel(kernel);
        free_table_schema(schema);
        return result;
    }
    
    // Track affected pages
    bool* affected_pages = NULL;
    int page_count = 0;
    
    // Count pages
    char compiled_dir[2048];
    snprintf(compiled_dir, sizeof(compiled_dir), "%s/compiled", base_dir);
    DIR* dir = opendir(compiled_dir);
    if (dir) {
        struct dirent* entry;
        while ((entry = readdir(dir)) != NULL) {
            if (strstr(entry->d_name, stmt->table_name) && 
                strstr(entry->d_name, ".so")) {
                page_count++;
            }
        }
        closedir(dir);
    }
    
    affected_pages = calloc(page_count, sizeof(bool));
    
    // Process each page
    int total_updated = 0;
    for (int page_num = 0; page_num < page_count; page_num++) {
        LoadedPage page;
        
        if (load_page(base_dir, stmt->table_name, page_num, &page) != 0) {
            continue;
        }
        
        int page_records;
        if (get_page_count(&page, &page_records) != 0) {
            unload_page(&page);
            continue;
        }
        
        // Read all records from page
        void** records = malloc(page_records * sizeof(void*));
        for (int i = 0; i < page_records; i++) {
            read_record(&page, i, &records[i]);
        }
        
        // Calculate the actual record size
        size_t record_size = calculate_record_size((const struct TableSchema*)schema);

        // Allocate space for full record structures
        void* matches = malloc(page_records * record_size);

        if (!matches) {
            fprintf(stderr, "Failed to allocate matches buffer\n");
            unload_page(&page);
            continue;
        }

        if (!matches) {
            fprintf(stderr, "Failed to allocate matches buffer\n");
            unload_page(&page);
            continue;
        }

        int match_count = execute_kernel(&loaded_kernel, records, page_records,
                                       matches, page_records);
        
        if (match_count > 0) {
            affected_pages[page_num] = true;
            total_updated += match_count;
            
            
                for (int i = 0; i < match_count; i++) {
                    #ifdef DEBUG
                    DEBUG("Found matching record %d/%d at address %p\n", 
                            i+1, match_count, ((void**)matches)[i]);
                    #endif
                    
                    update_record(((void**)matches)[i], stmt, schema, i);
                } 
            
            // TODO: Write updated records back to page
            // This requires regenerating the page data file
        }
        
        free(records);
        free(matches);
        unload_page(&page);
    }
    
    // Recompile affected pages
    for (int i = 0; i < page_count; i++) {
        if (affected_pages[i]) {
            recompile_data_page(schema, base_dir, i);
        }
    }
    
    // Update metadata
    if (total_updated > 0) {
        TableMetadata metadata;
        if (load_table_metadata(stmt->table_name, base_dir, &metadata) == 0) {
            time(&metadata.modified_at);
            update_table_metadata(&metadata, base_dir);
        }
    }
    
    result->rows_affected = total_updated;
    result->success = true;
    
    // Cleanup
    free(affected_pages);
    unload_kernel(&loaded_kernel);
    free_generated_kernel(kernel);
    free_table_schema(schema);
    
    return result;
}

/**
 * @brief Free update result
 */
void free_update_result(UpdateResult* result) {
    if (result) {
        free(result->error_message);
        free(result);
    }
}
