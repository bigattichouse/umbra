/**
 * @file index_compiler.h
 * @brief Interface for index compilation
 */

#ifndef UMBRA_INDEX_COMPILER_H
#define UMBRA_INDEX_COMPILER_H

#include "../schema/schema_parser.h"
#include "index_definition.h"

/**
 * @brief Compile an index
 * @param schema Table schema
 * @param index_def Index definition
 * @param base_dir Base directory
 * @param page_number Page number (-1 for all pages)
 * @return 0 on success, -1 on error
 */
int compile_index(const TableSchema* schema, const IndexDefinition* index_def,
                 const char* base_dir, int page_number);

/**
 * @brief Generate compilation script for an index
 * @param schema Table schema
 * @param index_def Index definition
 * @param base_dir Base directory
 * @param page_number Page number (-1 for all pages)
 * @return 0 on success, -1 on error
 */
int generate_index_compile_script(const TableSchema* schema, const IndexDefinition* index_def,
                                 const char* base_dir, int page_number);

/**
 * @brief Get path to an index shared object
 * @param schema Table schema
 * @param column_name Column name
 * @param base_dir Base directory
 * @param page_number Page number (-1 for all pages)
 * @param index_type Index type
 * @param output Output buffer for path
 * @param output_size Size of output buffer
 * @return 0 on success, -1 on error
 */
int get_index_so_path(const TableSchema* schema, const char* column_name,
                     const char* base_dir, int page_number,
                     IndexType index_type,
                     char* output, size_t output_size);

#endif /* UMBRA_INDEX_COMPILER_H */
