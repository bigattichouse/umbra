/**
 * @file kernel_loader.h
 * @brief Interface for loading compiled kernels
 */

#ifndef UMBRA_KERNEL_LOADER_H
#define UMBRA_KERNEL_LOADER_H

#include "../loader/so_loader.h"
#include "../schema/schema_parser.h"

/**
 * @struct LoadedKernel
 * @brief Represents a loaded query kernel
 */
typedef struct {
    LoadedLibrary library;      /**< Loaded shared object */
    char kernel_name[256];      /**< Kernel name */
    char table_name[64];        /**< Table name */
    int page_number;            /**< Page number (-1 for all pages) */
    void* kernel_function;      /**< Pointer to kernel function */
    bool loaded;                /**< Whether kernel is loaded */
} LoadedKernel;

/**
 * @brief Load a compiled kernel
 * @param kernel_path Path to kernel .so file
 * @param kernel_name Name of the kernel function
 * @param table_name Table name
 * @param page_number Page number
 * @param loaded_kernel Output kernel structure
 * @return 0 on success, -1 on error
 */
int load_kernel(const char* kernel_path, const char* kernel_name,
                const char* table_name, int page_number,
                LoadedKernel* loaded_kernel);

/**
 * @brief Unload a kernel
 * @param kernel Kernel to unload
 * @return 0 on success, -1 on error
 */
int unload_kernel(LoadedKernel* kernel);

/**
 * @brief Execute a loaded kernel
 * @param kernel Loaded kernel
 * @param data Input data array
 * @param data_count Number of input records
 * @param results Output results buffer
 * @param max_results Maximum number of results
 * @return Number of results or -1 on error
 */
int execute_kernel(const LoadedKernel* kernel, void* data, int data_count,
                   void* results, int max_results);

#endif /* UMBRA_KERNEL_LOADER_H */
