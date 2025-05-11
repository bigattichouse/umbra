/**
 * @file update_executor.h
 * @brief Interface for UPDATE execution
 */

#ifndef UMBRA_UPDATE_EXECUTOR_H
#define UMBRA_UPDATE_EXECUTOR_H

#include "../parser/update_parser.h"
#include "../schema/schema_parser.h"

/**
 * @struct UpdateResult
 * @brief Result of UPDATE operation
 */
typedef struct {
    int rows_affected;      /**< Number of rows updated */
    bool success;           /**< Whether operation succeeded */
    char* error_message;    /**< Error message if failed */
} UpdateResult;

/**
 * @brief Execute an UPDATE statement
 * @param stmt UPDATE statement to execute
 * @param base_dir Base directory for database
 * @return Update result
 */
UpdateResult* execute_update(const UpdateStatement* stmt, const char* base_dir);

/**
 * @brief Free update result
 * @param result Result to free
 */
void free_update_result(UpdateResult* result);

#endif /* UMBRA_UPDATE_EXECUTOR_H */
