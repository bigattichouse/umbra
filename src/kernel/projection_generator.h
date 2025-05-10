/**
 * @file projection_generator.h
 * @brief Interface for projection generation
 */

#ifndef UMBRA_PROJECTION_GENERATOR_H
#define UMBRA_PROJECTION_GENERATOR_H

#include "../parser/ast.h"
#include "../schema/schema_parser.h"

/**
 * @brief Generate projection structure definition
 * @param stmt SELECT statement
 * @param schema Table schema
 * @param output Output buffer for generated code
 * @param output_size Size of output buffer
 * @return 0 on success, -1 on error
 */
int generate_projection_struct(const SelectStatement* stmt, const TableSchema* schema,
                              char* output, size_t output_size);

/**
 * @brief Generate projection assignment code
 * @param stmt SELECT statement
 * @param schema Table schema
 * @param output Output buffer for generated code
 * @param output_size Size of output buffer
 * @return 0 on success, -1 on error
 */
int generate_projection_assignment(const SelectStatement* stmt, const TableSchema* schema,
                                  char* output, size_t output_size);

/**
 * @brief Validate select list against schema
 * @param stmt SELECT statement
 * @param schema Table schema
 * @return true if valid, false otherwise
 */
bool validate_select_list(const SelectStatement* stmt, const TableSchema* schema);

#endif /* UMBRA_PROJECTION_GENERATOR_H */
