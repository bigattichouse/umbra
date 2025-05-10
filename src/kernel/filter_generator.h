/**
 * @file filter_generator.h
 * @brief Interface for filter generation
 */

#ifndef UMBRA_FILTER_GENERATOR_H
#define UMBRA_FILTER_GENERATOR_H

#include "../parser/ast.h"
#include "../schema/schema_parser.h"

/**
 * @brief Generate C code for WHERE clause condition
 * @param expr WHERE clause expression
 * @param schema Table schema
 * @param output Output buffer for generated code
 * @param output_size Size of output buffer
 * @return 0 on success, -1 on error
 */
int generate_filter_condition(const Expression* expr, const TableSchema* schema,
                             char* output, size_t output_size);

/**
 * @brief Generate filter function for WHERE clause
 * @param expr WHERE clause expression
 * @param schema Table schema
 * @param function_name Name for the filter function
 * @param output Output buffer for generated code
 * @param output_size Size of output buffer
 * @return 0 on success, -1 on error
 */
int generate_filter_function(const Expression* expr, const TableSchema* schema,
                            const char* function_name, char* output, size_t output_size);

/**
 * @brief Validate that expression can be converted to filter
 * @param expr Expression to validate
 * @param schema Table schema
 * @return true if valid, false otherwise
 */
bool validate_filter_expression(const Expression* expr, const TableSchema* schema);

#endif /* UMBRA_FILTER_GENERATOR_H */
