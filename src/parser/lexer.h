/**
 * @file lexer.h
 * @brief Lexer interface
 */

#ifndef UMBRA_LEXER_H
#define UMBRA_LEXER_H

#include <stdbool.h>

/**
 * @enum TokenType
 * @brief SQL token types
 */
typedef enum {
    TOKEN_EOF = 0,
    TOKEN_ERROR,
    
    // Keywords
    TOKEN_SELECT,
    TOKEN_FROM,
    TOKEN_WHERE,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,
    TOKEN_AS,
    TOKEN_ASC,
    TOKEN_DESC,
    TOKEN_ORDER,
    TOKEN_BY,
    TOKEN_LIMIT,
    TOKEN_GROUP,
    
    // Identifiers and literals
    TOKEN_IDENTIFIER,
    TOKEN_STRING,
    TOKEN_NUMBER,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_NULL,
    
    // Operators
    TOKEN_EQUALS,       // =
    TOKEN_NOT_EQUALS,   // != or <>
    TOKEN_LESS,         // <
    TOKEN_LESS_EQUAL,   // <=
    TOKEN_GREATER,      // >
    TOKEN_GREATER_EQUAL,// >=
    TOKEN_PLUS,         // +
    TOKEN_MINUS,        // -
    TOKEN_STAR,         // *
    TOKEN_SLASH,        // /
    
    // Punctuation
    TOKEN_COMMA,        // ,
    TOKEN_DOT,          // .
    TOKEN_SEMICOLON,    // ;
    TOKEN_LPAREN,       // (
    TOKEN_RPAREN,       // )
    
    // Special
    TOKEN_ALL           // *
} TokenType;

/**
 * @struct Token
 * @brief Represents a lexical token
 */
typedef struct {
    TokenType type;
    char* value;
    int line;
    int column;
} Token;

/**
 * @struct Lexer
 * @brief Lexical analyzer state
 */
typedef struct {
    const char* input;
    int position;
    int length;
    int line;
    int column;
    Token current_token;
} Lexer;

/**
 * @brief Initialize lexer with input
 * @param lexer Lexer instance
 * @param input SQL input string
 */
void lexer_init(Lexer* lexer, const char* input);

/**
 * @brief Free lexer resources
 * @param lexer Lexer instance
 */
void lexer_free(Lexer* lexer);

/**
 * @brief Get next token
 * @param lexer Lexer instance
 * @return Next token
 */
Token lexer_next_token(Lexer* lexer);

/**
 * @brief Peek at next token without consuming it
 * @param lexer Lexer instance
 * @return Next token
 */
Token lexer_peek_token(Lexer* lexer);

/**
 * @brief Get token type name for debugging
 * @param type Token type
 * @return String representation of token type
 */
const char* token_type_to_string(TokenType type);

/**
 * @brief Free a token
 * @param token Token to free
 */
void token_free(Token* token);

#endif /* UMBRA_LEXER_H */
