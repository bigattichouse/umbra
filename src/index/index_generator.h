/**
 * @file index_generator.h
 * @brief Interface for index generation
 */

#ifndef UMBRA_INDEX_GENERATOR_H
#define UMBRA_INDEX_GENERATOR_H

#include "../schema/schema_parser.h"
#include "index_types.h"

/**
 * @brief Generate index header file for a table
 * @param schema Table schema
 * @param index_def Index definition
 * @param directory Output directory
 * @return 0 on success, -1 on error
 */
int generate_index_header(const TableSchema* schema, const IndexDefinition* index_def, 
                         const char* directory);

/**
 * @brief Generate index source file for a page
 * @param schema Table schema
 * @param index_def Index definition
 * @param base_dir Base directory
 * @param page_number Page number
 * @return 0 on success, -1 on error
 */
int generate_index_source(const TableSchema* schema, const IndexDefinition* index_def,
                         const char* base_dir, int page_number);

/**
 * @brief Build index from page data
 * @param schema Table schema
 * @param index_def Index definition
 * @param base_dir Base directory
 * @param page_number Page number
 * @param output Output buffer for generated index code
 * @param output_size Size of output buffer
 * @return Number of bytes written, or -1 on error
 */
int build_index_from_page(const TableSchema* schema, const IndexDefinition* index_def,
                         const char* base_dir, int page_number,
                         char* output, size_t output_size);

/**
 * @brief Generate index for column in table
 * @param schema Table schema
 * @param column_name Column to index
 * @param index_type Type of index
 * @param base_dir Base directory
 * @return 0 on success, -1 on error
 */
int generate_index_for_column(const TableSchema* schema, const char* column_name,
                             IndexType index_type, const char* base_dir);

/**
 * @brief Get the column index for a column name
 * @param schema Table schema
 * @param column_name Column name
 * @return Column index, or -1 if not found
 */
int get_column_index(const TableSchema* schema, const char* column_name);

#endif /* UMBRA_INDEX_GENERATOR_H */
