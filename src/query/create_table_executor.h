/**
 * @file create_table_executor.h
 * @brief Interface for CREATE TABLE execution
 */

#ifndef UMBRA_CREATE_TABLE_EXECUTOR_H
#define UMBRA_CREATE_TABLE_EXECUTOR_H

#include "../schema/schema_parser.h"

/**
 * @struct CreateTableResult
 * @brief Result of CREATE TABLE operation
 */
typedef struct {
    bool success;           /**< Whether operation succeeded */
    char* error_message;    /**< Error message if failed */
} CreateTableResult;

/**
 * @brief Execute a CREATE TABLE statement
 * @param create_statement CREATE TABLE SQL statement
 * @param base_dir Base directory for database
 * @return Create table result
 */
CreateTableResult* execute_create_table(const char* create_statement, const char* base_dir);

/**
 * @brief Free create table result
 * @param result Result to free
 */
void free_create_table_result(CreateTableResult* result);

#endif /* UMBRA_CREATE_TABLE_EXECUTOR_H */
