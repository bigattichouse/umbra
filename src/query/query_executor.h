/**
 * @file query_executor.h
 * @brief Interface for query execution
 */

#ifndef UMBRA_QUERY_EXECUTOR_H
#define UMBRA_QUERY_EXECUTOR_H

#include "../parser/ast.h"
#include "../schema/schema_parser.h"

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

#endif /* UMBRA_QUERY_EXECUTOR_H */
