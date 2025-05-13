/**
 * @file insert_parser.c
 * @brief Parses INSERT statements
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "parser_common.h"
#include "insert_parser.h"

/**
 * @brief Create a new INSERT statement
 */
static InsertStatement* create_insert(void) {
    return create_insert_statement();
}

/**
 * @brief Parse column list in INSERT
 */
static bool parse_column_list(Parser* parser, InsertStatement* stmt) {
    if (!match(parser, TOKEN_LPAREN)) {
        return true; // Column list is optional
    }
    
    while (true) {
        if (parser->current_token.type != TOKEN_IDENTIFIER) {
            parser_set_error(parser, "Expected column name");
            return false;
        }
        
        add_insert_column(stmt, parser->current_token.value);
        consume_token(parser);
        
        if (match(parser, TOKEN_COMMA)) {
            continue;
        } else if (match(parser, TOKEN_RPAREN)) {
            break;
        } else {
            parser_set_error(parser, "Expected ',' or ')' in column list");
            return false;
        }
    }
    
    return true;
}

/**
 * @brief Parse VALUES clause
 */
static bool parse_values_clause(Parser* parser, InsertStatement* stmt) {
    if (!expect(parser, TOKEN_VALUES, "Expected VALUES")) {
        return false;
    }
    
    if (!expect(parser, TOKEN_LPAREN, "Expected '(' after VALUES")) {
        return false;
    }
    
    while (true) {
        Expression* value = parse_expression(parser);
        if (!value) {
            return false;
        }
        
        add_insert_value(stmt, value);
        
        if (match(parser, TOKEN_COMMA)) {
            continue;
        } else if (match(parser, TOKEN_RPAREN)) {
            break;
        } else {
            parser_set_error(parser, "Expected ',' or ')' in VALUES clause");
            return false;
        }
    }
    
    return true;
}

/**
 * @brief Parse an INSERT statement
 */
InsertStatement* parse_insert_statement(Parser* parser) {
    if (!expect(parser, TOKEN_INSERT, "Expected INSERT")) {
        return NULL;
    }
    
    if (!expect(parser, TOKEN_INTO, "Expected INTO after INSERT")) {
        return NULL;
    }
    
    InsertStatement* stmt = create_insert();
    if (!stmt) {
        parser_set_error(parser, "Memory allocation failed");
        return NULL;
    }
    
    // Parse table name
    if (parser->current_token.type != TOKEN_IDENTIFIER) {
        parser_set_error(parser, "Expected table name");
        free_insert_statement(stmt);
        return NULL;
    }
    
    stmt->table_name = strdup(parser->current_token.value);
    consume_token(parser); 
    
    // Parse optional column list
    if (!parse_column_list(parser, stmt)) {
        free_insert_statement(stmt);
        return NULL;
    }
    
    // Parse VALUES clause
    if (!parse_values_clause(parser, stmt)) {
        free_insert_statement(stmt);
        return NULL;
    }
    
    return stmt;
}

/**
 * @brief Validate INSERT statement against schema
 */
bool validate_insert_statement(const InsertStatement* stmt, const TableSchema* schema) {
    if (!stmt || !schema) {
        return false;
    }
    
    // Check table name matches schema
    if (strcmp(stmt->table_name, schema->name) != 0) {
        return false;
    }
    
    // If columns are specified, validate they exist
    if (stmt->columns) {
        for (int i = 0; i < stmt->column_count; i++) {
            bool found = false;
            for (int j = 0; j < schema->column_count; j++) {
                if (strcmp(stmt->columns[i], schema->columns[j].name) == 0) {
                    found = true;
                    break;
                }
            }
            
            if (!found) {
                fprintf(stderr, "Column '%s' not found in table '%s'\n", 
                       stmt->columns[i], stmt->table_name);
                // Debug: print all columns in schema
                fprintf(stderr, "Available columns:\n");
                for (int k = 0; k < schema->column_count; k++) {
                    fprintf(stderr, "  %d: '%s'\n", k, schema->columns[k].name);
                }
                return false;
            }
        }
        
        // Check value count matches column count
        if (stmt->column_count != stmt->value_list.count) {
            fprintf(stderr, "Column count (%d) doesn't match value count (%d)\n",
                   stmt->column_count, stmt->value_list.count);
            return false;
        }
    } else {
        // No columns specified, must have values for all columns
        if (stmt->value_list.count != schema->column_count) {
            fprintf(stderr, "Value count (%d) doesn't match schema column count (%d)\n",
                   stmt->value_list.count, schema->column_count);
            return false;
        }
    }
    
    return true;
}
