/**
 * @file record_access.c
 * @brief Provides access to records from loaded pages
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include "record_access.h"
#include "error_handler.h"
#include "../schema/type_system.h"


/**
 * @brief Count the number of page files for a table
 */
static int count_page_files(const char* base_dir, const char* table_name) {
    if (!base_dir || !table_name) {
        return -1;
    }
    
    int count = 0;
    char compiled_dir[1024];
    snprintf(compiled_dir, sizeof(compiled_dir), "%s/compiled", base_dir);
    
    DIR* dir = opendir(compiled_dir);
    if (!dir) {
        set_error("Failed to open compiled directory: %s", compiled_dir);
        return -1;
    }
    
    // Count files that match the pattern {table}Data_{number}.so
    char prefix[256];
    snprintf(prefix, sizeof(prefix), "%sData_", table_name);
    size_t prefix_len = strlen(prefix);
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, prefix, prefix_len) == 0) {
            // Check if filename ends with .so
            size_t name_len = strlen(entry->d_name);
            if (name_len > 3 && strcmp(entry->d_name + name_len - 3, ".so") == 0) {
                count++;
            }
        }
    }
    
    closedir(dir);
    return count;
}

/**
 * @brief Initialize a table cursor
 */
int init_cursor(TableCursor* cursor, const char* base_dir, const char* table_name) {
    if (!cursor || !base_dir || !table_name) {
        set_error("Invalid parameters to init_cursor");
        return -1;
    }
    
    // Initialize cursor fields
    strncpy(cursor->base_dir, base_dir, sizeof(cursor->base_dir) - 1);
    cursor->base_dir[sizeof(cursor->base_dir) - 1] = '\0';
    
    strncpy(cursor->table_name, table_name, sizeof(cursor->table_name) - 1);
    cursor->table_name[sizeof(cursor->table_name) - 1] = '\0';
    
    cursor->current_page = 0;
    cursor->current_position = 0;
    cursor->initialized = false;
    cursor->at_end = false;
    
    // Count total pages
    cursor->total_pages = count_page_files(base_dir, table_name);
    if (cursor->total_pages < 0) {
        set_error("Failed to count page files");
        return -1;
    }
    
    if (cursor->total_pages == 0) {
        // Table has no pages, cursor is at the end
        cursor->at_end = true;
        cursor->initialized = true;
        return 0;
    }
    
    // Load first page
    if (load_page(base_dir, table_name, 0, &cursor->loaded_page) != 0) {
        set_error("Failed to load first page");
        return -1;
    }
    
    // Check if first page has records
    int page_count;
    if (get_page_count(&cursor->loaded_page, &page_count) != 0) {
        unload_page(&cursor->loaded_page);
        set_error("Failed to get page count");
        return -1;
    }
    
    if (page_count == 0) {
        // First page has no records, cursor is at the end
        cursor->at_end = true;
    }
    
    cursor->initialized = true;
    return 0;
}

/**
 * @brief Free resources used by a cursor
 */
int free_cursor(TableCursor* cursor) {
    if (!cursor || !cursor->initialized) {
        return 0;
    }
    
    // Unload current page if any
    if (cursor->loaded_page.valid) {
        if (unload_page(&cursor->loaded_page) != 0) {
            set_error("Failed to unload page");
            return -1;
        }
    }
    
    cursor->initialized = false;
    return 0;
}

/**
 * @brief Move cursor to next record
 */
int next_record(TableCursor* cursor) {
    if (!cursor || !cursor->initialized) {
        set_error("Cursor not initialized");
        return -1;
    }
    
    if (cursor->at_end) {
        return 1;
    }
    
    // Get current page record count
    int page_count;
    if (get_page_count(&cursor->loaded_page, &page_count) != 0) {
        set_error("Failed to get page count");
        return -1;
    }
    
    // Move to next position
    cursor->current_position++;
    
    // Check if reached end of current page
    if (cursor->current_position >= page_count) {
        // Try to move to next page
        cursor->current_page++;
        
        // Check if reached end of table
        if (cursor->current_page >= cursor->total_pages) {
            cursor->at_end = true;
            return 1;
        }
        
        // Unload current page
        if (unload_page(&cursor->loaded_page) != 0) {
            set_error("Failed to unload current page");
            return -1;
        }
        
        // Load next page
        if (load_page(cursor->base_dir, cursor->table_name, cursor->current_page, &cursor->loaded_page) != 0) {
            set_error("Failed to load next page");
            return -1;
        }
        
        // Reset position
        cursor->current_position = 0;
        
        // Check if new page has records
        if (get_page_count(&cursor->loaded_page, &page_count) != 0) {
            set_error("Failed to get page count");
            return -1;
        }
        
        if (page_count == 0) {
            // New page has no records, cursor is at the end
            cursor->at_end = true;
            return 1;
        }
    }
    
    return 0;
}

/**
 * @brief Reset cursor to beginning
 */
