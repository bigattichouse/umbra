/**
 * @file lexer.c
 * @brief Tokenizes SQL input
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lexer.h"

/**
 * @brief Keyword table for SQL
 */
typedef struct {
    const char* keyword;
    TokenType type;
} Keyword;

static const Keyword keywords[] = {
    {"SELECT", TOKEN_SELECT},
    {"INSERT", TOKEN_INSERT},
    {"UPDATE", TOKEN_UPDATE},
    {"DELETE", TOKEN_DELETE},
    {"INTO", TOKEN_INTO},
    {"VALUES", TOKEN_VALUES},
    {"SET", TOKEN_SET},
    {"FROM", TOKEN_FROM},
    {"WHERE", TOKEN_WHERE},
    {"AND", TOKEN_AND},
    {"OR", TOKEN_OR},
    {"NOT", TOKEN_NOT},
    {"AS", TOKEN_AS},
    {"ASC", TOKEN_ASC},
    {"DESC", TOKEN_DESC},
    {"ORDER", TOKEN_ORDER},
    {"BY", TOKEN_BY},
    {"LIMIT", TOKEN_LIMIT},
    {"GROUP", TOKEN_GROUP},
    {"TRUE", TOKEN_TRUE},
    {"FALSE", TOKEN_FALSE},
    {"NULL", TOKEN_NULL},
    {NULL, TOKEN_EOF}
};

/**
 * @brief Token type names for debugging
 */
static const char* token_type_names[] = {
    "EOF", "ERROR",
    "SELECT", "FROM", "WHERE", "AND", "OR", "NOT", "AS",
    "ASC", "DESC", "ORDER", "BY", "LIMIT", "GROUP",
    "IDENTIFIER", "STRING", "NUMBER", "TRUE", "FALSE", "NULL",
    "EQUALS", "NOT_EQUALS", "LESS", "LESS_EQUAL", "GREATER", "GREATER_EQUAL",
    "PLUS", "MINUS", "STAR", "SLASH",
    "COMMA", "DOT", "SEMICOLON", "LPAREN", "RPAREN", "ALL"
};

void lexer_init(Lexer* lexer, const char* input) {
    lexer->input = input;
    lexer->position = 0;
    lexer->length = strlen(input);
    lexer->line = 1;
    lexer->column = 1;
    lexer->current_token.type = TOKEN_EOF;
    lexer->current_token.value = NULL;
}

void lexer_free(Lexer* lexer) {
    if (lexer->current_token.value) {
        free(lexer->current_token.value);
        lexer->current_token.value = NULL;
    }
}

static void skip_whitespace(Lexer* lexer) {
    while (lexer->position < lexer->length && 
           isspace(lexer->input[lexer->position])) {
        if (lexer->input[lexer->position] == '\n') {
            lexer->line++;
            lexer->column = 1;
        } else {
            lexer->column++;
        }
        lexer->position++;
    }
}

static TokenType check_keyword(const char* identifier) {
    char upper[256];
    size_t len = strlen(identifier);
    if (len >= sizeof(upper)) {
        return TOKEN_IDENTIFIER;
    }
    
    for (size_t i = 0; i < len; i++) {
        upper[i] = toupper(identifier[i]);
    }
    upper[len] = '\0';
    
    for (int i = 0; keywords[i].keyword != NULL; i++) {
        if (strcmp(upper, keywords[i].keyword) == 0) {
            return keywords[i].type;
        }
    }
    
    return TOKEN_IDENTIFIER;
}

static Token make_token(TokenType type, const char* value, int line, int column) {
    Token token;
    token.type = type;
    token.value = value ? strdup(value) : NULL;
    token.line = line;
    token.column = column;
    return token;
}

static Token scan_identifier(Lexer* lexer) {
    int start = lexer->position;
    int start_column = lexer->column;
    
    while (lexer->position < lexer->length && 
           (isalnum(lexer->input[lexer->position]) || 
            lexer->input[lexer->position] == '_')) {
        lexer->position++;
        lexer->column++;
    }
    
    int length = lexer->position - start;
    char* identifier = malloc(length + 1);
    strncpy(identifier, &lexer->input[start], length);
    identifier[length] = '\0';
    
    TokenType type = check_keyword(identifier);
    
    if (type != TOKEN_IDENTIFIER) {
        free(identifier);
        return make_token(type, NULL, lexer->line, start_column);
    }
    
    Token token = make_token(TOKEN_IDENTIFIER, identifier, lexer->line, start_column);
    free(identifier);
    return token;
}

