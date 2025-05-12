/**
 * @file query_executor.c
 * @brief Executes queries using kernels
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <dirent.h>
#include "query_executor.h"
#include "select_executor.h"
#include "create_table_executor.h"
#include "../parser/lexer.h"
#include "../parser/select_parser.h"
#include "../parser/insert_parser.h"
#include "../parser/update_parser.h"
#include "../parser/delete_parser.h"
#include "../query/insert_executor.h"
#include "../query/update_executor.h"
#include "../query/delete_executor.h"
#include "../schema/metadata.h"
#include "../schema/directory_manager.h"
#include "../kernel/filter_generator.h"
#include "../kernel/projection_generator.h"
#include "../index/index_manager.h"  // For execute_create_index and free_create_index_result

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
    result->row_format = ROW_FORMAT_DIRECT; // Default to direct format
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
TableSchema* load_table_schema(const char* table_name, const char* base_dir) {
    // First, check if the table directory exists
    if (!table_directory_exists(table_name, base_dir)) {
        fprintf(stderr, "Table '%s' does not exist\n", table_name);
        return NULL;
    }
    
    // Try to load from JSON metadata file first
    TableSchema* schema = load_schema_metadata(table_name, base_dir);
    if (schema) {
        return schema;
    }
    
    // If that fails, return NULL - don't use mock data
    fprintf(stderr, "Failed to load schema metadata for table '%s'\n", table_name);
    return NULL;
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
    TableSchema* schema = NULL;  // Track schema to ensure proper cleanup
    
    if (strncasecmp(sql, "CREATE TABLE", 12) == 0) {
        // For CREATE TABLE, we don't need the full parser yet
        CreateTableResult* create_result = execute_create_table(sql, base_dir);
        
        if (!create_result->success) {
            set_query_error(result, "CREATE TABLE failed: %s", create_result->error_message);
            free_create_table_result(create_result);
            parser_free(&parser);
            lexer_free(&lexer);
            return NULL;
        }
        
        // Set success in query result
        result->success = true;
        result->row_count = 0;
        result->error_message = strdup("Table created successfully");
        
        free_create_table_result(create_result);
        parser_free(&parser);
        lexer_free(&lexer);
        return NULL;  // No AST node needed for CREATE TABLE
    } else if (strncasecmp(sql, "CREATE INDEX", 12) == 0) {
    // Handle CREATE INDEX statement
    CreateIndexResult* index_result = execute_create_index(sql, base_dir);
    
    if (!index_result->success) {
        set_query_error(result, "CREATE INDEX failed: %s", index_result->error_message);
        free_create_index_result(index_result);
        parser_free(&parser);
        lexer_free(&lexer);
        return NULL;
    }
    
    // Set success in query result
    result->success = true;
    result->row_count = 0;
    result->error_message = strdup("Index created successfully");
    
    free_create_index_result(index_result);
    parser_free(&parser);
    lexer_free(&lexer);
    return NULL;  // No AST node needed for CREATE INDEX
}
    
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
        
        // Load schema for validation
        schema = load_table_schema(stmt->from_table->table_name, base_dir);
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
        schema = NULL;
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
        
        // Load schema for validation
        schema = load_table_schema(stmt->table_name, base_dir);
        if (!schema) {
            set_query_error(result, "Table not found: %s", stmt->table_name);
            free_insert_statement(stmt);
            parser_free(&parser);
            lexer_free(&lexer);
            return NULL;
        }
        
        // Validate INSERT statement
        if (!validate_insert_statement(stmt, schema)) {
            set_query_error(result, "Invalid INSERT statement");
            free_insert_statement(stmt);
            free_table_schema(schema);
            parser_free(&parser);
            lexer_free(&lexer);
            return NULL;
        }
        
        free_table_schema(schema);
        schema = NULL;
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
        
        // Load schema for validation
        schema = load_table_schema(stmt->table_name, base_dir);
        if (!schema) {
            set_query_error(result, "Table not found: %s", stmt->table_name);
            free_update_statement(stmt);
            parser_free(&parser);
            lexer_free(&lexer);
            return NULL;
        }
        
        // Validate UPDATE statement
        if (!validate_update_statement(stmt, schema)) {
            set_query_error(result, "Invalid UPDATE statement");
            free_update_statement(stmt);
            free_table_schema(schema);
            parser_free(&parser);
            lexer_free(&lexer);
            return NULL;
        }
        
        free_table_schema(schema);
        schema = NULL;
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
        
        // Load schema for validation
        schema = load_table_schema(stmt->table_name, base_dir);
        if (!schema) {
            set_query_error(result, "Table not found: %s", stmt->table_name);
            free_delete_statement(stmt);
            parser_free(&parser);
            lexer_free(&lexer);
            return NULL;
        }
        
        // Validate DELETE statement
        if (!validate_delete_statement(stmt, schema)) {
            set_query_error(result, "Invalid DELETE statement");
            free_delete_statement(stmt);
            free_table_schema(schema);
            parser_free(&parser);
            lexer_free(&lexer);
            return NULL;
        }
        
        free_table_schema(schema);
        schema = NULL;
    } else {
        set_query_error(result, "Unsupported SQL statement");
        parser_free(&parser);
        lexer_free(&lexer);
        return NULL;
    }
    
    parser_free(&parser);
    lexer_free(&lexer);
    
    // Return the appropriate statement wrapped in a structure
    ASTNode* node = malloc(sizeof(ASTNode));
    if (!node) {
        // Free the parsed statement
        switch (stmt_type) {
            case AST_SELECT_STATEMENT:
                free_select_statement((SelectStatement*)parsed_stmt);
                break;
            case AST_INSERT_STATEMENT:
                free_insert_statement((InsertStatement*)parsed_stmt);
                break;
            case AST_UPDATE_STATEMENT:
                free_update_statement((UpdateStatement*)parsed_stmt);
                break;
            case AST_DELETE_STATEMENT:
                free_delete_statement((DeleteStatement*)parsed_stmt);
                break;
            default:
                break;
        }
        return NULL;
    }
    
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
        // For CREATE TABLE, result is already set
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
                result->result_schema->name[0] = '\0';
                result->result_schema->column_count = 1;
                result->result_schema->columns = malloc(sizeof(ColumnDefinition));
                strncpy(result->result_schema->columns[0].name, "rows_affected", 
                       sizeof(result->result_schema->columns[0].name) - 1);
                result->result_schema->columns[0].name[sizeof(result->result_schema->columns[0].name) - 1] = '\0';
                result->result_schema->columns[0].type = TYPE_INT;
                result->result_schema->columns[0].nullable = false;
                result->result_schema->columns[0].is_primary_key = false;
                result->result_schema->primary_key_columns = NULL;
                result->result_schema->primary_key_column_count = 0;
                
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
                result->result_schema->name[0] = '\0';
                result->result_schema->column_count = 1;
                result->result_schema->columns = malloc(sizeof(ColumnDefinition));
                strncpy(result->result_schema->columns[0].name, "rows_affected", 
                       sizeof(result->result_schema->columns[0].name) - 1);
                result->result_schema->columns[0].name[sizeof(result->result_schema->columns[0].name) - 1] = '\0';
                result->result_schema->columns[0].type = TYPE_INT;
                result->result_schema->columns[0].nullable = false;
                result->result_schema->columns[0].is_primary_key = false;
                result->result_schema->primary_key_columns = NULL;
                result->result_schema->primary_key_column_count = 0;
                
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
                result->result_schema->name[0] = '\0';
                result->result_schema->column_count = 1;
                result->result_schema->columns = malloc(sizeof(ColumnDefinition));
                strncpy(result->result_schema->columns[0].name, "rows_affected", 
                       sizeof(result->result_schema->columns[0].name) - 1);
                result->result_schema->columns[0].name[sizeof(result->result_schema->columns[0].name) - 1] = '\0';
                result->result_schema->columns[0].type = TYPE_INT;
                result->result_schema->columns[0].nullable = false;
                result->result_schema->columns[0].is_primary_key = false;
                result->result_schema->primary_key_columns = NULL;
                result->result_schema->primary_key_column_count = 0;
                
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
    
    // Free rows - but be careful about what we're freeing
    if (result->rows) {
        // For SELECT results, the row data might be pointers into loaded pages
        // which shouldn't be freed individually
        // Only free the rows array itself, not the individual row data
        
        // However, for INSERT/UPDATE/DELETE results, we do allocate the row data
        // Check the statement type based on the schema
        if (result->result_schema && 
            result->result_schema->column_count == 1 &&
            strcmp(result->result_schema->columns[0].name, "rows_affected") == 0) {
            // This is an INSERT/UPDATE/DELETE result - free the individual rows
            for (int i = 0; i < result->row_count; i++) {
                free(result->rows[i]);
            }
        }
        // Always free the rows array itself
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
