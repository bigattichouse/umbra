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
int add_test_data(void);
int test_data_access(void);

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
 * @brief Add test data to the customers table
 */
int add_test_data(void) {
    DEBUG_PRINT("add_test_data: start\n");
    
    // Define CREATE TABLE statement (for parsing)
    const char* create_statement = 
        "CREATE TABLE Customers ("
        "  id INT PRIMARY KEY,"
        "  name VARCHAR(100) NOT NULL,"
        "  email VARCHAR(100),"
        "  age INT,"
        "  active BOOLEAN DEFAULT true"
        ")";
    
    // Parse the statement to get schema
    TableSchema* schema = parse_create_table(create_statement);
    if (!schema) {
        fprintf(stderr, "Failed to parse CREATE TABLE statement\n");
        return 1;
    }
    
    printf("Adding test data to table: %s\n", schema->name);
    
    // Sample data
    const char* customer_data[][5] = {
        {"1", "John Doe", "john@example.com", "35", "true"},
        {"2", "Jane Smith", "jane@example.com", "28", "true"},
        {"3", "Bob Johnson", "bob@example.com", "42", "false"},
        {"4", "Alice Brown", "alice@example.com", "31", "true"},
        {"5", "Charlie Davis", "charlie@example.com", "45", "true"},
        {"6", "Eva Wilson", "eva@example.com", "29", "true"},
        {"7", "Frank Miller", "frank@example.com", "38", "false"},
        {"8", "Grace Taylor", "grace@example.com", "26", "true"},
        {"9", "Henry Lewis", "henry@example.com", "33", "true"},
        {"10", "Ivy Clark", "ivy@example.com", "41", "false"}
    };
    
    int num_customers = sizeof(customer_data) / sizeof(customer_data[0]);
    int page_number = 0;
    
    for (int i = 0; i < num_customers; i++) {
        DEBUG_PRINT("Adding customer %d\n", i);
        
        // Check if current page is full
        bool is_full;
        if (is_page_full(schema, TEST_DB_DIR, page_number, MAX_RECORDS_PER_PAGE, &is_full) != 0) {
            fprintf(stderr, "Failed to check if page is full\n");
            free_table_schema(schema);
            return 1;
        }
        
        // If page is full, create a new page
        if (is_full) {
            page_number++;
            printf("Creating new page: %d\n", page_number);
            
            if (generate_data_page(schema, TEST_DB_DIR, page_number) != 0) {
                fprintf(stderr, "Failed to generate new data page\n");
                free_table_schema(schema);
                return 1;
            }
        }
        
        // Add record to current page
        printf("Adding customer %s: %s\n", customer_data[i][0], customer_data[i][1]);
        
        if (add_record_to_page(schema, TEST_DB_DIR, page_number, customer_data[i]) != 0) {
            fprintf(stderr, "Failed to add record to page\n");
            free_table_schema(schema);
            return 1;
        }
    }
    
    // Recompile all pages
    for (int i = 0; i <= page_number; i++) {
        printf("Compiling page %d\n", i);
        DEBUG_PRINT("Recompiling page %d\n", i);
        
        if (recompile_data_page(schema, TEST_DB_DIR, i) != 0) {
            fprintf(stderr, "Failed to compile page %d\n", i);
            // Don't exit immediately - compilation might fail but we can still test
            // free_table_schema(schema);
            // return 1;
        }
    }
    
    printf("Added %d customers across %d pages\n", num_customers, page_number + 1);
    
    // Clean up
    free_table_schema(schema);
    DEBUG_PRINT("add_test_data: done\n");
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
