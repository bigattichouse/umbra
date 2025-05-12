/**
 * @file select_executor.c
 * @brief Executes SELECT queries
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "select_executor.h"
#include "../schema/type_system.h"
#include "../kernel/kernel_generator.h"
#include "../kernel/kernel_compiler.h"
#include "../kernel/kernel_loader.h"
#include "../kernel/filter_generator.h"
#include "../kernel/projection_generator.h"
#include "../loader/page_manager.h"
#include "../loader/record_access.h"
#include "../query/query_executor.h"  

/**
 * @brief Get maximum result size estimate
 */
static int estimate_max_results(const char* base_dir, const char* table_name) {
    // Suppress unused parameter warnings
    (void)base_dir;
    (void)table_name;
    
    // For now, return a conservative estimate
    // In practice, this would be based on statistics
    return 10000;
}

/**
 * @brief Execute kernel on a single page
 */
static int execute_kernel_on_page(const LoadedKernel* kernel, LoadedPage* page,
                                 void* results, int max_results, int current_count) {
    // Get page data count
    int page_count;
    if (get_page_count(page, &page_count) != 0) {
        return -1;
    }
    
    // The kernel expects a pointer to the first element of a contiguous array
    // of structs, not an array of pointers. The page's data is already in
    // a contiguous array, so we just need to get a pointer to the first record.
    void* first_record;
    if (page_count == 0 || read_record(page, 0, &first_record) != 0) {
        return 0; // No records to process
    }
    
    // Debug output
    #ifdef DEBUG
    fprintf(stderr, "[DEBUG] Passing %d records to kernel\n", page_count);
    
    // Show the raw data as the kernel will see it
    if (strcmp(page->table_name, "users") == 0) {
        for (int i = 0; i < page_count; i++) {
            void* record;
            if (read_record(page, i, &record) != 0) {
                break;
            }
            
            fprintf(stderr, "[DEBUG] Raw record %d data (first 600 bytes):\n", i);
            unsigned char* bytes = (unsigned char*)record;
            
            // Print enough bytes to cover the entire struct
            int bytes_to_print = 600;  // Should be enough for the users struct
            for (int j = 0; j < bytes_to_print; j++) {
                fprintf(stderr, "%02x ", bytes[j]);
                if ((j + 1) % 16 == 0) fprintf(stderr, "\n");
            }
            fprintf(stderr, "\n");
            
            // Try to interpret the fields manually based on known offsets
            int* id_ptr = (int*)record;
            char* name_ptr = (char*)((char*)record + 4);
            char* email_ptr = (char*)((char*)record + 4 + 256);
            int* age_ptr = (int*)((char*)record + 4 + 256 + 256);
            
            fprintf(stderr, "[DEBUG] Field interpretation - id: %d, name: '%s', email: '%s', age: %d\n",
                    *id_ptr, name_ptr, email_ptr, *age_ptr);
        }
    }
    #endif
    
    // Allocate space for kernel results
    int remaining = max_results - current_count;
    
    // The kernel expects the data as a contiguous array, which it already is
    // in the page. We pass the pointer to the first record, and the kernel
    // will treat it as an array.
    int results_from_page = execute_kernel(kernel, first_record, page_count,
                                          (char*)results + current_count * sizeof(void*), 
                                          remaining);
    
    #ifdef DEBUG
    fprintf(stderr, "[DEBUG] Kernel returned %d results\n", results_from_page);
    #endif
    
    return results_from_page;
}

/**
 * @brief Count pages for a table
 */
static int count_table_pages(const char* base_dir, const char* table_name) {
    char compiled_dir[2048];
    snprintf(compiled_dir, sizeof(compiled_dir), "%s/compiled", base_dir);
    
    DIR* dir = opendir(compiled_dir);
    if (!dir) {
        return 0;
    }
    
    int count = 0;
    struct dirent* entry;
    
    // Look for page .so files for this table
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "%sData_", table_name);
    size_t pattern_len = strlen(pattern);
    
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, pattern, pattern_len) == 0 &&
            strstr(entry->d_name, ".so") != NULL) {
            count++;
        }
    }
    
    closedir(dir);
    return count;
}

/**
 * @brief Build result schema for SELECT statement
 */
