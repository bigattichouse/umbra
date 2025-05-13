/**
 * @file select_parser.c
 * @brief Parses SELECT statements
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>  // Added for va_start, va_end
#include "parser_common.h"
#include "select_parser.h"

// Forward declarations - remove static for shared functions
Expression* parse_expression(Parser* parser);
static Expression* parse_primary(Parser* parser);
static Expression* parse_comparison(Parser* parser);
static Expression* parse_and(Parser* parser);
static Expression* parse_or(Parser* parser);
static Expression* parse_function_call(Parser* parser);

/**
 * @brief Initialize parser with lexer
 */
void parser_init(Parser* parser, Lexer* lexer) {
    parser->lexer = lexer;
    parser->error_message[0] = '\0';
    parser->has_error = false;
    parser->current_token = lexer_next_token(lexer);
}

/**
 * @brief Free parser resources
 */
void parser_free(Parser* parser) {
    // Parser doesn't own the lexer, so nothing to free
    (void)parser;
}

/**
 * @brief Parse a literal value
 */
static Expression* parse_literal(Parser* parser) {
    Expression* expr = create_expression(AST_LITERAL);
    
    switch (parser->current_token.type) {
        case TOKEN_NUMBER: {
            Literal* lit = create_literal(TYPE_FLOAT, parser->current_token.value);
            expr->data.literal = *lit;
            free(lit);
            parser->current_token = lexer_next_token(parser->lexer);
            break;
        }
        case TOKEN_STRING: {
            Literal* lit = create_literal(TYPE_TEXT, parser->current_token.value);
            expr->data.literal = *lit;
            free(lit);
            parser->current_token = lexer_next_token(parser->lexer);
            break;
        }
        case TOKEN_TRUE:
        case TOKEN_FALSE: {
            Literal* lit = create_literal(TYPE_BOOLEAN, 
                                        parser->current_token.type == TOKEN_TRUE ? "true" : "false");
            expr->data.literal = *lit;
            free(lit);
            parser->current_token = lexer_next_token(parser->lexer);
            break;
        }
        case TOKEN_NULL: {
            Literal* lit = create_literal(TYPE_TEXT, NULL);
            expr->data.literal = *lit;
            free(lit);
            parser->current_token = lexer_next_token(parser->lexer);
            break;
        }
        default:
            parser_set_error(parser, "Expected literal value");
            free(expr);
            return NULL;
    }
    
    return expr;
}

/**
 * @brief Parse a column reference
 */
static Expression* parse_column_ref(Parser* parser) {
    Expression* expr = create_expression(AST_COLUMN_REF);
    
    char* first_name = strdup(parser->current_token.value);
    parser->current_token = lexer_next_token(parser->lexer);
    
    // Check for table.column syntax
    if (match(parser, TOKEN_DOT)) {
        if (parser->current_token.type != TOKEN_IDENTIFIER) {
            parser_set_error(parser, "Expected column name after '.'");
            free(first_name);
            free(expr);
            return NULL;
        }
        
        ColumnRef* ref = create_column_ref(first_name, parser->current_token.value, NULL);
        expr->data.column = *ref;
        free(ref);
        parser->current_token = lexer_next_token(parser->lexer);
    } else {
        ColumnRef* ref = create_column_ref(NULL, first_name, NULL);
        expr->data.column = *ref;
        free(ref);
    }
    
    free(first_name);
    return expr;
}

/**
 * @brief Parse a comparison expression
 */
static Expression* parse_comparison(Parser* parser) {
    Expression* left = parse_primary(parser);
    if (!left) return NULL;
    
    while (true) {
        OperatorType op;
        switch (parser->current_token.type) {
            case TOKEN_EQUALS:      op = OP_EQUALS; break;
            case TOKEN_NOT_EQUALS:  op = OP_NOT_EQUALS; break;
            case TOKEN_LESS:        op = OP_LESS; break;
            case TOKEN_LESS_EQUAL:  op = OP_LESS_EQUAL; break;
            case TOKEN_GREATER:     op = OP_GREATER; break;
            case TOKEN_GREATER_EQUAL: op = OP_GREATER_EQUAL; break;
            default:
                return left;
        }
        
        parser->current_token = lexer_next_token(parser->lexer);
        Expression* right = parse_primary(parser);
        if (!right) {
            free_expression(left);
            return NULL;
        }
        
        Expression* expr = create_expression(AST_BINARY_OP);
        BinaryOp* bin_op = create_binary_op(op, left, right);
        expr->data.binary_op = *bin_op;
        free(bin_op);
        
        left = expr;
    }
    
    return left;
}

