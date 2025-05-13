/**
 * @file delete_parser.c
 * @brief Parses DELETE statements
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "parser_common.h"
#include "delete_parser.h" 

/**
 * @brief Create a new DELETE statement
 */
static DeleteStatement* create_delete_statement(void) {
    DeleteStatement* stmt = malloc(sizeof(DeleteStatement));
    if (!stmt) {
        return NULL;
    }
    
    stmt->table_name = NULL;
    stmt->where_clause = NULL;
    
    return stmt;
}

/**
 * @brief Parse WHERE clause for DELETE
 */
static bool parse_delete_where_clause(Parser* parser, DeleteStatement* stmt) {
    if (!match(parser, TOKEN_WHERE)) {
        return true; // WHERE is optional (but dangerous without it!)
    }
    
    stmt->where_clause = parse_expression(parser);
    return stmt->where_clause != NULL;
}

/**
 * @brief Parse a DELETE statement
 */
DeleteStatement* parse_delete_statement(Parser* parser) {
    if (!expect(parser, TOKEN_DELETE, "Expected DELETE")) {
        return NULL;
    }
    
    if (!expect(parser, TOKEN_FROM, "Expected FROM after DELETE")) {
        return NULL;
    }
    
    DeleteStatement* stmt = create_delete_statement();
    if (!stmt) {
        parser_set_error(parser, "Memory allocation failed");
        return NULL;
    }
    
    // Parse table name
    if (parser->current_token.type != TOKEN_IDENTIFIER) {
        parser_set_error(parser, "Expected table name");
        free_delete_statement(stmt);
        return NULL;
    }
    
    stmt->table_name = strdup(parser->current_token.value);
    consume_token(parser);
    
    // Parse optional WHERE clause
    if (!parse_delete_where_clause(parser, stmt)) {
        free_delete_statement(stmt);
        return NULL;
    }
    
    // Warn if no WHERE clause
    if (!stmt->where_clause) {
        fprintf(stderr, "WARNING: DELETE without WHERE clause will delete all records!\n");
    }
    
    return stmt;
}

/**
 * @brief Free a DELETE statement
 */
void free_delete_statement(DeleteStatement* stmt) {
    if (!stmt) {
        return;
    }
    
    free(stmt->table_name);
    
    if (stmt->where_clause) {
        free_expression(stmt->where_clause);
    }
    
    free(stmt);
}

/**
 * @brief Validate DELETE statement against schema
 */
bool validate_delete_statement(const DeleteStatement* stmt, const TableSchema* schema) {
    if (!stmt || !schema) {
        return false;
    }
    
    // Check table name matches schema
    if (strcmp(stmt->table_name, schema->name) != 0) {
        return false;
    }
    
    // Validate WHERE clause if present
    // (Additional validation could be added here)
    
    return true;
}
