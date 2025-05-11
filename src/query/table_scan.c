/**
 * @file table_scan.c
 * @brief Implementation of table scanning functionality
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "table_scan.h"
#include "../kernel/filter_generator.h"
#include "../kernel/projection_generator.h"
#include "../kernel/kernel_generator.h"
#include "../kernel/kernel_compiler.h"
#include "../kernel/kernel_loader.h"

/**
 * @brief Initialize a table scan
 */
int init_table_scan(TableScan* scan, const char* base_dir, const char* table_name,
                   Expression* filter, const int* projected_columns, int projection_count) {
    if (!scan || !base_dir || !table_name) {
        return -1;
    }
    
    // Initialize scan structure
    memset(scan, 0, sizeof(TableScan));
    
    strncpy(scan->base_dir, base_dir, sizeof(scan->base_dir) - 1);
    scan->base_dir[sizeof(scan->base_dir) - 1] = '\0';
    
    strncpy(scan->table_name, table_name, sizeof(scan->table_name) - 1);
    scan->table_name[sizeof(scan->table_name) - 1] = '\0';
    
    // Load table schema
    scan->schema = load_table_schema(table_name, base_dir);
    if (!scan->schema) {
        return -1;
    }
    
    // Initialize cursor
    if (init_cursor(&scan->cursor, base_dir, table_name) != 0) {
        free_table_schema(scan->schema);
        return -1;
    }
    
    // Store filter expression (make a copy if needed)
    scan->filter = filter;
    
    // Store projection information
    if (projected_columns && projection_count > 0) {
        scan->projected_columns = malloc(projection_count * sizeof(int));
        if (!scan->projected_columns) {
            free_cursor(&scan->cursor);
            free_table_schema(scan->schema);
            return -1;
        }
        
        memcpy(scan->projected_columns, projected_columns, projection_count * sizeof(int));
        scan->projection_count = projection_count;
    } else if (scan->schema) {
        // If no projection specified, project all columns
        scan->projection_count = scan->schema->column_count;
        scan->projected_columns = malloc(scan->projection_count * sizeof(int));
        if (!scan->projected_columns) {
            free_cursor(&scan->cursor);
            free_table_schema(scan->schema);
            return -1;
        }
        
        for (int i = 0; i < scan->projection_count; i++) {
            scan->projected_columns[i] = i;
        }
    }
    
    scan->initialized = true;
    scan->at_end = scan->cursor.at_end;
    
    return 0;
}

/**
 * @brief Free resources used by a table scan
 */
int free_table_scan(TableScan* scan) {
    if (!scan || !scan->initialized) {
        return 0;
    }
    
    if (scan->schema) {
        free_table_schema(scan->schema);
        scan->schema = NULL;
    }
    
    if (scan->projected_columns) {
        free(scan->projected_columns);
        scan->projected_columns = NULL;
    }
    
    free_cursor(&scan->cursor);
    
    scan->initialized = false;
    return 0;
}

/**
 * @brief Reset a table scan to the beginning
 */
int reset_table_scan(TableScan* scan) {
    if (!scan || !scan->initialized) {
        return -1;
    }
    
    if (reset_cursor(&scan->cursor) != 0) {
        return -1;
    }
    
    scan->at_end = scan->cursor.at_end;
    scan->current_record = NULL;
    
    return 0;
}

/**
 * @brief Check if record matches filter
 */
bool evaluate_filter(const TableScan* scan, const void* record) {
    if (!scan || !record || !scan->filter) {
        return true;  // No filter means all records match
    }
    
    // This is a simplified implementation
    // In a real implementation, we would interpret the filter expression
    // and evaluate it against the record
    
    // For demonstration purposes, let's assume all records match
    return true;
}

/**
 * @brief Move to the next record that matches filter
 */
int next_matching_record(TableScan* scan) {
    if (!scan || !scan->initialized) {
        return -1;
    }
    
    if (scan->at_end) {
        return 1;
    }
    
    // Get current record if we don't have one
    if (!scan->current_record) {
        if (get_current_record(&scan->cursor, &scan->current_record) != 0) {
            scan->at_end = true;
            return 1;
        }
    }
    
    do {
        // Check if current record matches filter
        if (evaluate_filter(scan, scan->current_record)) {
            // Current record matches, return success
            return 0;
        }
        
        // Move to next record
        int result = next_record(&scan->cursor);
        if (result != 0) {
            scan->at_end = true;
            scan->current_record = NULL;
            return result;
        }
        
        // Get the new current record
        if (get_current_record(&scan->cursor, &scan->current_record) != 0) {
            scan->at_end = true;
            return 1;
        }
    } while (!scan->at_end);
    
    return 1;
}

