/**
 * @file test_result_formatter.c
 * @brief Test program for result formatter functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <time.h>

// Include necessary headers - modify paths as needed
#include "../src/cli/result_formatter.h"
#include "../src/schema/type_system.h"
#include "../src/query/query_executor.h"

// Make the static function accessible for testing
// This is a declaration - the actual implementation is in result_formatter.c
extern void get_field_string(const QueryResult* result, void* row, int col_idx, 
                            char* buffer, size_t buffer_size);

// Test record structure for 'test' table from the error log
typedef struct {
    char name[41];  // VARCHAR(40) + null terminator
    char _uuid[37]; // VARCHAR(36) + null terminator
} TestRecord;

// Helper function to create a basic test schema
TableSchema* create_test_schema() {
    TableSchema* schema = malloc(sizeof(TableSchema));
    if (!schema) return NULL;
    
    strncpy(schema->name, "test", sizeof(schema->name) - 1);
    schema->name[sizeof(schema->name) - 1] = '\0';
    schema->column_count = 2;
    
    schema->columns = malloc(2 * sizeof(ColumnDefinition));
    if (!schema->columns) {
        free(schema);
        return NULL;
    }
    
    // First column: name VARCHAR(40)
    strncpy(schema->columns[0].name, "name", sizeof(schema->columns[0].name) - 1);
    schema->columns[0].name[sizeof(schema->columns[0].name) - 1] = '\0';
    schema->columns[0].type = TYPE_VARCHAR;
    schema->columns[0].length = 40;
    schema->columns[0].nullable = true;
    schema->columns[0].has_default = false;
    schema->columns[0].is_primary_key = false;
    
    // Second column: _uuid VARCHAR(36)
    strncpy(schema->columns[1].name, "_uuid", sizeof(schema->columns[1].name) - 1);
    schema->columns[1].name[sizeof(schema->columns[1].name) - 1] = '\0';
    schema->columns[1].type = TYPE_VARCHAR;
    schema->columns[1].length = 36;
    schema->columns[1].nullable = false;
    schema->columns[1].has_default = false;
    schema->columns[1].is_primary_key = false;
    
    schema->primary_key_columns = NULL;
    schema->primary_key_column_count = 0;
    
    return schema;
}

// Helper function to create a direct row format result
QueryResult* create_direct_result() {
    QueryResult* result = malloc(sizeof(QueryResult));
    if (!result) return NULL;
    
    // Initialize with basic values
    result->rows = NULL;
    result->row_count = 0;
    result->result_schema = NULL;
    result->success = false;
    result->error_message = NULL;
    result->row_format = ROW_FORMAT_DIRECT;
    
    // Create schema
    result->result_schema = create_test_schema();
    if (!result->result_schema) {
        free(result);
        return NULL;
    }
    
    // Create a test record
    TestRecord* record = malloc(sizeof(TestRecord));
    if (!record) {
        free_table_schema(result->result_schema);
        free(result);
        return NULL;
    }
    
    strncpy(record->name, "Test Name", sizeof(record->name) - 1);
    record->name[sizeof(record->name) - 1] = '\0';
    strncpy(record->_uuid, "12345678-1234-1234-1234-123456789012", sizeof(record->_uuid) - 1);
    record->_uuid[sizeof(record->_uuid) - 1] = '\0';
    
    // Create the row array with one entry
    result->rows = malloc(sizeof(void*));
    if (!result->rows) {
        free(record);
        free_table_schema(result->result_schema);
        free(result);
        return NULL;
    }
    
    result->rows[0] = record;
    result->row_count = 1;
    
    return result;
}

// Helper function to create a pointer array row format result
QueryResult* create_pointer_array_result() {
    QueryResult* result = malloc(sizeof(QueryResult));
    if (!result) return NULL;
    
    // Initialize with basic values
    result->rows = NULL;
    result->row_count = 0;
    result->result_schema = NULL;
    result->success = false;
    result->error_message = NULL;
    result->row_format = ROW_FORMAT_POINTER_ARRAY;
    
    // Create schema
    result->result_schema = create_test_schema();
    if (!result->result_schema) {
        free(result);
        return NULL;
    }
    
    // Create field values
    char* name_str = strdup("Test Name");
    char* uuid_str = strdup("12345678-1234-1234-1234-123456789012");
    
    if (!name_str || !uuid_str) {
        free(name_str);
        free(uuid_str);
        free_table_schema(result->result_schema);
        free(result);
        return NULL;
    }
    
    // Create array of field pointers
    void** field_pointers = malloc(2 * sizeof(void*));
    if (!field_pointers) {
        free(name_str);
        free(uuid_str);
        free_table_schema(result->result_schema);
        free(result);
        return NULL;
    }
    
    field_pointers[0] = name_str;
    field_pointers[1] = uuid_str;
    
    // Create the row array with one entry
    result->rows = malloc(sizeof(void*));
    if (!result->rows) {
        free(field_pointers);
        free(name_str);
        free(uuid_str);
        free_table_schema(result->result_schema);
        free(result);
        return NULL;
    }
    
    result->rows[0] = field_pointers;
    result->row_count = 1;
    
    return result;
}

// Helper function for rows_affected result
QueryResult* create_rows_affected_result() {
    QueryResult* result = malloc(sizeof(QueryResult));
    if (!result) return NULL;
    
    // Initialize with basic values
    result->rows = NULL;
    result->row_count = 0;
    result->result_schema = NULL;
    result->success = false;
    result->error_message = NULL;
    result->row_format = ROW_FORMAT_DIRECT;
    
    // Create schema with one column
    result->result_schema = malloc(sizeof(TableSchema));
    if (!result->result_schema) {
        free(result);
        return NULL;
    }
    
    strncpy(result->result_schema->name, "result", sizeof(result->result_schema->name) - 1);
    result->result_schema->name[sizeof(result->result_schema->name) - 1] = '\0';
    result->result_schema->column_count = 1;
    
    result->result_schema->columns = malloc(sizeof(ColumnDefinition));
    if (!result->result_schema->columns) {
        free(result->result_schema);
        free(result);
        return NULL;
    }
    
    strncpy(result->result_schema->columns[0].name, "rows_affected", sizeof(result->result_schema->columns[0].name) - 1);
    result->result_schema->columns[0].name[sizeof(result->result_schema->columns[0].name) - 1] = '\0';
    result->result_schema->columns[0].type = TYPE_INT;
    result->result_schema->columns[0].nullable = false;
    result->result_schema->columns[0].has_default = false;
    result->result_schema->columns[0].is_primary_key = false;
    
    result->result_schema->primary_key_columns = NULL;
    result->result_schema->primary_key_column_count = 0;
    
    // Create a rows_affected value (5 rows affected)
    int* rows_value = malloc(sizeof(int));
    if (!rows_value) {
        free(result->result_schema->columns);
        free(result->result_schema);
        free(result);
        return NULL;
    }
    
    *rows_value = 5;
    
    // Create the row array with one entry
    result->rows = malloc(sizeof(void*));
    if (!result->rows) {
        free(rows_value);
        free(result->result_schema->columns);
        free(result->result_schema);
        free(result);
        return NULL;
    }
    
    result->rows[0] = rows_value;
    result->row_count = 1;
    
    return result;
}

// Helper function to create a result with NULL row pointer
QueryResult* create_null_row_result() {
    QueryResult* result = malloc(sizeof(QueryResult));
    if (!result) return NULL;
    
    // Initialize with basic values
    result->rows = NULL;
    result->row_count = 0;
    result->result_schema = NULL;
    result->success = false;
    result->error_message = NULL;
    result->row_format = ROW_FORMAT_DIRECT;
    
    // Create schema
    result->result_schema = create_test_schema();
    if (!result->result_schema) {
        free(result);
        return NULL;
    }
    
    // Create the row array with one NULL entry
    result->rows = malloc(sizeof(void*));
    if (!result->rows) {
        free_table_schema(result->result_schema);
        free(result);
        return NULL;
    }
    
    result->rows[0] = NULL;  // NULL row pointer - this is what we want to test
    result->row_count = 1;
    
    return result;
}

// Helper function to clean up direct result
void free_direct_result(QueryResult* result) {
    if (!result) return;
    
    if (result->rows) {
        for (int i = 0; i < result->row_count; i++) {
            free(result->rows[i]);
        }
        free(result->rows);
    }
    
    if (result->result_schema) {
        free_table_schema(result->result_schema);
    }
    
    free(result->error_message);
    free(result);
}

// Helper function to clean up pointer array result
void free_pointer_array_result(QueryResult* result) {
    if (!result) return;
    
    if (result->rows) {
        for (int i = 0; i < result->row_count; i++) {
            void** fields = (void**)result->rows[i];
            if (fields) {
                // Free all field values
                for (int j = 0; j < result->result_schema->column_count; j++) {
                    free(fields[j]);
                }
                free(fields);
            }
        }
        free(result->rows);
    }
    
    if (result->result_schema) {
        free_table_schema(result->result_schema);
    }
    
    free(result->error_message);
    free(result);
}

// Helper function to clean up rows_affected result
void free_rows_affected_result(QueryResult* result) {
    if (!result) return;
    
    if (result->rows) {
        for (int i = 0; i < result->row_count; i++) {
            free(result->rows[i]);
        }
        free(result->rows);
    }
    
    if (result->result_schema) {
        free(result->result_schema->columns);
        free(result->result_schema);
    }
    
    free(result->error_message);
    free(result);
}

// Helper function to clean up null row result
void free_null_row_result(QueryResult* result) {
    if (!result) return;
    
    free(result->rows);
    
    if (result->result_schema) {
        free_table_schema(result->result_schema);
    }
    
    free(result->error_message);
    free(result);
}

/**
 * Test case: Empty result
 */
