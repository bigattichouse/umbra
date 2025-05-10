/**
 * @file command_mode.c
 * @brief Handles command-line SQL execution
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "command_mode.h"
#include "result_formatter.h"
#include "../query/query_executor.h"

/**
 * @brief Read SQL from file
 */
static char* read_sql_file(const char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Cannot open file '%s'\n", filename);
        return NULL;
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    // Allocate buffer
    char* buffer = malloc(size + 1);
    if (!buffer) {
        fclose(file);
        fprintf(stderr, "Error: Memory allocation failed\n");
        return NULL;
    }
    
    // Read file
    size_t read_size = fread(buffer, 1, size, file);
    buffer[read_size] = '\0';
    
    fclose(file);
    return buffer;
}

/**
 * @brief Parse output format string
 */
static OutputFormat parse_output_format(const char* format_str) {
    if (strcmp(format_str, "csv") == 0) {
        return FORMAT_CSV;
    } else if (strcmp(format_str, "json") == 0) {
        return FORMAT_JSON;
    } else {
        return FORMAT_TABLE;
    }
}

/**
 * @brief Execute a single SQL statement
 */
static int execute_sql_statement(const char* sql, const char* database_path, OutputFormat format) {
    QueryResult* result = execute_query(sql, database_path);
    
    if (!result) {
        fprintf(stderr, "Error: Query execution failed\n");
        return 1;
    }
    
    if (!result->success) {
        fprintf(stderr, "Error: %s\n", result->error_message);
        free_query_result(result);
        return 1;
    }
    
    // Format and display results
    format_query_result(result, format);
    
    free_query_result(result);
    return 0;
}

/**
 * @brief Execute command line SQL
 */
int execute_command_mode(const char* database_path, const char* command, 
                        const char* file, const char* output_format) {
    OutputFormat format = parse_output_format(output_format);
    char* sql = NULL;
    
    if (command) {
        // Execute command directly
        sql = strdup(command);
    } else if (file) {
        // Read SQL from file
        sql = read_sql_file(file);
        if (!sql) {
            return 1;
        }
    } else {
        fprintf(stderr, "Error: No command or file specified\n");
        return 1;
    }
    
    // Execute SQL
    int result = execute_sql_statement(sql, database_path, format);
    
    free(sql);
    return result;
}
