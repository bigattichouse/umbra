/**
 * @file insert_parser.h
 * @brief Interface for INSERT parsing
 */

#ifndef UMBRA_INSERT_PARSER_H
#define UMBRA_INSERT_PARSER_H

#include "ast.h"
#include "lexer.h"
#include "select_parser.h"  // For Parser struct
#include "../schema/schema_parser.h"  // For TableSchema

/**
 * @brief Parse an INSERT statement
 * @param parser Parser instance
 * @return Parsed INSERT statement or NULL on error
 */
InsertStatement* parse_insert_statement(Parser* parser);

/**
 * @brief Validate INSERT statement against schema
 * @param stmt INSERT statement
 * @param schema Table schema
 * @return true if valid, false otherwise
 */
bool validate_insert_statement(const InsertStatement* stmt, const TableSchema* schema);

#endif /* UMBRA_INSERT_PARSER_H */
