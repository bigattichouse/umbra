/**
 * @file kernel_compiler.c
 * @brief Compiles generated kernel code
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include "kernel_compiler.h"
#include "../schema/directory_manager.h"

/**
 * @brief Check if kernel is already compiled
 */
bool is_kernel_compiled(const char* kernel_name, const char* base_dir,
                       const char* table_name, int page_number) {
    char so_path[3072];
    
    if (page_number >= 0) {
        snprintf(so_path, sizeof(so_path), "%s/compiled/%s_%s_page_%d.so", 
                 base_dir, kernel_name, table_name, page_number);
    } else {
        snprintf(so_path, sizeof(so_path), "%s/compiled/%s_%s.so", 
                 base_dir, kernel_name, table_name);
    }
    
    struct stat st;
    return stat(so_path, &st) == 0;
}

/**
 * @brief Generate compilation script for kernel
 */
int generate_kernel_compile_script(const GeneratedKernel* kernel, const char* base_dir,
                                  const char* table_name, int page_number) {
    // Create scripts directory if it doesn't exist
    char scripts_dir[2048];
    snprintf(scripts_dir, sizeof(scripts_dir), "%s/scripts", base_dir);
    
    struct stat st;
    if (stat(scripts_dir, &st) != 0) {
        if (mkdir(scripts_dir, 0755) != 0) {
            fprintf(stderr, "Failed to create scripts directory: %s\n", strerror(errno));
            return -1;
        }
    }
    
    // Create script path - increase buffer size to avoid truncation
    char script_path[3072];
    if (page_number >= 0) {
        snprintf(script_path, sizeof(script_path), "%s/compile_%s_%s_page_%d.sh", 
                 scripts_dir, kernel->kernel_name, table_name, page_number);
    } else {
        snprintf(script_path, sizeof(script_path), "%s/compile_%s_%s.sh", 
                 scripts_dir, kernel->kernel_name, table_name);
    }
    
    // Open script file
    FILE* script = fopen(script_path, "w");
    if (!script) {
        fprintf(stderr, "Failed to open script file: %s\n", strerror(errno));
        return -1;
    }
    
    // Write script
    fprintf(script, "#!/bin/bash\n\n");
    fprintf(script, "# Compile kernel %s for table %s\n\n", kernel->kernel_name, table_name);
    fprintf(script, "CC=${CC:-gcc}\n");
    fprintf(script, "CFLAGS=\"-fPIC -shared -O2 -g\"\n\n");
    
    // Include paths
    fprintf(script, "INCLUDE_PATHS=\"-I%s/tables/%s\"\n\n", base_dir, table_name);
    
    // Source and output paths
    if (page_number >= 0) {
        fprintf(script, "SRC=\"%s/kernels/%s_%s_page_%d.c\"\n", 
                base_dir, kernel->kernel_name, table_name, page_number);
        fprintf(script, "OUT=\"%s/compiled/%s_%s_page_%d.so\"\n", 
                base_dir, kernel->kernel_name, table_name, page_number);
    } else {
        fprintf(script, "SRC=\"%s/kernels/%s_%s.c\"\n", 
                base_dir, kernel->kernel_name, table_name);
        fprintf(script, "OUT=\"%s/compiled/%s_%s.so\"\n", 
                base_dir, kernel->kernel_name, table_name);
    }
    
    // Create compiled directory if it doesn't exist
    fprintf(script, "\n# Create compiled directory if it doesn't exist\n");
    fprintf(script, "mkdir -p %s/compiled\n\n", base_dir);
    
    // Compile command
    fprintf(script, "# Compile the kernel\n");
    fprintf(script, "$CC $CFLAGS $INCLUDE_PATHS -o \"$OUT\" \"$SRC\"\n\n");
    
    fprintf(script, "if [ $? -eq 0 ]; then\n");
    fprintf(script, "    echo \"Successfully compiled kernel: $OUT\"\n");
    fprintf(script, "else\n");
    fprintf(script, "    echo \"Failed to compile kernel\"\n");
    fprintf(script, "    exit 1\n");
    fprintf(script, "fi\n");
    
    fclose(script);
    
    // Make script executable
    if (chmod(script_path, 0755) != 0) {
        fprintf(stderr, "Failed to make script executable: %s\n", strerror(errno));
        return -1;
    }
    
    return 0;
}

/**
 * @brief Compile generated kernel code
 */
int compile_kernel(const GeneratedKernel* kernel, const char* base_dir,
                  const char* table_name, int page_number,
                  char* output_path, size_t output_size) {
    // Check if already compiled
    if (is_kernel_compiled(kernel->kernel_name, base_dir, table_name, page_number)) {
        // Return path to existing .so
        if (page_number >= 0) {
            snprintf(output_path, output_size, "%s/compiled/%s_%s_page_%d.so", 
                     base_dir, kernel->kernel_name, table_name, page_number);
        } else {
            snprintf(output_path, output_size, "%s/compiled/%s_%s.so", 
                     base_dir, kernel->kernel_name, table_name);
        }
        return 0;
    }
    
    // Write kernel source
    if (write_kernel_source(kernel, base_dir, table_name, page_number) != 0) {
        fprintf(stderr, "Failed to write kernel source\n");
        return -1;
    }
    
    // Generate compilation script
    if (generate_kernel_compile_script(kernel, base_dir, table_name, page_number) != 0) {
        fprintf(stderr, "Failed to generate compilation script\n");
        return -1;
    }
    
    // Execute compilation script
    char script_path[3072];
    if (page_number >= 0) {
        snprintf(script_path, sizeof(script_path), "%s/scripts/compile_%s_%s_page_%d.sh", 
                 base_dir, kernel->kernel_name, table_name, page_number);
    } else {
        snprintf(script_path, sizeof(script_path), "%s/scripts/compile_%s_%s.sh", 
                 base_dir, kernel->kernel_name, table_name);
    }
    
    // Increase command buffer size to accommodate longer paths
    char command[4096];
    snprintf(command, sizeof(command), "bash %s", script_path);
    
    int result = system(command);
    if (result != 0) {
        fprintf(stderr, "Kernel compilation failed\n");
        return -1;
    }
    
    // Return path to compiled .so
    if (page_number >= 0) {
        snprintf(output_path, output_size, "%s/compiled/%s_%s_page_%d.so", 
                 base_dir, kernel->kernel_name, table_name, page_number);
    } else {
        snprintf(output_path, output_size, "%s/compiled/%s_%s.so", 
                 base_dir, kernel->kernel_name, table_name);
    }
    
    return 0;
}
