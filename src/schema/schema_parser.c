/**
 * @file schema_parser.c
 * @brief Parser implementation for CREATE TABLE statements
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>    // Add this for stat() and mkdir()
#include <sys/types.h>   // Add this for mode_t
#include <errno.h>       // Add this for errno
#include "schema_parser.h"

#ifdef DEBUG
#define DEBUG_PRINT(...) fprintf(stderr, "[DEBUG] " __VA_ARGS__)
#else
#define DEBUG_PRINT(...) do {} while(0)
#endif

/**
 * @struct Token
 * @brief Represents a token during parsing
 */
typedef struct {
    char* value;
    enum {
        TOKEN_IDENTIFIER,
        TOKEN_KEYWORD,
        TOKEN_SYMBOL,
        TOKEN_NUMBER,
        TOKEN_STRING,
        TOKEN_EOF
    } type;
} Token;

/**
 * @struct Tokenizer
 * @brief Tokenizes SQL input
 */
typedef struct {
    const char* input;
    int position;
    int length;
    Token current_token;
} Tokenizer;

/**
 * @brief Initialize a tokenizer with input
 */
static void init_tokenizer(Tokenizer* tokenizer, const char* input) {
    DEBUG_PRINT("init_tokenizer: input='%s'\n", input);
    tokenizer->input = input;
    tokenizer->position = 0;
    tokenizer->length = strlen(input);
    tokenizer->current_token.value = NULL;
    tokenizer->current_token.type = TOKEN_EOF;
}

/**
 * @brief Free tokenizer resources
 */
static void free_tokenizer(Tokenizer* tokenizer) {
    DEBUG_PRINT("free_tokenizer\n");
    free(tokenizer->current_token.value);
    tokenizer->current_token.value = NULL;
}

/**
 * @brief Skip whitespace in tokenizer
 */
static void skip_whitespace(Tokenizer* tokenizer) {
    while (tokenizer->position < tokenizer->length && 
           isspace(tokenizer->input[tokenizer->position])) {
        tokenizer->position++;
    }
}

/**
 * @brief Get next token
 */
static void next_token(Tokenizer* tokenizer) {
    // Free previous token if any
    free(tokenizer->current_token.value);
    tokenizer->current_token.value = NULL;
    
    skip_whitespace(tokenizer);
    
    if (tokenizer->position >= tokenizer->length) {
        tokenizer->current_token.type = TOKEN_EOF;
        DEBUG_PRINT("next_token: EOF\n");
        return;
    }
    
    // Check for symbols
    char c = tokenizer->input[tokenizer->position];
    if (c == '(' || c == ')' || c == ',' || c == ';') {
        tokenizer->current_token.value = malloc(2);
        tokenizer->current_token.value[0] = c;
        tokenizer->current_token.value[1] = '\0';
        tokenizer->current_token.type = TOKEN_SYMBOL;
        tokenizer->position++;
        DEBUG_PRINT("next_token: symbol='%s'\n", tokenizer->current_token.value);
        return;
    }
    
    // Check for identifiers and keywords
    if (isalpha(c) || c == '_') {
        int start = tokenizer->position;
        while (tokenizer->position < tokenizer->length && 
               (isalnum(tokenizer->input[tokenizer->position]) || 
                tokenizer->input[tokenizer->position] == '_')) {
            tokenizer->position++;
        }
        
        int length = tokenizer->position - start;
        tokenizer->current_token.value = malloc(length + 1);
        strncpy(tokenizer->current_token.value, tokenizer->input + start, length);
        tokenizer->current_token.value[length] = '\0';
        
        // Check if it's a keyword
        char* upper = strdup(tokenizer->current_token.value);
        for (int i = 0; upper[i]; i++) {
            upper[i] = toupper(upper[i]);
        }
        
        if (strcmp(upper, "CREATE") == 0 || 
            strcmp(upper, "TABLE") == 0 || 
            strcmp(upper, "INT") == 0 || 
            strcmp(upper, "VARCHAR") == 0 || 
            strcmp(upper, "TEXT") == 0 || 
            strcmp(upper, "FLOAT") == 0 || 
            strcmp(upper, "DATE") == 0 || 
            strcmp(upper, "BOOLEAN") == 0 ||
            strcmp(upper, "PRIMARY") == 0 ||
            strcmp(upper, "KEY") == 0 ||
            strcmp(upper, "NOT") == 0 ||
            strcmp(upper, "NULL") == 0 ||
            strcmp(upper, "DEFAULT") == 0) {
            tokenizer->current_token.type = TOKEN_KEYWORD;
        } else {
            tokenizer->current_token.type = TOKEN_IDENTIFIER;
        }
        
        free(upper);
        DEBUG_PRINT("next_token: %s='%s'\n", 
                   tokenizer->current_token.type == TOKEN_KEYWORD ? "keyword" : "identifier",
                   tokenizer->current_token.value);
        return;
    }
    
    // Check for numbers
    if (isdigit(c) || c == '-') {
        int start = tokenizer->position;
        if (c == '-') {
            tokenizer->position++;
        }
        
        while (tokenizer->position < tokenizer->length && 
               isdigit(tokenizer->input[tokenizer->position])) {
            tokenizer->position++;
        }
        
        int length = tokenizer->position - start;
        tokenizer->current_token.value = malloc(length + 1);
        strncpy(tokenizer->current_token.value, tokenizer->input + start, length);
        tokenizer->current_token.value[length] = '\0';
        tokenizer->current_token.type = TOKEN_NUMBER;
        DEBUG_PRINT("next_token: number='%s'\n", tokenizer->current_token.value);
        return;
    }
    
    // Handle other cases as symbols
    tokenizer->current_token.value = malloc(2);
    tokenizer->current_token.value[0] = c;
    tokenizer->current_token.value[1] = '\0';
    tokenizer->current_token.type = TOKEN_SYMBOL;
    tokenizer->position++;
    DEBUG_PRINT("next_token: unknown symbol='%s'\n", tokenizer->current_token.value);
}

