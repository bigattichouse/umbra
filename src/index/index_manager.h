/**
 * @file index_manager.h
 * @brief Interface for index management
 */

#ifndef UMBRA_INDEX_MANAGER_H
#define UMBRA_INDEX_MANAGER_H

#include "../schema/schema_parser.h"
#include "index_types.h"

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
 * @struct CreateIndexResult
 * @brief Result of CREATE INDEX operation
 */
typedef struct {
    bool success;           /**< Whether operation succeeded */
    char* error_message;    /**< Error message if failed */
} CreateIndexResult;

/**
 * @brief Initialize index manager for a table
 * @param manager Manager to initialize
 * @param table_name Table name
 * @param base_dir Base directory
 * @return 0 on success, -1 on error
 */
int init_index_manager(IndexManager* manager, const char* table_name, const char* base_dir);

/**
 * @brief Free resources used by index manager
 * @param manager Manager to free
 * @return 0 on success, -1 on error
 */
int free_index_manager(IndexManager* manager);

/**
 * @brief Create an index for a column
 * @param manager Index manager
 * @param column_name Column to index
 * @param index_type Type of index
 * @param base_dir Base directory
 * @return CREATE INDEX result
 */
CreateIndexResult* create_index(IndexManager* manager, const char* column_name,
                               IndexType index_type, const char* base_dir);

/**
 * @brief Drop an index
 * @param manager Index manager
 * @param index_name Index to drop
 * @param base_dir Base directory
 * @return 0 on success, -1 on error
 */
int drop_index(IndexManager* manager, const char* index_name, const char* base_dir);

/**
 * @brief Get indices for a table
 * @param table_name Table name
 * @param base_dir Base directory
 * @param indices Output array for indices
 * @param count Output parameter for index count
 * @return 0 on success, -1 on error
 */
int get_table_indices(const char* table_name, const char* base_dir,
                     IndexDefinition** indices, int* count);

/**
 * @brief Load a compiled index
 * @param schema Table schema
 * @param column_name Column name
 * @param base_dir Base directory
 * @param page_number Page number
 * @param index_type Index type
 * @return Loaded index library handle or NULL on error
 */
void* load_index(const TableSchema* schema, const char* column_name,
                const char* base_dir, int page_number, IndexType index_type);

/**
 * @brief Execute a CREATE INDEX statement
 * @param create_statement SQL CREATE INDEX statement
 * @param base_dir Base directory
 * @return Create index result
 */
CreateIndexResult* execute_create_index(const char* create_statement, const char* base_dir);

/**
 * @brief Free CREATE INDEX result
 * @param result Result to free
 */
void free_create_index_result(CreateIndexResult* result);

#endif /* UMBRA_INDEX_MANAGER_H */
