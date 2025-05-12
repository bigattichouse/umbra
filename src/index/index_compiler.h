/**
 * @file index_compiler.h
 * @brief Interface for index compilation
 */

#ifndef UMBRA_INDEX_COMPILER_H
#define UMBRA_INDEX_COMPILER_H

#include "../schema/schema_parser.h"
#include "index_types.h"

/**
 * @brief Compile index for a page
 * @param schema Table schema
 * @param index_def Index definition
 * @param base_dir Base directory
 * @param page_number Page number
 * @return 0 on success, -1 on error
 */
int compile_index(const TableSchema* schema, const IndexDefinition* index_def,
                 const char* base_dir, int page_number);

/**
 * @brief Generate compilation script for index
 * @param schema Table schema
 * @param index_def Index definition
 * @param base_dir Base directory
 * @param page_number Page number
 * @return 0 on success, -1 on error
 */
int generate_index_compile_script(const TableSchema* schema, const IndexDefinition* index_def,
                                 const char* base_dir, int page_number);

/**
 * @brief Check if index is already compiled
 * @param schema Table schema
 * @param column_name Column name
 * @param base_dir Base directory
 * @param page_number Page number
 * @return true if compiled, false otherwise
 */
bool is_index_compiled(const TableSchema* schema, const char* column_name,
                      const char* base_dir, int page_number);

/**
 * @brief Get path to compiled index
 * @param schema Table schema
 * @param column_name Column name
 * @param base_dir Base directory
 * @param page_number Page number
 * @param output Output buffer for path
 * @param output_size Size of output buffer
 * @return 0 on success, -1 on error
 */
int get_index_so_path(const TableSchema* schema, const char* column_name,
                     const char* base_dir, int page_number,
                     char* output, size_t output_size);

#endif /* UMBRA_INDEX_COMPILER_H */
