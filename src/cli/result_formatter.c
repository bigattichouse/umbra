/**
 * @file result_formatter.c
 * @brief Formats query results as tables/CSV/JSON
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "result_formatter.h"

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
    
    // TODO: Check actual data widths
    // For now, use fixed minimum width
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
    // TODO: Actually print row data
    printf("| %d rows returned\n", result->row_count);
    
    print_table_separator(widths, column_count);
    
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
    
    // TODO: Print actual row data
    printf("# %d rows returned\n", result->row_count);
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
    
    // TODO: Print actual row data
    printf("    // %d rows of data\n", result->row_count);
    
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
