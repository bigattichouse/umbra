/**
 * @file update_parser.h
 * @brief Interface for UPDATE parsing
 */

#ifndef UMBRA_UPDATE_PARSER_H
#define UMBRA_UPDATE_PARSER_H

#include "ast.h"
#include "lexer.h"
#include "select_parser.h"  // For Parser struct
#include "../schema/schema_parser.h"  // For TableSchema

/**
 * @brief Parse an UPDATE statement
 * @param parser Parser instance
 * @return Parsed UPDATE statement or NULL on error
 */
UpdateStatement* parse_update_statement(Parser* parser);

/**
 * @brief Free an UPDATE statement
 * @param stmt Statement to free
 */
void free_update_statement(UpdateStatement* stmt);

/**
 * @brief Validate UPDATE statement against schema
 * @param stmt UPDATE statement
 * @param schema Table schema
 * @return true if valid, false otherwise
 */
bool validate_update_statement(const UpdateStatement* stmt, const TableSchema* schema);

#endif /* UMBRA_UPDATE_PARSER_H */
