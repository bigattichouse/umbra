/**
 * @file update_parser.c
 * @brief Parses UPDATE statements
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "parser_common.h"
#include "update_parser.h"

/**
 * @brief Create a new UPDATE statement
 */
static UpdateStatement* create_update_statement(void) {
    UpdateStatement* stmt = malloc(sizeof(UpdateStatement));
    if (!stmt) {
        return NULL;
    }
    
    stmt->table_name = NULL;
    stmt->set_clauses = NULL;
    stmt->set_count = 0;
    stmt->where_clause = NULL;
    
    return stmt;
}

/**
 * @brief Parse SET clause
 */
static bool parse_set_clause(Parser* parser, UpdateStatement* stmt) {
    if (!expect(parser, TOKEN_SET, "Expected SET")) {
        return false;
    }
    
    while (true) {
        // Parse column name
        if (parser->current_token.type != TOKEN_IDENTIFIER) {
            parser_set_error(parser, "Expected column name in SET clause");
            return false;
        }
        
        char* column_name = strdup(parser->current_token.value);
        parser->current_token = lexer_next_token(parser->lexer);
        
        // Expect equals sign
        if (!expect(parser, TOKEN_EQUALS, "Expected '=' after column name")) {
            free(column_name);
            return false;
        }
        
        // Parse value expression
        Expression* value = parse_expression(parser);
        if (!value) {
            free(column_name);
            return false;
        }
        
        // Add SET clause
        stmt->set_clauses = realloc(stmt->set_clauses, 
                                   (stmt->set_count + 1) * sizeof(SetClause));
        stmt->set_clauses[stmt->set_count].column_name = column_name;
        stmt->set_clauses[stmt->set_count].value = value;
        stmt->set_count++;
        
        // Check for more SET clauses
        if (!match(parser, TOKEN_COMMA)) {
            break;
        }
    }
    
    return true;
}

/**
 * @brief Parse WHERE clause
 */
static bool parse_update_where_clause(Parser* parser, UpdateStatement* stmt) {
    if (!match(parser, TOKEN_WHERE)) {
        return true; // WHERE is optional
    }
    
    stmt->where_clause = parse_expression(parser);
    return stmt->where_clause != NULL;
}

/**
 * @brief Parse an UPDATE statement
 */
UpdateStatement* parse_update_statement(Parser* parser) {
    if (!expect(parser, TOKEN_UPDATE, "Expected UPDATE")) {
        return NULL;
    }
    
    UpdateStatement* stmt = create_update_statement();
    if (!stmt) {
        parser_set_error(parser, "Memory allocation failed");
        return NULL;
    }
    
    // Parse table name
    if (parser->current_token.type != TOKEN_IDENTIFIER) {
        parser_set_error(parser, "Expected table name");
        free_update_statement(stmt);
        return NULL;
    }
    
    stmt->table_name = strdup(parser->current_token.value);
    parser->current_token = lexer_next_token(parser->lexer);
    
    // Parse SET clause
    if (!parse_set_clause(parser, stmt)) {
        free_update_statement(stmt);
        return NULL;
    }
    
    // Parse optional WHERE clause
    if (!parse_update_where_clause(parser, stmt)) {
        free_update_statement(stmt);
        return NULL;
    }
    
    return stmt;
}

/**
 * @brief Free an UPDATE statement
 */
void free_update_statement(UpdateStatement* stmt) {
    if (!stmt) {
        return;
    }
    
    free(stmt->table_name);
    
    // Free SET clauses
    for (int i = 0; i < stmt->set_count; i++) {
        free(stmt->set_clauses[i].column_name);
        free_expression(stmt->set_clauses[i].value);
    }
    free(stmt->set_clauses);
    
    // Free WHERE clause
    if (stmt->where_clause) {
        free_expression(stmt->where_clause);
    }
    
    free(stmt);
}

/**
 * @brief Validate UPDATE statement against schema
 */
bool validate_update_statement(const UpdateStatement* stmt, const TableSchema* schema) {
    if (!stmt || !schema) {
        return false;
    }
    
    // Check table name matches schema
    if (strcmp(stmt->table_name, schema->name) != 0) {
        return false;
    }
    
    // Validate SET columns exist in schema
    for (int i = 0; i < stmt->set_count; i++) {
        bool found = false;
        for (int j = 0; j < schema->column_count; j++) {
            if (strcmp(stmt->set_clauses[i].column_name, schema->columns[j].name) == 0) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            fprintf(stderr, "Column '%s' not found in table '%s'\n", 
                   stmt->set_clauses[i].column_name, stmt->table_name);
            return false;
        }
    }
    
    // Check not trying to update primary key columns
    for (int i = 0; i < stmt->set_count; i++) {
        for (int j = 0; j < schema->column_count; j++) {
            if (strcmp(stmt->set_clauses[i].column_name, schema->columns[j].name) == 0 &&
                schema->columns[j].is_primary_key) {
                fprintf(stderr, "Cannot update primary key column '%s'\n", 
                       stmt->set_clauses[i].column_name);
                return false;
            }
        }
    }
    
    return true;
}
