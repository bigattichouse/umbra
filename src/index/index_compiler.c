/**
 * @file index_compiler.c
 * @brief Compiles index structures
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include "index_compiler.h"
#include "../schema/directory_manager.h"

/**
 * @brief Check if a file exists
 */
static bool file_exists(const char* path) {
    struct stat st;
    return stat(path, &st) == 0;
}

/**
 * @brief Get path to index compilation script
 */
static int get_index_script_path(const TableSchema* schema, const char* column_name,
                                const char* base_dir, int page_number,
                                char* output, size_t output_size) {
    return snprintf(output, output_size, "%s/scripts/compile_index_%s_%s_%d.sh",
                   base_dir, schema->name, column_name, page_number);
}

/**
 * @brief Get path to compiled index
 */
int get_index_so_path(const TableSchema* schema, const char* column_name,
                     const char* base_dir, int page_number,
                     IndexType index_type,
                     char* output, size_t output_size) {
    // Add index type to the path
    const char* type_str;
    switch (index_type) {
        case INDEX_BTREE:
            type_str = "btree";
            break;
        case INDEX_HASH:
            type_str = "hash";
            break;
        default:
            type_str = "unknown";
            break;
    }
    
    return snprintf(output, output_size, "%s/compiled/%s_%s_index_%s_%d.so",
                   base_dir, schema->name, type_str, column_name, page_number);
}

/**
 * @brief Check if index is already compiled
 */
bool is_index_compiled(const TableSchema* schema, const char* column_name,
                      const char* base_dir, int page_number) {
    char path[2048];
    
    if (get_index_so_path(schema, column_name, base_dir, page_number, path, sizeof(path)) < 0) {
        return false;
    }
    
    return file_exists(path);
}

/**
 * @brief Generate compilation script for index
 */
int generate_index_compile_script(const TableSchema* schema, const IndexDefinition* index_def,
                                 const char* base_dir, int page_number) {
    if (!schema || !index_def || !base_dir) {
        return -1;
    }
    
    // Create scripts directory if needed
    char scripts_dir[2048];
    snprintf(scripts_dir, sizeof(scripts_dir), "%s/scripts", base_dir);
    
    struct stat st;
    if (stat(scripts_dir, &st) != 0) {
        if (mkdir(scripts_dir, 0755) != 0) {
            fprintf(stderr, "Failed to create scripts directory: %s\n", strerror(errno));
            return -1;
        }
    }
    
    // Create compilation script path
    char script_path[2048];
    if (get_index_script_path(schema, index_def->column_name, base_dir, page_number,
                             script_path, sizeof(script_path)) < 0) {
        return -1;
    }
    
    // Open script file
    FILE* script = fopen(script_path, "w");
    if (!script) {
        fprintf(stderr, "Failed to open script file for writing: %s\n", strerror(errno));
        return -1;
    }
    
    // Write script
    fprintf(script,
        "#!/bin/bash\n\n"
        "# Compile index for %s.%s (page %d)\n\n"
        "CC=${CC:-gcc}\n"
        "CFLAGS=\"-fPIC -shared -O2 -g\"\n\n"
        "# Include paths\n"
        "INCLUDE_PATHS=\"-I%s -I%s/tables/%s\"\n\n"
        "# Source file\n"
        "SRC=\"%s/tables/%s/src/%s_index_%d.c\"\n\n"
        "# Create compiled directory if it doesn't exist\n"
        "mkdir -p %s/compiled\n\n"
        "# Output file\n"
        "OUT=\"%s/compiled/%s_index_%s_%d.so\"\n\n"
        "# Compile index\n"
        "$CC $CFLAGS $INCLUDE_PATHS -o \"$OUT\" \"$SRC\"\n\n"
        "if [ $? -eq 0 ]; then\n"
        "    echo \"Successfully compiled index: $OUT\"\n"
        "else\n"
        "    echo \"Failed to compile index\"\n"
        "    exit 1\n"
        "fi\n",
        schema->name, index_def->column_name, page_number,
        base_dir, base_dir, schema->name,
        base_dir, schema->name, index_def->column_name, page_number,
        base_dir,
        base_dir, schema->name, index_def->column_name, page_number);
    
    fclose(script);
    
    // Make script executable
    if (chmod(script_path, 0755) != 0) {
        fprintf(stderr, "Failed to make script executable: %s\n", strerror(errno));
        return -1;
    }
    
    return 0;
}

/**
 * @brief Compile index for a page
 */
int compile_index(const TableSchema* schema, const IndexDefinition* index_def,
                 const char* base_dir, int page_number) {
    if (!schema || !index_def || !base_dir) {
        return -1;
    }
    
    // Check if already compiled
    if (is_index_compiled(schema, index_def->column_name, base_dir, page_number)) {
        return 0;
    }
    
    // Generate compilation script
    if (generate_index_compile_script(schema, index_def, base_dir, page_number) != 0) {
        return -1;
    }
    
    // Get script path
    char script_path[2048];
    if (get_index_script_path(schema, index_def->column_name, base_dir, page_number,
                             script_path, sizeof(script_path)) < 0) {
        return -1;
    }
    
    // Execute script
    char command[2048 + 10]; // Allow for "bash " prefix
    snprintf(command, sizeof(command), "bash %s", script_path);
    
    int result = system(command);
    if (result != 0) {
        fprintf(stderr, "Failed to compile index: %s\n", strerror(errno));
        return -1;
    }
    
    return 0;
}
