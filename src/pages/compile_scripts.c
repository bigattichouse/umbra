/**
 * @file compile_scripts.c
 * @brief Generates compilation scripts for pages
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include "compile_scripts.h"
#include "page_template.h"
#include "../schema/directory_manager.h"

/**
 * @brief Generate compilation script for a data page
 */
int generate_compilation_script(const TableSchema* schema, const char* base_dir, int page_number) {
    if (!schema || !base_dir) {
        return -1;
    }
    
    // Create scripts directory
    char scripts_dir[2048];
    snprintf(scripts_dir, sizeof(scripts_dir), "%s/scripts", base_dir);
    
    struct stat st;
    if (stat(scripts_dir, &st) != 0) {
        // Directory doesn't exist, create it
        if (mkdir(scripts_dir, 0755) != 0) {
            fprintf(stderr, "Failed to create scripts directory: %s\n", strerror(errno));
            return -1;
        }
    } else if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Scripts path exists but is not a directory\n");
        return -1;
    }
    
    // Create script path
    char script_path[3072];
    if (get_compile_script_path(schema->name, base_dir, page_number, 
                               script_path, sizeof(script_path)) != 0) {
        return -1;
    }
    
    // Open script file
    FILE* script_file = fopen(script_path, "w");
    if (!script_file) {
        fprintf(stderr, "Failed to open %s for writing: %s\n", script_path, strerror(errno));
        return -1;
    }
    
    // Write compilation script
    fprintf(script_file, COMPILE_SCRIPT_TEMPLATE,
            page_number, schema->name,
            base_dir,
            base_dir, schema->name, page_number, base_dir, schema->name, schema->name, page_number,
            schema->name, page_number);
    
    // Close file
    fclose(script_file);
    
    // Make script executable
    if (chmod(script_path, 0755) != 0) {
        fprintf(stderr, "Failed to make script executable: %s\n", strerror(errno));
        return -1;
    }
    
    return 0;
}

/**
 * @brief Get path to a page's compilation script
 */
int get_compile_script_path(const char* table_name, const char* base_dir, 
                           int page_number, char* output, size_t output_size) {
    if (!table_name || !base_dir || !output || output_size <= 0) {
        return -1;
    }
    
    int written = snprintf(output, output_size, "%s/scripts/compile_%s_page_%d.sh", 
                          base_dir, table_name, page_number);
    
    if (written < 0 || (size_t)written >= output_size) {
        return -1;
    }
    
    return 0;
}

/**
 * @brief Generate compilation script for a filtered accessor
 */
int generate_filtered_compilation_script(const TableSchema* schema, const char* base_dir, 
                                        int page_number, const char* suffix) {
    if (!schema || !base_dir || !suffix) {
        return -1;
    }
    
    // Create scripts directory
    char scripts_dir[2048];
    snprintf(scripts_dir, sizeof(scripts_dir), "%s/scripts", base_dir);
    
    struct stat st;
    if (stat(scripts_dir, &st) != 0) {
        // Directory doesn't exist, create it
        if (mkdir(scripts_dir, 0755) != 0) {
            fprintf(stderr, "Failed to create scripts directory: %s\n", strerror(errno));
            return -1;
        }
    } else if (!S_ISDIR(st.st_mode)) {
        fprintf(stderr, "Scripts path exists but is not a directory\n");
        return -1;
    }
    
    // Create script path
    char script_path[3072];
    snprintf(script_path, sizeof(script_path), "%s/scripts/compile_%s_page_%d_%s.sh", 
             base_dir, schema->name, page_number, suffix);
    
    // Open script file
    FILE* script_file = fopen(script_path, "w");
    if (!script_file) {
        fprintf(stderr, "Failed to open %s for writing: %s\n", script_path, strerror(errno));
        return -1;
    }
    
    // Write compilation script
    fprintf(script_file,
        "#!/bin/bash\n\n"
        "# Compile %s data page %d for table %s\n\n"
        "CC=${CC:-gcc}\n"
        "CFLAGS=\"-fPIC -shared -O2 -g\"\n\n"
        "# Create compiled directory if it doesn't exist\n"
        "mkdir -p %s/compiled\n\n"
        "# Compile the data page\n"
        "$CC $CFLAGS -o %s/compiled/%sData_%d_%s.so %s/tables/%s/src/%sData_%d_%s.c\n\n"
        "echo \"Compiled %sData_%d_%s.so\"\n",
        suffix, page_number, schema->name,
        base_dir,
        base_dir, schema->name, page_number, suffix, base_dir, schema->name, schema->name, page_number, suffix,
        schema->name, page_number, suffix);
    
    // Close file
    fclose(script_file);
    
    // Make script executable
    if (chmod(script_path, 0755) != 0) {
        fprintf(stderr, "Failed to make script executable: %s\n", strerror(errno));
        return -1;
    }
    
    return 0;
}

/**
 * @brief Generate makefile for compiling all pages of a table
 */
int generate_table_makefile(const TableSchema* schema, const char* base_dir, int page_count) {
    if (!schema || !base_dir || page_count <= 0) {
        return -1;
    }
    
    // Get table directory
    char table_dir[2048];
    if (get_table_directory(schema->name, base_dir, table_dir, sizeof(table_dir)) != 0) {
        return -1;
    }
    
    // Create makefile path - increase buffer size
    char makefile_path[3072];
    snprintf(makefile_path, sizeof(makefile_path), "%s/Makefile", table_dir);
    
    // Open makefile
    FILE* makefile = fopen(makefile_path, "w");
    if (!makefile) {
        fprintf(stderr, "Failed to open %s for writing: %s\n", makefile_path, strerror(errno));
        return -1;
    }
    
    // Write makefile header
    fprintf(makefile,
        "# Makefile for %s table\n\n"
        "CC ?= gcc\n"
        "CFLAGS = -fPIC -shared -O2 -g\n\n"
        "COMPILED_DIR = %s/compiled\n"
        "SRC_DIR = %s/tables/%s/src\n\n"
        "# Ensure compiled directory exists\n"
        "$(COMPILED_DIR):\n"
        "\tmkdir -p $(COMPILED_DIR)\n\n"
        "# Target for all pages\n"
        "all: $(COMPILED_DIR)",
        schema->name,
        base_dir,
        base_dir, schema->name);
    
    // Add targets for each page
    for (int i = 0; i < page_count; i++) {
        fprintf(makefile, " page_%d", i);
    }
    
    fprintf(makefile, "\n\n");
    
    // Add individual page targets
    for (int i = 0; i < page_count; i++) {
        fprintf(makefile,
            "# Target for page %d\n"
            "page_%d: $(COMPILED_DIR)\n"
            "\t$(CC) $(CFLAGS) -o $(COMPILED_DIR)/%sData_%d.so $(SRC_DIR)/%sData_%d.c\n\n",
            i, i, schema->name, i, schema->name, i);
    }
    
    // Add clean target
    fprintf(makefile,
        "# Clean target\n"
        "clean:\n"
        "\trm -f $(COMPILED_DIR)/%sData_*.so\n\n"
        ".PHONY: all clean",
        schema->name);
    
    // Close file
    fclose(makefile);
    return 0;
}
