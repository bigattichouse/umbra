/**
 * @file kernel_loader.c
 * @brief Loads and executes compiled kernels
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "kernel_loader.h"

/**
 * @brief Function pointer type for kernel functions
 */
typedef int (*KernelFunction)(void* data, int count, void* results, int max_results);

/**
 * @brief Load a compiled kernel
 */
int load_kernel(const char* kernel_path, const char* kernel_name,
                const char* table_name, int page_number,
                LoadedKernel* loaded_kernel) {
    if (!kernel_path || !kernel_name || !table_name || !loaded_kernel) {
        return -1;
    }
    
    // Initialize kernel structure
    loaded_kernel->loaded = false;
    loaded_kernel->kernel_function = NULL;
    loaded_kernel->page_number = page_number;
    
    strncpy(loaded_kernel->kernel_name, kernel_name, sizeof(loaded_kernel->kernel_name) - 1);
    loaded_kernel->kernel_name[sizeof(loaded_kernel->kernel_name) - 1] = '\0';
    
    strncpy(loaded_kernel->table_name, table_name, sizeof(loaded_kernel->table_name) - 1);
    loaded_kernel->table_name[sizeof(loaded_kernel->table_name) - 1] = '\0';
    
    // Load the shared object
    if (load_library(kernel_path, &loaded_kernel->library) != 0) {
        fprintf(stderr, "Failed to load kernel library: %s\n", kernel_path);
        return -1;
    }
    
    // Get the kernel function
    if (get_function(&loaded_kernel->library, kernel_name, &loaded_kernel->kernel_function) != 0) {
        fprintf(stderr, "Failed to get kernel function: %s\n", kernel_name);
        unload_library(&loaded_kernel->library);
        return -1;
    }
    
    loaded_kernel->loaded = true;
    return 0;
}

/**
 * @brief Unload a kernel
 */
int unload_kernel(LoadedKernel* kernel) {
    if (!kernel || !kernel->loaded) {
        return 0;
    }
    
    int result = unload_library(&kernel->library);
    if (result != 0) {
        fprintf(stderr, "Failed to unload kernel library\n");
        return -1;
    }
    
    kernel->loaded = false;
    kernel->kernel_function = NULL;
    return 0;
}

/**
 * @brief Execute a loaded kernel
 */
int execute_kernel(const LoadedKernel* kernel, void* data, int data_count,
                   void* results, int max_results) {
    if (!kernel || !kernel->loaded || !kernel->kernel_function) {
        fprintf(stderr, "Kernel not loaded\n");
        return -1;
    }
    
    if (!data || data_count <= 0 || !results || max_results <= 0) {
        fprintf(stderr, "Invalid parameters to execute_kernel\n");
        return -1;
    }
    
    // Cast to function pointer and execute
    KernelFunction fn = (KernelFunction)kernel->kernel_function;
    return fn(data, data_count, results, max_results);
}
