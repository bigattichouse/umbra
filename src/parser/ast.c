/**
 * @file ast.c
 * @brief Abstract Syntax Tree implementations
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

/**
 * @brief Create a column reference
 */
ColumnRef* create_column_ref(const char* table_name, const char* column_name, const char* alias) {
    ColumnRef* ref = malloc(sizeof(ColumnRef));
    
    ref->table_name = table_name ? strdup(table_name) : NULL;
    ref->column_name = strdup(column_name);
    ref->alias = alias ? strdup(alias) : NULL;
    
    return ref;
}

/**
 * @brief Create a literal value
 */
Literal* create_literal(DataType type, const char* value) {
    Literal* lit = malloc(sizeof(Literal));
    lit->type = type;
    
    switch (type) {
        case TYPE_INT:
            lit->value.int_value = value ? atoi(value) : 0;
            break;
        case TYPE_FLOAT:
            lit->value.float_value = value ? atof(value) : 0.0;
            break;
        case TYPE_BOOLEAN:
            if (value) {
                lit->value.bool_value = (strcmp(value, "true") == 0 || 
                                        strcmp(value, "TRUE") == 0 || 
                                        strcmp(value, "1") == 0);
            } else {
                lit->value.bool_value = false;
            }
            break;
        case TYPE_VARCHAR:
        case TYPE_TEXT:
            lit->value.string_value = value ? strdup(value) : NULL;
            break;
        default:
            break;
    }
    
    return lit;
}

/**
 * @brief Create a binary operation
 */
BinaryOp* create_binary_op(OperatorType op, Expression* left, Expression* right) {
    BinaryOp* bin_op = malloc(sizeof(BinaryOp));
    bin_op->op = op;
    bin_op->left = left;
    bin_op->right = right;
    return bin_op;
}

/**
 * @brief Create a unary operation
 */
UnaryOp* create_unary_op(OperatorType op, Expression* operand) {
    UnaryOp* un_op = malloc(sizeof(UnaryOp));
    un_op->op = op;
    un_op->operand = operand;
    return un_op;
}

/**
 * @brief Create a function call
 */
FunctionCall* create_function_call(const char* function_name) {
    FunctionCall* call = malloc(sizeof(FunctionCall));
    call->function_name = strdup(function_name);
    call->arguments = NULL;
    call->argument_count = 0;
    return call;
}

/**
 * @brief Add argument to function call
 */
void add_function_argument(FunctionCall* call, Expression* arg) {
    call->arguments = realloc(call->arguments, 
                             (call->argument_count + 1) * sizeof(Expression*));
    call->arguments[call->argument_count] = arg;
    call->argument_count++;
}

/**
 * @brief Create an expression node
 */
Expression* create_expression(ASTNodeType type) {
    Expression* expr = malloc(sizeof(Expression));
    expr->type = type;
    return expr;
}

/**
 * @brief Create a SELECT statement
 */
SelectStatement* create_select_statement(void) {
    SelectStatement* stmt = malloc(sizeof(SelectStatement));
    
    stmt->select_list.expressions = NULL;
    stmt->select_list.count = 0;
    stmt->select_list.has_star = false;
    stmt->from_table = NULL;
    stmt->where_clause = NULL;
    stmt->order_by = NULL;
    stmt->order_by_count = 0;
    stmt->limit_count = -1;
    
    return stmt;
}

/**
 * @brief Add expression to select list
 */
void add_select_expression(SelectStatement* stmt, Expression* expr) {
    stmt->select_list.expressions = realloc(stmt->select_list.expressions,
                                           (stmt->select_list.count + 1) * sizeof(Expression*));
    stmt->select_list.expressions[stmt->select_list.count] = expr;
    stmt->select_list.count++;
}

/**
 * @brief Create an INSERT statement
 */
InsertStatement* create_insert_statement(void) {
    InsertStatement* stmt = malloc(sizeof(InsertStatement));
    
    stmt->table_name = NULL;
    stmt->columns = NULL;
    stmt->column_count = 0;
    stmt->value_list.values = NULL;
    stmt->value_list.count = 0;
    
    return stmt;
}

/**
 * @brief Add column to INSERT statement
 */
void add_insert_column(InsertStatement* stmt, const char* column_name) {
    stmt->columns = realloc(stmt->columns, 
                           (stmt->column_count + 1) * sizeof(char*));
    stmt->columns[stmt->column_count] = strdup(column_name);
    stmt->column_count++;
}

/**
 * @brief Add value to INSERT statement
 */
