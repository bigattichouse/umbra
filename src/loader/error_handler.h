/**
 * @file error_handler.h
 * @brief Error handling for loading operations
 */

#ifndef UMBRA_ERROR_HANDLER_H
#define UMBRA_ERROR_HANDLER_H

/**
 * @brief Set an error message
 * @param format Format string
 * @param ... Additional arguments
 */
void set_error(const char* format, ...);

/**
 * @brief Get the last error message
 * @return Error message or NULL if no error
 */
const char* get_last_error(void);

/**
 * @brief Clear the last error message
 */
void clear_error(void);

/**
 * @brief Check if an error has occurred
 * @return true if an error has occurred, false otherwise
 */
bool has_error(void);

#endif /* UMBRA_ERROR_HANDLER_H */