/**
 * @brief Get the current record
 */
int get_current_record(const TableScan* scan, void** record) {
    if (!scan || !scan->initialized || !record) {
        return -1;
    }
    
    if (scan->at_end) {
        return 1;
    }
    
    *record = scan->current_record;
    return 0;
}

/**
 * @brief Calculate memory requirements for projected record
 */
int get_projected_record_size(const TableScan* scan, size_t* size) {
    if (!scan || !scan->schema || !size) {
        return -1;
    }
    
    *size = 0;
    
    // Calculate size based on projected columns
    for (int i = 0; i < scan->projection_count; i++) {
        int col_idx = scan->projected_columns[i];
        
        if (col_idx < 0 || col_idx >= scan->schema->column_count) {
            return -1;
        }
        
        const ColumnDefinition* col = &scan->schema->columns[col_idx];
        
        switch (col->type) {
            case TYPE_INT:
                *size += sizeof(int);
                break;
            case TYPE_FLOAT:
                *size += sizeof(double);
                break;
            case TYPE_BOOLEAN:
                *size += sizeof(bool);
                break;
            case TYPE_VARCHAR:
                *size += col->length + 1;  // Include space for null terminator
                break;
            case TYPE_TEXT:
                *size += 4096;  // Fixed size for TEXT
                break;
            case TYPE_DATE:
                *size += sizeof(time_t);
                break;
            default:
                return -1;
        }
    }
    
    return 0;
}

/**
 * @brief Apply projection to record
 */
int apply_projection(const TableScan* scan, const void* source, void* destination) {
    if (!scan || !scan->schema || !source || !destination) {
        return -1;
    }
    
    // This is a simplified implementation
    // In a real implementation, we would need to know the memory layout
    // of both the source and destination structs
    
    // For now, just copy the entire record (no real projection)
    memcpy(destination, source, sizeof(void*));
    
    return 0;
}

/**
 * @brief Get a projected view of the current record
 */
int get_projected_record(const TableScan* scan, void* record) {
    if (!scan || !scan->initialized || !record) {
        return -1;
    }
    
    if (scan->at_end) {
        return 1;
    }
    
    return apply_projection(scan, scan->current_record, record);
}

/**
 * @brief Count records matching filter
 */
int count_matching_records(TableScan* scan, int* count) {
    if (!scan || !scan->initialized || !count) {
        return -1;
    }
    
    *count = 0;
    
    // Save current position
    TableCursor saved_cursor = scan->cursor;
    void* saved_record = scan->current_record;
    bool saved_at_end = scan->at_end;
    
    // Reset to beginning
    if (reset_table_scan(scan) != 0) {
        return -1;
    }
    
    // Count matching records
    while (next_matching_record(scan) == 0) {
        (*count)++;
    }
    
    // Restore original position
    scan->cursor = saved_cursor;
    scan->current_record = saved_record;
    scan->at_end = saved_at_end;
    
    return 0;
}

/**
 * @brief Create kernel for a table scan
 */
