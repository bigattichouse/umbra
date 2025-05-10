/**
 * @file metadata.h
 * @brief Metadata structure definitions
 */

#ifndef UMBRA_METADATA_H
#define UMBRA_METADATA_H

#include <time.h>
#include "schema_parser.h"

/**
 * @struct TableMetadata
 * @brief Metadata for a table
 */
typedef struct {
    char name[64];             /**< Table name */
    time_t created_at;         /**< Creation timestamp */
    time_t modified_at;        /**< Last modification timestamp */
    char creator[64];          /**< Creator username */
    int page_count;            /**< Number of data pages */
    int record_count;          /**< Total number of records */
    int page_size;             /**< Maximum records per page */
} TableMetadata;

/**
 * @struct DatabaseMetadata
 * @brief Metadata for the database
 */
typedef struct {
    char name[64];             /**< Database name */
    time_t created_at;         /**< Creation timestamp */
    int table_count;           /**< Number of tables */
    char* table_names;         /**< Array of table names */
    char version[32];          /**< Database version */
} DatabaseMetadata;

/**
 * @brief Create metadata for a new table
 * @param schema Table schema
 * @param creator Creator username
 * @param page_size Maximum records per page
 * @return New table metadata
 */
TableMetadata create_table_metadata(const TableSchema* schema, const char* creator, int page_size);

/**
 * @brief Save table metadata to file
 * @param metadata Table metadata
 * @param directory Directory to save to
 * @return 0 on success, -1 on error
 */
int save_table_metadata(const TableMetadata* metadata, const char* directory);

/**
 * @brief Load table metadata from file
 * @param table_name Table name
 * @param directory Directory to load from
 * @param metadata Output parameter for metadata
 * @return 0 on success, -1 on error
 */
int load_table_metadata(const char* table_name, const char* directory, TableMetadata* metadata);

/**
 * @brief Update table metadata
 * @param metadata Table metadata to update
 * @param directory Directory where metadata is stored
 * @return 0 on success, -1 on error
 */
int update_table_metadata(TableMetadata* metadata, const char* directory);

#endif /* UMBRA_METADATA_H */
