/**
 * @file so_loader.c
 * @brief Loads shared objects dynamically
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <errno.h>
#include "so_loader.h"

/**
 * @brief Load a shared object file
 */
int load_library(const char* path, LoadedLibrary* lib) {
    if (!path || !lib) {
        return -1;
    }
    
    // Check if library is already loaded
    if (lib->loaded && strcmp(lib->path, path) == 0) {
        // Already loaded, nothing to do
        return 0;
    }
    
    // Unload previous library if any
    if (lib->loaded) {
        if (unload_library(lib) != 0) {
            return -1;
        }
    }
    
    // Load the shared object
    lib->handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!lib->handle) {
        fprintf(stderr, "Failed to load library %s: %s\n", path, dlerror());
        return -1;
    }
    
    // Save library path
    strncpy(lib->path, path, sizeof(lib->path) - 1);
    lib->path[sizeof(lib->path) - 1] = '\0';
    
    lib->loaded = true;
    return 0;
}

/**
 * @brief Unload a previously loaded shared object
 */
int unload_library(LoadedLibrary* lib) {
    if (!lib || !lib->loaded) {
        return 0;
    }
    
    if (dlclose(lib->handle) != 0) {
        fprintf(stderr, "Failed to unload library: %s\n", dlerror());
        return -1;
    }
    
    lib->handle = NULL;
    lib->path[0] = '\0';
    lib->loaded = false;
    return 0;
}

/**
 * @brief Get a function pointer from a loaded library
 */
int get_function(const LoadedLibrary* lib, const char* function_name, void** function_ptr) {
    if (!lib || !lib->loaded || !function_name || !function_ptr) {
        return -1;
    }
    
    // Clear any existing error
    dlerror();
    
    // Get the function pointer
    *function_ptr = dlsym(lib->handle, function_name);
    
    // Check for errors
    const char* error = dlerror();
    if (error) {
        fprintf(stderr, "Failed to get function %s: %s\n", function_name, error);
        return -1;
    }
    
    return 0;
}

/**
 * @brief Check if a shared object file exists
 */
bool shared_object_exists(const char* path) {
    if (!path) {
        return false;
    }
    
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISREG(st.st_mode);
    }
    
    return false;
}

/**
 * @brief Get the full path to a table page shared object
 */
int get_page_so_path(const char* base_dir, const char* table_name, int page_number, 
                    char* output, size_t output_size) {
    if (!base_dir || !table_name || !output || output_size <= 0) {
        return -1;
    }
    
    int written = snprintf(output, output_size, "%s/compiled/%sData_%d.so", 
                          base_dir, table_name, page_number);
    
    if (written < 0 || (size_t)written >= output_size) {
        return -1;
    }
    
    return 0;
}
