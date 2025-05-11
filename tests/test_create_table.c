/**
 * @file test_create_table.c
 * @brief Test program for creating a table with sample data
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "src/schema/schema_parser.h"
#include "src/schema/schema_generator.h"
#include "src/schema/directory_manager.h"
#include "src/pages/page_generator.h"
#include "src/pages/page_splitter.h"
#include "src/loader/page_manager.h"
#include "src/loader/record_access.h"
#include "src/query/query_executor.h"  // Add this for load_table_schema

// Test data
#define TEST_DB_DIR "./test_db"
#define TEST_TABLE_NAME "Customers"
#define MAX_RECORDS_PER_PAGE 5

// Debug macro
#ifdef DEBUG
#define DEBUG_PRINT(...) fprintf(stderr, "[TEST DEBUG] " __VA_ARGS__)
#else
#define DEBUG_PRINT(...) do {} while(0)
#endif

// Function declarations
int create_test_table(void);
int add_test_data(void);  // Keep as void parameter to match usage
int test_data_access(void);

// Helper function prototype
static int add_test_data_to_table(const char* table_name, const char* base_dir);

int main(void) {
    printf("Umbra Test: Creating table with sample data\n");
    DEBUG_PRINT("Starting test\n");
    
    // Create test table
    if (create_test_table() != 0) {
        fprintf(stderr, "Failed to create test table\n");
        return 1;
    }
    
    // Add test data
    if (add_test_data() != 0) {
        fprintf(stderr, "Failed to add test data\n");
        return 1;
    }
    
    // Test data access
    if (test_data_access() != 0) {
        fprintf(stderr, "Failed to access test data\n");
        return 1;
    }
    
    printf("Test completed successfully\n");
    return 0;
}

/**
 * @brief Create a test customer table
 */
int create_test_table(void) {
    DEBUG_PRINT("create_test_table: start\n");
    
    // Define CREATE TABLE statement
    const char* create_statement = 
        "CREATE TABLE Customers ("
        "  id INT PRIMARY KEY,"
        "  name VARCHAR(100) NOT NULL,"
        "  email VARCHAR(100),"
        "  age INT,"
        "  active BOOLEAN DEFAULT true"
        ")";
    
    printf("Parsing SQL statement:\n%s\n", create_statement);
    DEBUG_PRINT("About to call parse_create_table\n");
    
    // Parse the statement
    TableSchema* schema = parse_create_table(create_statement);
    DEBUG_PRINT("parse_create_table returned %p\n", (void*)schema);
    
    if (!schema) {
        fprintf(stderr, "Failed to parse CREATE TABLE statement\n");
        return 1;
    }
    
    DEBUG_PRINT("Schema parsed, validating...\n");
    // Validate the schema
    if (!validate_schema(schema)) {
        fprintf(stderr, "Invalid schema\n");
        free_table_schema(schema);
        return 1;
    }
    
    printf("Schema parsed successfully. Table: %s, Columns: %d\n", 
           schema->name, schema->column_count);
    
    DEBUG_PRINT("Initializing database directory: %s\n", TEST_DB_DIR);
    // Initialize database directory
    if (initialize_database_directory(TEST_DB_DIR) != 0) {
        fprintf(stderr, "Failed to initialize database directory\n");
        free_table_schema(schema);
        return 1;
    }
    
    DEBUG_PRINT("Creating table directory structure\n");
    // Create table directory structure
    if (create_table_directory(schema->name, TEST_DB_DIR) != 0) {
        fprintf(stderr, "Failed to create table directory\n");
        free_table_schema(schema);
        return 1;
    }
    
    // Get the table directory path to place the header file correctly
    char table_dir[1024];
    if (get_table_directory(schema->name, TEST_DB_DIR, table_dir, sizeof(table_dir)) != 0) {
        fprintf(stderr, "Failed to get table directory\n");
        free_table_schema(schema);
        return 1;
    }
    
    DEBUG_PRINT("Generating header file in table directory: %s\n", table_dir);
    // Generate struct header file in the table directory
    if (generate_header_file(schema, table_dir) != 0) {
        fprintf(stderr, "Failed to generate header file\n");
        free_table_schema(schema);
        return 1;
    }
    
    DEBUG_PRINT("Generating initial data page\n");
    // Generate initial data page
    if (generate_data_page(schema, TEST_DB_DIR, 0) != 0) {
        fprintf(stderr, "Failed to generate initial data page\n");
        free_table_schema(schema);
        return 1;
    }
    
    printf("Created table structure for %s\n", schema->name);
    
    // Clean up
    DEBUG_PRINT("Freeing schema\n");
    free_table_schema(schema);
    DEBUG_PRINT("create_test_table: done\n");
    return 0;
}

