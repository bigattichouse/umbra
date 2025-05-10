/**
 * @file page_splitter.h
 * @brief Interface for page splitting logic
 */

#ifndef UMBRA_PAGE_SPLITTER_H
#define UMBRA_PAGE_SPLITTER_H

#include "../schema/schema_parser.h"
#include <stdbool.h>

/**
 * @brief Check if a page needs to be split
 * @param schema Table schema
 * @param base_dir Base directory for database
 * @param page_number Page number to check
 * @param max_records Maximum records per page
 * @param needs_split Output parameter set to true if page needs split
 * @return 0 on success, -1 on error
 */
int check_page_split(const TableSchema* schema, const char* base_dir, 
                    int page_number, int max_records, bool* needs_split);

/**
 * @brief Split a full page into two pages
 * @param schema Table schema
 * @param base_dir Base directory for database
 * @param full_page_number Page number to split
 * @param new_page_number New page number to create
 * @param max_records Maximum records per page
 * @param split_point Number of records to keep in original page
 * @return 0 on success, -1 on error
 */
int split_page(const TableSchema* schema, const char* base_dir, 
              int full_page_number, int new_page_number, 
              int max_records, int split_point);

/**
 * @brief Get metadata about pages in a table
 * @param schema Table schema
 * @param base_dir Base directory for database
 * @param page_count Output parameter for number of pages
 * @param record_count Output parameter for total number of records
 * @return 0 on success, -1 on error
 */
int get_table_page_info(const TableSchema* schema, const char* base_dir, 
                       int* page_count, int* record_count);

/**
 * @brief Find the best page for a new record
 * @param schema Table schema
 * @param base_dir Base directory for database
 * @param max_records Maximum records per page
 * @param best_page Output parameter for best page number
 * @return 0 on success, -1 on error
 */
int find_best_page_for_insert(const TableSchema* schema, const char* base_dir, 
                             int max_records, int* best_page);

#endif /* UMBRA_PAGE_SPLITTER_H */