/**
 * @brief Parse an AND expression
 */
static Expression* parse_and(Parser* parser) {
    Expression* left = parse_comparison(parser);
    if (!left) return NULL;
    
    while (match(parser, TOKEN_AND)) {
        Expression* right = parse_comparison(parser);
        if (!right) {
            free_expression(left);
            return NULL;
        }
        
        Expression* expr = create_expression(AST_BINARY_OP);
        BinaryOp* bin_op = create_binary_op(OP_AND, left, right);
        expr->data.binary_op = *bin_op;
        free(bin_op);
        
        left = expr;
    }
    
    return left;
}

/**
 * @brief Parse an OR expression
 */
static Expression* parse_or(Parser* parser) {
    Expression* left = parse_and(parser);
    if (!left) return NULL;
    
    while (match(parser, TOKEN_OR)) {
        Expression* right = parse_and(parser);
        if (!right) {
            free_expression(left);
            return NULL;
        }
        
        Expression* expr = create_expression(AST_BINARY_OP);
        BinaryOp* bin_op = create_binary_op(OP_OR, left, right);
        expr->data.binary_op = *bin_op;
        free(bin_op);
        
        left = expr;
    }
    
    return left;
}

/**
 * @brief Parse a general expression
 */
Expression* parse_expression(Parser* parser) {
    return parse_or(parser);
}

/**
 * @brief Parse SELECT list
 */
static bool parse_select_list(Parser* parser, SelectStatement* stmt) {
    // Check for SELECT *
    if (match(parser, TOKEN_STAR)) {
        stmt->select_list.has_star = true;
        return true;
    }
    
    // Parse expressions
    while (true) {
        Expression* expr = parse_expression(parser);
        if (!expr) {
            return false;
        }
        
        add_select_expression(stmt, expr);
        
        if (!match(parser, TOKEN_COMMA)) {
            break;
        }
    }
    
    return true;
}

/**
 * @brief Parse FROM clause
 */
static bool parse_from_clause(Parser* parser, SelectStatement* stmt) {
    if (!expect(parser, TOKEN_FROM, "Expected FROM after SELECT list")) {
        return false;
    }
    
    if (parser->current_token.type != TOKEN_IDENTIFIER) {
        parser_set_error(parser, "Expected table name after FROM");
        return false;
    }
    
    stmt->from_table = malloc(sizeof(TableRef));
    stmt->from_table->table_name = strdup(parser->current_token.value);
    stmt->from_table->alias = NULL;
    
    parser->current_token = lexer_next_token(parser->lexer);
    
    // Check for alias
    if (parser->current_token.type == TOKEN_AS) {
        parser->current_token = lexer_next_token(parser->lexer);
        if (parser->current_token.type != TOKEN_IDENTIFIER) {
            parser_set_error(parser, "Expected alias after AS");
            return false;
        }
        stmt->from_table->alias = strdup(parser->current_token.value);
        parser->current_token = lexer_next_token(parser->lexer);
    } else if (parser->current_token.type == TOKEN_IDENTIFIER) {
        // Implicit alias
        stmt->from_table->alias = strdup(parser->current_token.value);
        parser->current_token = lexer_next_token(parser->lexer);
    }
    
    return true;
}

/**
 * @brief Parse WHERE clause
 */
static bool parse_where_clause(Parser* parser, SelectStatement* stmt) {
    if (!match(parser, TOKEN_WHERE)) {
        return true; // WHERE is optional
    }
    
    stmt->where_clause = parse_expression(parser);
    return stmt->where_clause != NULL;
}

/**
 * @brief Parse a SELECT statement
 */
SelectStatement* parse_select_statement(Parser* parser) {
    if (!expect(parser, TOKEN_SELECT, "Expected SELECT")) {
        return NULL;
    }
    
    SelectStatement* stmt = create_select_statement();
    
    // Parse SELECT list
    if (!parse_select_list(parser, stmt)) {
        free_select_statement(stmt);
        return NULL;
    }
    
    // Parse FROM clause
    if (!parse_from_clause(parser, stmt)) {
        free_select_statement(stmt);
        return NULL;
    }
    
    // Parse WHERE clause (optional)
    if (!parse_where_clause(parser, stmt)) {
        free_select_statement(stmt);
        return NULL;
    }
    
    // TODO: Parse ORDER BY, LIMIT, etc.
    
    return stmt;
}

