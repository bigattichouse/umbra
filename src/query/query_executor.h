/**
 * @file query_executor.h
 * @brief Interface for query execution
 */

#ifndef UMBRA_QUERY_EXECUTOR_H
#define UMBRA_QUERY_EXECUTOR_H

#include "../parser/ast.h"
#include "../schema/schema_parser.h"

typedef enum {
    ROW_FORMAT_DIRECT,         /* Row is a direct pointer to a record */
    ROW_FORMAT_POINTER_ARRAY   /* Row is an array of pointers to fields */
} RowFormat;

/**
 * @struct QueryResult
 * @brief Result of query execution
 */
typedef struct {
    void** rows;                /**< Array of result rows */
    int row_count;              /**< Number of rows returned */
    TableSchema* result_schema; /**< Schema of result set */
    bool success;               /**< Whether query succeeded */
    char* error_message;        /**< Error message if failed */
    RowFormat row_format;       /**< Format of row data */
} QueryResult;

/**
 * @brief Execute a SQL query
 * @param sql SQL query string
 * @param base_dir Base directory for database
 * @return Query result
 */
QueryResult* execute_query(const char* sql, const char* base_dir);

/**
 * @brief Free query result
 * @param result Query result to free
 */
void free_query_result(QueryResult* result);

/**
 * @brief Print query result to stdout
 * @param result Query result to print
 */
void print_query_result(const QueryResult* result);

/**
 * @brief Load table schema from metadata
 * @param table_name Table name
 * @param base_dir Base directory
 * @return Schema or NULL on error
 */
TableSchema* load_table_schema(const char* table_name, const char* base_dir);

#endif /* UMBRA_QUERY_EXECUTOR_H */
