/**
 * @file page_manager.h
 * @brief Interface for page management
 */

#ifndef UMBRA_PAGE_MANAGER_H
#define UMBRA_PAGE_MANAGER_H

#include "so_loader.h"
#include "../schema/schema_parser.h"

/**
 * @struct LoadedPage
 * @brief Represents a loaded data page
 */
typedef struct {
    LoadedLibrary library;      /**< Loaded shared object */
    char table_name[64];        /**< Table name */
    int page_number;            /**< Page number */
    int (*count)(void);         /**< Function pointer to count() */
    void* (*read)(int);         /**< Function pointer to read() */
    bool valid;                 /**< Whether the page is valid */
} LoadedPage;

/**
 * @brief Load a data page
 * @param base_dir Base directory for database
 * @param table_name Table name
 * @param page_number Page number
 * @param page Output parameter for loaded page
 * @return 0 on success, -1 on error
 */
int load_page(const char* base_dir, const char* table_name, int page_number, LoadedPage* page);

/**
 * @brief Unload a previously loaded page
 * @param page Page to unload
 * @return 0 on success, -1 on error
 */
int unload_page(LoadedPage* page);

/**
 * @brief Get the count of records in a loaded page
 * @param page Loaded page
 * @param count Output parameter for record count
 * @return 0 on success, -1 on error
 */
int get_page_count(const LoadedPage* page, int* count);

/**
 * @brief Read a record from a loaded page
 * @param page Loaded page
 * @param pos Position of the record
 * @param record Output parameter for record pointer
 * @return 0 on success, -1 on error
 */
int read_record(const LoadedPage* page, int pos, void** record);

/**
 * @brief Check if a page exists
 * @param base_dir Base directory for database
 * @param table_name Table name
 * @param page_number Page number
 * @return true if page exists, false otherwise
 */
bool page_exists(const char* base_dir, const char* table_name, int page_number);

#endif /* UMBRA_PAGE_MANAGER_H */
