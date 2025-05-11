/**
 * @file ast.h
 * @brief Abstract Syntax Tree definitions
 */

#ifndef UMBRA_AST_H
#define UMBRA_AST_H

#include <stdbool.h>
#include "../schema/type_system.h"

/**
 * Forward declarations
 */
typedef struct ASTNode ASTNode;
typedef struct SelectStatement SelectStatement;
typedef struct InsertStatement InsertStatement;
typedef struct UpdateStatement UpdateStatement;
typedef struct DeleteStatement DeleteStatement;
typedef struct Expression Expression;

/**
 * @struct SetClause
 * @brief Represents a SET clause in UPDATE
 */
typedef struct {
    char* column_name;      /**< Column to update */
    Expression* value;      /**< New value expression */
} SetClause;

/**
 * @struct UpdateStatement
 * @brief Represents an UPDATE statement
 */
struct UpdateStatement {
    char* table_name;       /**< Target table name */
    SetClause* set_clauses; /**< SET clauses */
    int set_count;          /**< Number of SET clauses */
    Expression* where_clause; /**< WHERE condition (optional) */
};

/**
 * @struct DeleteStatement
 * @brief Represents a DELETE statement
 */
struct DeleteStatement {
    char* table_name;       /**< Target table name */
    Expression* where_clause; /**< WHERE condition (optional) */
};

/**
 * @enum ASTNodeType
 * @brief Types of AST nodes
 */
typedef enum {
    AST_SELECT_STATEMENT,
    AST_INSERT_STATEMENT,
    AST_UPDATE_STATEMENT,
    AST_DELETE_STATEMENT,
    AST_EXPRESSION,
    AST_COLUMN_REF,
    AST_LITERAL,
    AST_BINARY_OP,
    AST_UNARY_OP,
    AST_FUNCTION_CALL,
    AST_STAR
} ASTNodeType;

/**
 * @enum OperatorType
 * @brief Types of operators
 */
typedef enum {
    OP_EQUALS,
    OP_NOT_EQUALS,
    OP_LESS,
    OP_LESS_EQUAL,
    OP_GREATER,
    OP_GREATER_EQUAL,
    OP_AND,
    OP_OR,
    OP_NOT,
    OP_PLUS,
    OP_MINUS,
    OP_MULTIPLY,
    OP_DIVIDE
} OperatorType;

/**
 * @struct ColumnRef
 * @brief References a column in a table
 */
typedef struct {
    char* table_name;   /**< Table name (optional) */
    char* column_name;  /**< Column name */
    char* alias;        /**< Column alias (optional) */
} ColumnRef;

/**
 * @struct Literal
 * @brief Represents a literal value
 */
typedef struct {
    DataType type;
    union {
        int int_value;
        double float_value;
        char* string_value;
        bool bool_value;
    } value;
} Literal;

/**
 * @struct BinaryOp
 * @brief Binary operation
 */
typedef struct {
    OperatorType op;
    Expression* left;
    Expression* right;
} BinaryOp;

/**
 * @struct UnaryOp
 * @brief Unary operation
 */
typedef struct {
    OperatorType op;
    Expression* operand;
} UnaryOp;

/**
 * @struct FunctionCall
 * @brief Function call expression
 */
typedef struct {
    char* function_name;
    Expression** arguments;
    int argument_count;
} FunctionCall;

/**
 * @struct Expression
 * @brief Generic expression node
 */
struct Expression {
    ASTNodeType type;
    union {
        ColumnRef column;
        Literal literal;
        BinaryOp binary_op;
        UnaryOp unary_op;
        FunctionCall function;
    } data;
};

/**
 * @struct SelectList
 * @brief List of selected columns/expressions
 */
typedef struct {
    Expression** expressions;
    int count;
    bool has_star;  /**< SELECT * */
} SelectList;

/**
 * @struct TableRef
 * @brief Reference to a table in FROM clause
 */
typedef struct {
    char* table_name;
    char* alias;
} TableRef;

/**
 * @struct OrderByClause
 * @brief ORDER BY clause
 */
typedef struct {
    Expression* expression;
    bool ascending;
} OrderByClause;

/**
 * @struct SelectStatement
 * @brief Complete SELECT statement AST
 */
struct SelectStatement {
    SelectList select_list;
    TableRef* from_table;
    Expression* where_clause;
    OrderByClause* order_by;
    int order_by_count;
    int limit_count;
};

/**
 * @struct ValueList
 * @brief List of values for INSERT
 */
typedef struct {
    Expression** values;
    int count;
} ValueList;

/**
 * @struct InsertStatement
 * @brief INSERT statement representation
 */
struct InsertStatement {
    char* table_name;
    char** columns;          /**< Column names (optional) */
    int column_count;
    ValueList value_list;
};

/**
 * @struct ASTNode
 * @brief Generic AST node
 */
struct ASTNode {
    ASTNodeType type;
    union {
        SelectStatement* select_stmt;
        InsertStatement* insert_stmt;
        UpdateStatement* update_stmt;
        DeleteStatement* delete_stmt;
        Expression* expression;
    } data;
};

/**
 * @brief Create a column reference
 */
ColumnRef* create_column_ref(const char* table_name, const char* column_name, const char* alias);

/**
 * @brief Create a literal value
 */
Literal* create_literal(DataType type, const char* value);

/**
 * @brief Create a binary operation
 */
BinaryOp* create_binary_op(OperatorType op, Expression* left, Expression* right);

/**
 * @brief Create a unary operation
 */
UnaryOp* create_unary_op(OperatorType op, Expression* operand);

/**
 * @brief Create a function call
 */
FunctionCall* create_function_call(const char* function_name);

/**
 * @brief Add argument to function call
 */
void add_function_argument(FunctionCall* call, Expression* arg);

/**
 * @brief Create an expression node
 */
Expression* create_expression(ASTNodeType type);

/**
 * @brief Create a SELECT statement
 */
SelectStatement* create_select_statement(void);

/**
 * @brief Add expression to select list
 */
void add_select_expression(SelectStatement* stmt, Expression* expr);

/**
 * @brief Create an INSERT statement
 */
InsertStatement* create_insert_statement(void);

/**
 * @brief Add column to INSERT statement
 */
void add_insert_column(InsertStatement* stmt, const char* column_name);

/**
 * @brief Add value to INSERT statement
 */
void add_insert_value(InsertStatement* stmt, Expression* value);

/**
 * @brief Free AST node and all children
 */
void free_ast_node(ASTNode* node);

/**
 * @brief Free expression and all children
 */
void free_expression(Expression* expr);

/**
 * @brief Free SELECT statement and all children
 */
void free_select_statement(SelectStatement* stmt);

/**
 * @brief Free INSERT statement and all children
 */
void free_insert_statement(InsertStatement* stmt);

/**
 * @brief Free UPDATE statement and all children
 */
void free_update_statement(UpdateStatement* stmt);

/**
 * @brief Free DELETE statement and all children
 */
void free_delete_statement(DeleteStatement* stmt);

/**
 * @brief Get operator name for debugging
 */
const char* operator_to_string(OperatorType op);

#endif /* UMBRA_AST_H */
