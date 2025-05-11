/**
 * @file delete_executor.c
 * @brief Executes DELETE operations
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <dirent.h>
#include <time.h>
#include "delete_executor.h"
#include "../schema/metadata.h"
#include "../loader/page_manager.h"
#include "../loader/record_access.h"
#include "../kernel/kernel_generator.h"
#include "../kernel/kernel_compiler.h"
#include "../kernel/kernel_loader.h"
#include "../pages/page_generator.h"
#include "../pages/page_splitter.h"
#include "../query/query_executor.h"

/**
 * @brief Create delete result
 */
static DeleteResult* create_delete_result(void) {
    DeleteResult* result = malloc(sizeof(DeleteResult));
    result->rows_affected = 0;
    result->success = false;
    result->error_message = NULL;
    return result;
}

/**
 * @brief Set error in result
 */
static void set_delete_error(DeleteResult* result, const char* format, ...) {
    result->success = false;
    
    va_list args;
    va_start(args, format);
    
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    
    result->error_message = strdup(buffer);
    
    va_end(args);
}

/**
 * @brief Generate delete kernel
 */
static GeneratedKernel* generate_delete_kernel(const DeleteStatement* stmt, 
                                              const TableSchema* schema,
                                              const char* base_dir) {
    // Create a synthetic SELECT statement to use existing kernel generator
    SelectStatement select_stmt;
    select_stmt.from_table = malloc(sizeof(TableRef));
    select_stmt.from_table->table_name = strdup(stmt->table_name);
    select_stmt.from_table->alias = NULL;
    
    // Use WHERE clause from DELETE
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
 * @brief Execute a DELETE statement
 */
DeleteResult* execute_delete(const DeleteStatement* stmt, const char* base_dir) {
    DeleteResult* result = create_delete_result();
    
    if (!stmt || !base_dir) {
        set_delete_error(result, "Invalid parameters");
        return result;
    }
    
    // Load table schema using the shared function
    TableSchema* schema = load_table_schema(stmt->table_name, base_dir);
    if (!schema) {
        set_delete_error(result, "Table '%s' not found", stmt->table_name);
        return result;
    }
    
    // Validate statement
    if (!validate_delete_statement(stmt, schema)) {
        set_delete_error(result, "DELETE statement validation failed");
        free_table_schema(schema);
        return result;
    }
    
    // Generate kernel for finding matching records
    GeneratedKernel* kernel = NULL;
    LoadedKernel loaded_kernel;
    bool kernel_loaded = false;
    
    if (stmt->where_clause) {
        kernel = generate_delete_kernel(stmt, schema, base_dir);
        if (!kernel) {
            set_delete_error(result, "Failed to generate delete kernel");
            free_table_schema(schema);
            return result;
        }
        
        // Compile kernel
        char kernel_path[2048];
        if (compile_kernel(kernel, base_dir, stmt->table_name, -1, 
                          kernel_path, sizeof(kernel_path)) != 0) {
            set_delete_error(result, "Failed to compile delete kernel");
            free_generated_kernel(kernel);
            free_table_schema(schema);
            return result;
        }
        
        // Load kernel
        if (load_kernel(kernel_path, kernel->kernel_name, stmt->table_name, -1, 
                       &loaded_kernel) != 0) {
            set_delete_error(result, "Failed to load delete kernel");
            free_generated_kernel(kernel);
            free_table_schema(schema);
            return result;
        }
        kernel_loaded = true;
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
    int total_deleted = 0;
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
        
        void** records = malloc(page_records * sizeof(void*));
        for (int i = 0; i < page_records; i++) {
            read_record(&page, i, &records[i]);
        }
        
        int delete_count = 0;
        
        if (stmt->where_clause && kernel_loaded) {
            // Use kernel to find matching records
            void* matches = malloc(page_records * sizeof(void*));
            delete_count = execute_kernel(&loaded_kernel, records, page_records,
                                        matches, page_records);
            
            // TODO: Mark matching records for deletion
            // This requires updating the data files
            
            free(matches);
        } else {
            // No WHERE clause - delete all records
            delete_count = page_records;
            
            // TODO: Clear the entire page
        }
        
        if (delete_count > 0) {
            affected_pages[page_num] = true;
            total_deleted += delete_count;
        }
        
        free(records);
        unload_page(&page);
    }
    
    // Recompile affected pages
    for (int i = 0; i < page_count; i++) {
        if (affected_pages[i]) {
            recompile_data_page(schema, base_dir, i);
        }
    }
    
    // Update metadata
    if (total_deleted > 0) {
        TableMetadata metadata;
        if (load_table_metadata(stmt->table_name, base_dir, &metadata) == 0) {
            metadata.record_count -= total_deleted;
            time(&metadata.modified_at);
            update_table_metadata(&metadata, base_dir);
        }
    }
    
    result->rows_affected = total_deleted;
    result->success = true;
    
    // Cleanup
    free(affected_pages);
    if (kernel_loaded) {
        unload_kernel(&loaded_kernel);
    }
    if (kernel) {
        free_generated_kernel(kernel);
    }
    free_table_schema(schema);
    
    return result;
}

/**
 * @brief Free delete result
 */
void free_delete_result(DeleteResult* result) {
    if (result) {
        free(result->error_message);
        free(result);
    }
}
