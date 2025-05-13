/**
 * @file record_access.h
 * @brief Interface for record access
 */

#ifndef UMBRA_RECORD_ACCESS_H
#define UMBRA_RECORD_ACCESS_H

#include "page_manager.h"
#include "../schema/schema_parser.h"

extern const int _UUID_COLUMN_INDEX;

/**
 * @struct TableCursor
 * @brief Cursor for iterating through table records
 */
typedef struct {
    char base_dir[1024];         /**< Base directory for database */
    char table_name[64];         /**< Table name */
    int current_page;            /**< Current page number */
    int current_position;        /**< Current position in page */
    LoadedPage loaded_page;      /**< Currently loaded page */
    int total_pages;             /**< Total number of pages */
    bool initialized;            /**< Whether cursor is initialized */
    bool at_end;                 /**< Whether cursor is at the end */
} TableCursor;

/**
 * @brief Initialize a table cursor
 * @param cursor Cursor to initialize
 * @param base_dir Base directory for database
 * @param table_name Table name
 * @return 0 on success, -1 on error
 */
int init_cursor(TableCursor* cursor, const char* base_dir, const char* table_name);

/**
 * @brief Free resources used by a cursor
 * @param cursor Cursor to free
 * @return 0 on success, -1 on error
 */
int free_cursor(TableCursor* cursor);

/**
 * @brief Move cursor to next record
 * @param cursor Cursor to move
 * @return 0 on success, -1 on error, 1 if at end
 */
int next_record(TableCursor* cursor);

/**
 * @brief Reset cursor to beginning
 * @param cursor Cursor to reset
 * @return 0 on success, -1 on error
 */
int reset_cursor(TableCursor* cursor);

/**
 * @brief Get current record from cursor
 * @param cursor Cursor to get record from
 * @param record Output parameter for record pointer
 * @return 0 on success, -1 on error
 */
int get_current_record(const TableCursor* cursor, void** record);

/**
 * @brief Count total records in a table
 * @param base_dir Base directory for database
 * @param table_name Table name
 * @param count Output parameter for record count
 * @return 0 on success, -1 on error
 */
int count_table_records(const char* base_dir, const char* table_name, int* count);


/**
 * @brief Get field pointer from a record by field name
 * @param record Pointer to record
 * @param schema Table schema
 * @param field_name Field name to retrieve
 * @return Pointer to field value or NULL if not found
 */
void* get_field_from_record(void* record, const TableSchema* schema, const char* field_name);

/**
 * @brief Get field pointer from a record by field index
 * @param record Pointer to record
 * @param schema Table schema
 * @param field_idx Field index to retrieve
 * @return Pointer to field value or NULL if index invalid
 */
void* get_field_by_index(void* record, const TableSchema* schema, int field_idx);

/**
 * @brief Find UUID column index in schema
 * @param schema Table schema
 * @return Index of UUID column or -1 if not found
 */
int find_uuid_column_index(const TableSchema* schema);

/**
 * @brief Get UUID string from record
 * @param record Pointer to record
 * @param schema Table schema
 * @return Duplicated UUID string (caller must free) or NULL if not found
 */
char* get_uuid_from_record(void* record, const TableSchema* schema);

#endif /* UMBRA_RECORD_ACCESS_H */
