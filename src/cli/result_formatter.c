/**
 * @file result_formatter.c
 * @brief Formats query results as tables/CSV/JSON
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "result_formatter.h"

/**
 * @brief Get string representation of a field value
 */
static void get_field_string(const QueryResult* result, void* row, int col_idx, 
                            char* buffer, size_t buffer_size) {
    if (!result || !result->result_schema || !row || !buffer || buffer_size <= 0) {
        buffer[0] = '\0';
        return;
    }
    
    const ColumnDefinition* col = &result->result_schema->columns[col_idx];
    void* field_value = NULL;
    
    // Calculate the field offset - this is a simplified approach
    // In reality, we'd need to know the exact memory layout of the struct
    char* record = (char*)row;
    
    // Get field based on type - this is a simplified approach
    // We're assuming the struct fields are in the same order as the schema columns
    // In a real implementation, we would need to know the exact layout
    switch (col->type) {
        case TYPE_INT: {
            int value = *(int*)record;
            snprintf(buffer, buffer_size, "%d", value);
            break;
        }
        case TYPE_FLOAT: {
            double value = *(double*)record;
            snprintf(buffer, buffer_size, "%.6f", value);
            break;
        }
        case TYPE_VARCHAR:
        case TYPE_TEXT: {
            const char* value = (const char*)record;
            if (value) {
                strncpy(buffer, value, buffer_size - 1);
                buffer[buffer_size - 1] = '\0';
            } else {
                strncpy(buffer, "NULL", buffer_size - 1);
                buffer[buffer_size - 1] = '\0';
            }
            break;
        }
        case TYPE_BOOLEAN: {
            bool value = *(bool*)record;
            strncpy(buffer, value ? "true" : "false", buffer_size - 1);
            buffer[buffer_size - 1] = '\0';
            break;
        }
        case TYPE_DATE: {
            time_t value = *(time_t*)record;
            struct tm* tm_info = localtime(&value);
            if (tm_info) {
                strftime(buffer, buffer_size, "%Y-%m-%d", tm_info);
            } else {
                strncpy(buffer, "INVALID_DATE", buffer_size - 1);
                buffer[buffer_size - 1] = '\0';
            }
            break;
        }
        default:
            strncpy(buffer, "UNKNOWN_TYPE", buffer_size - 1);
            buffer[buffer_size - 1] = '\0';
            break;
    }
}

/**
 * @brief Calculate column widths for table formatting
 */
static void calculate_column_widths(const QueryResult* result, int* widths) {
    if (!result || !result->result_schema || !widths) {
        return;
    }
    
    // Start with column name lengths
    for (int i = 0; i < result->result_schema->column_count; i++) {
        widths[i] = strlen(result->result_schema->columns[i].name);
    }
    
    // Check actual data widths
    char buffer[1024];
    for (int row = 0; row < result->row_count; row++) {
        for (int col = 0; col < result->result_schema->column_count; col++) {
            get_field_string(result, result->rows[row], col, buffer, sizeof(buffer));
            int len = strlen(buffer);
            if (len > widths[col]) {
                widths[col] = len;
            }
        }
    }
    
    // Ensure a minimum width
    for (int i = 0; i < result->result_schema->column_count; i++) {
        if (widths[i] < 10) {
            widths[i] = 10;
        }
    }
}

/**
 * @brief Print table separator
 */
static void print_table_separator(int* widths, int column_count) {
    printf("+");
    for (int i = 0; i < column_count; i++) {
        for (int j = 0; j < widths[i] + 2; j++) {
            printf("-");
        }
        printf("+");
    }
    printf("\n");
}

/**
 * @brief Format result as ASCII table
 */
void format_as_table(const QueryResult* result) {
    if (!result || !result->result_schema) {
        printf("No results to display\n");
        return;
    }
    
    int column_count = result->result_schema->column_count;
    int* widths = malloc(column_count * sizeof(int));
    
    calculate_column_widths(result, widths);
    
    // Print header
    print_table_separator(widths, column_count);
    
    printf("|");
    for (int i = 0; i < column_count; i++) {
        printf(" %-*s |", widths[i], result->result_schema->columns[i].name);
    }
    printf("\n");
    
    print_table_separator(widths, column_count);
    
    // Print rows
    char buffer[1024];
    for (int row = 0; row < result->row_count; row++) {
        printf("|");
        for (int col = 0; col < column_count; col++) {
            get_field_string(result, result->rows[row], col, buffer, sizeof(buffer));
            
            // Print field with proper padding
            if (result->result_schema->columns[col].type == TYPE_INT ||
                result->result_schema->columns[col].type == TYPE_FLOAT) {
                // Right align numbers
                printf(" %*s |", widths[col], buffer);
            } else {
                // Left align text
                printf(" %-*s |", widths[col], buffer);
            }
        }
        printf("\n");
    }
    
    print_table_separator(widths, column_count);
    
    // Print row count summary
    printf("%d row%s returned\n", result->row_count, result->row_count == 1 ? "" : "s");
    
    free(widths);
}