TableSchema* build_result_schema(const SelectStatement* stmt, const TableSchema* source_schema) {
    TableSchema* result_schema = malloc(sizeof(TableSchema));
    
    strncpy(result_schema->name, "result", sizeof(result_schema->name) - 1);
    result_schema->name[sizeof(result_schema->name) - 1] = '\0';
    
    if (stmt->select_list.has_star) {
        // Copy all columns from source schema
        result_schema->column_count = source_schema->column_count;
        result_schema->columns = malloc(result_schema->column_count * sizeof(ColumnDefinition));
        
        for (int i = 0; i < result_schema->column_count; i++) {
            result_schema->columns[i] = source_schema->columns[i];
        }
    } else {
        // Only selected columns
        result_schema->column_count = stmt->select_list.count;
        result_schema->columns = malloc(result_schema->column_count * sizeof(ColumnDefinition));
        
        for (int i = 0; i < stmt->select_list.count; i++) {
            Expression* expr = stmt->select_list.expressions[i];
            
            if (expr->type == AST_COLUMN_REF) {
                // Find column in source schema
                const char* col_name = expr->data.column.column_name;
                bool found = false;
                
                for (int j = 0; j < source_schema->column_count; j++) {
                    if (strcmp(source_schema->columns[j].name, col_name) == 0) {
                        result_schema->columns[i] = source_schema->columns[j];
                        found = true;
                        break;
                    }
                }
                
                if (!found) {
                    free(result_schema->columns);
                    free(result_schema);
                    return NULL;
                }
            }
        }
    }
    
    // No primary key in result set
    result_schema->primary_key_columns = NULL;
    result_schema->primary_key_column_count = 0;
    
    return result_schema;
}

/**
 * @brief Execute a SELECT statement
 */
int execute_select(const SelectStatement* stmt, const char* base_dir, QueryResult* result) {
        const char* table_name = stmt->from_table->table_name;
    
    // Load source table schema using the shared function
    TableSchema* source_schema = load_table_schema(table_name, base_dir);
    if (!source_schema) {
        result->error_message = strdup("Table not found");
        return -1;
    }
    if (stmt->where_clause) {
        fprintf(stderr, "[DEBUG] SELECT has WHERE clause\n");
    }
    
    // Generate kernel for query
    GeneratedKernel* kernel = generate_select_kernel(stmt, source_schema, base_dir);
    if (!kernel) {
        free_table_schema(source_schema);
        result->error_message = strdup("Failed to generate query kernel");
        return -1;
    }
    
    // Compile kernel
    char kernel_path[2048];
    if (compile_kernel(kernel, base_dir, table_name, -1, kernel_path, sizeof(kernel_path)) != 0) {
        free_generated_kernel(kernel);
        free_table_schema(source_schema);
        result->error_message = strdup("Failed to compile query kernel");
        return -1;
    }
    
    // Load compiled kernel
    LoadedKernel loaded_kernel;
    if (load_kernel(kernel_path, kernel->kernel_name, table_name, -1, &loaded_kernel) != 0) {
        free_generated_kernel(kernel);
        free_table_schema(source_schema);
        result->error_message = strdup("Failed to load query kernel");
        return -1;
    }
    
    // Build result schema
    result->result_schema = build_result_schema(stmt, source_schema);
    if (!result->result_schema) {
        unload_kernel(&loaded_kernel);
        free_generated_kernel(kernel);
        free_table_schema(source_schema);
        result->error_message = strdup("Failed to build result schema");
        return -1;
    }
    
    // Allocate result buffer
    int max_results = estimate_max_results(base_dir, table_name);
    void* results_buffer = malloc(max_results * sizeof(void*));
    if (!results_buffer) {
        unload_kernel(&loaded_kernel);
        free_generated_kernel(kernel);
        free_table_schema(source_schema);
        result->error_message = strdup("Failed to allocate results buffer");
        return -1;
    }
    
    // Execute kernel on each page
    int total_results = 0;
    int page_count = count_table_pages(base_dir, table_name);
    
    for (int page_num = 0; page_num < page_count && total_results < max_results; page_num++) {
        LoadedPage page;
        
        if (load_page(base_dir, table_name, page_num, &page) != 0) {
            continue;
        }
        
        int page_results = execute_kernel_on_page(&loaded_kernel, &page,
                                                 results_buffer, max_results, total_results);
        
        unload_page(&page);
        
        fprintf(stderr, "[DEBUG] Page %d returned %d results\n", page_num, page_results);
        
        if (page_results < 0) {
            break;
        }
        
        total_results += page_results;
    }
    
    fprintf(stderr, "[DEBUG] Total results: %d\n", total_results);
    

    // Set result data
    result->rows = (void**)results_buffer;
    result->row_count = total_results;
    result->success = true;
    result->row_format = ROW_FORMAT_DIRECT; // Or ROW_FORMAT_POINTER_ARRAY depending on how your kernel returns data
    
    // Cleanup
    unload_kernel(&loaded_kernel);
    free_generated_kernel(kernel);
    free_table_schema(source_schema);
    
    return 0;
}
