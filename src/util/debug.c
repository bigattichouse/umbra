/**
 * @file debug.c
 * @brief Implementation of debug utility functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <stdbool.h>
#include <pthread.h>
#include "debug.h"

/**
 * @brief Global debug state
 */
static struct {
    bool enabled;
    pthread_mutex_t mutex;
    bool initialized;
} debug_state = {
    .enabled = false,
    .initialized = false
};

/**
 * @brief Initialize debug system
 */
void debug_init(bool enabled) {
    if (!debug_state.initialized) {
        pthread_mutex_init(&debug_state.mutex, NULL);
        debug_state.initialized = true;
    }
    
    pthread_mutex_lock(&debug_state.mutex);
    debug_state.enabled = enabled;
    pthread_mutex_unlock(&debug_state.mutex);
}

/**
 * @brief Enable or disable debug output
 */
void debug_set_enabled(bool enabled) {
    if (!debug_state.initialized) {
        debug_init(enabled);
        return;
    }
    
    pthread_mutex_lock(&debug_state.mutex);
    debug_state.enabled = enabled;
    pthread_mutex_unlock(&debug_state.mutex);
}

/**
 * @brief Check if debug output is enabled
 */
bool debug_is_enabled(void) {
    if (!debug_state.initialized) {
        return false;
    }
    
    bool result;
    pthread_mutex_lock(&debug_state.mutex);
    result = debug_state.enabled;
    pthread_mutex_unlock(&debug_state.mutex);
    
    return result;
}

/**
 * @brief Get the base filename from a path
 */
static const char* get_basename(const char* path) {
    const char* basename = strrchr(path, '/');
    if (basename) {
        return basename + 1;
    } else {
        return path;
    }
}

/**
 * @brief Print a debug message
 */
void debug_print(const char* file, int line, const char* format, ...) {
    if (!debug_state.initialized) {
        debug_init(false);
    }
    
    bool enabled;
    pthread_mutex_lock(&debug_state.mutex);
    enabled = debug_state.enabled;
    pthread_mutex_unlock(&debug_state.mutex);
    
    if (!enabled) {
        return;
    }
    
    va_list args;
    va_start(args, format);
    debug_vprint(file, line, format, args);
    va_end(args);
}

/**
 * @brief Same as debug_print but with va_list
 */
void debug_vprint(const char* file, int line, const char* format, va_list args) {
    if (!debug_state.initialized) {
        debug_init(false);
    }
    
    bool enabled;
    pthread_mutex_lock(&debug_state.mutex);
    enabled = debug_state.enabled;
    pthread_mutex_unlock(&debug_state.mutex);
    
    if (!enabled) {
        return;
    }
    
    // Get current time
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    // Print header with timestamp, file and line
    fprintf(stderr, "[DEBUG %s %s:%d] ", time_str, get_basename(file), line);
    
    // Print the message
    vfprintf(stderr, format, args);
    
    // Add newline if not present
    size_t len = strlen(format);
    if (len == 0 || format[len - 1] != '\n') {
        fputc('\n', stderr);
    }
    
    // Flush immediately to ensure output appears right away
    fflush(stderr);
}
