/**
 * @file delete_parser.h
 * @brief Interface for DELETE parsing
 */

#ifndef UMBRA_DELETE_PARSER_H
#define UMBRA_DELETE_PARSER_H

#include "ast.h"
#include "lexer.h"
#include "select_parser.h"  // For Parser struct
#include "../schema/schema_parser.h"  // For TableSchema

/**
 * @brief Parse a DELETE statement
 * @param parser Parser instance
 * @return Parsed DELETE statement or NULL on error
 */
DeleteStatement* parse_delete_statement(Parser* parser);

/**
 * @brief Free a DELETE statement
 * @param stmt Statement to free
 */
void free_delete_statement(DeleteStatement* stmt);

/**
 * @brief Validate DELETE statement against schema
 * @param stmt DELETE statement
 * @param schema Table schema
 * @return true if valid, false otherwise
 */
bool validate_delete_statement(const DeleteStatement* stmt, const TableSchema* schema);

#endif /* UMBRA_DELETE_PARSER_H */