int create_scan_kernel(const TableScan* scan, char* kernel_name, size_t kernel_name_size) {
    if (!scan || !scan->schema || !kernel_name) {
        return -1;
    }
    
    // Create a synthetic SELECT statement to use existing kernel generator
    SelectStatement select_stmt;
    memset(&select_stmt, 0, sizeof(SelectStatement));
    
    // Set up FROM clause
    select_stmt.from_table = malloc(sizeof(TableRef));
    select_stmt.from_table->table_name = strdup(scan->table_name);
    select_stmt.from_table->alias = NULL;
    
    // Set up WHERE clause (reuse filter from scan)
    select_stmt.where_clause = scan->filter;
    
    // Set up SELECT list
    if (scan->projection_count == scan->schema->column_count) {
        // SELECT * case
        select_stmt.select_list.has_star = true;
    } else {
        // SELECT with specific columns
        select_stmt.select_list.expressions = malloc(scan->projection_count * sizeof(Expression*));
        
        for (int i = 0; i < scan->projection_count; i++) {
            int col_idx = scan->projected_columns[i];
            
            if (col_idx < 0 || col_idx >= scan->schema->column_count) {
                // Clean up
                free(select_stmt.from_table->table_name);
                free(select_stmt.from_table);
                free(select_stmt.select_list.expressions);
                return -1;
            }
            
            // Create a column reference expression
            Expression* expr = create_expression(AST_COLUMN_REF);
            expr->data.column.column_name = strdup(scan->schema->columns[col_idx].name);
            expr->data.column.table_name = NULL;
            expr->data.column.alias = NULL;
            
            select_stmt.select_list.expressions[i] = expr;
        }
        
        select_stmt.select_list.count = scan->projection_count;
        select_stmt.select_list.has_star = false;
    }
    
    // Generate kernel
    GeneratedKernel* kernel = generate_select_kernel(&select_stmt, scan->schema, scan->base_dir);
    
    if (!kernel) {
        // Clean up
        free(select_stmt.from_table->table_name);
        free(select_stmt.from_table);
        
        if (!select_stmt.select_list.has_star) {
            for (int i = 0; i < select_stmt.select_list.count; i++) {
                free_expression(select_stmt.select_list.expressions[i]);
            }
            free(select_stmt.select_list.expressions);
        }
        
        return -1;
    }
    
    // Compile kernel
    char kernel_path[2048];
    if (compile_kernel(kernel, scan->base_dir, scan->table_name, -1, 
                      kernel_path, sizeof(kernel_path)) != 0) {
        // Clean up
        free_generated_kernel(kernel);
        free(select_stmt.from_table->table_name);
        free(select_stmt.from_table);
        
        if (!select_stmt.select_list.has_star) {
            for (int i = 0; i < select_stmt.select_list.count; i++) {
                free_expression(select_stmt.select_list.expressions[i]);
            }
            free(select_stmt.select_list.expressions);
        }
        
        return -1;
    }
    
    // Copy kernel name
    strncpy(kernel_name, kernel->kernel_name, kernel_name_size - 1);
    kernel_name[kernel_name_size - 1] = '\0';
    
    // Clean up
    free_generated_kernel(kernel);
    free(select_stmt.from_table->table_name);
    free(select_stmt.from_table);
    
    if (!select_stmt.select_list.has_star) {
        for (int i = 0; i < select_stmt.select_list.count; i++) {
            free_expression(select_stmt.select_list.expressions[i]);
        }
        free(select_stmt.select_list.expressions);
    }
    
    return 0;
}

/**
 * @brief Execute a table scan with a compiled kernel
 */
int execute_kernel_scan(const TableScan* scan, void* results, int max_results, int* result_count) {
    if (!scan || !scan->schema || !results || !result_count) {
        return -1;
    }
    
    // Create and compile kernel
    char kernel_name[256];
    if (create_scan_kernel(scan, kernel_name, sizeof(kernel_name)) != 0) {
        return -1;
    }
    
    // Load kernel
    LoadedKernel loaded_kernel;
    char kernel_path[2048];
    
    // Construct kernel path
    snprintf(kernel_path, sizeof(kernel_path), "%s/compiled/%s_%s.so", 
             scan->base_dir, kernel_name, scan->table_name);
    
    if (load_kernel(kernel_path, kernel_name, scan->table_name, -1, &loaded_kernel) != 0) {
        return -1;
    }
    
    // Execute kernel on all pages
    *result_count = 0;
    int total_pages = 0;
    
    // Count table pages
    char compiled_dir[2048];
    snprintf(compiled_dir, sizeof(compiled_dir), "%s/compiled", scan->base_dir);
    
    DIR* dir = opendir(compiled_dir);
    if (!dir) {
        unload_kernel(&loaded_kernel);
        return -1;
    }
    
    // Look for page .so files for this table
    struct dirent* entry;
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "%sData_", scan->table_name);
    size_t pattern_len = strlen(pattern);
    
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, pattern, pattern_len) == 0 &&
            strstr(entry->d_name, ".so") != NULL) {
            total_pages++;
        }
    }
    
    closedir(dir);
    
    // Process each page
    for (int page_num = 0; page_num < total_pages; page_num++) {
        LoadedPage page;
        
        if (load_page(scan->base_dir, scan->table_name, page_num, &page) != 0) {
            continue;
        }
        
        // Get page record count
        int page_count;
        if (get_page_count(&page, &page_count) != 0 || page_count == 0) {
            unload_page(&page);
            continue;
        }
        
        // Get pointer to first record
        void* first_record;
        if (read_record(&page, 0, &first_record) != 0) {
            unload_page(&page);
            continue;
        }
        
        // Execute kernel on this page
        int page_results = execute_kernel(&loaded_kernel, first_record, page_count,
                                         (char*)results + *result_count, 
                                         max_results - *result_count);
        
        if (page_results < 0) {
            unload_page(&page);
            unload_kernel(&loaded_kernel);
            return -1;
        }
        
        *result_count += page_results;
        
        unload_page(&page);
        
        if (*result_count >= max_results) {
            break;
        }
    }
    
    unload_kernel(&loaded_kernel);
    return 0;
}

/**
 * @brief Apply filter to record
 */
bool apply_filter(const TableScan* scan, const void* record) {
    return evaluate_filter(scan, record);
}