void add_insert_value(InsertStatement* stmt, Expression* value) {
    stmt->value_list.values = realloc(stmt->value_list.values,
                                     (stmt->value_list.count + 1) * sizeof(Expression*));
    stmt->value_list.values[stmt->value_list.count] = value;
    stmt->value_list.count++;
}

/**
 * @brief Free column reference
 */
static void free_column_ref(ColumnRef* ref) {
    if (!ref) return;
    free(ref->table_name);
    free(ref->column_name);
    free(ref->alias);
}

/**
 * @brief Free literal
 */
static void free_literal(Literal* lit) {
    if (!lit) return;
    if (lit->type == TYPE_VARCHAR || lit->type == TYPE_TEXT) {
        free(lit->value.string_value);
    }
}

/**
 * @brief Free expression and all children
 */
void free_expression(Expression* expr) {
    if (!expr) return;
    
    switch (expr->type) {
        case AST_COLUMN_REF:
            free_column_ref(&expr->data.column);
            break;
            
        case AST_LITERAL:
            free_literal(&expr->data.literal);
            break;
            
        case AST_BINARY_OP:
            free_expression(expr->data.binary_op.left);
            free_expression(expr->data.binary_op.right);
            break;
            
        case AST_UNARY_OP:
            free_expression(expr->data.unary_op.operand);
            break;
            
        case AST_FUNCTION_CALL:
            free(expr->data.function.function_name);
            for (int i = 0; i < expr->data.function.argument_count; i++) {
                free_expression(expr->data.function.arguments[i]);
            }
            free(expr->data.function.arguments);
            break;
            
        case AST_STAR:
            // Nothing to free for star
            break;
            
        default:
            break;
    }
    
    free(expr);
}

/**
 * @brief Free SELECT statement and all children
 */
void free_select_statement(SelectStatement* stmt) {
    if (!stmt) return;
    
    // Free select list
    for (int i = 0; i < stmt->select_list.count; i++) {
        free_expression(stmt->select_list.expressions[i]);
    }
    free(stmt->select_list.expressions);
    
    // Free from table
    if (stmt->from_table) {
        free(stmt->from_table->table_name);
        free(stmt->from_table->alias);
        free(stmt->from_table);
    }
    
    // Free where clause
    free_expression(stmt->where_clause);
    
    // Free order by clauses
    if (stmt->order_by) {
        for (int i = 0; i < stmt->order_by_count; i++) {
            free_expression(stmt->order_by[i].expression);
        }
        free(stmt->order_by);
    }
    
    // Free the statement itself
    free(stmt);
}

/**
 * @brief Free INSERT statement and all children
 */
void free_insert_statement(InsertStatement* stmt) {
    if (!stmt) return;
    
    // Free table name
    free(stmt->table_name);
    
    // Free column names
    for (int i = 0; i < stmt->column_count; i++) {
        free(stmt->columns[i]);
    }
    free(stmt->columns);
    
    // Free values
    for (int i = 0; i < stmt->value_list.count; i++) {
        free_expression(stmt->value_list.values[i]);
    }
    free(stmt->value_list.values);
    
    // Free the statement itself
    free(stmt);
}

/**
 * @brief Free AST node and all children
 */
void free_ast_node(ASTNode* node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_SELECT_STATEMENT:
            free_select_statement(node->data.select_stmt);
            break;
            
        case AST_INSERT_STATEMENT:
            free_insert_statement(node->data.insert_stmt);
            break;
            
        case AST_UPDATE_STATEMENT:
            free_update_statement(node->data.update_stmt);
            break;
            
        case AST_DELETE_STATEMENT:
            free_delete_statement(node->data.delete_stmt);
            break;
            
        case AST_EXPRESSION:
            free_expression(node->data.expression);
            break;
            
          
            
        default:
            break;
    }
    
    free(node);
}

/**
 * @brief Get operator name for debugging
 */
const char* operator_to_string(OperatorType op) {
    switch (op) {
        case OP_EQUALS:         return "=";
        case OP_NOT_EQUALS:     return "!=";
        case OP_LESS:           return "<";
        case OP_LESS_EQUAL:     return "<=";
        case OP_GREATER:        return ">";
        case OP_GREATER_EQUAL:  return ">=";
        case OP_AND:            return "AND";
        case OP_OR:             return "OR";
        case OP_NOT:            return "NOT";
        case OP_PLUS:           return "+";
        case OP_MINUS:          return "-";
        case OP_MULTIPLY:       return "*";
        case OP_DIVIDE:         return "/";
        default:                return "UNKNOWN";
    }
}
