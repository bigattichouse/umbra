/**
 * @file page_generator.c
 * @brief Generates data files and page headers
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include "page_generator.h"
#include "accessor_generator.h"
#include "compile_scripts.h"
#include "../schema/schema_generator.h"
#include "../schema/directory_manager.h"

/**
 * @brief Generate a new data page for a table
 */
int generate_data_page(const TableSchema* schema, const char* base_dir, int page_number) {
    if (!schema || !base_dir) {
        return -1;
    }
    
    // Get table directory
    char table_dir[1024];
    if (get_table_directory(schema->name, base_dir, table_dir, sizeof(table_dir)) != 0) {
        return -1;
    }
    
    // Generate empty data page header file
    if (generate_empty_data_page(schema, table_dir, page_number) != 0) {
        fprintf(stderr, "Failed to generate empty data page\n");
        return -1;
    }
    
    // Generate accessor functions
    if (generate_accessor_file(schema, base_dir, page_number) != 0) {
        fprintf(stderr, "Failed to generate accessor functions\n");
        return -1;
    }
    
    // Generate compilation script
    if (generate_compilation_script(schema, base_dir, page_number) != 0) {
        fprintf(stderr, "Failed to generate compilation script\n");
        return -1;
    }
    
    return 0;
}

/**
 * @brief Add record to a data page
 */
/**
 * @brief Add record to a data page
 */
int add_record_to_page(const TableSchema* schema, const char* base_dir, 
                       int page_number, const char** values) {
    if (!schema || !base_dir || !values) {
        return -1;
    }
    
    // Get data directory path
    char data_dir[1024];
    if (get_data_directory(schema->name, base_dir, data_dir, sizeof(data_dir)) != 0) {
        return -1;
    }
    
    // Create data file path
    char data_path[2048];
    snprintf(data_path, sizeof(data_path), "%s/%sData.%d.dat.h", 
             data_dir, schema->name, page_number);
    
    // Open data file for appending
    FILE* data_file = fopen(data_path, "a");
    if (!data_file) {
        fprintf(stderr, "Failed to open %s for appending: %s\n", data_path, strerror(errno));
        return -1;
    }
    
    // Write the record
    fprintf(data_file, "{");
    
    // Add each field
    for (int i = 0; i < schema->column_count; i++) {
        const ColumnDefinition* col = &schema->columns[i];
        const char* value = values[i];
        
        // Handle NULL values
        if (value == NULL || strcmp(value, "NULL") == 0) {
            // For strings, use empty string; for numbers, use 0; for booleans, use false
            switch (col->type) {
                case TYPE_VARCHAR:
                case TYPE_TEXT:
                    fprintf(data_file, "\"\"");
                    break;
                case TYPE_INT:
                    fprintf(data_file, "0");
                    break;
                case TYPE_FLOAT:
                    fprintf(data_file, "0.0");
                    break;
                case TYPE_BOOLEAN:
                    fprintf(data_file, "false");
                    break;
                case TYPE_DATE:
                    fprintf(data_file, "0");
                    break;
                default:
                    fprintf(data_file, "0");
                    break;
            }
        } else {
            // Quote string values
            if (col->type == TYPE_VARCHAR || col->type == TYPE_TEXT) {
                fprintf(data_file, "\"%s\"", value);
            } else if (col->type == TYPE_BOOLEAN) {
                // Convert boolean to lowercase
                if (strcmp(value, "true") == 0 || strcmp(value, "TRUE") == 0 || strcmp(value, "1") == 0) {
                    fprintf(data_file, "true");
                } else {
                    fprintf(data_file, "false");
                }
            } else if (col->type == TYPE_INT) {
                // Ensure integer format (no decimal point)
                int int_val = atoi(value);
                fprintf(data_file, "%d", int_val);
            } else if (col->type == TYPE_FLOAT) {
                // Ensure float format
                double float_val = atof(value);
                fprintf(data_file, "%f", float_val);
            } else {
                // Other types are written as-is
                fprintf(data_file, "%s", value);
            }
        }
        
        // Add comma except for last field
        if (i < schema->column_count - 1) {
            fprintf(data_file, ", ");
        }
    }
    
    fprintf(data_file, "},\n");
    
    // Close file
    fclose(data_file);
    return 0;
}

/**
 * @brief Count the number of records in a data page file
 */
static int count_records_in_file(const char* data_path) {
    FILE* file = fopen(data_path, "r");
    if (!file) {
        // File doesn't exist, return 0
        return 0;
    }
    
    int count = 0;
    char line[4096];
    
    while (fgets(line, sizeof(line), file)) {
        // Count lines that end with "},\n"
        size_t len = strlen(line);
        if (len >= 3 && line[len-3] == '}' && line[len-2] == ',' && line[len-1] == '\n') {
            count++;
        }
    }
    
    fclose(file);
    return count;
}

/**
 * @brief Check if a page is full
 */
int is_page_full(const TableSchema* schema, const char* base_dir, 
                int page_number, int max_records, bool* is_full) {
    if (!schema || !base_dir || !is_full) {
        return -1;
    }
    
    // Get data directory path
    char data_dir[1024];
    if (get_data_directory(schema->name, base_dir, data_dir, sizeof(data_dir)) != 0) {
        return -1;
    }
    
    // Create data file path
    char data_path[2048];
    snprintf(data_path, sizeof(data_path), "%s/%sData.%d.dat.h", 
             data_dir, schema->name, page_number);
    
    // Count records in the file
    int record_count = count_records_in_file(data_path);
    
    *is_full = (record_count >= max_records);
    return 0;
}

/**
 * @brief Get the number of records in a page
 */
int get_page_record_count(const TableSchema* schema, const char* base_dir, 
                         int page_number, int* record_count) {
    if (!schema || !base_dir || !record_count) {
        return -1;
    }
    
    // Get data directory path
    char data_dir[1024];
    if (get_data_directory(schema->name, base_dir, data_dir, sizeof(data_dir)) != 0) {
        return -1;
    }
    
    // Create data file path
    char data_path[2048];
    snprintf(data_path, sizeof(data_path), "%s/%sData.%d.dat.h", 
             data_dir, schema->name, page_number);
    
    // Count records in the file
    *record_count = count_records_in_file(data_path);
    return 0;
}

/**
 * @brief Recompile a data page
 */
int recompile_data_page(const TableSchema* schema, const char* base_dir, int page_number) {
    if (!schema || !base_dir) {
        return -1;
    }
    
    // Get compile script path
    char script_path[1024];
    if (get_compile_script_path(schema->name, base_dir, page_number, 
                               script_path, sizeof(script_path)) != 0) {
        fprintf(stderr, "Failed to get compile script path\n");
        return -1;
    }
    
    // Execute the compilation script
    char command[2048];  // Increased to ensure enough space
    snprintf(command, sizeof(command), "bash %s", script_path);
    
    int result = system(command);
    if (result != 0) {
        fprintf(stderr, "Failed to execute compilation script: %s\n", script_path);
        return -1;
    }
    
    return 0;
}