/**
 * @brief Add test data to the default test table
 */
int add_test_data(void) {
    DEBUG_PRINT("add_test_data: start\n");
    int result = add_test_data_to_table(TEST_TABLE_NAME, TEST_DB_DIR);
    DEBUG_PRINT("add_test_data: %s\n", result == 0 ? "success" : "failure");
    return result;
}

/**
 * @brief Add test data to a specific table
 */
static int add_test_data_to_table(const char* table_name, const char* base_dir) {
    // Load schema
    TableSchema* schema = load_table_schema(table_name, base_dir);
    if (!schema) {
        fprintf(stderr, "Failed to load schema for table: %s\n", table_name);
        return -1;
    }
    
    printf("Adding test data to table: %s\n", table_name);
    
    // Allocate space for customer data on heap instead of stack
    // Allocate more than necessary to ensure we have enough space
    char** customer_data = malloc(schema->column_count * sizeof(char*));
    if (!customer_data) {
        fprintf(stderr, "Memory allocation failed\n");
        free_table_schema(schema);
        return -1;
    }
    
    // Initialize all to NULL
    for (int i = 0; i < schema->column_count; i++) {
        customer_data[i] = NULL;
    }
    
    // Generate some sample data
    const char* names[] = {
        "John Doe", "Jane Smith", "Bob Johnson", "Alice Brown", "Charlie Davis",
        "Eva Wilson", "Frank Miller", "Grace Taylor", "Henry Lewis", "Ivy Clark"
    };
    
    const char* emails[] = {
        "john@example.com", "jane@example.com", "bob@example.com", 
        "alice@example.com", "charlie@example.com", "eva@example.com",
        "frank@example.com", "grace@example.com", "henry@example.com",
        "ivy@example.com"
    };
    
    int page = 0;
    bool is_full = false;
    
    // Insert 10 customers
    for (int i = 0; i < 10; i++) {
        printf("Adding customer %d: %s\n", i + 1, names[i]);
        fprintf(stderr, "[TEST DEBUG] Adding customer %d\n", i);
        
        // Set values for each column
        for (int j = 0; j < schema->column_count; j++) {
            const ColumnDefinition* col = &schema->columns[j];
            
            if (customer_data[j]) {
                free(customer_data[j]);
                customer_data[j] = NULL;
            }
            
            if (strcmp(col->name, "id") == 0) {
                customer_data[j] = malloc(16);
                snprintf(customer_data[j], 16, "%d", i + 1);
            } else if (strcmp(col->name, "name") == 0) {
                customer_data[j] = strdup(names[i]);
            } else if (strcmp(col->name, "email") == 0) {
                customer_data[j] = strdup(emails[i]);
            } else if (strcmp(col->name, "age") == 0) {
                customer_data[j] = malloc(16);
                snprintf(customer_data[j], 16, "%d", 25 + i);
            } else if (strcmp(col->name, "active") == 0) {
                customer_data[j] = strdup(i % 2 == 0 ? "true" : "false");
            } else if (strcmp(col->name, "_uuid") == 0) {
                // Generate a fake UUID for testing
                customer_data[j] = malloc(37);
                snprintf(customer_data[j], 37, "00000000-0000-0000-0000-%012d", i + 1);
            } else {
                // For other columns, use NULL
                customer_data[j] = strdup("NULL");
            }
        }
        
        // Check if page is full before adding
        if (is_page_full(schema, base_dir, page, 5, &is_full) == 0) {
            if (is_full) {
                page++;
                printf("Creating new page: %d\n", page);
                
                // Generate new page
                if (generate_data_page(schema, base_dir, page) != 0) {
                    fprintf(stderr, "Failed to generate new page\n");
                    
                    // Clean up
                    for (int j = 0; j < schema->column_count; j++) {
                        free(customer_data[j]);
                    }
                    free(customer_data);
                    free_table_schema(schema);
                    return -1;
                }
            }
        }
        
        // Add record to page
        if (add_record_to_page(schema, base_dir, page, (const char**)customer_data) != 0) {
            fprintf(stderr, "Failed to add record to page\n");
            
            // Clean up
            for (int j = 0; j < schema->column_count; j++) {
                free(customer_data[j]);
            }
            free(customer_data);
            free_table_schema(schema);
            return -1;
        }
        
        // Recompile the page
        if (recompile_data_page(schema, base_dir, page) != 0) {
            fprintf(stderr, "Failed to recompile page\n");
            
            // Clean up
            for (int j = 0; j < schema->column_count; j++) {
                free(customer_data[j]);
            }
            free(customer_data);
            free_table_schema(schema);
            return -1;
        }
    }
    
    // Clean up
    for (int j = 0; j < schema->column_count; j++) {
        free(customer_data[j]);
    }
    free(customer_data);
    free_table_schema(schema);
    return 0;
}

