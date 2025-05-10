/**
 * @file page_manager.c
 * @brief Manages loaded pages and caching
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "page_manager.h"
#include "error_handler.h"

/**
 * @brief Initialize a LoadedPage structure
 */
static void initialize_loaded_page(LoadedPage* page) {
    if (!page) {
        return;
    }
    
    memset(&page->library, 0, sizeof(LoadedLibrary));
    page->table_name[0] = '\0';
    page->page_number = -1;
    page->count = NULL;
    page->read = NULL;
    page->valid = false;
}

/**
 * @brief Load a data page
 */
int load_page(const char* base_dir, const char* table_name, int page_number, LoadedPage* page) {
    if (!base_dir || !table_name || !page) {
        set_error("Invalid parameters to load_page");
        return -1;
    }
    
    // Initialize page structure
    initialize_loaded_page(page);
    
    // Get shared object path
    char so_path[1024];
    if (get_page_so_path(base_dir, table_name, page_number, so_path, sizeof(so_path)) != 0) {
        set_error("Failed to get shared object path");
        return -1;
    }
    
    // Check if shared object exists
    if (!shared_object_exists(so_path)) {
        set_error("Shared object does not exist: %s", so_path);
        return -1;
    }
    
    // Load the shared object
    if (load_library(so_path, &page->library) != 0) {
        set_error("Failed to load library: %s", so_path);
        return -1;
    }
    
    // Get function pointers
    void* count_ptr = NULL;
    if (get_function(&page->library, "count", &count_ptr) != 0) {
        set_error("Failed to get count function");
        unload_library(&page->library);
        return -1;
    }
    
    void* read_ptr = NULL;
    if (get_function(&page->library, "read", &read_ptr) != 0) {
        set_error("Failed to get read function");
        unload_library(&page->library);
        return -1;
    }
    
    // Set page information
    strncpy(page->table_name, table_name, sizeof(page->table_name) - 1);
    page->table_name[sizeof(page->table_name) - 1] = '\0';
    
    page->page_number = page_number;
    page->count = (int (*)(void))count_ptr;
    page->read = (void* (*)(int))read_ptr;
    page->valid = true;
    
    return 0;
}

/**
 * @brief Unload a previously loaded page
 */
int unload_page(LoadedPage* page) {
    if (!page || !page->valid) {
        return 0;
    }
    
    int result = unload_library(&page->library);
    if (result != 0) {
        set_error("Failed to unload page library");
        return -1;
    }
    
    initialize_loaded_page(page);
    return 0;
}

/**
 * @brief Get the count of records in a loaded page
 */
int get_page_count(const LoadedPage* page, int* count) {
    if (!page || !page->valid || !count) {
        set_error("Invalid parameters to get_page_count");
        return -1;
    }
    
    if (!page->count) {
        set_error("Count function not loaded");
        return -1;
    }
    
    *count = page->count();
    return 0;
}

/**
 * @brief Read a record from a loaded page
 */
int read_record(const LoadedPage* page, int pos, void** record) {
    if (!page || !page->valid || !record) {
        set_error("Invalid parameters to read_record");
        return -1;
    }
    
    if (!page->read) {
        set_error("Read function not loaded");
        return -1;
    }
    
    *record = page->read(pos);
    if (!*record) {
        set_error("Record position out of bounds: %d", pos);
        return -1;
    }
    
    return 0;
}

/**
 * @brief Check if a page exists
 */
bool page_exists(const char* base_dir, const char* table_name, int page_number) {
    if (!base_dir || !table_name) {
        return false;
    }
    
    char so_path[1024];
    if (get_page_so_path(base_dir, table_name, page_number, so_path, sizeof(so_path)) != 0) {
        return false;
    }
    
    return shared_object_exists(so_path);
}