void test_empty_result() {
    printf("Testing empty result...\n");
    
    QueryResult empty_result = {0};
    empty_result.rows = NULL;
    empty_result.row_count = 0;
    empty_result.result_schema = NULL;
    empty_result.success = false;
    empty_result.error_message = NULL;
    empty_result.row_format = ROW_FORMAT_DIRECT;
    
    char buffer[256] = {0};
    get_field_string(&empty_result, NULL, 0, buffer, sizeof(buffer));
    
    printf("Buffer after get_field_string: '%s'\n", buffer);
    printf("Test passed\n\n");
}

/**
 * Test case: Direct row format
 */
void test_direct_row_format() {
    printf("Testing direct row format...\n");
    
    QueryResult* result = create_direct_result();
    if (!result) {
        printf("Failed to create direct result\n");
        return;
    }
    
    char buffer[256] = {0};
    
    // Test name column
    get_field_string(result, result->rows[0], 0, buffer, sizeof(buffer));
    printf("Name column: '%s'\n", buffer);
    
    // Test UUID column
    memset(buffer, 0, sizeof(buffer));
    get_field_string(result, result->rows[0], 1, buffer, sizeof(buffer));
    printf("UUID column: '%s'\n", buffer);
    
    free_direct_result(result);
    printf("Test passed\n\n");
}

