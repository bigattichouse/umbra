/**
 * @file select_executor.h
 * @brief Interface for SELECT execution
 */

#ifndef UMBRA_SELECT_EXECUTOR_H
#define UMBRA_SELECT_EXECUTOR_H

#include "../parser/ast.h"
#include "query_executor.h"

/**
 * @brief Execute a SELECT statement
 * @param stmt SELECT statement AST
 * @param base_dir Base directory for database
 * @param result Query result to populate
 * @return 0 on success, -1 on error
 */
int execute_select(const SelectStatement* stmt, const char* base_dir, QueryResult* result);

/**
 * @brief Build result schema for SELECT statement
 * @param stmt SELECT statement AST
 * @param source_schema Source table schema
 * @return Result schema or NULL on error
 */
TableSchema* build_result_schema(const SelectStatement* stmt, const TableSchema* source_schema);

#endif /* UMBRA_SELECT_EXECUTOR_H */
