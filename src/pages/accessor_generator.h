/**
 * @file accessor_generator.h
 * @brief Interface for accessor function generation
 */

#ifndef UMBRA_ACCESSOR_GENERATOR_H
#define UMBRA_ACCESSOR_GENERATOR_H

#include "../schema/schema_parser.h"

/**
 * @brief Generate accessor function file for a table page
 * @param schema Table schema
 * @param base_dir Base directory for database
 * @param page_number Page number
 * @return 0 on success, -1 on error
 */
int generate_accessor_file(const TableSchema* schema, const char* base_dir, int page_number);

/**
 * @brief Generate query accessor for filtered reads
 * @param schema Table schema
 * @param base_dir Base directory for database
 * @param page_number Page number
 * @param where_clause SQL WHERE clause for filtering
 * @return 0 on success, -1 on error
 */
int generate_filtered_accessor(const TableSchema* schema, const char* base_dir, 
                              int page_number, const char* where_clause);

/**
 * @brief Generate projection accessor for selected columns
 * @param schema Table schema
 * @param base_dir Base directory for database
 * @param page_number Page number
 * @param columns Array of column indices to project
 * @param column_count Number of columns to project
 * @return 0 on success, -1 on error
 */
int generate_projection_accessor(const TableSchema* schema, const char* base_dir, 
                               int page_number, const int* columns, int column_count);

#endif /* UMBRA_ACCESSOR_GENERATOR_H */
