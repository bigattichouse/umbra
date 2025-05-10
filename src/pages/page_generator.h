/**
 * @file page_generator.h
 * @brief Interface for page generation
 */

#ifndef UMBRA_PAGE_GENERATOR_H
#define UMBRA_PAGE_GENERATOR_H

#include "../schema/schema_parser.h"

/**
 * @brief Generate a new data page for a table
 * @param schema Table schema
 * @param base_dir Base directory for database
 * @param page_number Page number to generate
 * @return 0 on success, -1 on error
 */
int generate_data_page(const TableSchema* schema, const char* base_dir, int page_number);

/**
 * @brief Add record to a data page
 * @param schema Table schema
 * @param base_dir Base directory for database
 * @param page_number Page number to add to
 * @param values Array of column values (strings)
 * @return 0 on success, -1 on error
 */
int add_record_to_page(const TableSchema* schema, const char* base_dir, 
                       int page_number, const char** values);

/**
 * @brief Check if a page is full
 * @param schema Table schema
 * @param base_dir Base directory for database
 * @param page_number Page number to check
 * @param max_records Maximum records per page
 * @param is_full Output parameter set to true if page is full
 * @return 0 on success, -1 on error
 */
int is_page_full(const TableSchema* schema, const char* base_dir, 
                int page_number, int max_records, bool* is_full);

/**
 * @brief Get the number of records in a page
 * @param schema Table schema
 * @param base_dir Base directory for database
 * @param page_number Page number to check
 * @param record_count Output parameter for record count
 * @return 0 on success, -1 on error
 */
int get_page_record_count(const TableSchema* schema, const char* base_dir, 
                         int page_number, int* record_count);

/**
 * @brief Recompile a data page
 * @param schema Table schema
 * @param base_dir Base directory for database
 * @param page_number Page number to recompile
 * @return 0 on success, -1 on error
 */
int recompile_data_page(const TableSchema* schema, const char* base_dir, int page_number);

#endif /* UMBRA_PAGE_GENERATOR_H */