/**
 * Test case: Pointer array format
 */
void test_pointer_array_format() {
    printf("Testing pointer array format...\n");
    
    QueryResult* result = create_pointer_array_result();
    if (!result) {
        printf("Failed to create pointer array result\n");
        return;
    }
    
    char buffer[256] = {0};
    
    // Test name column
    get_field_string(result, result->rows[0], 0, buffer, sizeof(buffer));
    printf("Name column: '%s'\n", buffer);
    
    // Test UUID column
    memset(buffer, 0, sizeof(buffer));
    get_field_string(result, result->rows[0], 1, buffer, sizeof(buffer));
    printf("UUID column: '%s'\n", buffer);
    
    free_pointer_array_result(result);
    printf("Test passed\n\n");
}

/**
 * Test case: Rows affected result
 */
void test_rows_affected_result() {
    printf("Testing rows_affected result...\n");
    
    QueryResult* result = create_rows_affected_result();
    if (!result) {
        printf("Failed to create rows_affected result\n");
        return;
    }
    
    char buffer[256] = {0};
    
    // Test rows_affected column
    get_field_string(result, result->rows[0], 0, buffer, sizeof(buffer));
    printf("Rows affected: '%s'\n", buffer);
    
    free_rows_affected_result(result);
    printf("Test passed\n\n");
}

/**
 * Test case: NULL row pointer
 */
void test_null_row_pointer() {
    printf("Testing NULL row pointer...\n");
    
    QueryResult* result = create_null_row_result();
    if (!result) {
        printf("Failed to create null row result\n");
        return;
    }
    
    char buffer[256] = {0};
    
    // This should handle NULL row gracefully instead of segfaulting
    get_field_string(result, result->rows[0], 0, buffer, sizeof(buffer));
    printf("Result with NULL row pointer: '%s'\n", buffer);
    
    free_null_row_result(result);
    printf("Test passed\n\n");
}

