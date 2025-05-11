/**
 * @file tableScan.h
 * @brief Interface for scanning table data across pages
 */

#ifndef UMBRA_TABLE_SCAN_H
#define UMBRA_TABLE_SCAN_H

#include <stdbool.h>
#include "src/loader/page_manager.h"
#include "src/loader/record_access.h"
#include "src/schema/schema_parser.h"
#include "src/parser/ast.h"

/**
 * @struct TableScan
 * @brief Represents a scan operation over a table
 */
typedef struct {
    char base_dir[1024];         /**< Base directory for database */
    char table_name[64];         /**< Table name */
    TableSchema* schema;         /**< Table schema */
    TableCursor cursor;          /**< Table cursor for iterating through records */
    Expression* filter;          /**< Filter expression (WHERE clause) */
    int* projected_columns;      /**< Indices of columns to project */
    int projection_count;        /**< Number of projected columns */
    void* current_record;        /**< Pointer to current record */
    bool initialized;            /**< Whether scan is initialized */
    bool at_end;                 /**< Whether scan is at the end */
} TableScan;

/**
 * @brief Initialize a table scan
 * @param scan Scan to initialize
 * @param base_dir Base directory for database
 * @param table_name Table name
 * @param filter Filter expression (WHERE clause, can be NULL)
 * @param projected_columns Indices of columns to project (NULL for all)
 * @param projection_count Number of projected columns
 * @return 0 on success, -1 on error
 */
int init_table_scan(TableScan* scan, const char* base_dir, const char* table_name,
                   Expression* filter, const int* projected_columns, int projection_count);

/**
 * @brief Free resources used by a table scan
 * @param scan Scan to free
 * @return 0 on success, -1 on error
 */
int free_table_scan(TableScan* scan);

/**
 * @brief Reset a table scan to the beginning
 * @param scan Scan to reset
 * @return 0 on success, -1 on error
 */
int reset_table_scan(TableScan* scan);

/**
 * @brief Move to the next record that matches filter
 * @param scan Scan to move
 * @return 0 on success, -1 on error, 1 if at end
 */
int next_matching_record(TableScan* scan);

/**
 * @brief Get the current record
 * @param scan Scan to get record from
 * @param record Output parameter for record pointer
 * @return 0 on success, -1 on error
 */
int get_current_record(const TableScan* scan, void** record);

/**
 * @brief Get a projected view of the current record
 * @param scan Scan to get projection from
 * @param record Output parameter for projected record
 * @return 0 on success, -1 on error
 */
int get_projected_record(const TableScan* scan, void* record);

/**
 * @brief Count records matching filter
 * @param scan Scan to count records in
 * @param count Output parameter for record count
 * @return 0 on success, -1 on error
 */
int count_matching_records(TableScan* scan, int* count);

/**
 * @brief Apply filter to record
 * @param scan Scan with filter
 * @param record Record to check
 * @return true if record matches filter, false otherwise
 */
bool apply_filter(const TableScan* scan, const void* record);

/**
 * @brief Create kernel for a table scan
 * @param scan Scan to create kernel for
 * @param kernel_name Output buffer for kernel name
 * @param kernel_name_size Size of kernel name buffer
 * @return 0 on success, -1 on error
 */
int create_scan_kernel(const TableScan* scan, char* kernel_name, size_t kernel_name_size);

/**
 * @brief Execute a table scan with a compiled kernel
 * @param scan Scan to execute
 * @param results Output buffer for results
 * @param max_results Maximum number of results
 * @param result_count Output parameter for result count
 * @return 0 on success, -1 on error
 */
int execute_kernel_scan(const TableScan* scan, void* results, int max_results, int* result_count);

/**
 * @brief Apply projection to record
 * @param scan Scan with projection
 * @param source Source record
 * @param destination Destination buffer for projected record
 * @return 0 on success, -1 on error
 */
int apply_projection(const TableScan* scan, const void* source, void* destination);

/**
 * @brief Check if record matches filter without compilation
 * @param scan Scan with filter
 * @param record Record to check
 * @return true if record matches filter, false otherwise
 */
bool evaluate_filter(const TableScan* scan, const void* record);

/**
 * @brief Calculate memory requirements for projected record
 * @param scan Scan with projection information
 * @param size Output parameter for required memory size
 * @return 0 on success, -1 on error
 */
int get_projected_record_size(const TableScan* scan, size_t* size);

#endif /* UMBRA_TABLE_SCAN_H */
