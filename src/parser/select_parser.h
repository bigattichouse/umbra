/**
 * @file select_parser.h
 * @brief Interface for SELECT parsing
 */

#ifndef UMBRA_SELECT_PARSER_H
#define UMBRA_SELECT_PARSER_H

#include "ast.h"
#include "lexer.h"

/**
 * @struct Parser
 * @brief Parser state
 */
typedef struct {
    Lexer* lexer;
    Token current_token;
    char error_message[256];
    bool has_error;
} Parser;

/**
 * @brief Initialize parser with lexer
 * @param parser Parser instance
 * @param lexer Initialized lexer
 */
void parser_init(Parser* parser, Lexer* lexer);

/**
 * @brief Free parser resources
 * @param parser Parser instance
 */
void parser_free(Parser* parser);

/**
 * @brief Parse a SELECT statement
 * @param parser Parser instance
 * @return Parsed SELECT statement or NULL on error
 */
SelectStatement* parse_select_statement(Parser* parser);

/**
 * @brief Get parser error message
 * @param parser Parser instance
 * @return Error message or NULL if no error
 */
const char* parser_get_error(const Parser* parser);

/**
 * @brief Check if parser has error
 * @param parser Parser instance
 * @return true if parser has error, false otherwise
 */
bool parser_has_error(const Parser* parser);

#endif /* UMBRA_SELECT_PARSER_H */
