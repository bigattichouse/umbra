/**
 * @file type_system.c
 * @brief Type conversion and validation logic
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "type_system.h"

/**
 * @brief Type information table
 */
static const TypeInfo TYPE_INFO_TABLE[] = {
    { TYPE_UNKNOWN, "UNKNOWN", 0, false, false },
    { TYPE_INT, "INT", sizeof(int), false, false },
    { TYPE_VARCHAR, "VARCHAR", sizeof(char), true, true },
    { TYPE_TEXT, "TEXT", sizeof(char), true, false },
    { TYPE_FLOAT, "FLOAT", sizeof(double), false, false },
    { TYPE_DATE, "DATE", sizeof(time_t), false, false },
    { TYPE_BOOLEAN, "BOOLEAN", sizeof(bool), false, false }
};

/**
 * @brief Number of elements in type info table
 */
static const int TYPE_INFO_COUNT = sizeof(TYPE_INFO_TABLE) / sizeof(TypeInfo);

/**
 * @brief Get information about a data type
 */
TypeInfo get_type_info(DataType type) {
    for (int i = 0; i < TYPE_INFO_COUNT; i++) {
        if (TYPE_INFO_TABLE[i].type == type) {
            return TYPE_INFO_TABLE[i];
        }
    }
    return TYPE_INFO_TABLE[0]; // Return unknown type
}

/**
 * @brief Get data type from string name
 */
DataType get_type_from_name(const char* type_name) {
    if (!type_name) {
        return TYPE_UNKNOWN;
    }
    
    // Convert to uppercase for comparison
    char* upper = strdup(type_name);
    for (int i = 0; upper[i]; i++) {
        upper[i] = toupper(upper[i]);
    }
    
    DataType result = TYPE_UNKNOWN;
    
    for (int i = 0; i < TYPE_INFO_COUNT; i++) {
        if (strcmp(upper, TYPE_INFO_TABLE[i].name) == 0) {
            result = TYPE_INFO_TABLE[i].type;
            break;
        }
    }
    
    free(upper);
    return result;
}

/**
 * @brief Check if a string is a valid integer
 */
static bool is_valid_int(const char* value) {
    if (!value || !*value) {
        return false;
    }
    
    // Check for negative sign
    if (*value == '-') {
        value++;
    }
    
    // Must have at least one digit
    if (!*value) {
        return false;
    }
    
    // All characters must be digits
    while (*value) {
        if (!isdigit(*value)) {
            return false;
        }
        value++;
    }
    
    return true;
}

/**
 * @brief Check if a string is a valid float
 */
static bool is_valid_float(const char* value) {
    if (!value || !*value) {
        return false;
    }
    
    // Check for negative sign
    if (*value == '-') {
        value++;
    }
    
    // Must have at least one digit or a decimal point
    if (!*value) {
        return false;
    }
    
    bool has_digit = false;
    bool has_decimal = false;
    
    while (*value) {
        if (isdigit(*value)) {
            has_digit = true;
        } else if (*value == '.' && !has_decimal) {
            has_decimal = true;
        } else {
            return false;
        }
        value++;
    }
    
    return has_digit;
}

/**
 * @brief Check if a string is a valid date (YYYY-MM-DD)
 */
static bool is_valid_date(const char* value) {
    if (!value || strlen(value) != 10) {
        return false;
    }
    
    // Check format YYYY-MM-DD
    if (value[4] != '-' || value[7] != '-') {
        return false;
    }
    
    // Check year
    for (int i = 0; i < 4; i++) {
        if (!isdigit(value[i])) {
            return false;
        }
    }
    
    // Check month
    if (!isdigit(value[5]) || !isdigit(value[6])) {
        return false;
    }
    
    int month = (value[5] - '0') * 10 + (value[6] - '0');
    if (month < 1 || month > 12) {
        return false;
    }
    
    // Check day
    if (!isdigit(value[8]) || !isdigit(value[9])) {
        return false;
    }
    
    int day = (value[8] - '0') * 10 + (value[9] - '0');
    if (day < 1 || day > 31) {
        return false;
    }
    
    // More detailed validation could be added for specific months and leap years
    
    return true;
}

/**
 * @brief Check if a string is a valid boolean
 */
