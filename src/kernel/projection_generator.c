/**
 * @file projection_generator.c
 * @brief Generates projection functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "projection_generator.h"

/**
 * @brief Get C type for a data type
 */
static const char* get_c_type(DataType type) {
    switch (type) {
        case TYPE_INT:      return "int";
        case TYPE_FLOAT:    return "double";
        case TYPE_BOOLEAN:  return "bool";
        case TYPE_DATE:     return "time_t";
        case TYPE_VARCHAR:  return "char";
        case TYPE_TEXT:     return "char";
        default:            return "void";
    }
}

/**
 * @brief Find column in schema
 */
static int find_column_index(const TableSchema* schema, const char* column_name) {
    for (int i = 0; i < schema->column_count; i++) {
        if (strcmp(schema->columns[i].name, column_name) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Generate projection structure definition
 */
int generate_projection_struct(const SelectStatement* stmt, const TableSchema* schema,
                              char* output, size_t output_size) {
    if (stmt->select_list.has_star) {
        // SELECT * uses the full schema
        snprintf(output, output_size,
            "/* Result structure matches full schema */\n"
            "typedef %s KernelResult;\n\n",
            schema->name);
        return 0;
    }
    
    // Generate custom projection structure
    char buffer[4096];
    char* current = buffer;
    size_t remaining = sizeof(buffer);
    
    int written = snprintf(current, remaining,
        "/**\n"
        " * @struct KernelResult\n"
        " * @brief Result structure for projected columns\n"
        " */\n"
        "typedef struct {\n");
    
    if (written < 0 || (size_t)written >= remaining) {
        return -1;
    }
    
    current += written;
    remaining -= written;
    
    // Add each selected column
    for (int i = 0; i < stmt->select_list.count; i++) {
        Expression* expr = stmt->select_list.expressions[i];
        
        if (expr->type == AST_COLUMN_REF) {
            const char* col_name = expr->data.column.column_name;
            int col_idx = find_column_index(schema, col_name);
            
            if (col_idx < 0) {
                continue;
            }
            
            const ColumnDefinition* col = &schema->columns[col_idx];
            const char* c_type = get_c_type(col->type);
            
            if (col->type == TYPE_VARCHAR || col->type == TYPE_TEXT) {
                int size = (col->type == TYPE_VARCHAR) ? col->length : 4096;
                written = snprintf(current, remaining,
                    "    %s %s[%d];\n", c_type, col_name, size + 1);
            } else {
                written = snprintf(current, remaining,
                    "    %s %s;\n", c_type, col_name);
            }
            
            if (written < 0 || (size_t)written >= remaining) {
                return -1;
            }
            
            current += written;
            remaining -= written;
        }
    }
    
    written = snprintf(current, remaining, "} KernelResult;\n\n");
    
    if (written < 0 || (size_t)written >= remaining) {
        return -1;
    }
    
    // Copy to output buffer
    if (strlen(buffer) >= output_size) {
        return -1;
    }
    
    strcpy(output, buffer);
    return 0;
}

/**
 * @brief Generate projection assignment code
 */
int generate_projection_assignment(const SelectStatement* stmt, const TableSchema* schema,
                                  char* output, size_t output_size) {
    char buffer[4096];
    char* current = buffer;
    size_t remaining = sizeof(buffer);
    
    // For each selected column, generate assignment
    for (int i = 0; i < stmt->select_list.count; i++) {
        Expression* expr = stmt->select_list.expressions[i];
        
        if (expr->type == AST_COLUMN_REF) {
            const char* col_name = expr->data.column.column_name;
            int col_idx = find_column_index(schema, col_name);
            
            if (col_idx < 0) {
                continue;
            }
            
            const ColumnDefinition* col = &schema->columns[col_idx];
            int written;
            
            if (col->type == TYPE_VARCHAR || col->type == TYPE_TEXT) {
                // String copy
                written = snprintf(current, remaining,
                    "        strcpy(results[result_count].%s, record->%s);\n",
                    col_name, col_name);
            } else {
                // Direct assignment
                written = snprintf(current, remaining,
                    "        results[result_count].%s = record->%s;\n",
                    col_name, col_name);
            }
            
            if (written < 0 || (size_t)written >= remaining) {
                return -1;
            }
            
            current += written;
            remaining -= written;
        }
    }
    
    // Copy to output buffer
    if (strlen(buffer) >= output_size) {
        return -1;
    }
    
    strcpy(output, buffer);
    return 0;
}

/**
 * @brief Validate select list against schema
 */
bool validate_select_list(const SelectStatement* stmt, const TableSchema* schema) {
    if (!stmt || !schema) {
        return false;
    }
    
    // SELECT * is always valid
    if (stmt->select_list.has_star) {
        return true;
    }
    
    // Check for COUNT(*)
    if (stmt->select_list.count == 1) {
        Expression* expr = stmt->select_list.expressions[0];
        if (expr->type == AST_FUNCTION_CALL &&
            strcasecmp(expr->data.function.function_name, "COUNT") == 0 &&
            expr->data.function.argument_count == 1 &&
            expr->data.function.arguments[0]->type == AST_STAR) {
            return true;  // COUNT(*) is valid
        }
    }
    
    // Validate each selected expression
    for (int i = 0; i < stmt->select_list.count; i++) {
        Expression* expr = stmt->select_list.expressions[i];
        
        if (expr->type == AST_COLUMN_REF) {
            const char* col_name = expr->data.column.column_name;
            if (find_column_index(schema, col_name) < 0) {
                return false;
            }
        } else {
            // We only support column references and COUNT(*) in projections for now
            return false;
        }
    }
    
    return true;
}