/**
 * @brief Test accessing the customer data
 */
int test_data_access(void) {
    DEBUG_PRINT("test_data_access: start\n");
    printf("Testing data access\n");
    
    // Initialize cursor
    TableCursor cursor;
    if (init_cursor(&cursor, TEST_DB_DIR, TEST_TABLE_NAME) != 0) {
        fprintf(stderr, "Failed to initialize cursor\n");
        return 1;
    }
    
    // Get total record count
    int total_records;
    if (count_table_records(TEST_DB_DIR, TEST_TABLE_NAME, &total_records) != 0) {
        fprintf(stderr, "Failed to count table records\n");
        free_cursor(&cursor);
        return 1;
    }
    
    printf("Total records: %d\n", total_records);
    
    // Define the customer structure to match the schema
    // This should match the structure generated by the schema_generator
    typedef struct {
        int id;
        char name[101];
        char email[101];
        int age;
        bool active;
        char _uuid[37];  // Include the UUID field that gets added
    } Customer;
    
    // Iterate through all records
    printf("\nCustomer List:\n");
    printf("ID | Name            | Email                | Age | Active\n");
    printf("---+-----------------+----------------------+-----+-------\n");
    
    int count = 0;
    while (!cursor.at_end) {
        // Get current record
        void* record;
        if (get_current_record(&cursor, &record) != 0) {
            fprintf(stderr, "Failed to get current record\n");
            free_cursor(&cursor);
            return 1;
        }
        
        // Cast to our Customer structure
        Customer* customer = (Customer*)record;
        
        printf("%2d | %-15s | %-20s | %3d | %s\n",
               customer->id,
               customer->name,
               customer->email,
               customer->age,
               customer->active ? "true" : "false");
        
        count++;
        
        // Move to next record
        int result = next_record(&cursor);
        if (result < 0) {
            fprintf(stderr, "Failed to move to next record\n");
            free_cursor(&cursor);
            return 1;
        }
    }
    
    printf("\nRead %d records successfully\n", count);
    
    // Clean up
    if (free_cursor(&cursor) != 0) {
        fprintf(stderr, "Failed to free cursor\n");
        return 1;
    }
    
    DEBUG_PRINT("test_data_access: done\n");
    return 0;
}
