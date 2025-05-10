/**
 * @file kernel_generator.h
 * @brief Interface for kernel generation
 */

#ifndef UMBRA_KERNEL_GENERATOR_H
#define UMBRA_KERNEL_GENERATOR_H

#include "../parser/ast.h"
#include "../schema/schema_parser.h"

/**
 * @struct GeneratedKernel
 * @brief Represents generated kernel code
 */
typedef struct {
    char* code;              /**< Generated C code */
    char* kernel_name;       /**< Name of the kernel function */
    char** dependencies;     /**< Required header files */
    int dependency_count;    /**< Number of dependencies */
} GeneratedKernel;

/**
 * @brief Generate kernel for SELECT statement
 * @param stmt SELECT statement AST
 * @param schema Table schema
 * @param base_dir Base directory for database
 * @return Generated kernel or NULL on error
 */
GeneratedKernel* generate_select_kernel(const SelectStatement* stmt, 
                                       const TableSchema* schema,
                                       const char* base_dir);

/**
 * @brief Free generated kernel
 * @param kernel Kernel to free
 */
void free_generated_kernel(GeneratedKernel* kernel);

/**
 * @brief Get kernel name for a SELECT statement
 * @param stmt SELECT statement
 * @param output Buffer for kernel name
 * @param output_size Size of output buffer
 * @return 0 on success, -1 on error
 */
int get_kernel_name(const SelectStatement* stmt, char* output, size_t output_size);

/**
 * @brief Generate kernel source file
 * @param kernel Generated kernel
 * @param base_dir Base directory
 * @param table_name Table name
 * @param page_number Page number (-1 for all pages)
 * @return 0 on success, -1 on error
 */
int write_kernel_source(const GeneratedKernel* kernel, const char* base_dir,
                       const char* table_name, int page_number);

#endif /* UMBRA_KERNEL_GENERATOR_H */
