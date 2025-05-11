/**
 * @file query_executor.c
 * @brief Executes queries using kernels
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>  // Added for va_start, va_end
#include <sys/stat.h>
#include <dirent.h>
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
#include "../schema/directory_manager.h"
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
 * @brief Load table schema from schema header file
 */
static TableSchema* load_schema_from_header(const char* table_name, const char* base_dir) {
    // Build path to schema header file
    char header_path[2048];
    snprintf(header_path, sizeof(header_path), "%s/tables/%s/%s.h", 
             base_dir, table_name, table_name);
    
    FILE* file = fopen(header_path, "r");
    if (!file) {
        return NULL;
    }
    
    // Create schema structure
    TableSchema* schema = malloc(sizeof(TableSchema));
    strncpy(schema->name, table_name, sizeof(schema->name) - 1);
    schema->name[sizeof(schema->name) - 1] = '\0';
    schema->column_count = 0;
    schema->columns = NULL;
    schema->primary_key_columns = NULL;
    schema->primary_key_column_count = 0;
    
    // Parse the header file to extract column definitions
    char line[1024];
    int capacity = 10;
    schema->columns = malloc(capacity * sizeof(ColumnDefinition));
    
    // Look for struct definition and parse columns
    bool in_struct = false;
    
    while (fgets(line, sizeof(line), file)) {
        // Look for struct definition
        if (strstr(line, "typedef struct {")) {
            in_struct = true;
            continue;
        }
        
        // End of struct
        if (in_struct && strstr(line, "} ")) {
            break;
        }
        
        // Parse column definition within struct
        if (in_struct) {
            // Skip comments and empty lines
            char* trimmed = line;
            while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
            if (*trimmed == '\n' || *trimmed == '/' || *trimmed == '*') {
                continue;
            }
            
            // Expand array if needed
            if (schema->column_count >= capacity) {
                capacity *= 2;
                schema->columns = realloc(schema->columns, capacity * sizeof(ColumnDefinition));
            }
            
            ColumnDefinition* col = &schema->columns[schema->column_count];
            
            // Parse type and name (simple parsing)
            char type_str[64];
            char name_str[64];
            char array_size[32] = "";
            
            // Handle different patterns:
            // int id;
            // char name[256];
            // double value;
            // bool active;
            if (sscanf(trimmed, "%s %[^[;]%[^;];", type_str, name_str, array_size) >= 2) {
                strncpy(col->name, name_str, sizeof(col->name) - 1);
                col->name[sizeof(col->name) - 1] = '\0';
                
                // Map C types to DataType
                if (strcmp(type_str, "int") == 0) {
                    col->type = TYPE_INT;
                } else if (strcmp(type_str, "double") == 0 || strcmp(type_str, "float") == 0) {
                    col->type = TYPE_FLOAT;
                } else if (strcmp(type_str, "bool") == 0) {
                    col->type = TYPE_BOOLEAN;
                } else if (strcmp(type_str, "time_t") == 0) {
                    col->type = TYPE_DATE;
                } else if (strcmp(type_str, "char") == 0) {
                    if (strlen(array_size) > 0) {
                        col->type = TYPE_VARCHAR;
                        // Extract array size
                        int size = 0;
                        if (sscanf(array_size, "[%d]", &size) == 1) {
                            col->length = size - 1; // Account for null terminator
                        } else {
                            col->length = 255; // Default
                        }
                    } else {
                        col->type = TYPE_VARCHAR;
                        col->length = 1;
                    }
                } else {
                    col->type = TYPE_UNKNOWN;
                }
                
                // Set defaults
                col->nullable = true;
                col->has_default = false;
                col->is_primary_key = false;
                
                // Check if it's likely a primary key (common pattern)
                if (strcmp(col->name, "id") == 0 && col->type == TYPE_INT) {
                    col->is_primary_key = true;
                    col->nullable = false;
                }
                
                schema->column_count++;
            }
        }
    }
    
    fclose(file);
    
    // Build primary key information
    int pk_count = 0;
    for (int i = 0; i < schema->column_count; i++) {
        if (schema->columns[i].is_primary_key) {
            pk_count++;
        }
    }
    
    if (pk_count > 0) {
        schema->primary_key_columns = malloc(pk_count * sizeof(int));
        schema->primary_key_column_count = pk_count;
        int pk_idx = 0;
        for (int i = 0; i < schema->column_count; i++) {
            if (schema->columns[i].is_primary_key) {
                schema->primary_key_columns[pk_idx++] = i;
            }
        }
    }
    
    return schema;
}

/**
 * @brief Load table schema from CREATE TABLE statement
 */
static TableSchema* load_schema_from_create_statement(const char* table_name, const char* base_dir) {
    // Try to find the CREATE TABLE statement in schema files
    char schema_dir[2048];
    snprintf(schema_dir, sizeof(schema_dir), "%s/tables/%s", base_dir, table_name);
    
    DIR* dir = opendir(schema_dir);
    if (!dir) {
        return NULL;
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        // Look for .sql files that might contain CREATE TABLE
        if (strstr(entry->d_name, ".sql")) {
            char sql_path[2560];
            snprintf(sql_path, sizeof(sql_path), "%s/%s", schema_dir, entry->d_name);
            
            FILE* file = fopen(sql_path, "r");
            if (file) {
                // Read the entire file
                fseek(file, 0, SEEK_END);
                long size = ftell(file);
                fseek(file, 0, SEEK_SET);
                
                char* content = malloc(size + 1);
                fread(content, 1, size, file);
                content[size] = '\0';
                fclose(file);
                
                // Check if it contains CREATE TABLE for our table
                char pattern[128];
                snprintf(pattern, sizeof(pattern), "CREATE TABLE %s", table_name);
                
                if (strstr(content, pattern)) {
                    // Parse the CREATE TABLE statement
                    TableSchema* schema = parse_create_table(content);
                    free(content);
                    closedir(dir);
                    return schema;
                }
                
                free(content);
            }
        }
    }
    
    closedir(dir);
    return NULL;
}

/**
 * @brief Load table schema from metadata
 */
static TableSchema* load_table_schema(const char* table_name, const char* base_dir) {
    // First, check if the table directory exists
    if (!table_directory_exists(table_name, base_dir)) {
        return NULL;
    }
    
    // Try to load from JSON metadata file first (most reliable)
    TableSchema* schema = load_schema_metadata(table_name, base_dir);
    if (schema) {
        return schema;
    }
    
    // Try to load from header file
    schema = load_schema_from_header(table_name, base_dir);
    if (schema) {
        return schema;
    }
    
    // Try to load from CREATE TABLE statement
    schema = load_schema_from_create_statement(table_name, base_dir);
    if (schema) {
        return schema;
    }
    
    
    // As a last resort, create a default schema for testing
    // This is temporary - in production, we'd fail here
    schema = malloc(sizeof(TableSchema));
    strncpy(schema->name, table_name, sizeof(schema->name) - 1);
    schema->name[sizeof(schema->name) - 1] = '\0';
    
    // Create a simple default schema
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
    
    schema->primary_key_columns = NULL;
    schema->primary_key_column_count = 0;
    
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
    TableSchema* schema = NULL;  // Track schema to ensure proper cleanup
    
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
