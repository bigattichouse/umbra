/**
 * @file filter_generator.c
 * @brief Generates filter functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "filter_generator.h"

/**
 * @brief Generate comparison operator
 */
static const char* operator_to_c_operator(OperatorType op) {
    switch (op) {
        case OP_EQUALS:         return "==";
        case OP_NOT_EQUALS:     return "!=";
        case OP_LESS:           return "<";
        case OP_LESS_EQUAL:     return "<=";
        case OP_GREATER:        return ">";
        case OP_GREATER_EQUAL:  return ">=";
        case OP_AND:            return "&&";
        case OP_OR:             return "||";
        default:                return NULL;
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
 * @brief Generate column reference code
 */
static int generate_column_ref(const ColumnRef* ref, const TableSchema* schema,
                              char* output, size_t output_size) {
    int col_idx = find_column_index(schema, ref->column_name);
    if (col_idx < 0) {
        return -1;
    }
    
    const ColumnDefinition* col = &schema->columns[col_idx];
    
    // For string types, use strcmp
    if (col->type == TYPE_VARCHAR || col->type == TYPE_TEXT) {
        snprintf(output, output_size, "record->%s", ref->column_name);
    } else {
        snprintf(output, output_size, "record->%s", ref->column_name);
    }
    
    return 0;
}

/**
 * @brief Generate literal value code
 */
static int generate_literal(const Literal* lit, char* output, size_t output_size) {
    switch (lit->type) {
        case TYPE_INT:
            snprintf(output, output_size, "%d", lit->value.int_value);
            break;
        case TYPE_FLOAT:
            snprintf(output, output_size, "%f", lit->value.float_value);
            break;
        case TYPE_BOOLEAN:
            snprintf(output, output_size, "%s", lit->value.bool_value ? "true" : "false");
            break;
        case TYPE_VARCHAR:
        case TYPE_TEXT:
            snprintf(output, output_size, "\"%s\"", lit->value.string_value);
            break;
        default:
            return -1;
    }
    return 0;
}

/**
 * @brief Generate expression code
 */
static int generate_expression(const Expression* expr, const TableSchema* schema,
                              char* output, size_t output_size) {
    switch (expr->type) {
        case AST_COLUMN_REF:
            return generate_column_ref(&expr->data.column, schema, output, output_size);
            
        case AST_LITERAL:
            return generate_literal(&expr->data.literal, output, output_size);
            
        case AST_BINARY_OP: {
            char left_code[1024];
            char right_code[1024];
            
            if (generate_expression(expr->data.binary_op.left, schema, 
                                   left_code, sizeof(left_code)) != 0) {
                return -1;
            }
            
            if (generate_expression(expr->data.binary_op.right, schema, 
                                   right_code, sizeof(right_code)) != 0) {
                return -1;
            }
            
            const char* op = operator_to_c_operator(expr->data.binary_op.op);
            if (!op) {
                return -1;
            }
            
            // Handle string comparisons
            bool is_string_compare = false;
            if (expr->data.binary_op.left->type == AST_COLUMN_REF) {
                int col_idx = find_column_index(schema, expr->data.binary_op.left->data.column.column_name);
                if (col_idx >= 0) {
                    const ColumnDefinition* col = &schema->columns[col_idx];
                    is_string_compare = (col->type == TYPE_VARCHAR || col->type == TYPE_TEXT);
                }
            }
            
            if (is_string_compare && (expr->data.binary_op.op == OP_EQUALS || 
                                     expr->data.binary_op.op == OP_NOT_EQUALS)) {
                snprintf(output, output_size, "(strcmp(%s, %s) %s 0)",
                        left_code, right_code,
                        expr->data.binary_op.op == OP_EQUALS ? "==" : "!=");
            } else {
                snprintf(output, output_size, "(%s %s %s)", left_code, op, right_code);
            }
            break;
        }
            
        default:
            return -1;
    }
    
    return 0;
}

/**
 * @brief Generate C code for WHERE clause condition
 */
int generate_filter_condition(const Expression* expr, const TableSchema* schema,
                             char* output, size_t output_size) {
    int result = generate_expression(expr, schema, output, output_size);
    
    #ifdef DEBUG
    if (result == 0) {
        fprintf(stderr, "[DEBUG] Generated filter condition: %s\n", output);
    }
    #endif
    
    return result;
}

/**
 * @brief Generate filter function for WHERE clause
 */
int generate_filter_function(const Expression* expr, const TableSchema* schema,
                            const char* function_name, char* output, size_t output_size) {
    char condition[4096];
    if (generate_filter_condition(expr, schema, condition, sizeof(condition)) != 0) {
        return -1;
    }
    
    snprintf(output, output_size,
        "/**\n"
        " * @brief Filter function for WHERE clause\n"
        " * @param record Record to evaluate\n"
        " * @return true if record matches filter, false otherwise\n"
        " */\n"
        "static bool %s(const %s* record) {\n"
        "    return %s;\n"
        "}\n",
        function_name, schema->name, condition);
    
    return 0;
}

/**
 * @brief Validate that expression can be converted to filter
 */
bool validate_filter_expression(const Expression* expr, const TableSchema* schema) {
    if (!expr || !schema) {
        return false;
    }
    
    switch (expr->type) {
        case AST_COLUMN_REF:
            return find_column_index(schema, expr->data.column.column_name) >= 0;
            
        case AST_LITERAL:
            return true;
            
        case AST_BINARY_OP:
            return validate_filter_expression(expr->data.binary_op.left, schema) &&
                   validate_filter_expression(expr->data.binary_op.right, schema);
            
        default:
            return false;
    }
}
