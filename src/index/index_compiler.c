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
 * @brief Get index type as string
 */
static const char* get_index_type_string(IndexType index_type) {
    switch (index_type) {
        case INDEX_TYPE_BTREE:
            return "btree";
        case INDEX_TYPE_HASH:
            return "hash";
        default:
            return "unknown";
    }
}

/**
 * @brief Get path to index source file
 */
static int get_index_source_path(const TableSchema* schema, const char* column_name,
                                const char* base_dir, int page_number,
                                IndexType index_type,
                                char* output, size_t output_size) {
    const char* type_str = get_index_type_string(index_type);
    
    char src_dir[2048];
    snprintf(src_dir, sizeof(src_dir), "%s/tables/%s/src", base_dir, schema->name);
    
    // Use the same pattern as in index_generator.c
    return snprintf(output, output_size, "%s/%s_%s_index_%s_%d.c", 
                    src_dir, schema->name, type_str, column_name, page_number);
}

/**
 * @brief Get path to index compilation script
 */
static int get_index_script_path(const TableSchema* schema, const char* column_name,
                                const char* base_dir, int page_number,
                                IndexType index_type,
                                char* output, size_t output_size) {
    const char* type_str = get_index_type_string(index_type);
    
    return snprintf(output, output_size, "%s/scripts/compile_index_%s_%s_%s_%d.sh",
                   base_dir, schema->name, type_str, column_name, page_number);
}

/**
 * @brief Get path to compiled index
 */
int get_index_so_path(const TableSchema* schema, const char* column_name,
                     const char* base_dir, int page_number,
                     IndexType index_type,
                     char* output, size_t output_size) {
    const char* type_str = get_index_type_string(index_type);
    
    return snprintf(output, output_size, "%s/compiled/%s_%s_index_%s_%d.so",
                   base_dir, schema->name, type_str, column_name, page_number);
}

/**
 * @brief Check if index is already compiled
 */
bool is_index_compiled(const TableSchema* schema, const char* column_name,
                      const char* base_dir, int page_number,
                      IndexType index_type) {
    char path[2048];
    
    if (get_index_so_path(schema, column_name, base_dir, page_number, index_type, path, sizeof(path)) < 0) {
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
    
    // Create compiled directory if needed
    char compiled_dir[2048];
    snprintf(compiled_dir, sizeof(compiled_dir), "%s/compiled", base_dir);
    
    if (stat(compiled_dir, &st) != 0) {
        if (mkdir(compiled_dir, 0755) != 0) {
            fprintf(stderr, "Failed to create compiled directory: %s\n", strerror(errno));
            return -1;
        }
    }  
    
    // Get type string
    const char* type_str = get_index_type_string(index_def->type);
    
    // Get source file path
    char src_path[2048];
    if (get_index_source_path(schema, index_def->column_name, base_dir, page_number, 
                             index_def->type, src_path, sizeof(src_path)) < 0) {
        fprintf(stderr, "Path too long for source file\n");
        return -1;
    }
    
    // Verify source file exists
    if (!file_exists(src_path)) {
        fprintf(stderr, "Source file not found: %s\n", src_path);
        return -1;
    }
    
    // Create compilation script path
    char script_path[2048];
    if (get_index_script_path(schema, index_def->column_name, base_dir, page_number, 
                             index_def->type, script_path, sizeof(script_path)) < 0) {
        fprintf(stderr, "Path too long for script file\n");
        return -1;
    }
    
    // Open script file
    FILE* script = fopen(script_path, "w");
    if (!script) {
        fprintf(stderr, "Failed to open script file for writing: %s\n", strerror(errno));
        return -1;
    }
    
    // Get output path
    char out_path[2048];
    if (get_index_so_path(schema, index_def->column_name, base_dir, page_number, 
                         index_def->type, out_path, sizeof(out_path)) < 0) {
        fprintf(stderr, "Path too long for output file\n");
        fclose(script);
        return -1;
    }
    
    // Write script
    fprintf(script,
        "#!/bin/bash\n\n"
        "# Compile %s index for %s.%s (page %d)\n\n"
        "CC=${CC:-gcc}\n"
        "CFLAGS=\"-fPIC -shared -O2 -g\"\n\n"
        "# Include paths\n"
        "INCLUDE_PATHS=\"-I%s -I%s/tables/%s\"\n\n"
        "# Source file\n"
        "SRC=\"%s\"\n\n"
        "# Create compiled directory if it doesn't exist\n"
        "mkdir -p %s/compiled\n\n"
        "# Output file\n"
        "OUT=\"%s\"\n\n"
        "# Compile index\n"
        "echo \"Compiling index: $SRC -> $OUT\"\n"
        "$CC $CFLAGS $INCLUDE_PATHS -o \"$OUT\" \"$SRC\"\n\n"
        "if [ $? -eq 0 ]; then\n"
        "    echo \"Successfully compiled index: $OUT\"\n"
        "else\n"
        "    echo \"Failed to compile index\"\n"
        "    exit 1\n"
        "fi\n",
        type_str, schema->name, index_def->column_name, page_number,
        base_dir, base_dir, schema->name,
        src_path,
        base_dir,
        out_path);
    
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
    if (is_index_compiled(schema, index_def->column_name, base_dir, page_number, index_def->type)) {
        return 0;
    }
    
    // Generate compilation script
    if (generate_index_compile_script(schema, index_def, base_dir, page_number) != 0) {
        fprintf(stderr, "Failed to generate compilation script\n");
        return -1;
    }
    
    // Get script path
    char script_path[2048];
    if (get_index_script_path(schema, index_def->column_name, base_dir, page_number, 
                             index_def->type, script_path, sizeof(script_path)) < 0) {
        fprintf(stderr, "Path too long for script file\n");
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
