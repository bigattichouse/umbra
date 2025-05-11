/**
 * @file type_system.h
 * @brief Defines supported data types
 */

#ifndef UMBRA_TYPE_SYSTEM_H
#define UMBRA_TYPE_SYSTEM_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

/**
 * @enum DataType
 * @brief Supported data types
 */
typedef enum {
    TYPE_UNKNOWN = 0,
    TYPE_INT,
    TYPE_VARCHAR,
    TYPE_TEXT,
    TYPE_FLOAT,
    TYPE_DATE,
    TYPE_BOOLEAN
} DataType;

/**
 * @struct TypeInfo
 * @brief Information about a data type
 */
typedef struct {
    DataType type;
    const char* name;
    size_t base_size;
    bool is_variable_length;
    bool requires_length;
} TypeInfo;

/**
 * @brief Get information about a data type
 * @param type Data type
 * @return Type information
 */
TypeInfo get_type_info(DataType type);

/**
 * @brief Get data type from string name
 * @param type_name Type name string
 * @return Data type enum value
 */
DataType get_type_from_name(const char* type_name);

/**
 * @brief Validate a value for a data type
 * @param value Value to validate
 * @param type Data type
 * @param length Length (for variable-length types)
 * @return true if valid, false otherwise
 */
bool validate_value(const char* value, DataType type, int length);

/**
 * @brief Convert a value from text to its proper type
 * @param value Text value
 * @param type Target data type
 * @param output Output buffer
 * @param output_size Size of output buffer
 * @return 0 on success, -1 on error
 */
int convert_value(const char* value, DataType type, void* output, size_t output_size);

/**
 * @brief Convert a value to text representation
 * @param value Value to convert
 * @param type Data type of value
 * @param output Output buffer
 * @param output_size Size of output buffer
 * @return Number of characters written, or -1 on error
 */
int convert_to_text(const void* value, DataType type, char* output, size_t output_size);

/* Forward declaration to resolve circular dependency */
struct TableSchema;

/**
 * @brief Calculate the size of a record based on schema
 * @param schema Table schema
 * @return Size of a single record in bytes
 */
size_t calculate_record_size(const struct TableSchema* schema);

#endif /* UMBRA_TYPE_SYSTEM_H */
