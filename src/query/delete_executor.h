/**
 * @file delete_executor.h
 * @brief Interface for DELETE execution
 */

#ifndef UMBRA_DELETE_EXECUTOR_H
#define UMBRA_DELETE_EXECUTOR_H

#include "../parser/delete_parser.h"
#include "../schema/schema_parser.h"

/**
 * @struct DeleteResult
 * @brief Result of DELETE operation
 */
typedef struct {
    int rows_affected;      /**< Number of rows deleted */
    bool success;           /**< Whether operation succeeded */
    char* error_message;    /**< Error message if failed */
} DeleteResult;

/**
 * @brief Execute a DELETE statement
 * @param stmt DELETE statement to execute
 * @param base_dir Base directory for database
 * @return Delete result
 */
DeleteResult* execute_delete(const DeleteStatement* stmt, const char* base_dir);

/**
 * @brief Free delete result
 * @param result Result to free
 */
void free_delete_result(DeleteResult* result);

#endif /* UMBRA_DELETE_EXECUTOR_H */
