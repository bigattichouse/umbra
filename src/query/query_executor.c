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
#include "../parser/insert_parser.h"
#include "../parser/update_parser.h"
#include "../parser/delete_parser.h"
#include "../query/insert_executor.h"
#include "../query/update_executor.h"
#include "../query/delete_executor.h"
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
static ASTNode* parse_query(const char* sql, const char* base_dir, 
                           QueryResult* result) {
    // Initialize lexer
    Lexer lexer;
    lexer_init(&lexer, sql);
    
    // Initialize parser
    Parser parser;
    parser_init(&parser, &lexer);
    
    // Check for query type
    void* parsed_stmt = NULL;
    ASTNodeType stmt_type;
    
    if (parser.current_token.type == TOKEN_SELECT) {
        // Parse SELECT statement
        SelectStatement* stmt = parse_select_statement(&parser);
        
        if (!stmt) {
            set_query_error(result, "Failed to parse SELECT statement: %s", 
                           parser_get_error(&parser));
            parser_free(&parser);
            lexer_free(&lexer);
            return NULL;
        }
        
        parsed_stmt = stmt;
        stmt_type = AST_SELECT_STATEMENT;
    } else if (parser.current_token.type == TOKEN_INSERT) {
        // Parse INSERT statement
        InsertStatement* stmt = parse_insert_statement(&parser);
        
        if (!stmt) {
            set_query_error(result, "Failed to parse INSERT statement: %s", 
                           parser_get_error(&parser));
            parser_free(&parser);
            lexer_free(&lexer);
            return NULL;
        }
        
        parsed_stmt = stmt;
        stmt_type = AST_INSERT_STATEMENT;
    } else if (parser.current_token.type == TOKEN_UPDATE) {
        // Parse UPDATE statement
        UpdateStatement* stmt = parse_update_statement(&parser);
        
        if (!stmt) {
            set_query_error(result, "Failed to parse UPDATE statement: %s", 
                           parser_get_error(&parser));
            parser_free(&parser);
            lexer_free(&lexer);
            return NULL;
        }
        
        parsed_stmt = stmt;
        stmt_type = AST_UPDATE_STATEMENT;
    } else if (parser.current_token.type == TOKEN_DELETE) {
        // Parse DELETE statement
        DeleteStatement* stmt = parse_delete_statement(&parser);
        
        if (!stmt) {
            set_query_error(result, "Failed to parse DELETE statement: %s", 
                           parser_get_error(&parser));
            parser_free(&parser);
            lexer_free(&lexer);
            return NULL;
        }
        
        parsed_stmt = stmt;
        stmt_type = AST_DELETE_STATEMENT;
    } else {
        set_query_error(result, "Unsupported SQL statement");
        parser_free(&parser);
        lexer_free(&lexer);
        return NULL;
    }
    
    if (stmt_type == AST_SELECT_STATEMENT) {
        SelectStatement* select_stmt = (SelectStatement*)parsed_stmt;
        
        // Load schema for validation
        TableSchema* schema = load_table_schema(select_stmt->from_table->table_name, base_dir);
        if (!schema) {
            set_query_error(result, "Table not found: %s", select_stmt->from_table->table_name);
            free_select_statement(select_stmt);
            parser_free(&parser);
            lexer_free(&lexer);
            return NULL;
        }
        
        // Validate SELECT list
        if (!validate_select_list(select_stmt, schema)) {
            set_query_error(result, "Invalid column in SELECT list");
            free_select_statement(select_stmt);
            free_table_schema(schema);
            parser_free(&parser);
            lexer_free(&lexer);
            return NULL;
        }
        
        // Validate WHERE clause if present
        if (select_stmt->where_clause && !validate_filter_expression(select_stmt->where_clause, schema)) {
            set_query_error(result, "Invalid WHERE clause");
            free_select_statement(select_stmt);
            free_table_schema(schema);
            parser_free(&parser);
            lexer_free(&lexer);
            return NULL;
        }
        
        free_table_schema(schema);
    } else if (stmt_type == AST_INSERT_STATEMENT) {
        InsertStatement* insert_stmt = (InsertStatement*)parsed_stmt;
        
        // Load schema for validation
        TableSchema* schema = load_table_schema(insert_stmt->table_name, base_dir);
        if (!schema) {
            set_query_error(result, "Table not found: %s", insert_stmt->table_name);
            free_insert_statement(insert_stmt);
            parser_free(&parser);
            lexer_free(&lexer);
            return NULL;
        }
        
        // Validate INSERT statement
        if (!validate_insert_statement(insert_stmt, schema)) {
            set_query_error(result, "Invalid INSERT statement");
            free_insert_statement(insert_stmt);
            free_table_schema(schema);
            parser_free(&parser);
            lexer_free(&lexer);
            return NULL;
        }
        
        free_table_schema(schema);
    } else if (stmt_type == AST_UPDATE_STATEMENT) {
        UpdateStatement* update_stmt = (UpdateStatement*)parsed_stmt;
        
        // Load schema for validation
        TableSchema* schema = load_table_schema(update_stmt->table_name, base_dir);
        if (!schema) {
            set_query_error(result, "Table not found: %s", update_stmt->table_name);
            free_update_statement(update_stmt);
            parser_free(&parser);
            lexer_free(&lexer);
            return NULL;
        }
        
        // Validate UPDATE statement
        if (!validate_update_statement(update_stmt, schema)) {
            set_query_error(result, "Invalid UPDATE statement");
            free_update_statement(update_stmt);
            free_table_schema(schema);
            parser_free(&parser);
            lexer_free(&lexer);
            return NULL;
        }
        
        free_table_schema(schema);
    } else if (stmt_type == AST_DELETE_STATEMENT) {
        DeleteStatement* delete_stmt = (DeleteStatement*)parsed_stmt;
        
        // Load schema for validation
        TableSchema* schema = load_table_schema(delete_stmt->table_name, base_dir);
        if (!schema) {
            set_query_error(result, "Table not found: %s", delete_stmt->table_name);
            free_delete_statement(delete_stmt);
            parser_free(&parser);
            lexer_free(&lexer);
            return NULL;
        }
        
        // Validate DELETE statement
        if (!validate_delete_statement(delete_stmt, schema)) {
            set_query_error(result, "Invalid DELETE statement");
            free_delete_statement(delete_stmt);
            free_table_schema(schema);
            parser_free(&parser);
            lexer_free(&lexer);
            return NULL;
        }
        
        free_table_schema(schema);
    }
    
    parser_free(&parser);
    lexer_free(&lexer);
    
    // Return the appropriate statement wrapped in a structure
    ASTNode* node = malloc(sizeof(ASTNode));
    node->type = stmt_type;
    node->data.select_stmt = NULL;
    node->data.insert_stmt = NULL;
    node->data.update_stmt = NULL;
    node->data.delete_stmt = NULL;
    
    switch (stmt_type) {
        case AST_SELECT_STATEMENT:
            node->data.select_stmt = (SelectStatement*)parsed_stmt;
            break;
        case AST_INSERT_STATEMENT:
            node->data.insert_stmt = (InsertStatement*)parsed_stmt;
            break;
        case AST_UPDATE_STATEMENT:
            node->data.update_stmt = (UpdateStatement*)parsed_stmt;
            break;
        case AST_DELETE_STATEMENT:
            node->data.delete_stmt = (DeleteStatement*)parsed_stmt;
            break;
        default:
            free(node);
            return NULL;
    }
    
    return node;
}

