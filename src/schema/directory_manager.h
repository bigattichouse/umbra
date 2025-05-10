/**
 * @file directory_manager.h
 * @brief Interface for directory structure management
 */

#ifndef UMBRA_DIRECTORY_MANAGER_H
#define UMBRA_DIRECTORY_MANAGER_H

#include <stdbool.h>

/**
 * @brief Create a table directory structure
 * @param table_name Name of the table
 * @param base_dir Base directory for database
 * @return 0 on success, -1 on error
 */
int create_table_directory(const char* table_name, const char* base_dir);

/**
 * @brief Check if a table directory exists
 * @param table_name Name of the table
 * @param base_dir Base directory for database
 * @return true if exists, false otherwise
 */
bool table_directory_exists(const char* table_name, const char* base_dir);

/**
 * @brief Get table directory path
 * @param table_name Name of the table
 * @param base_dir Base directory for database
 * @param output Output buffer for path
 * @param output_size Size of output buffer
 * @return 0 on success, -1 on error
 */
int get_table_directory(const char* table_name, const char* base_dir, 
                       char* output, size_t output_size);

/**
 * @brief Get data directory path for a table
 * @param table_name Name of the table
 * @param base_dir Base directory for database
 * @param output Output buffer for path
 * @param output_size Size of output buffer
 * @return 0 on success, -1 on error
 */
int get_data_directory(const char* table_name, const char* base_dir, 
                      char* output, size_t output_size);

/**
 * @brief Get source directory path for a table
 * @param table_name Name of the table
 * @param base_dir Base directory for database
 * @param output Output buffer for path
 * @param output_size Size of output buffer
 * @return 0 on success, -1 on error
 */
int get_source_directory(const char* table_name, const char* base_dir, 
                        char* output, size_t output_size);

/**
 * @brief Initialize the database directory structure
 * @param base_dir Base directory for database
 * @return 0 on success, -1 on error
 */
int initialize_database_directory(const char* base_dir);

#endif /* UMBRA_DIRECTORY_MANAGER_H */
