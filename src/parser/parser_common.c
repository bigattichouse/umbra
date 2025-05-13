/**
 * @file parser_common.c
 * @brief Common parser utilities implementation with reference-counted tokens
 */

#include "parser_common.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * @brief Set parser error
 */
void parser_set_error(Parser* parser, const char* format, ...) {
    parser->has_error = true;
    
    va_list args;
    va_start(args, format);
    vsnprintf(parser->error_message, sizeof(parser->error_message), format, args);
    va_end(args);
}

void consume_token(Parser* parser) {
    Token next_token = lexer_next_token(parser->lexer);
    token_unref(&parser->current_token);
    parser->current_token = next_token;
}

/**
 * @brief Check if current token matches expected type
 */
bool match(Parser* parser, TokenType type) {
    if (parser->current_token.type == type) {
        // Get the next token
        Token next_token = lexer_next_token(parser->lexer);
        
        // Unref current token before replacing it
        token_unref(&parser->current_token);
        
        // Replace with the new token (already has ref_count=1 from lexer_next_token)
        parser->current_token = next_token;
        
        return true;
    }
    return false;
}

/**
 * @brief Expect a specific token type
 */
bool expect(Parser* parser, TokenType type, const char* error_msg) {
    if (!match(parser, type)) {
        parser_set_error(parser, "%s. Got %s", error_msg, 
                  token_type_to_string(parser->current_token.type));
        return false;
    }
    return true;
}