static bool is_valid_boolean(const char* value) {
    if (!value) {
        return false;
    }
    
    // Convert to lowercase for comparison
    char* lower = strdup(value);
    for (int i = 0; lower[i]; i++) {
        lower[i] = tolower(lower[i]);
    }
    
    bool result = (strcmp(lower, "true") == 0 || 
                  strcmp(lower, "false") == 0 ||
                  strcmp(lower, "1") == 0 ||
                  strcmp(lower, "0") == 0);
    
    free(lower);
    return result;
}

/**
 * @brief Validate a value for a data type
 */
bool validate_value(const char* value, DataType type, int length) {
    if (!value) {
        return false;
    }
    
    switch (type) {
        case TYPE_INT:
            return is_valid_int(value);
        
        case TYPE_VARCHAR:
            return strlen(value) <= length;
        
        case TYPE_TEXT:
            return true; // No validation for TEXT
        
        case TYPE_FLOAT:
            return is_valid_float(value);
        
        case TYPE_DATE:
            return is_valid_date(value);
        
        case TYPE_BOOLEAN:
            return is_valid_boolean(value);
        
        default:
            return false;
    }
}

/**
 * @brief Convert a value from text to its proper type
 */
int convert_value(const char* value, DataType type, void* output, size_t output_size) {
    if (!value || !output) {
        return -1;
    }
    
    switch (type) {
        case TYPE_INT: {
            if (output_size < sizeof(int)) {
                return -1;
            }
            int* out_int = (int*)output;
            *out_int = atoi(value);
            return 0;
        }
        
        case TYPE_VARCHAR:
        case TYPE_TEXT: {
            strncpy((char*)output, value, output_size - 1);
            ((char*)output)[output_size - 1] = '\0';
            return 0;
        }
        
        case TYPE_FLOAT: {
            if (output_size < sizeof(double)) {
                return -1;
            }
            double* out_float = (double*)output;
            *out_float = atof(value);
            return 0;
        }
        
        case TYPE_DATE: {
            if (output_size < sizeof(time_t)) {
                return -1;
            }
            
            struct tm tm_val;
            memset(&tm_val, 0, sizeof(tm_val));
            
            // Parse YYYY-MM-DD
            sscanf(value, "%d-%d-%d", 
                   &tm_val.tm_year, &tm_val.tm_mon, &tm_val.tm_mday);
            
            tm_val.tm_year -= 1900; // Years since 1900
            tm_val.tm_mon -= 1;     // Months since January
            
            time_t* out_date = (time_t*)output;
            *out_date = mktime(&tm_val);
            
            return 0;
        }
        
        case TYPE_BOOLEAN: {
            if (output_size < sizeof(bool)) {
                return -1;
            }
            
            bool* out_bool = (bool*)output;
            
            // Convert to lowercase for comparison
            char* lower = strdup(value);
            for (int i = 0; lower[i]; i++) {
                lower[i] = tolower(lower[i]);
            }
            
            *out_bool = (strcmp(lower, "true") == 0 || strcmp(lower, "1") == 0);
            
            free(lower);
            return 0;
        }
        
        default:
            return -1;
    }
}

/**
 * @brief Convert a value to text representation
 */
int convert_to_text(const void* value, DataType type, char* output, size_t output_size) {
    if (!value || !output || output_size <= 0) {
        return -1;
    }
    
    switch (type) {
        case TYPE_INT: {
            int val = *(const int*)value;
            return snprintf(output, output_size, "%d", val);
        }
        
        case TYPE_VARCHAR:
        case TYPE_TEXT: {
            const char* str = (const char*)value;
            strncpy(output, str, output_size - 1);
            output[output_size - 1] = '\0';
            return strlen(output);
        }
        
        case TYPE_FLOAT: {
            double val = *(const double*)value;
            return snprintf(output, output_size, "%f", val);
        }
        
        case TYPE_DATE: {
            time_t val = *(const time_t*)value;
            struct tm* tm_val = localtime(&val);
            
            if (!tm_val) {
                return -1;
            }
            
            return strftime(output, output_size, "%Y-%m-%d", tm_val);
        }
        
        case TYPE_BOOLEAN: {
            bool val = *(const bool*)value;
            return snprintf(output, output_size, "%s", val ? "true" : "false");
        }
        
        default:
            return -1;
    }
}
