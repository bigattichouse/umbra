/**
 * @file lexer.c
 * @brief Tokenizes SQL input with reference-counted tokens
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

/**
 * @brief Create a new token with ref count 1
 */
Token token_create(TokenType type, const char* value, int line, int column) {
    Token token;
    token.type = type;
    token.value = value ? strdup(value) : NULL;
    token.line = line;
    token.column = column;
    token.ref_count = 1;
    return token;
}

/**
 * @brief Increment reference count
 */
void token_ref(Token* token) {
    if (token) {
        token->ref_count++;
    }
}

/**
 * @brief Decrement reference count, free if needed
 */
void token_unref(Token* token) {
    if (token && token->ref_count > 0) {
        token->ref_count--;
        if (token->ref_count == 0 && token->value) {
            free(token->value);
            token->value = NULL;
        }
    }
}

/**
 * @brief Copy token (incrementing ref count of value)
 */
Token token_copy(const Token* token) {
    Token copy = *token; // Make a shallow copy
    if (copy.value) {
        copy.ref_count = 1; // New copy starts with ref_count of 1
    }
    return copy;
}

void lexer_init(Lexer* lexer, const char* input) {
    lexer->input = input;
    lexer->position = 0;
    lexer->length = strlen(input);
    lexer->line = 1;
    lexer->column = 1;
    lexer->current_token.type = TOKEN_EOF;
    lexer->current_token.value = NULL;
    lexer->current_token.ref_count = 0;
}

void lexer_free(Lexer* lexer) {
    token_unref(&lexer->current_token);
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
        return token_create(type, NULL, lexer->line, start_column);
    }
    
    Token token = token_create(TOKEN_IDENTIFIER, identifier, lexer->line, start_column);
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
            
            Token token = token_create(TOKEN_STRING, string, lexer->line, start_column);
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
    return token_create(TOKEN_ERROR, "Unterminated string", lexer->line, start_column);
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
    
    Token token = token_create(TOKEN_NUMBER, number, lexer->line, start_column);
    free(number);
    return token;
}