/**
 * @brief Check if current token matches expected value
 */
static bool match(Tokenizer* tokenizer, const char* expected) {
    if (tokenizer->current_token.value && 
        strcasecmp(tokenizer->current_token.value, expected) == 0) {
        DEBUG_PRINT("match: matched '%s'\n", expected);
        next_token(tokenizer);
        return true;
    }
    return false;
}

/**
 * @brief Expect a specific token value
 */
static bool expect(Tokenizer* tokenizer, const char* expected) {
    if (!match(tokenizer, expected)) {
        fprintf(stderr, "Expected '%s', got '%s'\n", 
                expected, 
                tokenizer->current_token.value ? tokenizer->current_token.value : "EOF");
        return false;
    }
    return true;
}

/**
 * @brief Parse a data type from the tokenizer
 */
static DataType parse_data_type(Tokenizer* tokenizer, int* length) {
    *length = 0;
    
    if (tokenizer->current_token.value == NULL) {
        DEBUG_PRINT("parse_data_type: no token\n");
        return TYPE_UNKNOWN;
    }
    
    char* upper = strdup(tokenizer->current_token.value);
    for (int i = 0; upper[i]; i++) {
        upper[i] = toupper(upper[i]);
    }
    
    DataType type = TYPE_UNKNOWN;
    
    DEBUG_PRINT("parse_data_type: checking type '%s'\n", upper);
    
    if (strcmp(upper, "INT") == 0) {
        type = TYPE_INT;
    } else if (strcmp(upper, "VARCHAR") == 0) {
        type = TYPE_VARCHAR;
    } else if (strcmp(upper, "TEXT") == 0) {
        type = TYPE_TEXT;
    } else if (strcmp(upper, "FLOAT") == 0) {
        type = TYPE_FLOAT;
    } else if (strcmp(upper, "DATE") == 0) {
        type = TYPE_DATE;
    } else if (strcmp(upper, "BOOLEAN") == 0) {
        type = TYPE_BOOLEAN;
    }
    
    free(upper);
    
    if (type == TYPE_UNKNOWN) {
        DEBUG_PRINT("parse_data_type: unknown type\n");
        return type;
    }
    
    next_token(tokenizer);
    
    // Handle VARCHAR length
    if (type == TYPE_VARCHAR) {
        if (match(tokenizer, "(")) {
            // Parse length
            if (tokenizer->current_token.type == TOKEN_NUMBER) {
                *length = atoi(tokenizer->current_token.value);
                DEBUG_PRINT("parse_data_type: VARCHAR length=%d\n", *length);
                next_token(tokenizer);
                expect(tokenizer, ")");
            }
        }
    }
    
    DEBUG_PRINT("parse_data_type: type=%d, length=%d\n", type, *length);
    return type;
}

