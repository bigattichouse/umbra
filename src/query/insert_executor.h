/**
 * @file insert_executor.h
 * @brief Interface for INSERT execution
 */

#ifndef UMBRA_INSERT_EXECUTOR_H
#define UMBRA_INSERT_EXECUTOR_H

#include "../parser/insert_parser.h"
#include "../schema/schema_parser.h"

/**
 * @struct InsertResult
 * @brief Result of INSERT operation
 */
typedef struct {
    int rows_affected;      /**< Number of rows inserted */
    bool success;           /**< Whether operation succeeded */
    char* error_message;    /**< Error message if failed */
} InsertResult;

/**
 * @brief Execute an INSERT statement
 * @param stmt INSERT statement to execute
 * @param base_dir Base directory for database
 * @return Insert result
 */
InsertResult* execute_insert(const InsertStatement* stmt, const char* base_dir);

/**
 * @brief Free insert result
 * @param result Result to free
 */
void free_insert_result(InsertResult* result);

#endif /* UMBRA_INSERT_EXECUTOR_H */
