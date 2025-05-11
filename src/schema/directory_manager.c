/**
 * @file directory_manager.c
 * @brief Creates and manages directory structures for tables
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>  // For PATH_MAX
#include "directory_manager.h"

// Define maximum path length if not defined
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

// Forward declarations for static functions
static int create_directory_if_not_exists(const char* path);

/**
 * @brief Create a directory if it doesn't exist
 */
static int create_directory_if_not_exists(const char* path) {
    struct stat st;
    
    // Check if directory already exists
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return 0; // Directory exists
        }
        fprintf(stderr, "Path exists but is not a directory: %s\n", path);
        return -1;
    }
    
    // Create the directory
    if (mkdir(path, 0755) != 0) {
        fprintf(stderr, "Failed to create directory %s: %s\n", path, strerror(errno));
        return -1;
    }
    
    return 0;
}

/**
 * @brief Initialize the database directory structure
 */
int initialize_database_directory(const char* base_dir) {
    if (!base_dir) {
        return -1;
    }
    
    // Create base directory
    if (create_directory_if_not_exists(base_dir) != 0) {
        return -1;
    }
    
    // Create subdirectories
    char path[PATH_MAX];
    int len;
    
    // Tables directory (to store table metadata)
    len = snprintf(path, sizeof(path), "%s/tables", base_dir);
    if (len < 0 || (size_t)len >= sizeof(path)) {
        fprintf(stderr, "Path too long: %s/tables\n", base_dir);
        return -1;
    }
    if (create_directory_if_not_exists(path) != 0) {
        return -1;
    }
    
    // Permissions directory
    len = snprintf(path, sizeof(path), "%s/permissions", base_dir);
    if (len < 0 || (size_t)len >= sizeof(path)) {
        fprintf(stderr, "Path too long: %s/permissions\n", base_dir);
        return -1;
    }
    if (create_directory_if_not_exists(path) != 0) {
        return -1;
    }
    
    // Compiled directory (for .so files)
    len = snprintf(path, sizeof(path), "%s/compiled", base_dir);
    if (len < 0 || (size_t)len >= sizeof(path)) {
        fprintf(stderr, "Path too long: %s/compiled\n", base_dir);
        return -1;
    }
    if (create_directory_if_not_exists(path) != 0) {
        return -1;
    }
    
    return 0;
}

/**
 * @brief Create a table directory structure
 */
int create_table_directory(const char* table_name, const char* base_dir) {
    if (!table_name || !base_dir) {
        return -1;
    }
    
    // Create base directory if it doesn't exist
    if (initialize_database_directory(base_dir) != 0) {
        return -1;
    }
    
    // Create table directory
    char table_dir[PATH_MAX];
    int len = snprintf(table_dir, sizeof(table_dir), "%s/tables/%s", base_dir, table_name);
    if (len < 0 || (size_t)len >= sizeof(table_dir)) {
        fprintf(stderr, "Path too long: %s/tables/%s\n", base_dir, table_name);
        return -1;
    }
    if (create_directory_if_not_exists(table_dir) != 0) {
        return -1;
    }
    
    // Create metadata directory
    char metadata_dir[PATH_MAX];
    len = snprintf(metadata_dir, sizeof(metadata_dir), "%s/metadata", table_dir);
    if (len < 0 || (size_t)len >= sizeof(metadata_dir)) {
        fprintf(stderr, "Path too long: %s/metadata\n", table_dir);
        return -1;
    }
    if (create_directory_if_not_exists(metadata_dir) != 0) {
        return -1;
    }
    
    // Create data directory
    char data_dir[PATH_MAX];
    len = snprintf(data_dir, sizeof(data_dir), "%s/data", table_dir);
    if (len < 0 || (size_t)len >= sizeof(data_dir)) {
        fprintf(stderr, "Path too long: %s/data\n", table_dir);
        return -1;
    }
    if (create_directory_if_not_exists(data_dir) != 0) {
        return -1;
    }
    
    // Create source directory
    char src_dir[PATH_MAX];
    len = snprintf(src_dir, sizeof(src_dir), "%s/src", table_dir);
    if (len < 0 || (size_t)len >= sizeof(src_dir)) {
        fprintf(stderr, "Path too long: %s/src\n", table_dir);
        return -1;
    }
    if (create_directory_if_not_exists(src_dir) != 0) {
        return -1;
    }
    
    return 0;
}