/**
 * @brief Parse a column definition
 */
static ColumnDefinition parse_column_definition(Tokenizer* tokenizer) {
    DEBUG_PRINT("parse_column_definition: start\n");
    ColumnDefinition column;
    memset(&column, 0, sizeof(ColumnDefinition));
    
    // Parse column name
    if (tokenizer->current_token.type == TOKEN_IDENTIFIER) {
        strncpy(column.name, tokenizer->current_token.value, sizeof(column.name) - 1);
        column.name[sizeof(column.name) - 1] = '\0';
        DEBUG_PRINT("parse_column_definition: name='%s'\n", column.name);
        next_token(tokenizer);
    } else {
        fprintf(stderr, "Expected column name\n");
        return column;
    }
    
    // Parse data type
    column.type = parse_data_type(tokenizer, &column.length);
    
    // Parse constraints
    column.nullable = true; // Default to nullable
    
    while (tokenizer->current_token.type == TOKEN_KEYWORD) {
        char* upper = strdup(tokenizer->current_token.value);
        for (int i = 0; upper[i]; i++) {
            upper[i] = toupper(upper[i]);
        }
        
        DEBUG_PRINT("parse_column_definition: checking constraint '%s'\n", upper);
        
        if (strcmp(upper, "NOT") == 0) {
            next_token(tokenizer);
            if (match(tokenizer, "NULL")) {
                column.nullable = false;
                DEBUG_PRINT("parse_column_definition: NOT NULL\n");
            }
        } else if (strcmp(upper, "DEFAULT") == 0) {
            next_token(tokenizer);
            // Parse default value
            if (tokenizer->current_token.value) {
                strncpy(column.default_value, tokenizer->current_token.value, 
                        sizeof(column.default_value) - 1);
                column.default_value[sizeof(column.default_value) - 1] = '\0';
                column.has_default = true;
                DEBUG_PRINT("parse_column_definition: DEFAULT '%s'\n", column.default_value);
                next_token(tokenizer);
            }
        } else if (strcmp(upper, "PRIMARY") == 0) {
            next_token(tokenizer);
            if (match(tokenizer, "KEY")) {
                column.is_primary_key = true;
                DEBUG_PRINT("parse_column_definition: PRIMARY KEY\n");
            }
        } else {
            free(upper);
            break;
        }
        
        free(upper);
    }
    
    DEBUG_PRINT("parse_column_definition: done\n");
    return column;
}

/**
 * @brief Parse the CREATE TABLE statement
 */
