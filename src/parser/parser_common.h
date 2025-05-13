/**
 * @file parser_common.h
 * @brief Common parser utilities shared between statement parsers
 */

#ifndef UMBRA_PARSER_COMMON_H
#define UMBRA_PARSER_COMMON_H

#include "select_parser.h"
#include "lexer.h"
#include <stdarg.h>

/**
 * @brief Set parser error
 */
void parser_set_error(Parser* parser, const char* format, ...);

/**
 * @brief Check if current token matches expected type
 */
bool match(Parser* parser, TokenType type);

/**
 * @brief Expect a specific token type
 */
bool expect(Parser* parser, TokenType type, const char* error_msg);

/**
 * @brief Parse a general expression
 */
Expression* parse_expression(Parser* parser);

void consume_token(Parser* parser);

void cleanup_parser_and_lexer(Parser* parser, Lexer* lexer);

#endif /* UMBRA_PARSER_COMMON_H */