/**
 * @brief Execute a SQL query
 */
QueryResult* execute_query(const char* sql, const char* base_dir) {
    QueryResult* result = create_query_result();
    
    // Parse query
    ASTNode* node = parse_query(sql, base_dir, result);
    if (!node) {
        return result;
    }
    
    // Execute based on statement type
    switch (node->type) {
        case AST_SELECT_STATEMENT: {
            // Execute SELECT statement
            if (execute_select(node->data.select_stmt, base_dir, result) != 0) {
                // Error message already set by execute_select
            } else {
                result->success = true;
            }
            free_select_statement(node->data.select_stmt);
            break;
        }
        
        case AST_INSERT_STATEMENT: {
            // Execute INSERT statement
            InsertResult* insert_result = execute_insert(node->data.insert_stmt, base_dir);
            
            if (insert_result->success) {
                result->success = true;
                result->row_count = 1;
                
                // Create a simple result set for INSERT
                result->result_schema = malloc(sizeof(TableSchema));
                result->result_schema->column_count = 1;
                result->result_schema->columns = malloc(sizeof(ColumnDefinition));
                strncpy(result->result_schema->columns[0].name, "rows_affected", 
                       sizeof(result->result_schema->columns[0].name) - 1);
                result->result_schema->columns[0].type = TYPE_INT;
                
                // Create result row
                result->rows = malloc(sizeof(void*));
                int* row_data = malloc(sizeof(int));
                *row_data = insert_result->rows_affected;
                result->rows[0] = row_data;
                result->row_count = 1;
            } else {
                result->error_message = strdup(insert_result->error_message);
            }
            
            free_insert_result(insert_result);
            free_insert_statement(node->data.insert_stmt);
            break;
        }
        
        case AST_UPDATE_STATEMENT: {
            // Execute UPDATE statement
            UpdateResult* update_result = execute_update(node->data.update_stmt, base_dir);
            
            if (update_result->success) {
                result->success = true;
                result->row_count = 1;
                
                // Create a simple result set for UPDATE
                result->result_schema = malloc(sizeof(TableSchema));
                result->result_schema->column_count = 1;
                result->result_schema->columns = malloc(sizeof(ColumnDefinition));
                strncpy(result->result_schema->columns[0].name, "rows_affected", 
                       sizeof(result->result_schema->columns[0].name) - 1);
                result->result_schema->columns[0].type = TYPE_INT;
                
                // Create result row
                result->rows = malloc(sizeof(void*));
                int* row_data = malloc(sizeof(int));
                *row_data = update_result->rows_affected;
                result->rows[0] = row_data;
                result->row_count = 1;
            } else {
                result->error_message = strdup(update_result->error_message);
            }
            
            free_update_result(update_result);
            free_update_statement(node->data.update_stmt);
            break;
        }
        
        case AST_DELETE_STATEMENT: {
            // Execute DELETE statement
            DeleteResult* delete_result = execute_delete(node->data.delete_stmt, base_dir);
            
            if (delete_result->success) {
                result->success = true;
                result->row_count = 1;
                
                // Create a simple result set for DELETE
                result->result_schema = malloc(sizeof(TableSchema));
                result->result_schema->column_count = 1;
                result->result_schema->columns = malloc(sizeof(ColumnDefinition));
                strncpy(result->result_schema->columns[0].name, "rows_affected", 
                       sizeof(result->result_schema->columns[0].name) - 1);
                result->result_schema->columns[0].type = TYPE_INT;
                
                // Create result row
                result->rows = malloc(sizeof(void*));
                int* row_data = malloc(sizeof(int));
                *row_data = delete_result->rows_affected;
                result->rows[0] = row_data;
                result->row_count = 1;
            } else {
                result->error_message = strdup(delete_result->error_message);
            }
            
            free_delete_result(delete_result);
            free_delete_statement(node->data.delete_stmt);
            break;
        }
        
        default:
            set_query_error(result, "Unsupported statement type");
            break;
    }
    
    free(node);
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