static Token scan_string(Lexer* lexer) {
    int start_column = lexer->column;
    char quote = lexer->input[lexer->position++]; // ' or "
    lexer->column++;
    
    int start = lexer->position;
    
    while (lexer->position < lexer->length) {
        if (lexer->input[lexer->position] == quote) {
            int length = lexer->position - start;
            char* string = malloc(length + 1);
            strncpy(string, &lexer->input[start], length);
            string[length] = '\0';
            
            lexer->position++;
            lexer->column++;
            
            Token token = make_token(TOKEN_STRING, string, lexer->line, start_column);
            free(string);
            return token;
        }
        
        if (lexer->input[lexer->position] == '\n') {
            lexer->line++;
            lexer->column = 1;
        } else {
            lexer->column++;
        }
        
        lexer->position++;
    }
    
    // Unterminated string
    return make_token(TOKEN_ERROR, "Unterminated string", lexer->line, start_column);
}

static Token scan_number(Lexer* lexer) {
    int start = lexer->position;
    int start_column = lexer->column;
    
    while (lexer->position < lexer->length && 
           (isdigit(lexer->input[lexer->position]) ||
            lexer->input[lexer->position] == '.')) {
        lexer->position++;
        lexer->column++;
    }
    
    int length = lexer->position - start;
    char* number = malloc(length + 1);
    strncpy(number, &lexer->input[start], length);
    number[length] = '\0';
    
    Token token = make_token(TOKEN_NUMBER, number, lexer->line, start_column);
    free(number);
    return token;
}

