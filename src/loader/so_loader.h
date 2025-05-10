/**
 * @file so_loader.h
 * @brief Interface for shared object loading
 */

#ifndef UMBRA_SO_LOADER_H
#define UMBRA_SO_LOADER_H

#include <stdbool.h>

/**
 * @struct LoadedLibrary
 * @brief Represents a dynamically loaded shared object
 */
typedef struct {
    void* handle;               /**< Handle to the loaded library */
    char path[1024];            /**< Path to the library file */
    bool loaded;                /**< Whether the library is loaded */
} LoadedLibrary;

/**
 * @brief Load a shared object file
 * @param path Path to the shared object file
 * @param lib Output parameter for loaded library
 * @return 0 on success, -1 on error
 */
int load_library(const char* path, LoadedLibrary* lib);

/**
 * @brief Unload a previously loaded shared object
 * @param lib Library to unload
 * @return 0 on success, -1 on error
 */
int unload_library(LoadedLibrary* lib);

/**
 * @brief Get a function pointer from a loaded library
 * @param lib Loaded library
 * @param function_name Name of the function to get
 * @param function_ptr Output parameter for function pointer
 * @return 0 on success, -1 on error
 */
int get_function(const LoadedLibrary* lib, const char* function_name, void** function_ptr);

/**
 * @brief Check if a shared object file exists
 * @param path Path to check
 * @return true if file exists, false otherwise
 */
bool shared_object_exists(const char* path);

/**
 * @brief Get the full path to a table page shared object
 * @param base_dir Base directory for database
 * @param table_name Table name
 * @param page_number Page number
 * @param output Output buffer for path
 * @param output_size Size of output buffer
 * @return 0 on success, -1 on error
 */
int get_page_so_path(const char* base_dir, const char* table_name, int page_number, 
                    char* output, size_t output_size);

#endif /* UMBRA_SO_LOADER_H */