Token lexer_next_token(Lexer* lexer) {
    // Release the current token first
    token_unref(&lexer->current_token);
    
    skip_whitespace(lexer);
    
    if (lexer->position >= lexer->length) {
        // End of input
        lexer->current_token = token_create(TOKEN_EOF, NULL, lexer->line, lexer->column);
        return token_copy(&lexer->current_token);
    }
    
    char c = lexer->input[lexer->position];
    int column = lexer->column;
    
    // Single character tokens
    switch (c) {
        case ',':
            lexer->position++; lexer->column++;
            lexer->current_token = token_create(TOKEN_COMMA, NULL, lexer->line, column);
            return token_copy(&lexer->current_token);
            
        case '.':
            lexer->position++; lexer->column++;
            lexer->current_token = token_create(TOKEN_DOT, NULL, lexer->line, column);
            return token_copy(&lexer->current_token);
            
        case ';':
            lexer->position++; lexer->column++;
            lexer->current_token = token_create(TOKEN_SEMICOLON, NULL, lexer->line, column);
            return token_copy(&lexer->current_token);
            
        case '(':
            lexer->position++; lexer->column++;
            lexer->current_token = token_create(TOKEN_LPAREN, NULL, lexer->line, column);
            return token_copy(&lexer->current_token);
            
        case ')':
            lexer->position++; lexer->column++;
            lexer->current_token = token_create(TOKEN_RPAREN, NULL, lexer->line, column);
            return token_copy(&lexer->current_token);
            
        case '+':
            lexer->position++; lexer->column++;
            lexer->current_token = token_create(TOKEN_PLUS, NULL, lexer->line, column);
            return token_copy(&lexer->current_token);
            
        case '-':
            lexer->position++; lexer->column++;
            lexer->current_token = token_create(TOKEN_MINUS, NULL, lexer->line, column);
            return token_copy(&lexer->current_token);
            
        case '*':
            lexer->position++; lexer->column++;
            lexer->current_token = token_create(TOKEN_STAR, NULL, lexer->line, column);
            return token_copy(&lexer->current_token);
            
        case '/':
            lexer->position++; lexer->column++;
            lexer->current_token = token_create(TOKEN_SLASH, NULL, lexer->line, column);
            return token_copy(&lexer->current_token);
            
        case '=':
            lexer->position++; lexer->column++;
            lexer->current_token = token_create(TOKEN_EQUALS, NULL, lexer->line, column);
            return token_copy(&lexer->current_token);
            
        case '<':
            lexer->position++; lexer->column++;
            if (lexer->position < lexer->length) {
                if (lexer->input[lexer->position] == '=') {
                    lexer->position++; lexer->column++;
                    lexer->current_token = token_create(TOKEN_LESS_EQUAL, NULL, lexer->line, column);
                } else if (lexer->input[lexer->position] == '>') {
                    lexer->position++; lexer->column++;
                    lexer->current_token = token_create(TOKEN_NOT_EQUALS, NULL, lexer->line, column);
                } else {
                    lexer->current_token = token_create(TOKEN_LESS, NULL, lexer->line, column);
                }
            } else {
                lexer->current_token = token_create(TOKEN_LESS, NULL, lexer->line, column);
            }
            return token_copy(&lexer->current_token);
            
        case '>':
            lexer->position++; lexer->column++;
            if (lexer->position < lexer->length && lexer->input[lexer->position] == '=') {
                lexer->position++; lexer->column++;
                lexer->current_token = token_create(TOKEN_GREATER_EQUAL, NULL, lexer->line, column);
            } else {
                lexer->current_token = token_create(TOKEN_GREATER, NULL, lexer->line, column);
            }
            return token_copy(&lexer->current_token);
            
        case '!':
            if (lexer->position + 1 < lexer->length && lexer->input[lexer->position + 1] == '=') {
                lexer->position += 2; lexer->column += 2;
                lexer->current_token = token_create(TOKEN_NOT_EQUALS, NULL, lexer->line, column);
                return token_copy(&lexer->current_token);
            }
            break;
    }
    
    // Strings
    if (c == '\'' || c == '"') {
        lexer->current_token = scan_string(lexer);
        return token_copy(&lexer->current_token);
    }
    
    // Numbers
    if (isdigit(c)) {
        lexer->current_token = scan_number(lexer);
        return token_copy(&lexer->current_token);
    }
    
    // Identifiers and keywords
    if (isalpha(c) || c == '_') {
        lexer->current_token = scan_identifier(lexer);
        return token_copy(&lexer->current_token);
    }
    
    // Error token for unexpected characters
    lexer->position++; lexer->column++;
    
    char error_msg[32];
    snprintf(error_msg, sizeof(error_msg), "Unexpected character: %c", c);
    lexer->current_token = token_create(TOKEN_ERROR, error_msg, lexer->line, column);
    
    return token_copy(&lexer->current_token);
}

Token lexer_peek_token(Lexer* lexer) {
    // Save current state
    int saved_position = lexer->position;
    int saved_line = lexer->line;
    int saved_column = lexer->column;
    
    // Save current token
    Token saved_token = token_copy(&lexer->current_token);
    
    // Get next token
    Token next_token = lexer_next_token(lexer);
    
    // Restore state
    lexer->position = saved_position;
    lexer->line = saved_line;
    lexer->column = saved_column;
    
    // Clean up current token and restore the saved one
    token_unref(&lexer->current_token);
    lexer->current_token = saved_token;
    
    return next_token;
}

const char* token_type_to_string(TokenType type) {
    if (type >= 0 && type < sizeof(token_type_names) / sizeof(token_type_names[0])) {
        return token_type_names[type];
    }
    return "UNKNOWN";
}

void token_free(Token* token) {
    token_unref(token);
}
