/**
 * @file command_history.c
 * @brief Manages command history for interactive mode
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <readline/history.h>
#include "command_history.h"

#define MAX_HISTORY_SIZE 1000

/**
 * @brief Get history file path
 */
int get_history_file_path(char* buffer, size_t size) {
    const char* home = getenv("HOME");
    if (!home) {
        home = ".";
    }
    
    // Create .umbra directory if it doesn't exist
    char umbra_dir[1024];
    snprintf(umbra_dir, sizeof(umbra_dir), "%s/.umbra", home);
    
    struct stat st;
    if (stat(umbra_dir, &st) != 0) {
        // Directory doesn't exist, create it
        if (mkdir(umbra_dir, 0755) != 0) {
            // Failed to create directory, use current directory
            snprintf(buffer, size, ".umbra_history");
            return 0;
        }
    }
    
    snprintf(buffer, size, "%s/.umbra/history", home);
    return 0;
}

/**
 * @brief Initialize command history
 */
int init_command_history(void) {
    char history_file[1024];
    if (get_history_file_path(history_file, sizeof(history_file)) != 0) {
        return -1;
    }
    
    // Set history file
    using_history();
    stifle_history(MAX_HISTORY_SIZE);
    
    // Read history from file
    if (read_history(history_file) != 0) {
        // File doesn't exist yet, that's okay
    }
    
    return 0;
}

/**
 * @brief Save command history to file
 */
int save_command_history(void) {
    char history_file[1024];
    if (get_history_file_path(history_file, sizeof(history_file)) != 0) {
        return -1;
    }
    
    // Write history to file
    if (write_history(history_file) != 0) {
        fprintf(stderr, "Warning: Failed to save command history\n");
        return -1;
    }
    
    return 0;
}