Token lexer_next_token(Lexer* lexer) {
    // Create a token to return (this will be a copy)
    Token result;
    
    // Free the lexer's current token value if it exists
    if (lexer->current_token.value) {
        free(lexer->current_token.value);
        lexer->current_token.value = NULL;
    }
    
    skip_whitespace(lexer);
    
    if (lexer->position >= lexer->length) {
        // End of input
        result.type = TOKEN_EOF;
        result.value = NULL;
        result.line = lexer->line;
        result.column = lexer->column;
        
        // Update lexer's current token
        lexer->current_token = result;
        return result;
    }
    
    char c = lexer->input[lexer->position];
    int column = lexer->column;
    
    // Single character tokens
    switch (c) {
        case ',':
            lexer->position++; lexer->column++;
            result.type = TOKEN_COMMA;
            result.value = NULL;
            result.line = lexer->line;
            result.column = column;
            lexer->current_token = result;
            return result;
            
        case '.':
            lexer->position++; lexer->column++;
            result.type = TOKEN_DOT;
            result.value = NULL;
            result.line = lexer->line;
            result.column = column;
            lexer->current_token = result;
            return result;
            
        case ';':
            lexer->position++; lexer->column++;
            result.type = TOKEN_SEMICOLON;
            result.value = NULL;
            result.line = lexer->line;
            result.column = column;
            lexer->current_token = result;
            return result;
            
        case '(':
            lexer->position++; lexer->column++;
            result.type = TOKEN_LPAREN;
            result.value = NULL;
            result.line = lexer->line;
            result.column = column;
            lexer->current_token = result;
            return result;
            
        case ')':
            lexer->position++; lexer->column++;
            result.type = TOKEN_RPAREN;
            result.value = NULL;
            result.line = lexer->line;
            result.column = column;
            lexer->current_token = result;
            return result;
            
        case '+':
            lexer->position++; lexer->column++;
            result.type = TOKEN_PLUS;
            result.value = NULL;
            result.line = lexer->line;
            result.column = column;
            lexer->current_token = result;
            return result;
            
        case '-':
            lexer->position++; lexer->column++;
            result.type = TOKEN_MINUS;
            result.value = NULL;
            result.line = lexer->line;
            result.column = column;
            lexer->current_token = result;
            return result;
            
        case '*':
            lexer->position++; lexer->column++;
            result.type = TOKEN_STAR;
            result.value = NULL;
            result.line = lexer->line;
            result.column = column;
            lexer->current_token = result;
            return result;
            
        case '/':
            lexer->position++; lexer->column++;
            result.type = TOKEN_SLASH;
            result.value = NULL;
            result.line = lexer->line;
            result.column = column;
            lexer->current_token = result;
            return result;
            
        case '=':
            lexer->position++; lexer->column++;
            result.type = TOKEN_EQUALS;
            result.value = NULL;
            result.line = lexer->line;
            result.column = column;
            lexer->current_token = result;
            return result;
            
        case '<':
            lexer->position++; lexer->column++;
            if (lexer->position < lexer->length) {
                if (lexer->input[lexer->position] == '=') {
                    lexer->position++; lexer->column++;
                    result.type = TOKEN_LESS_EQUAL;
                    result.value = NULL;
                } else if (lexer->input[lexer->position] == '>') {
                    lexer->position++; lexer->column++;
                    result.type = TOKEN_NOT_EQUALS;
                    result.value = NULL;
                } else {
                    result.type = TOKEN_LESS;
                    result.value = NULL;
                }
            } else {
                result.type = TOKEN_LESS;
                result.value = NULL;
            }
            result.line = lexer->line;
            result.column = column;
            lexer->current_token = result;
            return result;
            
        case '>':
            lexer->position++; lexer->column++;
            if (lexer->position < lexer->length && lexer->input[lexer->position] == '=') {
                lexer->position++; lexer->column++;
                result.type = TOKEN_GREATER_EQUAL;
                result.value = NULL;
            } else {
                result.type = TOKEN_GREATER;
                result.value = NULL;
            }
            result.line = lexer->line;
            result.column = column;
            lexer->current_token = result;
            return result;
            
        case '!':
            if (lexer->position + 1 < lexer->length && lexer->input[lexer->position + 1] == '=') {
                lexer->position += 2; lexer->column += 2;
                result.type = TOKEN_NOT_EQUALS;
                result.value = NULL;
                result.line = lexer->line;
                result.column = column;
                lexer->current_token = result;
                return result;
            }
            break;
    }
    
    // Strings
    if (c == '\'' || c == '"') {
        result = scan_string(lexer);
        lexer->current_token = result;
        // Note: scan_string already allocates memory for the value
        return result;
    }
    
    // Numbers
    if (isdigit(c)) {
        result = scan_number(lexer);
        lexer->current_token = result;
        // Note: scan_number already allocates memory for the value
        return result;
    }
    
    // Identifiers and keywords
    if (isalpha(c) || c == '_') {
        result = scan_identifier(lexer);
        lexer->current_token = result;
        // Note: scan_identifier already allocates memory for the value
        return result;
    }
    
    // Error token for unexpected characters
    lexer->position++; lexer->column++;
    result.type = TOKEN_ERROR;
    
    char error_msg[32];
    snprintf(error_msg, sizeof(error_msg), "Unexpected character: %c", c);
    result.value = strdup(error_msg);
    
    result.line = lexer->line;
    result.column = column;
    
    lexer->current_token = result;
    return result;
}

Token lexer_peek_token(Lexer* lexer) {
    // Save current state
    int saved_position = lexer->position;
    int saved_line = lexer->line;
    int saved_column = lexer->column;
    
    // Save current token value
    char* saved_value = lexer->current_token.value ? strdup(lexer->current_token.value) : NULL;
    TokenType saved_type = lexer->current_token.type;
    int saved_line_token = lexer->current_token.line;
    int saved_column_token = lexer->current_token.column;
    
    // Temporarily set current_token.value to NULL to prevent it from being freed
    lexer->current_token.value = NULL;
    
    // Get next token
    Token next_token = lexer_next_token(lexer);
    
    // Restore state
    lexer->position = saved_position;
    lexer->line = saved_line;
    lexer->column = saved_column;
    
    // Clean up current token if needed
    if (lexer->current_token.value) {
        free(lexer->current_token.value);
    }
    
    // Restore original token
    lexer->current_token.type = saved_type;
    lexer->current_token.line = saved_line_token;
    lexer->current_token.column = saved_column_token;
    lexer->current_token.value = saved_value;  // Restored to original duplicated value
    
    return next_token;
}


const char* token_type_to_string(TokenType type) {
    if (type >= 0 && type < sizeof(token_type_names) / sizeof(token_type_names[0])) {
        return token_type_names[type];
    }
    return "UNKNOWN";
}

void token_free(Token* token) {
    if (token && token->value) {
        free(token->value);
        token->value = NULL;
    }
}