/**
 * @brief Check if a table directory exists
 */
bool table_directory_exists(const char* table_name, const char* base_dir) {
    if (!table_name || !base_dir) {
        return false;
    }
    
    char table_dir[PATH_MAX];
    int len = snprintf(table_dir, sizeof(table_dir), "%s/tables/%s", base_dir, table_name);
    if (len < 0 || (size_t)len >= sizeof(table_dir)) {
        return false;
    }
    
    struct stat st;
    if (stat(table_dir, &st) == 0 && S_ISDIR(st.st_mode)) {
        return true;
    }
    
    return false;
}

/**
 * @brief Get table directory path
 */
int get_table_directory(const char* table_name, const char* base_dir, 
                       char* output, size_t output_size) {
    if (!table_name || !base_dir || !output || output_size <= 0) {
        return -1;
    }
    
    int written = snprintf(output, output_size, "%s/tables/%s", base_dir, table_name);
    if (written < 0 || (size_t)written >= output_size) {
        return -1;
    }
    
    return 0;
}

/**
 * @brief Get data directory path for a table
 */
int get_data_directory(const char* table_name, const char* base_dir, 
                      char* output, size_t output_size) {
    if (!table_name || !base_dir || !output || output_size <= 0) {
        return -1;
    }
    
    int written = snprintf(output, output_size, "%s/tables/%s/data", base_dir, table_name);
    if (written < 0 || (size_t)written >= output_size) {
        return -1;
    }
    
    return 0;
}

/**
 * @brief Get source directory path for a table
 */
int get_source_directory(const char* table_name, const char* base_dir, 
                        char* output, size_t output_size) {
    if (!table_name || !base_dir || !output || output_size <= 0) {
        return -1;
    }
    
    int written = snprintf(output, output_size, "%s/tables/%s/src", base_dir, table_name);
    if (written < 0 || (size_t)written >= output_size) {
        return -1;
    }
    
    return 0;
}

/**
 * @brief Get compiled directory path
 */
int get_compiled_directory(const char* base_dir, char* output, size_t output_size) {
    if (!base_dir || !output || output_size <= 0) {
        return -1;
    }
    
    int written = snprintf(output, output_size, "%s/compiled", base_dir);
    if (written < 0 || (size_t)written >= output_size) {
        return -1;
    }
    
    return 0;
}

/**
 * @brief Get path for a compiled shared object file
 */
int get_compiled_so_path(const char* table_name, int page_number, const char* base_dir,
                        char* output, size_t output_size) {
    if (!table_name || !base_dir || !output || output_size <= 0) {
        return -1;
    }
    
    int written = snprintf(output, output_size, "%s/compiled/%s.%d.so", 
                          base_dir, table_name, page_number);
    if (written < 0 || (size_t)written >= output_size) {
        return -1;
    }
    
    return 0;
}

/**
 * @brief Get path for a data header file
 */
int get_data_header_path(const char* table_name, int page_number, const char* base_dir,
                        char* output, size_t output_size) {
    if (!table_name || !base_dir || !output || output_size <= 0) {
        return -1;
    }
    
    int written = snprintf(output, output_size, "%s/tables/%s/data/%s.%d.dat.h", 
                          base_dir, table_name, table_name, page_number);
    if (written < 0 || (size_t)written >= output_size) {
        return -1;
    }
    
    return 0;
}

/**
 * @brief Get path for a page source file
 */
int get_page_source_path(const char* table_name, int page_number, const char* base_dir,
                        char* output, size_t output_size) {
    if (!table_name || !base_dir || !output || output_size <= 0) {
        return -1;
    }
    
    int written = snprintf(output, output_size, "%s/tables/%s/src/%s.%d.c", 
                          base_dir, table_name, table_name, page_number);
    if (written < 0 || (size_t)written >= output_size) {
        return -1;
    }
    
    return 0;
}