/**
 * @brief Escape CSV value
 */
static void print_csv_value(const char* value) {
    if (!value) {
        return;
    }
    
    // Check if value needs quotes
    bool needs_quotes = strchr(value, ',') != NULL || 
                       strchr(value, '"') != NULL || 
                       strchr(value, '\n') != NULL;
    
    if (needs_quotes) {
        printf("\"");
        for (const char* p = value; *p; p++) {
            if (*p == '"') {
                printf("\"\"");
            } else {
                printf("%c", *p);
            }
        }
        printf("\"");
    } else {
        printf("%s", value);
    }
}

/**
 * @brief Format result as CSV
 */
void format_as_csv(const QueryResult* result) {
    if (!result || !result->result_schema) {
        return;
    }
    
    int column_count = result->result_schema->column_count;
    
    // Print header
    for (int i = 0; i < column_count; i++) {
        if (i > 0) printf(",");
        print_csv_value(result->result_schema->columns[i].name);
    }
    printf("\n");
    
    // Print rows
    char buffer[1024];
    for (int row = 0; row < result->row_count; row++) {
        for (int col = 0; col < column_count; col++) {
            if (col > 0) printf(",");
            
            get_field_string(result, result->rows[row], col, buffer, sizeof(buffer));
            print_csv_value(buffer);
        }
        printf("\n");
    }
}

/**
 * @brief Escape JSON string
 */
static void print_json_string(const char* str) {
    printf("\"");
    for (const char* p = str; p && *p; p++) {
        switch (*p) {
            case '\\': printf("\\\\"); break;
            case '\"': printf("\\\""); break;
            case '\b': printf("\\b"); break;
            case '\f': printf("\\f"); break;
            case '\n': printf("\\n"); break;
            case '\r': printf("\\r"); break;
            case '\t': printf("\\t"); break;
            default:
                if (*p < 32) {
                    printf("\\u%04x", *p);
                } else {
                    putchar(*p);
                }
        }
    }
    printf("\"");
}

/**
 * @brief Format result as JSON
 */
void format_as_json(const QueryResult* result) {
    if (!result || !result->result_schema) {
        printf("null\n");
        return;
    }
    
    printf("{\n");
    printf("  \"rows\": %d,\n", result->row_count);
    printf("  \"columns\": [\n");
    
    // Print column definitions
    for (int i = 0; i < result->result_schema->column_count; i++) {
        printf("    {\n");
        printf("      \"name\": \"%s\",\n", result->result_schema->columns[i].name);
        printf("      \"type\": \"%s\"\n", get_type_info(result->result_schema->columns[i].type).name);
        printf("    }");
        
        if (i < result->result_schema->column_count - 1) {
            printf(",");
        }
        printf("\n");
    }
    
    printf("  ],\n");
    printf("  \"data\": [\n");
    
    // Print row data
    char buffer[1024];
    for (int row = 0; row < result->row_count; row++) {
        printf("    {");
        for (int col = 0; col < result->result_schema->column_count; col++) {
            printf("\n      ");
            print_json_string(result->result_schema->columns[col].name);
            printf(": ");
            
            get_field_string(result, result->rows[row], col, buffer, sizeof(buffer));
            
            // Format based on column type
            switch (result->result_schema->columns[col].type) {
                case TYPE_INT:
                case TYPE_FLOAT:
                case TYPE_BOOLEAN:
                    // Numeric and boolean values without quotes
                    printf("%s", buffer);
                    break;
                default:
                    // String values with quotes
                    print_json_string(buffer);
                    break;
            }
            
            if (col < result->result_schema->column_count - 1) {
                printf(",");
            }
        }
        printf("\n    }");
        
        if (row < result->row_count - 1) {
            printf(",");
        }
        printf("\n");
    }
    
    printf("  ]\n");
    printf("}\n");
}

/**
 * @brief Format and display query results
 */
void format_query_result(const QueryResult* result, OutputFormat format) {
    switch (format) {
        case FORMAT_TABLE:
            format_as_table(result);
            break;
        case FORMAT_CSV:
            format_as_csv(result);
            break;
        case FORMAT_JSON:
            format_as_json(result);
            break;
        default:
            format_as_table(result);
            break;
    }
}