TableSchema* parse_create_table(const char* create_statement) {
    DEBUG_PRINT("parse_create_table: start\n");
    if (create_statement == NULL) {
        DEBUG_PRINT("parse_create_table: NULL statement\n");
        return NULL;
    }
    
    Tokenizer tokenizer;
    init_tokenizer(&tokenizer, create_statement);
    next_token(&tokenizer);
    
    // Expect CREATE TABLE
    if (!match(&tokenizer, "CREATE") || !match(&tokenizer, "TABLE")) {
        fprintf(stderr, "Expected CREATE TABLE statement\n");
        free_tokenizer(&tokenizer);
        return NULL;
    }
    
    // Allocate schema
    TableSchema* schema = malloc(sizeof(TableSchema));
    if (!schema) {
        fprintf(stderr, "Failed to allocate schema\n");
        free_tokenizer(&tokenizer);
        return NULL;
    }
    memset(schema, 0, sizeof(TableSchema));
    
    // Parse table name
    if (tokenizer.current_token.type == TOKEN_IDENTIFIER) {
        strncpy(schema->name, tokenizer.current_token.value, sizeof(schema->name) - 1);
        schema->name[sizeof(schema->name) - 1] = '\0';
        DEBUG_PRINT("parse_create_table: table name='%s'\n", schema->name);
        next_token(&tokenizer);
    } else {
        fprintf(stderr, "Expected table name\n");
        free(schema);
        free_tokenizer(&tokenizer);
        return NULL;
    }
    
    // Expect opening parenthesis
    if (!expect(&tokenizer, "(")) {
        free(schema);
        free_tokenizer(&tokenizer);
        return NULL;
    }
    
    // Allocate initial space for columns
    int capacity = 10;
    schema->columns = malloc(capacity * sizeof(ColumnDefinition));
    if (!schema->columns) {
        fprintf(stderr, "Failed to allocate columns\n");
        free(schema);
        free_tokenizer(&tokenizer);
        return NULL;
    }
    schema->column_count = 0;
    
    // Parse column definitions
    while (tokenizer.current_token.type != TOKEN_SYMBOL || 
           strcmp(tokenizer.current_token.value, ")") != 0) {
        
        // Check for comma between columns
        if (schema->column_count > 0) {
            if (!expect(&tokenizer, ",")) {
                free(schema->columns);
                free(schema);
                free_tokenizer(&tokenizer);
                return NULL;
            }
        }
        
        // Check if we need to expand columns array
        if (schema->column_count >= capacity) {
            capacity *= 2;
            schema->columns = realloc(schema->columns, capacity * sizeof(ColumnDefinition));
            if (!schema->columns) {
                fprintf(stderr, "Failed to reallocate columns\n");
                free(schema);
                free_tokenizer(&tokenizer);
                return NULL;
            }
        }
        
        // Parse column definition
        schema->columns[schema->column_count] = parse_column_definition(&tokenizer);
        schema->column_count++;
        DEBUG_PRINT("parse_create_table: parsed column %d\n", schema->column_count);
    }
    
    // Expect closing parenthesis
    if (!expect(&tokenizer, ")")) {
        free(schema->columns);
        free(schema);
        free_tokenizer(&tokenizer);
        return NULL;
    }
    
    // Build primary key list
    schema->primary_key_column_count = 0;
    for (int i = 0; i < schema->column_count; i++) {
        if (schema->columns[i].is_primary_key) {
            schema->primary_key_column_count++;
        }
    }
    
    if (schema->primary_key_column_count > 0) {
        schema->primary_key_columns = malloc(schema->primary_key_column_count * sizeof(int));
        if (!schema->primary_key_columns) {
            fprintf(stderr, "Failed to allocate primary key columns\n");
            free(schema->columns);
            free(schema);
            free_tokenizer(&tokenizer);
            return NULL;
        }
        int pk_index = 0;
        for (int i = 0; i < schema->column_count; i++) {
            if (schema->columns[i].is_primary_key) {
                schema->primary_key_columns[pk_index++] = i;
            }
        }
    }
    
    free_tokenizer(&tokenizer);
    DEBUG_PRINT("parse_create_table: completed successfully\n");
    return schema;
}

/**
 * @brief Free a previously allocated table schema
 */
void free_table_schema(TableSchema* schema) {
    DEBUG_PRINT("free_table_schema: start\n");
    if (schema) {
        free(schema->columns);
        free(schema->primary_key_columns);
        free(schema);
    }
    DEBUG_PRINT("free_table_schema: done\n");
}

/**
 * @brief Check if a schema is valid
 */
bool validate_schema(const TableSchema* schema) {
    DEBUG_PRINT("validate_schema: start\n");
    if (!schema || schema->column_count <= 0) {
        DEBUG_PRINT("validate_schema: invalid schema or no columns\n");
        return false;
    }
    
    // Check for duplicate column names
    for (int i = 0; i < schema->column_count; i++) {
        for (int j = i + 1; j < schema->column_count; j++) {
            if (strcmp(schema->columns[i].name, schema->columns[j].name) == 0) {
                fprintf(stderr, "Duplicate column name: %s\n", schema->columns[i].name);
                return false;
            }
        }
    }
    
    // Check for valid data types
    for (int i = 0; i < schema->column_count; i++) {
        if (schema->columns[i].type == TYPE_UNKNOWN) {
            fprintf(stderr, "Unknown data type for column: %s\n", schema->columns[i].name);
            return false;
        }
    }
    
    DEBUG_PRINT("validate_schema: schema is valid\n");
    return true;
}

/**
 * @brief Save schema to metadata file (JSON format)
 * @param schema Schema to save
 * @param base_dir Base directory for database
 * @return 0 on success, -1 on error
 */
