/**
 * @file kernel_generator.c
 * @brief Generates kernel code from AST
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include "kernel_generator.h"
#include "filter_generator.h"
#include "projection_generator.h"
#include "../schema/directory_manager.h"

/**
 * @brief Generate a unique kernel name
 */
int get_kernel_name(const SelectStatement* stmt, char* output, size_t output_size) {
    // Generate hash-based name for kernel
    unsigned int hash = 5381;
    
    // Hash table name
    if (stmt->from_table) {
        const char* table = stmt->from_table->table_name;
        while (*table) {
            hash = ((hash << 5) + hash) + *table;
            table++;
        }
    }
    
    // Hash WHERE clause (simplified - in practice, we'd traverse the AST)
    if (stmt->where_clause) {
        hash = ((hash << 5) + hash) + stmt->where_clause->type;
    }
    
    // Add timestamp for uniqueness
    time_t now = time(NULL);
    hash = ((hash << 5) + hash) + (unsigned int)now;
    
    snprintf(output, output_size, "kernel_%x", hash);
    return 0;
}

/**
 * @brief Initialize generated kernel
 */
static GeneratedKernel* init_kernel(void) {
    GeneratedKernel* kernel = malloc(sizeof(GeneratedKernel));
    kernel->code = NULL;
    kernel->kernel_name = NULL;
    kernel->dependencies = NULL;
    kernel->dependency_count = 0;
    return kernel;
}

/**
 * @brief Add dependency to kernel
 */
static void add_dependency(GeneratedKernel* kernel, const char* header) {
    kernel->dependencies = realloc(kernel->dependencies, 
                                  (kernel->dependency_count + 1) * sizeof(char*));
    kernel->dependencies[kernel->dependency_count] = strdup(header);
    kernel->dependency_count++;
}

/**
 * @brief Generate includes section
 */
static void generate_includes(GeneratedKernel* kernel, const TableSchema* schema,
                             char** code, size_t* code_size) {
    char buffer[1024];
    
    // Standard includes
    snprintf(buffer, sizeof(buffer), 
        "#include <stdio.h>\n"
        "#include <stdlib.h>\n"
        "#include <string.h>\n"
        "#include <stdbool.h>\n"
        "#include \"../../%s.h\"\n\n",
        schema->name);
    
    *code = realloc(*code, *code_size + strlen(buffer) + 1);
    strcat(*code, buffer);
    *code_size += strlen(buffer);
    
    // Add schema header as dependency
    char schema_header[256];
    snprintf(schema_header, sizeof(schema_header), "%s.h", schema->name);
    add_dependency(kernel, schema_header);
}

/**
 * @brief Generate result structure
 */
static void generate_result_struct(const SelectStatement* stmt, const TableSchema* schema,
                                  char** code, size_t* code_size) {
    char buffer[4096];
    
    // For SELECT *, use full schema
    if (stmt->select_list.has_star) {
        snprintf(buffer, sizeof(buffer),
            "/* Result structure matches full schema */\n"
            "typedef %s KernelResult;\n\n",
            schema->name);
    } else {
        // Generate projection structure
        generate_projection_struct(stmt, schema, buffer, sizeof(buffer));
    }
    
    *code = realloc(*code, *code_size + strlen(buffer) + 1);
    strcat(*code, buffer);
    *code_size += strlen(buffer);
}

/**
 * @brief Generate kernel function
 */
