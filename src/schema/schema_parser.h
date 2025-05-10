/**
 * @file schema_parser.h
 * @brief Parser interface for schema definitions
 */

#ifndef UMBRA_SCHEMA_PARSER_H
#define UMBRA_SCHEMA_PARSER_H

#include <stdbool.h>
#include "../schema/type_system.h"

/**
 * @struct ColumnDefinition
 * @brief Represents a column in a table schema
 */
typedef struct {
    char name[64];               /**< Column name */
    DataType type;               /**< Column data type */
    int length;                  /**< Length for variable-length types */
    bool nullable;               /**< Whether the column can be NULL */
    char default_value[256];     /**< Default value expression (if any) */
    bool has_default;            /**< Whether a default value is defined */
    bool is_primary_key;         /**< Whether this column is part of primary key */
} ColumnDefinition;

/**
 * @struct TableSchema
 * @brief Represents a complete table schema
 */
typedef struct {
    char name[64];                      /**< Table name */
    ColumnDefinition* columns;          /**< Array of column definitions */
    int column_count;                   /**< Number of columns */
    int* primary_key_columns;           /**< Array of primary key column indices */
    int primary_key_column_count;       /**< Number of primary key columns */
} TableSchema;

/**
 * @brief Parse a CREATE TABLE statement
 * @param create_statement The SQL CREATE TABLE statement
 * @return Parsed table schema or NULL on error
 */
TableSchema* parse_create_table(const char* create_statement);

/**
 * @brief Free a previously allocated table schema
 * @param schema The schema to free
 */
void free_table_schema(TableSchema* schema);

/**
 * @brief Check if a schema is valid
 * @param schema The schema to check
 * @return True if schema is valid, false otherwise
 */
bool validate_schema(const TableSchema* schema);

#endif /* UMBRA_SCHEMA_PARSER_H */