int save_schema_metadata(const TableSchema* schema, const char* base_dir) {
    if (!schema || !base_dir) {
        return -1;
    }
    
    // Create metadata directory if it doesn't exist
    char metadata_dir[2048];
    snprintf(metadata_dir, sizeof(metadata_dir), "%s/tables/%s/metadata", 
             base_dir, schema->name);
    
    struct stat st;
    if (stat(metadata_dir, &st) != 0) {
        if (mkdir(metadata_dir, 0755) != 0) {
            fprintf(stderr, "Failed to create metadata directory: %s\n", strerror(errno));
            return -1;
        }
    }
    
    // Create schema metadata file path
    char metadata_path[2560];
    snprintf(metadata_path, sizeof(metadata_path), "%s/schema.json", metadata_dir);
    
    FILE* file = fopen(metadata_path, "w");
    if (!file) {
        fprintf(stderr, "Failed to open schema metadata file for writing: %s\n", strerror(errno));
        return -1;
    }
    
    // Write schema in JSON format
    fprintf(file, "{\n");
    fprintf(file, "  \"name\": \"%s\",\n", schema->name);
    fprintf(file, "  \"column_count\": %d,\n", schema->column_count);
    fprintf(file, "  \"columns\": [\n");
    
    for (int i = 0; i < schema->column_count; i++) {
        const ColumnDefinition* col = &schema->columns[i];
        fprintf(file, "    {\n");
        fprintf(file, "      \"name\": \"%s\",\n", col->name);
        fprintf(file, "      \"type\": \"%s\",\n", get_type_info(col->type).name);
        fprintf(file, "      \"length\": %d,\n", col->length);
        fprintf(file, "      \"nullable\": %s,\n", col->nullable ? "true" : "false");
        fprintf(file, "      \"has_default\": %s,\n", col->has_default ? "true" : "false");
        
        if (col->has_default) {
            fprintf(file, "      \"default_value\": \"%s\",\n", col->default_value);
        }
        
        fprintf(file, "      \"is_primary_key\": %s\n", col->is_primary_key ? "true" : "false");
        fprintf(file, "    }%s\n", i < schema->column_count - 1 ? "," : "");
    }
    
    fprintf(file, "  ],\n");
    fprintf(file, "  \"primary_key_column_count\": %d", schema->primary_key_column_count);
    
    if (schema->primary_key_column_count > 0 && schema->primary_key_columns) {
        fprintf(file, ",\n  \"primary_key_columns\": [");
        for (int i = 0; i < schema->primary_key_column_count; i++) {
            fprintf(file, "%d", schema->primary_key_columns[i]);
            if (i < schema->primary_key_column_count - 1) {
                fprintf(file, ", ");
            }
        }
        fprintf(file, "]");
    }
    
    fprintf(file, "\n}\n");
    
    fclose(file);
    return 0;
}

/**
 * @brief Load schema from metadata file (JSON format)
 * @param table_name Table name
 * @param base_dir Base directory for database
 * @return Loaded schema or NULL on error
 */