static void generate_kernel_function(const SelectStatement* stmt, const TableSchema* schema,
                                    const char* kernel_name, char** code, size_t* code_size) {
    char buffer[8192];
    
    // Function signature
    snprintf(buffer, sizeof(buffer),
        "/**\n"
        " * @brief Execute compiled query kernel\n"
        " * @param data Data array\n"
        " * @param count Number of records\n"
        " * @param results Output results\n"
        " * @param max_results Maximum number of results\n"
        " * @return Number of matching records\n"
        " */\n"
        "int %s(%s* data, int count, KernelResult* results, int max_results) {\n"
        "    int result_count = 0;\n\n",
        kernel_name, schema->name);
    
    *code = realloc(*code, *code_size + strlen(buffer) + 1);
    strcat(*code, buffer);
    *code_size += strlen(buffer);
    
    // Main processing loop
    snprintf(buffer, sizeof(buffer),
        "    for (int i = 0; i < count && result_count < max_results; i++) {\n"
        "        %s* record = &data[i];\n",
        schema->name);
    
    *code = realloc(*code, *code_size + strlen(buffer) + 1);
    strcat(*code, buffer);
    *code_size += strlen(buffer);
    
    // Generate WHERE clause filter
    if (stmt->where_clause) {
        char filter_code[4096];
        if (generate_filter_condition(stmt->where_clause, schema, filter_code, sizeof(filter_code)) == 0) {
            snprintf(buffer, sizeof(buffer),
                "        \n"
                "        /* WHERE clause */\n"
                "        if (!(%s)) {\n"
                "            continue;\n"
                "        }\n",
                filter_code);
            
            *code = realloc(*code, *code_size + strlen(buffer) + 1);
            strcat(*code, buffer);
            *code_size += strlen(buffer);
        }
    }
    
    // Generate projection
    if (stmt->select_list.has_star) {
        // Copy entire record
        snprintf(buffer, sizeof(buffer),
            "        \n"
            "        /* Copy entire record */\n"
            "        results[result_count] = *record;\n");
    } else {
        // Generate projection code
        char projection_code[4096];
        if (generate_projection_assignment(stmt, schema, projection_code, sizeof(projection_code)) == 0) {
            snprintf(buffer, sizeof(buffer),
                "        \n"
                "        /* Project selected columns */\n"
                "%s",
                projection_code);
        }
    }
    
    *code = realloc(*code, *code_size + strlen(buffer) + 1);
    strcat(*code, buffer);
    *code_size += strlen(buffer);
    
    // End of loop
    snprintf(buffer, sizeof(buffer),
        "        result_count++;\n"
        "    }\n"
        "    \n"
        "    return result_count;\n"
        "}\n");
    
    *code = realloc(*code, *code_size + strlen(buffer) + 1);
    strcat(*code, buffer);
    *code_size += strlen(buffer);
}

/**
 * @brief Generate kernel for SELECT statement
 */
GeneratedKernel* generate_select_kernel(const SelectStatement* stmt, 
                                       const TableSchema* schema,
                                       const char* base_dir) {
    GeneratedKernel* kernel = init_kernel();
    
    // Generate kernel name
    char kernel_name[256];
    if (get_kernel_name(stmt, kernel_name, sizeof(kernel_name)) != 0) {
        free_generated_kernel(kernel);
        return NULL;
    }
    
    kernel->kernel_name = strdup(kernel_name);
    
    // Initialize code buffer
    size_t code_size = 0;
    kernel->code = malloc(1);
    kernel->code[0] = '\0';
    
    // Generate includes
    generate_includes(kernel, schema, &kernel->code, &code_size);
    
    // Generate result structure
    generate_result_struct(stmt, schema, &kernel->code, &code_size);
    
    // Generate kernel function
    generate_kernel_function(stmt, schema, kernel_name, &kernel->code, &code_size);
    
    return kernel;
}

/**
 * @brief Free generated kernel
 */
void free_generated_kernel(GeneratedKernel* kernel) {
    if (!kernel) return;
    
    free(kernel->code);
    free(kernel->kernel_name);
    
    for (int i = 0; i < kernel->dependency_count; i++) {
        free(kernel->dependencies[i]);
    }
    free(kernel->dependencies);
    
    free(kernel);
}

/**
 * @brief Write kernel source file
 */
int write_kernel_source(const GeneratedKernel* kernel, const char* base_dir,
                       const char* table_name, int page_number) {
    if (!kernel || !base_dir || !table_name) {
        return -1;
    }
    
    // Create kernels directory
    char kernels_dir[1024];
    snprintf(kernels_dir, sizeof(kernels_dir), "%s/kernels", base_dir);
    
    struct stat st;
    if (stat(kernels_dir, &st) != 0) {
        if (mkdir(kernels_dir, 0755) != 0) {
            fprintf(stderr, "Failed to create kernels directory: %s\n", strerror(errno));
            return -1;
        }
    }
    
    // Create source file path
    char src_path[1024];
    if (page_number >= 0) {
        snprintf(src_path, sizeof(src_path), "%s/%s_%s_page_%d.c", 
                 kernels_dir, kernel->kernel_name, table_name, page_number);
    } else {
        snprintf(src_path, sizeof(src_path), "%s/%s_%s.c", 
                 kernels_dir, kernel->kernel_name, table_name);
    }
    
    // Write source file
    FILE* file = fopen(src_path, "w");
    if (!file) {
        fprintf(stderr, "Failed to open %s for writing: %s\n", src_path, strerror(errno));
        return -1;
    }
    
    fprintf(file, "/* Generated kernel for %s table */\n", table_name);
    fprintf(file, "/* Kernel: %s */\n\n", kernel->kernel_name);
    
    fprintf(file, "%s", kernel->code);
    
    fclose(file);
    return 0;
}
