/**
 * @file parser_common.c
 * @brief Common parser utilities implementation
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

/**
 * @brief Check if current token matches expected type
 */
bool match(Parser* parser, TokenType type) {
    if (parser->current_token.type == type) {
        // Free the value before getting the next token
        if (parser->current_token.value) {
            free(parser->current_token.value);
            parser->current_token.value = NULL;
        }
        
        // Get the next token
        Token next_token = lexer_next_token(parser->lexer);
        
        // Copy the token fields
        parser->current_token.type = next_token.type;
        parser->current_token.line = next_token.line;
        parser->current_token.column = next_token.column;
        
        // Handle the value field carefully to avoid leaks
        if (next_token.value) {
            // Create a deep copy of the value
            parser->current_token.value = strdup(next_token.value);
            // No need to free next_token.value as lexer manages its own memory
        } else {
            parser->current_token.value = NULL;
        }
        
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