/**
 * @brief Get parser error message
 */
const char* parser_get_error(const Parser* parser) {
    return parser->has_error ? parser->error_message : NULL;
}

/**
 * @brief Check if parser has error
 */
bool parser_has_error(const Parser* parser) {
    return parser->has_error;
}

/**
 * @brief Parse a function call
 */
/**
 * @brief Parse a function call
 */
/**
 * @brief Parse a function call
 */
static Expression* parse_function_call(Parser* parser) {
    if (parser->current_token.type != TOKEN_IDENTIFIER) {
        parser_set_error(parser, "Expected function name");
        return NULL;
    }
    
    char* function_name = strdup(parser->current_token.value);
    parser->current_token = lexer_next_token(parser->lexer);
    
    if (!expect(parser, TOKEN_LPAREN, "Expected '(' after function name")) {
        free(function_name);
        return NULL;
    }
    
    // Create function call expression
    Expression* expr = create_expression(AST_FUNCTION_CALL);
    if (!expr) {
        free(function_name);
        parser_set_error(parser, "Memory allocation failed");
        return NULL;
    }
    
    FunctionCall* func_call = create_function_call(function_name);
    free(function_name); // Function name copied to func_call
    
    if (!func_call) {
        free_expression(expr);
        parser_set_error(parser, "Memory allocation failed");
        return NULL;
    }
    
    expr->data.function = *func_call;
    free(func_call);
    
    // Parse arguments
    if (parser->current_token.type == TOKEN_RPAREN) {
        // No arguments
        parser->current_token = lexer_next_token(parser->lexer);
    } else if (parser->current_token.type == TOKEN_STAR) {
        // Special case for COUNT(*)
        Expression* star_expr = create_expression(AST_STAR);
        if (!star_expr) {
            free_expression(expr);
            parser_set_error(parser, "Memory allocation failed");
            return NULL;
        }
        
        add_function_argument(&expr->data.function, star_expr);
        parser->current_token = lexer_next_token(parser->lexer);
        
        if (!expect(parser, TOKEN_RPAREN, "Expected ')' after '*'")) {
            free_expression(expr);
            return NULL;
        }
    } else {
        // Parse comma-separated list of arguments
        while (true) {
            Expression* arg = parse_expression(parser);
            if (!arg) {
                free_expression(expr);
                return NULL;
            }
            
            add_function_argument(&expr->data.function, arg);
            
            if (match(parser, TOKEN_COMMA)) {
                continue;
            } else if (match(parser, TOKEN_RPAREN)) {
                break;
            } else {
                parser_set_error(parser, "Expected ',' or ')' in argument list");
                free_expression(expr);
                return NULL;
            }
        }
    }
    
    return expr;
}

static Expression* parse_primary(Parser* parser) {
    switch (parser->current_token.type) {
        case TOKEN_NUMBER:
        case TOKEN_STRING:
        case TOKEN_TRUE:
        case TOKEN_FALSE:
        case TOKEN_NULL:
            return parse_literal(parser);
            
        case TOKEN_IDENTIFIER: {
            // Look ahead to see if this is a function call
            Token next_token = lexer_peek_token(parser->lexer);
            if (next_token.type == TOKEN_LPAREN) {
                return parse_function_call(parser);
            } else {
                return parse_column_ref(parser);
            }
        }
        
        case TOKEN_LPAREN: {
            parser->current_token = lexer_next_token(parser->lexer);
            Expression* expr = parse_expression(parser);
            if (!expr) return NULL;
            
            if (!expect(parser, TOKEN_RPAREN, "Expected ')' after expression")) {
                free_expression(expr);
                return NULL;
            }
            return expr;
        }
        
        case TOKEN_STAR: {
            Expression* expr = create_expression(AST_STAR);
            parser->current_token = lexer_next_token(parser->lexer);
            return expr;
        }
        
        default:
            parser_set_error(parser, "Expected expression");
            return NULL;
    }
}
