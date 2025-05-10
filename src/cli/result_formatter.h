/**
 * @file result_formatter.h
 * @brief Interface for formatting results
 */

#ifndef UMBRA_RESULT_FORMATTER_H
#define UMBRA_RESULT_FORMATTER_H

#include "../query/query_executor.h"

/**
 * @enum OutputFormat
 * @brief Output format types
 */
typedef enum {
    FORMAT_TABLE,
    FORMAT_CSV,
    FORMAT_JSON
} OutputFormat;

/**
 * @brief Format and display query results
 * @param result Query result to format
 * @param format Output format
 */
void format_query_result(const QueryResult* result, OutputFormat format);

/**
 * @brief Format result as ASCII table
 * @param result Query result
 */
void format_as_table(const QueryResult* result);

/**
 * @brief Format result as CSV
 * @param result Query result
 */
void format_as_csv(const QueryResult* result);

/**
 * @brief Format result as JSON
 * @param result Query result
 */
void format_as_json(const QueryResult* result);

#endif /* UMBRA_RESULT_FORMATTER_H */
