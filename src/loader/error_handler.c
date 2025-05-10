/**
 * @file error_handler.c
 * @brief Implements error handling logic
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>
#include "error_handler.h"

/**
 * @brief Maximum length of error message
 */
#define MAX_ERROR_LENGTH 1024

/**
 * @brief Error message buffer
 */
static char error_buffer[MAX_ERROR_LENGTH];

/**
 * @brief Flag indicating if an error has occurred
 */
static bool error_occurred = false;

/**
 * @brief Set an error message
 */
void set_error(const char* format, ...) {
    if (!format) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    
    vsnprintf(error_buffer, MAX_ERROR_LENGTH, format, args);
    error_buffer[MAX_ERROR_LENGTH - 1] = '\0';
    error_occurred = true;
    
    va_end(args);
}

/**
 * @brief Get the last error message
 */
const char* get_last_error(void) {
    if (!error_occurred) {
        return NULL;
    }
    
    return error_buffer;
}

/**
 * @brief Clear the last error message
 */
void clear_error(void) {
    error_buffer[0] = '\0';
    error_occurred = false;
}

/**
 * @brief Check if an error has occurred
 */
bool has_error(void) {
    return error_occurred;
}
