/**
 * @file index_manager.h
 * @brief Interface for index management
 */

#ifndef UMBRA_INDEX_MANAGER_H
#define UMBRA_INDEX_MANAGER_H

#include <stdbool.h>
#include "../schema/schema_parser.h"
#include "index_definition.h"

/**
 * @struct IndexManager
 * @brief Manages indices for a table
 */
typedef struct {
    char table_name[64];      /**< Table name */
    IndexDefinition* indices; /**< Array of indices */
    int index_count;          /**< Number of indices */
} IndexManager;

/**
 * @brief Initialize an index manager
 * @param manager Manager to initialize
 * @param table_name Table name
 * @return 0 on success, -1 on error
 */
int init_index_manager(IndexManager* manager, const char* table_name);

/**
 * @brief Free resources used by an index manager
 * @param manager Manager to free
 * @return 0 on success, -1 on error
 */
int free_index_manager(IndexManager* manager);

/**
 * @brief Save index metadata to file
 * @param manager Index manager
 * @param base_dir Base directory for database
 * @return 0 on success, -1 on error
 */
int save_index_metadata(const IndexManager* manager, const char* base_dir);

/**
 * @brief Load index metadata from file
 * @param manager Index manager to populate
 * @param base_dir Base directory for database
 * @return 0 on success, -1 on error
 */
int load_index_metadata(IndexManager* manager, const char* base_dir);

/**
 * @brief Check if a column is indexed
 * @param manager Index manager
 * @param column_name Column name
 * @return true if indexed, false otherwise
 */
bool is_column_indexed(const IndexManager* manager, const char* column_name);

/**
 * @brief Create a new index
 * @param manager Index manager
 * @param column_name Column name
 * @param index_type Index type
 * @param base_dir Base directory for database
 * @return 0 on success, -1 on error
 */
int create_index(IndexManager* manager, const char* column_name, 
                IndexType index_type, const char* base_dir);

/**
 * @brief Drop an index
 * @param manager Index manager
 * @param index_name Index name
 * @param base_dir Base directory for database
 * @return 0 on success, -1 on error
 */
int drop_index(IndexManager* manager, const char* index_name, const char* base_dir);

/**
 * @brief Get indices for a table
 * @param base_dir Base directory for database
 * @param table_name Table name
 * @param indices Output parameter for index array
 * @param count Output parameter for index count
 * @return 0 on success, -1 on error
 */
int get_table_indices(const char* base_dir, const char* table_name,
                     IndexDefinition** indices, int* count);

/**
 * @brief Load an index
 * @param schema Table schema
 * @param column_name Column name
 * @param base_dir Base directory
 * @param page_number Page number (-1 for all pages)
 * @param index_type Index type
 * @return Index handle or NULL on error
 */
void* load_index(const TableSchema* schema, const char* column_name,
                const char* base_dir, int page_number, IndexType index_type);

/**
 * @brief Parse a CREATE INDEX statement
 * @param sql SQL statement
 * @param table_name Output buffer for table name
 * @param table_name_size Size of table name buffer
 * @param column_name Output buffer for column name
 * @param column_name_size Size of column name buffer
 * @param index_type Output parameter for index type
 * @return 0 on success, -1 on error
 */
int parse_create_index(const char* sql, char* table_name, size_t table_name_size,
                      char* column_name, size_t column_name_size, IndexType* index_type);

/**
 * @brief Execute a CREATE INDEX statement
 * @param create_statement CREATE INDEX SQL statement
 * @param base_dir Base directory for database
 * @return Result of operation (must be freed with free_create_index_result)
 */
CreateIndexResult* execute_create_index(const char* create_statement, const char* base_dir);

#endif /* UMBRA_INDEX_MANAGER_H */