int reset_cursor(TableCursor* cursor) {
    if (!cursor || !cursor->initialized) {
        set_error("Cursor not initialized");
        return -1;
    }
    
    // Unload current page if any
    if (cursor->loaded_page.valid) {
        if (unload_page(&cursor->loaded_page) != 0) {
            set_error("Failed to unload page");
            return -1;
        }
    }
    
    // Reset position
    cursor->current_page = 0;
    cursor->current_position = 0;
    cursor->at_end = false;
    
    // Check if table has pages
    if (cursor->total_pages == 0) {
        cursor->at_end = true;
        return 0;
    }
    
    // Load first page
    if (load_page(cursor->base_dir, cursor->table_name, 0, &cursor->loaded_page) != 0) {
        set_error("Failed to load first page");
        return -1;
    }
    
    // Check if first page has records
    int page_count;
    if (get_page_count(&cursor->loaded_page, &page_count) != 0) {
        unload_page(&cursor->loaded_page);
        set_error("Failed to get page count");
        return -1;
    }
    
    if (page_count == 0) {
        // First page has no records, cursor is at the end
        cursor->at_end = true;
    }
    
    return 0;
}

/**
 * @brief Get current record from cursor
 */
int get_current_record(const TableCursor* cursor, void** record) {
    if (!cursor || !cursor->initialized || !record) {
        set_error("Invalid parameters to get_current_record");
        return -1;
    }
    
    if (cursor->at_end) {
        set_error("Cursor is at the end");
        return -1;
    }
    
    return read_record(&cursor->loaded_page, cursor->current_position, record);
}

/**
 * @brief Count total records in a table
 */
int count_table_records(const char* base_dir, const char* table_name, int* count) {
    if (!base_dir || !table_name || !count) {
        set_error("Invalid parameters to count_table_records");
        return -1;
    }
    
    *count = 0;
    
    // Count page files
    int page_count = count_page_files(base_dir, table_name);
    if (page_count < 0) {
        set_error("Failed to count page files");
        return -1;
    }
    
    if (page_count == 0) {
        return 0;
    }
    
    // Load each page and sum record counts
    LoadedPage page;
    
    for (int i = 0; i < page_count; i++) {
        if (load_page(base_dir, table_name, i, &page) != 0) {
            set_error("Failed to load page %d", i);
            return -1;
        }
        
        int page_records;
        if (get_page_count(&page, &page_records) != 0) {
            unload_page(&page);
            set_error("Failed to get page count");
            return -1;
        }
        
        *count += page_records;
        
        if (unload_page(&page) != 0) {
            set_error("Failed to unload page");
            return -1;
        }
    }
    
    return 0;
}

/**
 * @brief Find UUID column index in schema
 */
int find_uuid_column_index(const TableSchema* schema) {
    // Look for the _uuid column
    for (int i = 0; i < schema->column_count; i++) {
        if (strcmp(schema->columns[i].name, "_uuid") == 0) {
            return i;
        }
    }
    
    // If not found, return -1
    return -1;
}

/**
 * @brief Get field pointer by index - implementation depends on how records are structured
 */
void* get_field_by_index(void* record, const TableSchema* schema, int field_idx) {
    if (!record || !schema || field_idx < 0 || field_idx >= schema->column_count) {
        return NULL;
    }
    
    // This implementation assumes a specific memory layout
    // In a production system, this would be generated based on the actual schema
    // or use a more sophisticated approach
    
    char* record_bytes = (char*)record;
    size_t offset = 0;
    
    // Calculate offset to the desired field, accounting for type sizes and alignment
    for (int i = 0; i < field_idx; i++) {
        const ColumnDefinition* col = &schema->columns[i];
        size_t field_size = 0;
        
        switch (col->type) {
            case TYPE_INT:
                // Align to 4-byte boundary
                offset = (offset + 3) & ~3;
                field_size = sizeof(int);
                break;
            case TYPE_FLOAT:
                // Align to 8-byte boundary
                offset = (offset + 7) & ~7;
                field_size = sizeof(double);
                break;
            case TYPE_BOOLEAN:
                field_size = sizeof(bool);
                break;
            case TYPE_DATE:
                // Align to 8-byte boundary
                offset = (offset + 7) & ~7;
                field_size = sizeof(time_t);
                break;
            case TYPE_VARCHAR:
                field_size = col->length + 1;  // Include null terminator
                break;
            case TYPE_TEXT:
                field_size = 4096;  // Fixed size for TEXT
                break;
            default:
                return NULL;
        }
        
        offset += field_size;
    }
    
    // Return pointer to the field
    return record_bytes + offset;
}

/**
 * @brief Get field pointer by name
 */
void* get_field_from_record(void* record, const TableSchema* schema, const char* field_name) {
    if (!record || !schema || !field_name) {
        return NULL;
    }
    
    // Find field index in schema
    int field_idx = -1;
    for (int i = 0; i < schema->column_count; i++) {
        if (strcmp(schema->columns[i].name, field_name) == 0) {
            field_idx = i;
            break;
        }
    }
    
    if (field_idx < 0) {
        return NULL;
    }
    
    return get_field_by_index(record, schema, field_idx);
}

/**
 * @brief Get UUID string from record
 */
char* get_uuid_from_record(void* record, const TableSchema* schema) {
    int uuid_idx = find_uuid_column_index(schema);
    if (uuid_idx < 0) {
        return NULL;
    }
    
    char* uuid_ptr = (char*)get_field_by_index(record, schema, uuid_idx);
    if (!uuid_ptr || !*uuid_ptr) {
        return NULL;
    }
    
    return strdup(uuid_ptr);
}
