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
#include "../schema/type_system.h"
#include "../loader/page_manager.h"
#include "../loader/record_access.h"  // Updated to use record_access functions
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
 * @brief Physically delete records from a data file
 */ 
static int delete_records_from_file(const char* data_path, const TableSchema* schema, 
                                   void** matches, int match_count) {
    if (match_count == 0) {
        return 0;  // Nothing to delete
    }

    // Extract UUIDs from matching records for easy comparison
    // Using record_access.c function instead of local implementation
    char** match_uuids = malloc(match_count * sizeof(char*));
    if (!match_uuids) {
        fprintf(stderr, "Failed to allocate memory for UUIDs\n");
        return -1;
    }
    
    // Extract UUIDs with better error handling
    for (int i = 0; i < match_count; i++) {
        match_uuids[i] = get_uuid_from_record(matches[i], schema);
        
        if (!match_uuids[i]) {
            #ifdef DEBUG
            fprintf(stderr, "[DEBUG] Failed to get UUID for record %d\n", i);
            #endif
        }
    }
    
    // Open the file for reading
    FILE* read_file = fopen(data_path, "r");
    if (!read_file) {
        // Clean up UUIDs
        for (int i = 0; i < match_count; i++) {
            free(match_uuids[i]);
        }
        free(match_uuids);
        
        fprintf(stderr, "Failed to open data file for reading: %s\n", data_path);
        return -1;
    }
    
    // Read the entire file into memory
    char** lines = NULL;
    int line_count = 0;
    char buffer[4096];
    
    while (fgets(buffer, sizeof(buffer), read_file)) {
        char** new_lines = realloc(lines, (line_count + 1) * sizeof(char*));
        if (!new_lines) {
            fprintf(stderr, "Memory allocation failed while reading file\n");
            
            // Clean up already allocated lines
            for (int i = 0; i < line_count; i++) {
                free(lines[i]);
            }
            free(lines);
            
            // Clean up UUIDs
            for (int i = 0; i < match_count; i++) {
                free(match_uuids[i]);
            }
            free(match_uuids);
            
            fclose(read_file);
            return -1;
        }
        
        lines = new_lines;
        lines[line_count] = strdup(buffer);
        if (!lines[line_count]) {
            fprintf(stderr, "Memory allocation failed for line %d\n", line_count);
            
            // Clean up already allocated lines
            for (int i = 0; i < line_count; i++) {
                free(lines[i]);
            }
            free(lines);
            
            // Clean up UUIDs
            for (int i = 0; i < match_count; i++) {
                free(match_uuids[i]);
            }
            free(match_uuids);
            
            fclose(read_file);
            return -1;
        }
        
        line_count++;
    }
    
    fclose(read_file);
    
    // Create an array to track record lines and which ones to delete
    typedef struct {
        int line_index;
        bool should_delete;
    } RecordLine;
    
    RecordLine* record_lines = NULL;
    int record_count = 0;
    
    // First, find all record lines (those containing struct initializers)
    for (int i = 0; i < line_count; i++) {
        // Check if this line represents a record
        size_t len = strlen(lines[i]);
        if (len >= 3 && lines[i][len-3] == '}' && lines[i][len-2] == ',' && lines[i][len-1] == '\n') {
            RecordLine* new_record_lines = realloc(record_lines, (record_count + 1) * sizeof(RecordLine));
            if (!new_record_lines) {
                fprintf(stderr, "Memory allocation failed for record lines\n");
                
                // Clean up
                free(record_lines);
                for (int j = 0; j < line_count; j++) {
                    free(lines[j]);
                }
                free(lines);
                
                for (int j = 0; j < match_count; j++) {
                    free(match_uuids[j]);
                }
                free(match_uuids);
                
                return -1;
            }
            
            record_lines = new_record_lines;
            record_lines[record_count].line_index = i;
            record_lines[record_count].should_delete = false;
            record_count++;
        }
    }

    // Mark records for deletion by scanning for UUIDs in the file content
    int marked_for_deletion = 0;
    
    for (int i = 0; i < match_count; i++) {
        if (!match_uuids[i]) continue;
        
        // Look for this UUID in each record line
        for (int j = 0; j < record_count; j++) {
            int line_idx = record_lines[j].line_index;
            if (record_lines[j].should_delete) continue;  // Already marked
            
            // Check if this line contains the UUID
            if (strstr(lines[line_idx], match_uuids[i])) {
                record_lines[j].should_delete = true;
                marked_for_deletion++;
                #ifdef DEBUG
                fprintf(stderr, "[DEBUG] Marked record at line %d for deletion (UUID: %s)\n", 
                        line_idx, match_uuids[i]);
                #endif
                break;  // Found this UUID, move to next
            }
        }
    }

    // Clean up UUID strings
    for (int i = 0; i < match_count; i++) {
        free(match_uuids[i]);
    }
    free(match_uuids);
    
    // If nothing to delete, clean up and return
    if (marked_for_deletion == 0) {
        for (int i = 0; i < line_count; i++) {
            free(lines[i]);
        }
        free(lines);
        free(record_lines);
        return 0;
    }
    
    // Open file for writing
    FILE* write_file = fopen(data_path, "w");
    if (!write_file) {
        // Clean up
        for (int i = 0; i < line_count; i++) {
            free(lines[i]);
        }
        free(lines);
        free(record_lines);
        
        fprintf(stderr, "Failed to open data file for writing: %s\n", data_path);
        return -1;
    }
    
    // Write header
    fprintf(write_file, "/*This file autogenerated, do not edit manually*/\n");
    
    // Write lines, skipping deleted records
    int deleted = 0;
    int record_idx = 0;
    
    for (int line_idx = 0; line_idx < line_count; line_idx++) {
        // Check if this is a record line to delete
        bool skip = false;
        
        if (record_idx < record_count && line_idx == record_lines[record_idx].line_index) {
            if (record_lines[record_idx].should_delete) {
                skip = true;
                deleted++;
            }
            record_idx++;
        }
        
        if (!skip) {
            fputs(lines[line_idx], write_file);
        }
    }
    
    fclose(write_file);
    
    // Clean up
    for (int i = 0; i < line_count; i++) {
        free(lines[i]);
    }
    free(lines);
    free(record_lines);
    
    return deleted;
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
        
        if (page_records == 0) {
            unload_page(&page);
            continue;
        }
        
        // Get data file path
        char data_path[2048];
        snprintf(data_path, sizeof(data_path), "%s/tables/%s/data/%sData.%d.dat.h", 
                 base_dir, stmt->table_name, stmt->table_name, page_num);
        
        if (stmt->where_clause && kernel_loaded) {
            // Use kernel to find matching records
            size_t record_size = sizeof(void*); // We'll store pointers not copies
            
            // Allocate space for matching record pointers
            void** matches = malloc(page_records * record_size);
            if (!matches) {
                fprintf(stderr, "Failed to allocate matches buffer\n");
                unload_page(&page);
                continue;
            }
            
            // Track matched records
            int match_count = 0;
            
            // Get pointer to first record - using record_access.c function
            void* first_record;
            if (read_record(&page, 0, &first_record) != 0) {
                free(matches);
                unload_page(&page);
                continue;
            }
            
            // Get the record size from schema
            size_t struct_size = calculate_record_size((const struct TableSchema*)schema);
            
            // Allocate space for kernel results
            void* kernel_results = malloc(page_records * struct_size);
            if (!kernel_results) {
                free(matches);
                unload_page(&page);
                continue;
            }
            
            // Execute kernel to find matching records
            int kernel_result_count = execute_kernel(
                &loaded_kernel, first_record, page_records, kernel_results, page_records);
            
            #ifdef DEBUG
            fprintf(stderr, "[DEBUG] Kernel found %d matching records in page %d\n", 
                    kernel_result_count, page_num);
            #endif
            
            if (kernel_result_count > page_records) {
                fprintf(stderr, "Warning: Found more matching records (%d) than expected (%d)\n", 
                        kernel_result_count, page_records);
                kernel_result_count = page_records; // Truncate to avoid overflow
            }

            // Store pointers to matches
            for (int i = 0; i < kernel_result_count && match_count < page_records; i++) {
                matches[match_count++] = (char*)kernel_results + (i * struct_size);
            }
            
            // Now physically delete the records
            if (match_count > 0) {
                int deleted = delete_records_from_file(data_path, schema, matches, match_count);
                
                if (deleted > 0) {
                    affected_pages[page_num] = true;
                    total_deleted += deleted;
                    #ifdef DEBUG
                    fprintf(stderr, "[DEBUG] Deleted %d records from page %d\n", deleted, page_num);
                    #endif
                }
            }
            
            // Clean up
            free(kernel_results);
            free(matches);
        } else {
            // No WHERE clause - delete all records
            // Just write an empty data file with only the header
            FILE* file = fopen(data_path, "w");
            if (file) {
                fprintf(file, "/*This file autogenerated, do not edit manually*/\n");
                fclose(file);
                
                affected_pages[page_num] = true;
                total_deleted += page_records;
            }
        }
        
        unload_page(&page);
    }
    
    // Recompile affected pages
    for (int i = 0; i < page_count; i++) {
        if (affected_pages[i]) {
            #ifdef DEBUG
            fprintf(stderr, "[DEBUG] Recompiling page %d\n", i);
            #endif
            if (recompile_data_page(schema, base_dir, i) != 0) {
                fprintf(stderr, "Warning: Failed to recompile page %d\n", i);
            }
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
