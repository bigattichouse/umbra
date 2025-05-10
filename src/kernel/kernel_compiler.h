/**
 * @file kernel_compiler.h
 * @brief Interface for kernel compilation
 */

#ifndef UMBRA_KERNEL_COMPILER_H
#define UMBRA_KERNEL_COMPILER_H

#include "kernel_generator.h"

/**
 * @brief Compile generated kernel code
 * @param kernel Generated kernel
 * @param base_dir Base directory
 * @param table_name Table name
 * @param page_number Page number (-1 for all pages)
 * @param output_path Output path for compiled .so file
 * @param output_size Size of output path buffer
 * @return 0 on success, -1 on error
 */
int compile_kernel(const GeneratedKernel* kernel, const char* base_dir,
                  const char* table_name, int page_number,
                  char* output_path, size_t output_size);

/**
 * @brief Generate compilation script for kernel
 * @param kernel Generated kernel
 * @param base_dir Base directory
 * @param table_name Table name
 * @param page_number Page number (-1 for all pages)
 * @return 0 on success, -1 on error
 */
int generate_kernel_compile_script(const GeneratedKernel* kernel, const char* base_dir,
                                  const char* table_name, int page_number);

/**
 * @brief Check if kernel is already compiled
 * @param kernel_name Kernel name
 * @param base_dir Base directory
 * @param table_name Table name
 * @param page_number Page number (-1 for all pages)
 * @return true if compiled, false otherwise
 */
bool is_kernel_compiled(const char* kernel_name, const char* base_dir,
                       const char* table_name, int page_number);

#endif /* UMBRA_KERNEL_COMPILER_H */
