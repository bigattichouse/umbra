/**
 * @file compile_scripts.h
 * @brief Compilation script interface
 */

#ifndef UMBRA_COMPILE_SCRIPTS_H
#define UMBRA_COMPILE_SCRIPTS_H
#include "../schema/schema_parser.h"

/**
 * @brief Generate compilation script for a data page
 * @param schema Table schema
 * @param base_dir Base directory for database
 * @param page_number Page number
 * @return 0 on success, -1 on error
 */
int generate_compilation_script(const TableSchema* schema, const char* base_dir, int page_number);

/**
 * @brief Get path to a page's compilation script
 * @param table_name Table name
 * @param base_dir Base directory for database
 * @param page_number Page number
 * @param output Output buffer for script path
 * @param output_size Size of output buffer
 * @return 0 on success, -1 on error
 */
int get_compile_script_path(const char* table_name, const char* base_dir, 
                           int page_number, char* output, size_t output_size);

/**
 * @brief Generate compilation script for a filtered accessor
 * @param schema Table schema
 * @param base_dir Base directory for database
 * @param page_number Page number
 * @param suffix Suffix for generated file (e.g. "filtered")
 * @return 0 on success, -1 on error
 */
int generate_filtered_compilation_script(const TableSchema* schema, const char* base_dir, 
                                        int page_number, const char* suffix);

/**
 * @brief Generate makefile for compiling all pages of a table
 * @param schema Table schema
 * @param base_dir Base directory for database
 * @param page_count Number of pages
 * @return 0 on success, -1 on error
 */
int generate_table_makefile(const TableSchema* schema, const char* base_dir, int page_count);

#endif /* UMBRA_COMPILE_SCRIPTS_H */
