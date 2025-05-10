/**
 * @file schema_generator.h
 * @brief Interface for C struct generation
 */

#ifndef UMBRA_SCHEMA_GENERATOR_H
#define UMBRA_SCHEMA_GENERATOR_H

#include "schema_parser.h"

/**
 * @brief Generate C struct definition for a table schema
 * @param schema Table schema
 * @param output Buffer to write C struct definition to
 * @param output_size Size of output buffer
 * @return Number of bytes written, or -1 on error
 */
int generate_struct_definition(const TableSchema* schema, char* output, size_t output_size);

/**
 * @brief Generate header file for a table schema
 * @param schema Table schema
 * @param directory Directory to write header file to
 * @return 0 on success, -1 on error
 */
int generate_header_file(const TableSchema* schema, const char* directory);

/**
 * @brief Generate empty data page header file
 * @param schema Table schema
 * @param directory Directory to write data file to
 * @param page_number Page number
 * @return 0 on success, -1 on error
 */
int generate_empty_data_page(const TableSchema* schema, const char* directory, int page_number);

/**
 * @brief Generate accessor functions source file
 * @param schema Table schema
 * @param directory Directory to write source file to
 * @param page_number Page number
 * @return 0 on success, -1 on error
 */
int generate_accessor_functions(const TableSchema* schema, const char* directory, int page_number);

#endif /* UMBRA_SCHEMA_GENERATOR_H */
