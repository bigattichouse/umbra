/**
 * @file query_executor.c
 * @brief Executes queries using kernels
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>  // Added for va_start, va_end
#include "query_executor.h"
#include "select_executor.h"
#include "../parser/lexer.h"
#include "../parser/select_parser.h"
#include "../schema/metadata.h"
#include "../kernel/filter_generator.h"  // Added for validate_filter_expression
#include "../kernel/projection_generator.h"  // Added for validate_select_list

/**
 * @brief Create query result
 */
static QueryResult* create_query_result(void) {
    QueryResult* result = malloc(sizeof(QueryResult));
    result->rows = NULL;
    result->row_count = 0;
    result->result_schema = NULL;
    result->success = false;
    result->error_message = NULL;
    return result;
}

/**
 * @brief Set query error
 */
static void set_query_error(QueryResult* result, const char* format, ...) {
    result->success = false;
    
    va_list args;
    va_start(args, format);
    
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    
    result->error_message = strdup(buffer);
    
    va_end(args);
}

/**
 * @brief Load table schema from metadata
 */
static TableSchema* load_table_schema(const char* table_name, const char* base_dir) {
    // TODO: Use base_dir to load actual schema from metadata files
    (void)base_dir;  // Suppress unused parameter warning
    
    // For now, we'll create a simple placeholder schema
    // In a real implementation, this would read from schema files
    
    TableSchema* schema = malloc(sizeof(TableSchema));
    strncpy(schema->name, table_name, sizeof(schema->name) - 1);
    schema->name[sizeof(schema->name) - 1] = '\0';
    
    // TODO: Load actual schema from metadata files
    // For testing, create a simple schema
    schema->column_count = 3;
    schema->columns = malloc(3 * sizeof(ColumnDefinition));
    
    // Column 1: id (INT)
    strncpy(schema->columns[0].name, "id", sizeof(schema->columns[0].name) - 1);
    schema->columns[0].type = TYPE_INT;
    schema->columns[0].nullable = false;
    schema->columns[0].is_primary_key = true;
    
    // Column 2: name (VARCHAR)
    strncpy(schema->columns[1].name, "name", sizeof(schema->columns[1].name) - 1);
    schema->columns[1].type = TYPE_VARCHAR;
    schema->columns[1].length = 255;
    schema->columns[1].nullable = true;
    schema->columns[1].is_primary_key = false;
    
    // Column 3: age (INT)
    strncpy(schema->columns[2].name, "age", sizeof(schema->columns[2].name) - 1);
    schema->columns[2].type = TYPE_INT;
    schema->columns[2].nullable = true;
    schema->columns[2].is_primary_key = false;
    
    return schema;
}

/**
 * @brief Parse and validate query
 */
static SelectStatement* parse_query(const char* sql, const char* base_dir, 
                                   QueryResult* result) {
    // Initialize lexer
    Lexer lexer;
    lexer_init(&lexer, sql);
    
    // Initialize parser
    Parser parser;
    parser_init(&parser, &lexer);
    
    // Check for query type
    if (parser.current_token.type != TOKEN_SELECT) {
        set_query_error(result, "Only SELECT queries are supported currently");
        parser_free(&parser);
        lexer_free(&lexer);
        return NULL;
    }
    
    // Parse SELECT statement
    SelectStatement* stmt = parse_select_statement(&parser);
    
    if (!stmt) {
        set_query_error(result, "Failed to parse SELECT statement: %s", 
                       parser_get_error(&parser));
        parser_free(&parser);
        lexer_free(&lexer);
        return NULL;
    }
    
    // Load schema for validation
    TableSchema* schema = load_table_schema(stmt->from_table->table_name, base_dir);
    if (!schema) {
        set_query_error(result, "Table not found: %s", stmt->from_table->table_name);
        free_select_statement(stmt);
        parser_free(&parser);
        lexer_free(&lexer);
        return NULL;
    }
    
    // Validate SELECT list
    if (!validate_select_list(stmt, schema)) {
        set_query_error(result, "Invalid column in SELECT list");
        free_select_statement(stmt);
        free_table_schema(schema);
        parser_free(&parser);
        lexer_free(&lexer);
        return NULL;
    }
    
    // Validate WHERE clause if present
    if (stmt->where_clause && !validate_filter_expression(stmt->where_clause, schema)) {
        set_query_error(result, "Invalid WHERE clause");
        free_select_statement(stmt);
        free_table_schema(schema);
        parser_free(&parser);
        lexer_free(&lexer);
        return NULL;
    }
    
    free_table_schema(schema);
    parser_free(&parser);
    lexer_free(&lexer);
    
    return stmt;
}

/**
 * @brief Execute a SQL query
 */
QueryResult* execute_query(const char* sql, const char* base_dir) {
    QueryResult* result = create_query_result();
    
    // Parse query
    SelectStatement* stmt = parse_query(sql, base_dir, result);
    if (!stmt) {
        return result;
    }
    
    // Execute SELECT statement
    if (execute_select(stmt, base_dir, result) != 0) {
        // Error message already set by execute_select
        free_select_statement(stmt);
        return result;
    }
    
    result->success = true;
    free_select_statement(stmt);
    return result;
}

/**
 * @brief Free query result
 */
void free_query_result(QueryResult* result) {
    if (!result) return;
    
    // Free rows
    if (result->rows) {
        for (int i = 0; i < result->row_count; i++) {
            free(result->rows[i]);
        }
        free(result->rows);
    }
    
    // Free schema
    if (result->result_schema) {
        free_table_schema(result->result_schema);
    }
    
    // Free error message
    free(result->error_message);
    
    free(result);
}

/**
 * @brief Print query result to stdout
 */
void print_query_result(const QueryResult* result) {
    if (!result) {
        printf("NULL result\n");
        return;
    }
    
    if (!result->success) {
        printf("Query failed: %s\n", result->error_message);
        return;
    }
    
    printf("Query succeeded: %d rows\n", result->row_count);
    
    // TODO: Implement proper result formatting
    // For now, just print row count
}
