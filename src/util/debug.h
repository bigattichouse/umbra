/**
 * @file debug.h
 * @brief Debug utility functions
 */

#ifndef UMBRA_DEBUG_H
#define UMBRA_DEBUG_H

#include <stdbool.h>
#include <stdarg.h>

/**
 * @brief Initialize debug system
 * @param enabled Initial enabled state
 */
void debug_init(bool enabled);

/**
 * @brief Enable or disable debug output
 * @param enabled New enabled state
 */
void debug_set_enabled(bool enabled);

/**
 * @brief Check if debug output is enabled
 * @return true if enabled, false otherwise
 */
bool debug_is_enabled(void);

/**
 * @brief Print a debug message
 * @param file Source file name
 * @param line Line number
 * @param format Format string (printf style)
 * @param ... Additional arguments
 */
void debug_print(const char* file, int line, const char* format, ...);

/**
 * @brief Same as debug_print but with va_list
 * @param file Source file name
 * @param line Line number
 * @param format Format string (printf style)
 * @param args Variable argument list
 */
void debug_vprint(const char* file, int line, const char* format, va_list args);

/**
 * @brief Debug macro that includes file and line information
 */
#ifndef UMBRA_DEBUG
#ifdef UMBRA_DEBUG_ENABLED
#define UMBRA_DEBUG(format, ...) debug_print(__FILE__, __LINE__, format, ##__VA_ARGS__)
#else
#define UMBRA_DEBUG(format, ...) ((void)0)
#endif
#endif

/* Backward compatibility with existing DEBUG usage if not already defined */
#ifndef DEBUG
#define DEBUG UMBRA_DEBUG
#endif

#endif /* UMBRA_DEBUG_H */
