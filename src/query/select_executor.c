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
    
    // Load source table schema
    TableSchema* source_schema = load_table_schema(table_name, base_dir);
    if (!source_schema) {
        result->error_message = strdup("Table not found");
        return -1;
    }
    
    // Check if this is a COUNT(*) query
    bool is_count_star = false;
    if (stmt->select_list.count == 1) {
        Expression* expr = stmt->select_list.expressions[0];
        if (expr->type == AST_FUNCTION_CALL &&
            strcasecmp(expr->data.function.function_name, "COUNT") == 0 &&
            expr->data.function.argument_count == 1 &&
            expr->data.function.arguments[0]->type == AST_STAR) {
            is_count_star = true;
        }
    }
    
    // Generate kernel for query
    GeneratedKernel* kernel = generate_select_kernel(stmt, source_schema, base_dir);
    if (!kernel) {
        free_table_schema(source_schema);
        result->error_message = strdup("Failed to generate query kernel");
        return -1;
    }
    
    // Compile and load kernel
    char kernel_path[2048];
    if (compile_kernel(kernel, base_dir, table_name, -1, kernel_path, sizeof(kernel_path)) != 0) {
        free_generated_kernel(kernel);
        free_table_schema(source_schema);
        result->error_message = strdup("Failed to compile query kernel");
        return -1;
    }
    
    LoadedKernel loaded_kernel;
    if (load_kernel(kernel_path, kernel->kernel_name, table_name, -1, &loaded_kernel) != 0) {
        free_generated_kernel(kernel);
        free_table_schema(source_schema);
        result->error_message = strdup("Failed to load query kernel");
        return -1;
    }
    
    // Build result schema
    if (is_count_star) {
        // Create a result schema for COUNT(*)
        result->result_schema = malloc(sizeof(TableSchema));
        strncpy(result->result_schema->name, "result", sizeof(result->result_schema->name) - 1);
        result->result_schema->name[sizeof(result->result_schema->name) - 1] = '\0';
        
        result->result_schema->column_count = 1;
        result->result_schema->columns = malloc(sizeof(ColumnDefinition));
        
        strncpy(result->result_schema->columns[0].name, "COUNT(*)", 
               sizeof(result->result_schema->columns[0].name) - 1);
        result->result_schema->columns[0].name[sizeof(result->result_schema->columns[0].name) - 1] = '\0';
        
        result->result_schema->columns[0].type = TYPE_INT;
        result->result_schema->columns[0].nullable = false;
        result->result_schema->columns[0].is_primary_key = false;
        
        result->result_schema->primary_key_column_count = 0;
        result->result_schema->primary_key_columns = NULL;
    } else {
        result->result_schema = build_result_schema(stmt, source_schema);
    }
    
    if (!result->result_schema) {
        unload_kernel(&loaded_kernel);
        free_generated_kernel(kernel);
        free_table_schema(source_schema);
        result->error_message = strdup("Failed to build result schema");
        return -1;
    }
    
    if (is_count_star) {
        // For COUNT(*), we just need a single int
        int total_count = 0;
        
        // Execute kernel on each page
        int page_count = count_table_pages(base_dir, table_name);
        
        for (int page_num = 0; page_num < page_count; page_num++) {
            LoadedPage page;
            
            if (load_page(base_dir, table_name, page_num, &page) != 0) {
                continue;
            }
            
            // Execute kernel on this page
            int page_records;
            if (get_page_count(&page, &page_records) != 0 || page_records == 0) {
                unload_page(&page);
                continue;
            }
            
            // Get pointer to raw data
            void* first_record;
            if (read_record(&page, 0, &first_record) == 0) {
                // Execute kernel
                int page_count = 0;
                int result_count = execute_kernel(&loaded_kernel, first_record, page_records,
                                              &page_count, 1);
                
                if (result_count > 0) {
                    // Add this page's count to the total
                    total_count += page_count;
                }
            }
            
            unload_page(&page);
        }
        
        // Create result row with the count
        result->rows = malloc(sizeof(void*));
        int* count_result = malloc(sizeof(int));
        *count_result = total_count;
        result->rows[0] = count_result;
        result->row_count = 1;
        result->success = true;
        result->row_format = ROW_FORMAT_DIRECT;
   } else {
        // Calculate record size based on schema - fix: add proper cast
        size_t record_size = calculate_record_size((const struct TableSchema*)source_schema);
        
        // Allocate result buffer for raw data
        int max_results = estimate_max_results(base_dir, table_name);
        char* raw_results_buffer = malloc(max_results * record_size);
        if (!raw_results_buffer) {
            free_table_schema(result->result_schema);
            result->result_schema = NULL;
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
            
            // Execute kernel on this page
            int page_count;
            if (get_page_count(&page, &page_count) != 0) {
                unload_page(&page);
                continue;
            }
            
            // Get pointer to raw data
            void* first_record;
            if (page_count > 0 && read_record(&page, 0, &first_record) == 0) {
                // Calculate where to place the next batch of results
                void* results_pos = raw_results_buffer + (total_results * record_size);
                
                // Execute kernel
                int results_from_page = execute_kernel(&loaded_kernel, first_record, page_count,
                                                      results_pos, max_results - total_results);
                
                fprintf(stderr, "[DEBUG] Page %d returned %d results\n", page_num, results_from_page);
                
                if (results_from_page > 0) {
                    total_results += results_from_page;
                }
            }
            
            unload_page(&page);
        }
        
        fprintf(stderr, "[DEBUG] Total results: %d\n", total_results);
        
        // Now create an array of pointers to each record in the raw buffer
        void** row_pointers = malloc(total_results * sizeof(void*));
        if (!row_pointers) {
            free(raw_results_buffer);
            free_table_schema(result->result_schema);
            result->result_schema = NULL;
            unload_kernel(&loaded_kernel);
            free_generated_kernel(kernel);
            free_table_schema(source_schema);
            result->error_message = strdup("Failed to allocate row pointers");
            return -1;
        }
        
        // Set up pointers to each record in the buffer
        for (int i = 0; i < total_results; i++) {
            row_pointers[i] = raw_results_buffer + (i * record_size);
        }
        
        // Set result data
        result->rows = row_pointers;
        result->row_count = total_results;
        result->success = true;
        result->raw_data_buffer = raw_results_buffer; // Store the buffer for later freeing
        result->row_format = ROW_FORMAT_DIRECT;
    }
    
    // Cleanup
    unload_kernel(&loaded_kernel);
    free_generated_kernel(kernel);
    free_table_schema(source_schema);
    
    return 0;
}
