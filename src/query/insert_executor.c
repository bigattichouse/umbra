/**
 * @file insert_executor.c
 * @brief Executes INSERT operations
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#include "insert_executor.h"
#include "../schema/metadata.h"
#include "../schema/type_system.h"
#include "../loader/page_manager.h"
#include "../loader/record_access.h"
#include "../pages/page_generator.h"
#include "../pages/page_splitter.h"
#include "../query/query_executor.h"
#include "../util/uuid_utils.h"

/**
 * @brief Create insert result
 */
static InsertResult* create_insert_result(void) {
    InsertResult* result = malloc(sizeof(InsertResult));
    result->rows_affected = 0;
    result->success = false;
    result->error_message = NULL;
    return result;
}

/**
 * @brief Set error in result
 */
static void set_insert_error(InsertResult* result, const char* format, ...) {
    result->success = false;
    
    va_list args;
    va_start(args, format);
    
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    
    result->error_message = strdup(buffer);
    
    va_end(args);
}

/**
 * @brief Evaluate literal expression
 */
static char* evaluate_literal(const Expression* expr) {
    if (expr->type != AST_LITERAL) {
        return NULL;
    }
    
    char buffer[256];
    const Literal* lit = &expr->data.literal;
    
    switch (lit->type) {
        case TYPE_INT:
            snprintf(buffer, sizeof(buffer), "%d", lit->value.int_value);
            break;
        case TYPE_FLOAT:
            snprintf(buffer, sizeof(buffer), "%f", lit->value.float_value);
            break;
        case TYPE_BOOLEAN:
            snprintf(buffer, sizeof(buffer), "%s", lit->value.bool_value ? "true" : "false");
            break;
        case TYPE_VARCHAR:
        case TYPE_TEXT:
            return strdup(lit->value.string_value);
        default:
            return NULL;
    }
    
    return strdup(buffer);
}

/**
 * @brief Execute an INSERT statement
 */
InsertResult* execute_insert(const InsertStatement* stmt, const char* base_dir) {
    InsertResult* result = create_insert_result();
    
    if (!stmt || !base_dir) {
        set_insert_error(result, "Invalid parameters");
        return result;
    }
    
    // Load table schema using the shared function
    TableSchema* schema = load_table_schema(stmt->table_name, base_dir);
    if (!schema) {
        set_insert_error(result, "Table '%s' not found", stmt->table_name);
        return result;
    }
    
    // Validate statement
    if (!validate_insert_statement(stmt, schema)) {
        set_insert_error(result, "INSERT statement validation failed");
        free_table_schema(schema);
        return result;
    }
    
    // Find UUID column index
    int uuid_idx = _UUID_COLUMN_INDEX;
    
    // Prepare values for insertion
    const char** values = malloc(schema->column_count * sizeof(char*));
    
    // If column list provided, map values to correct positions
    if (stmt->column_count > 0) {
        // Initialize all values to NULL
        for (int i = 0; i < schema->column_count; i++) {
            values[i] = NULL;
        }
        
        // Map provided values to correct columns
        for (int i = 0; i < stmt->column_count; i++) {
            // Find column index in schema
            int schema_idx = -1;
            for (int j = 0; j < schema->column_count; j++) {
                if (strcmp(stmt->columns[i], schema->columns[j].name) == 0) {
                    schema_idx = j;
                    break;
                }
            }
            
            if (schema_idx >= 0) {
                values[schema_idx] = evaluate_literal(stmt->value_list.values[i]);
            }
        }
        
        // Set defaults for missing columns
        for (int i = 0; i < schema->column_count; i++) {
            if (values[i] == NULL) {
                if (schema->columns[i].has_default) {
                    values[i] = strdup(schema->columns[i].default_value);
                } else if (schema->columns[i].nullable) {
                    values[i] = strdup("NULL");
                } else {
                    // For non-nullable columns without defaults, use zero values
                    switch (schema->columns[i].type) {
                        case TYPE_INT:
                            values[i] = strdup("0");
                            break;
                        case TYPE_FLOAT:
                            values[i] = strdup("0.0");
                            break;
                        case TYPE_BOOLEAN:
                            values[i] = strdup("false");
                            break;
                        case TYPE_VARCHAR:
                        case TYPE_TEXT:
                            values[i] = strdup("");
                            break;
                        default:
                            values[i] = strdup("0");
                            break;
                    }
                }
            }
        }
    } else {
        // No column list - values in order
        for (int i = 0; i < schema->column_count; i++) {
            values[i] = evaluate_literal(stmt->value_list.values[i]);
        }
    }
    
    // Generate and add UUID (always overwrite any provided UUID)
    char uuid_str[37]; // 36 chars + null terminator
    generate_uuid(uuid_str);
    
    // Free existing UUID value if any
    if (values[uuid_idx] != NULL) {
        free((char*)values[uuid_idx]);
    }
    
    // Set the UUID value
    values[uuid_idx] = strdup(uuid_str);
    
    // Find best page for insertion
    int page_number;
    if (find_best_page_for_insert(schema, base_dir, 1000, &page_number) != 0) {
        set_insert_error(result, "Failed to find suitable page for insertion");
        
        // Clean up
        for (int i = 0; i < schema->column_count; i++) {
            free((char*)values[i]);
        }
        free(values);
        free_table_schema(schema);
        return result;
    }
    
    // Add record to page
    if (add_record_to_page(schema, base_dir, page_number, values) != 0) {
        set_insert_error(result, "Failed to add record to page");
        
        // Clean up
        for (int i = 0; i < schema->column_count; i++) {
            free((char*)values[i]);
        }
        free(values);
        free_table_schema(schema);
        return result;
    }
    
    // Recompile the page
    if (recompile_data_page(schema, base_dir, page_number) != 0) {
        set_insert_error(result, "Failed to recompile data page");
        
        // Clean up
        for (int i = 0; i < schema->column_count; i++) {
            free((char*)values[i]);
        }
        free(values);
        free_table_schema(schema);
        return result;
    }
    
    // Update metadata
    TableMetadata metadata;
    if (load_table_metadata(stmt->table_name, base_dir, &metadata) == 0) {
        metadata.record_count++;
        time(&metadata.modified_at);
        update_table_metadata(&metadata, base_dir);
    }
    
    result->rows_affected = 1;
    result->success = true;
    
    // Clean up
    for (int i = 0; i < schema->column_count; i++) {
        free((char*)values[i]);
    }
    free(values);
    free_table_schema(schema);
    
    return result;
}

/**
 * @brief Free insert result
 */
void free_insert_result(InsertResult* result) {
    if (result) {
        free(result->error_message);
        free(result);
    }
}
