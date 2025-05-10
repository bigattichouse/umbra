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
    DataType type;                  /**< Type identifier */
    const char* name;               /**< Type name */
    size_t size;                    /**< Size in bytes */
    bool variable_length;           /**< Whether type has variable length */
    bool requires_length;           /**< Whether type requires length parameter */
} TypeInfo;

/**
 * @brief Get information about a data type
 * @param type Data type
 * @return TypeInfo structure for the type
 */
TypeInfo get_type_info(DataType type);

/**
 * @brief Get data type from string name
 * @param type_name Type name string
 * @return DataType value or TYPE_UNKNOWN if not found
 */
DataType get_type_from_name(const char* type_name);

/**
 * @brief Convert a value from text to its proper type
 * @param value Text value
 * @param type Target data type
 * @param output Buffer to store converted value
 * @param output_size Size of output buffer
 * @return 0 on success, -1 on error
 */
int convert_value(const char* value, DataType type, void* output, size_t output_size);

/**
 * @brief Convert a value to text representation
 * @param value Input value pointer
 * @param type Data type
 * @param output Text output buffer
 * @param output_size Size of output buffer
 * @return Number of bytes written, or -1 on error
 */
int convert_to_text(const void* value, DataType type, char* output, size_t output_size);

/**
 * @brief Validate a value for a data type
 * @param value Value to validate
 * @param type Type to validate against
 * @param length Length for variable-length types
 * @return true if valid, false otherwise
 */
bool validate_value(const char* value, DataType type, int length);

#endif /* UMBRA_TYPE_SYSTEM_H */