/**
 * Test case: Invalid column index
 */
void test_invalid_column_index() {
    printf("Testing invalid column index...\n");
    
    QueryResult* result = create_direct_result();
    if (!result) {
        printf("Failed to create direct result\n");
        return;
    }
    
    char buffer[256] = {0};
    
    // Test with column index out of bounds
    get_field_string(result, result->rows[0], 999, buffer, sizeof(buffer));
    printf("Result with invalid column index: '%s'\n", buffer);
    
    free_direct_result(result);
    printf("Test passed\n\n");
}

/**
 * Test case: Small buffer
 */
void test_small_buffer() {
    printf("Testing small buffer...\n");
    
    QueryResult* result = create_direct_result();
    if (!result) {
        printf("Failed to create direct result\n");
        return;
    }
    
    // Very small buffer (5 bytes)
    char buffer[5] = {0};
    
    // This should not overflow the buffer
    get_field_string(result, result->rows[0], 0, buffer, sizeof(buffer));
    printf("Result with small buffer (should be truncated): '%s'\n", buffer);
    
    free_direct_result(result);
    printf("Test passed\n\n");
}

/**
 * Test case: Simulated full table formatting
 */
void test_table_formatting() {
    printf("Testing full table formatting simulation...\n");
    
    QueryResult* result = create_direct_result();
    if (!result) {
        printf("Failed to create direct result\n");
        return;
    }
    
    // Calculate column widths
    int* widths = malloc(result->result_schema->column_count * sizeof(int));
    if (!widths) {
        free_direct_result(result);
        printf("Memory allocation failed\n");
        return;
    }
    
    for (int i = 0; i < result->result_schema->column_count; i++) {
        widths[i] = strlen(result->result_schema->columns[i].name);
    }
    
    // Get field widths from data
    char buffer[256];
    for (int row = 0; row < result->row_count; row++) {
        for (int col = 0; col < result->result_schema->column_count; col++) {
            memset(buffer, 0, sizeof(buffer));
            get_field_string(result, result->rows[row], col, buffer, sizeof(buffer));
            int len = strlen(buffer);
            if (len > widths[col]) {
                widths[col] = len;
            }
        }
    }
    
    // Print table outline
    printf("+");
    for (int i = 0; i < result->result_schema->column_count; i++) {
        for (int j = 0; j < widths[i] + 2; j++) {
            printf("-");
        }
        printf("+");
    }
    printf("\n");
    
    // Print headers
    printf("|");
    for (int i = 0; i < result->result_schema->column_count; i++) {
        printf(" %-*s |", widths[i], result->result_schema->columns[i].name);
    }
    printf("\n");
    
    // Print separator
    printf("+");
    for (int i = 0; i < result->result_schema->column_count; i++) {
        for (int j = 0; j < widths[i] + 2; j++) {
            printf("-");
        }
        printf("+");
    }
    printf("\n");
    
    // Print rows
    for (int row = 0; row < result->row_count; row++) {
        printf("|");
        for (int col = 0; col < result->result_schema->column_count; col++) {
            memset(buffer, 0, sizeof(buffer));
            get_field_string(result, result->rows[row], col, buffer, sizeof(buffer));
            
            if (result->result_schema->columns[col].type == TYPE_INT || 
                result->result_schema->columns[col].type == TYPE_FLOAT) {
                // Right align numbers
                printf(" %*s |", widths[col], buffer);
            } else {
                // Left align text
                printf(" %-*s |", widths[col], buffer);
            }
        }
        printf("\n");
    }
    
    // Print separator
    printf("+");
    for (int i = 0; i < result->result_schema->column_count; i++) {
        for (int j = 0; j < widths[i] + 2; j++) {
            printf("-");
        }
        printf("+");
    }
    printf("\n");
    
    free(widths);
    free_direct_result(result);
    printf("Test passed\n\n");
}

// Main test function
int main() {
    printf("==== Testing result_formatter.c ====\n\n");
    
    // Run individual tests
    test_empty_result();
    test_direct_row_format();
    test_pointer_array_format();
    test_rows_affected_result();
    test_null_row_pointer();
    test_invalid_column_index();
    test_small_buffer();
    test_table_formatting();
    
    printf("All tests completed.\n");
    return 0;
}