TableSchema* load_schema_metadata(const char* table_name, const char* base_dir) {
    if (!table_name || !base_dir) {
        return NULL;
    }
    
    // Create schema metadata file path
    char metadata_path[2560];
    snprintf(metadata_path, sizeof(metadata_path), "%s/tables/%s/metadata/schema.json", 
             base_dir, table_name);
    
    FILE* file = fopen(metadata_path, "r");
    if (!file) {
        return NULL;
    }
    
    // Read entire file
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* content = malloc(file_size + 1);
    if (!content) {
        fclose(file);
        return NULL;
    }
    
    fread(content, 1, file_size, file);
    content[file_size] = '\0';
    fclose(file);
    
    // Create schema
    TableSchema* schema = malloc(sizeof(TableSchema));
    memset(schema, 0, sizeof(TableSchema));
    
    #ifdef DEBUG
    fprintf(stderr, "[DEBUG] JSON content:\n%s\n", content);
    #endif
    
    // Better JSON parsing with bounds checking
    char* ptr = content;
    char* end = content + file_size;  // End of buffer
    int column_idx = -1;
    bool in_columns_array = false;
    bool in_primary_keys = false;
    int brace_depth = 0;
    int array_depth = 0;  // Track array depth separately
    int pk_idx = 0;
    
    // Simple state machine for parsing
    while (ptr < end && *ptr) {
        // Skip whitespace
        while (ptr < end && *ptr && isspace(*ptr)) ptr++;
        
        if (ptr >= end) break;
        
        if (*ptr == '{') {
            brace_depth++;
            // If we're in the columns array and at depth 2, this is a new column
            if (in_columns_array && brace_depth == 2) {
                column_idx++;
                #ifdef DEBUG
                fprintf(stderr, "[DEBUG] Starting column %d (brace_depth=%d)\n", column_idx, brace_depth);
                #endif
            }
            ptr++;
        } else if (*ptr == '}') {
            brace_depth--;
            ptr++;
        } else if (*ptr == '[') {
            array_depth++;
            ptr++;
        } else if (*ptr == ']') {
            array_depth--;
            if (in_columns_array) {
                in_columns_array = false;
                column_idx = -1;
                #ifdef DEBUG
                fprintf(stderr, "[DEBUG] Exiting columns array\n");
                #endif
            }
            if (in_primary_keys) in_primary_keys = false;
            ptr++;
        } else if (*ptr == '"') {
            // Parse a key-value pair
            ptr++; // Skip opening quote
            if (ptr >= end) break;
            
            char key[256];
            int i = 0;
            while (ptr < end && *ptr && *ptr != '"' && i < 255) {
                key[i++] = *ptr++;
            }
            key[i] = '\0';
            
            if (ptr < end && *ptr == '"') ptr++; // Skip closing quote
            while (ptr < end && *ptr && *ptr != ':') ptr++; // Skip to colon
            if (ptr < end && *ptr == ':') ptr++; // Skip colon
            while (ptr < end && *ptr && isspace(*ptr)) ptr++; // Skip whitespace
            
            if (ptr >= end) break;
            
            #ifdef DEBUG
            if (in_columns_array && column_idx >= 0) {
                fprintf(stderr, "[DEBUG] Parsing key '%s' for column %d\n", key, column_idx);
            }
            #endif
            
            // Now parse the value based on the key
            if (strcmp(key, "name") == 0 && !in_columns_array && brace_depth == 1) {
                // Top-level name field
                if (*ptr == '"') {
                    ptr++; // Skip opening quote
                    if (ptr >= end) break;
                    i = 0;
                    while (ptr < end && *ptr && *ptr != '"' && i < 63) {
                        schema->name[i++] = *ptr++;
                    }
                    schema->name[i] = '\0';
                    if (ptr < end && *ptr == '"') ptr++; // Skip closing quote
                }
            } else if (strcmp(key, "column_count") == 0) {
                schema->column_count = atoi(ptr);
                schema->columns = malloc(schema->column_count * sizeof(ColumnDefinition));
                memset(schema->columns, 0, schema->column_count * sizeof(ColumnDefinition));
                while (ptr < end && *ptr && *ptr != ',' && *ptr != '}') ptr++;
            } else if (strcmp(key, "columns") == 0) {
                in_columns_array = true;
                column_idx = -1;
                #ifdef DEBUG
                fprintf(stderr, "[DEBUG] Entering columns array\n");
                #endif
            } else if (strcmp(key, "primary_key_column_count") == 0) {
                schema->primary_key_column_count = atoi(ptr);
                if (schema->primary_key_column_count > 0) {
                    schema->primary_key_columns = malloc(schema->primary_key_column_count * sizeof(int));
                }
                while (ptr < end && *ptr && *ptr != ',' && *ptr != '}') ptr++;
            } else if (strcmp(key, "primary_key_columns") == 0) {
                in_primary_keys = true;
                pk_idx = 0;
            } else if (in_columns_array && column_idx >= 0 && column_idx < schema->column_count) {
                ColumnDefinition* col = &schema->columns[column_idx];
                
                if (strcmp(key, "name") == 0) {
                    if (*ptr == '"') {
                        ptr++; // Skip opening quote
                        if (ptr >= end) break;
                        i = 0;
                        while (ptr < end && *ptr && *ptr != '"' && i < 63) {
                            col->name[i++] = *ptr++;
                        }
                        col->name[i] = '\0';
                        if (ptr < end && *ptr == '"') ptr++; // Skip closing quote
                        #ifdef DEBUG
                        fprintf(stderr, "[DEBUG] Set column %d name to '%s'\n", column_idx, col->name);
                        #endif
                    }
                } else if (strcmp(key, "type") == 0) {
                    if (*ptr == '"') {
                        ptr++; // Skip opening quote
                        if (ptr >= end) break;
                        char type_str[32];
                        i = 0;
                        while (ptr < end && *ptr && *ptr != '"' && i < 31) {
                            type_str[i++] = *ptr++;
                        }
                        type_str[i] = '\0';
                        col->type = get_type_from_name(type_str);
                        if (ptr < end && *ptr == '"') ptr++; // Skip closing quote
                        #ifdef DEBUG
                        fprintf(stderr, "[DEBUG] Set column %d type to %d (%s)\n", column_idx, col->type, type_str);
                        #endif
                    }
                } else if (strcmp(key, "length") == 0) {
                    col->length = atoi(ptr);
                    while (ptr < end && *ptr && *ptr != ',' && *ptr != '}') ptr++;
                } else if (strcmp(key, "nullable") == 0) {
                    if (ptr + 4 <= end && strncmp(ptr, "true", 4) == 0) {
                        col->nullable = true;
                        ptr += 4;
                    } else if (ptr + 5 <= end && strncmp(ptr, "false", 5) == 0) {
                        col->nullable = false;
                        ptr += 5;
                    }
                } else if (strcmp(key, "has_default") == 0) {
                    if (ptr + 4 <= end && strncmp(ptr, "true", 4) == 0) {
                        col->has_default = true;
                        ptr += 4;
                    } else if (ptr + 5 <= end && strncmp(ptr, "false", 5) == 0) {
                        col->has_default = false;
                        ptr += 5;
                    }
                } else if (strcmp(key, "default_value") == 0) {
                    if (*ptr == '"') {
                        ptr++; // Skip opening quote
                        if (ptr >= end) break;
                        i = 0;
                        while (ptr < end && *ptr && *ptr != '"' && i < 255) {
                            col->default_value[i++] = *ptr++;
                        }
                        col->default_value[i] = '\0';
                        if (ptr < end && *ptr == '"') ptr++; // Skip closing quote
                    }
                } else if (strcmp(key, "is_primary_key") == 0) {
                    if (ptr + 4 <= end && strncmp(ptr, "true", 4) == 0) {
                        col->is_primary_key = true;
                        ptr += 4;
                    } else if (ptr + 5 <= end && strncmp(ptr, "false", 5) == 0) {
                        col->is_primary_key = false;
                        ptr += 5;
                    }
                } else {
                    // Skip unknown keys in column objects
                    if (*ptr == '"') {
                        // Skip string value
                        ptr++; // Skip opening quote
                        while (ptr < end && *ptr && *ptr != '"') {
                            if (*ptr == '\\' && ptr + 1 < end) ptr++; // Skip escaped character
                            ptr++;
                        }
                        if (ptr < end && *ptr == '"') ptr++; // Skip closing quote
                    } else {
                        // Skip other values
                        while (ptr < end && *ptr && *ptr != ',' && *ptr != '}') ptr++;
                    }
                }
            } else if (in_primary_keys && pk_idx < schema->primary_key_column_count) {
                schema->primary_key_columns[pk_idx++] = atoi(ptr);
                while (ptr < end && *ptr && *ptr != ',' && *ptr != ']') ptr++;
            } else {
                // Skip unknown values
                if (*ptr == '"') {
                    // Skip string value
                    ptr++; // Skip opening quote
                    while (ptr < end && *ptr && *ptr != '"') {
                        if (*ptr == '\\' && ptr + 1 < end) ptr++; // Skip escaped character
                        ptr++;
                    }
                    if (ptr < end && *ptr == '"') ptr++; // Skip closing quote
                } else {
                    // Skip other values
                    while (ptr < end && *ptr && *ptr != ',' && *ptr != '}' && *ptr != ']') ptr++;
                }
            }
        } else if (*ptr == ',') {
            ptr++; // Skip comma
        } else {
            ptr++;
        }
    }
    
    #ifdef DEBUG
    fprintf(stderr, "[DEBUG] Parsed schema: name=%s, columns=%d\n", schema->name, schema->column_count);
    for (int i = 0; i < schema->column_count; i++) {
        fprintf(stderr, "[DEBUG] Column %d: name=%s, type=%d\n", i, schema->columns[i].name, schema->columns[i].type);
    }
    #endif
    
    free(content);
    return schema;
}
