/**
 * @file index_generator.h
 * @brief Interface for index generation
 */

#ifndef UMBRA_INDEX_GENERATOR_H
#define UMBRA_INDEX_GENERATOR_H

#include "../schema/schema_parser.h"
#include "index_definition.h"

/**
 * @brief Generate header file for an index
 * @param schema Table schema
 * @param index_def Index definition
 * @param base_dir Base directory
 * @return 0 on success, -1 on error
 */
int generate_index_header(const TableSchema* schema, const IndexDefinition* index_def,
                         const char* base_dir);

/**
 * @brief Generate source file for an index
 * @param schema Table schema
 * @param index_def Index definition
 * @param base_dir Base directory
 * @param page_number Page number (-1 for all pages)
 * @return 0 on success, -1 on error
 */
int generate_index_source(const TableSchema* schema, const IndexDefinition* index_def,
                         const char* base_dir, int page_number);

/**
 * @brief Build index from a data page
 * @param schema Table schema
 * @param index_def Index definition
 * @param base_dir Base directory
 * @param page_number Page number
 * @return 0 on success, -1 on error
 */
int build_index_from_page(const TableSchema* schema, const IndexDefinition* index_def,
                         const char* base_dir, int page_number);

/**
 * @brief Generate index for a column
 * @param schema Table schema
 * @param column_name Column name
 * @param index_type Index type
 * @param base_dir Base directory
 * @return 0 on success, -1 on error
 */
int generate_index_for_column(const TableSchema* schema, const char* column_name,
                            IndexType index_type, const char* base_dir);

/**
 * @brief Get column index in the schema
 * @param schema Table schema
 * @param column_name Column name
 * @return Column index or -1 if not found
 */
int get_column_index(const TableSchema* schema, const char* column_name);

#endif /* UMBRA_INDEX_GENERATOR_H */
