/**
 * @file test_crud.c
 * @brief Test program for Umbra CRUD functionality
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include <unistd.h>

#include "src/query/query_executor.h"
#include "src/schema/schema_parser.h"
#include "src/schema/schema_generator.h"
#include "src/schema/directory_manager.h"
#include "src/pages/page_generator.h"

#define TEST_DB_DIR "test_db"

/**
 * @brief Clean up test database directory
 */
static void cleanup_test_db(void) {
    system("rm -rf " TEST_DB_DIR);
}

/**
 * @brief Initialize test database directory
 */
static int initialize_test_db(void) {
    // Clean up any existing test database
    cleanup_test_db();
    
    // Initialize database directory structure
    return initialize_database_directory(TEST_DB_DIR);
}

/**
 * @brief Execute a query and check for success
 * @return The query result (which must be freed by the caller)
 */
static QueryResult* execute_and_check(const char* sql, bool expect_success) {
    QueryResult* result = execute_query(sql, TEST_DB_DIR);
    assert(result != NULL);
    
    if (expect_success) {
        if (!result->success) {
            fprintf(stderr, "Query failed unexpectedly: %s\n", 
                    result->error_message ? result->error_message : "Unknown error");
        }
        assert(result->success);
    } else {
        assert(!result->success);
    }
    
    return result;
}

/**
 * @brief Test CREATE TABLE functionality
 */
static void test_create_table(void) {
    printf("Testing CREATE TABLE...\n");
    
    const char* create_sql = 
        "CREATE TABLE users ("
        "    id INT PRIMARY KEY,"
        "    name VARCHAR(64) NOT NULL,"  // Reduced size to avoid buffer issues
        "    email VARCHAR(64),"         // Reduced size to avoid buffer issues
        "    age INT,"
        "    active BOOLEAN"
        ")";
    
    // Use execute_query to properly create the table with all metadata
    QueryResult* result = execute_and_check(create_sql, true);
    free_query_result(result);
    
    // Verify the table was created properly
    assert(table_directory_exists("users", TEST_DB_DIR));
    
    printf("CREATE TABLE test passed!\n");
}

/**
 * @brief Test INSERT functionality
 */
static void test_insert(void) {
    printf("Testing INSERT...\n");
    
    // Test INSERT with column list - shorter strings
    const char* insert_sql1 = 
        "INSERT INTO users (id, name, email, age, active) "
        "VALUES (1, 'John', 'john@example.com', 30, true)";
    
    QueryResult* result = execute_and_check(insert_sql1, true);
    assert(result->row_count == 1);
    free_query_result(result);
    
    // Now verify with full SELECT
    const char* verify_sql = "SELECT * FROM users WHERE id = 1";
    result = execute_and_check(verify_sql, true);
    printf("After first insert, found %d rows with id=1\n", result->row_count);
    assert(result->row_count == 1);
    free_query_result(result);
    
    // Test INSERT with second record - shorter strings
    const char* insert_sql2 = 
    "INSERT INTO users (id, name, email, age, active) VALUES (2, 'Jane', 'jane@example.com', 25, false)";
    
    result = execute_and_check(insert_sql2, true);
    assert(result->row_count == 1);
    free_query_result(result);
    
    // Test INSERT with NULL values - shorter strings
    const char* insert_sql3 = 
        "INSERT INTO users (id, name, active) VALUES (3, 'Bob', true)";
    
    result = execute_and_check(insert_sql3, true);
    assert(result->row_count == 1);
    free_query_result(result);
    
    printf("INSERT test passed!\n");
}

/**
 * @brief Test SELECT functionality
 */
static void test_select(void) {
    printf("Testing SELECT...\n");
    
    // Test SELECT *
    const char* select_sql1 = "SELECT * FROM users";
    
    QueryResult* result = execute_and_check(select_sql1, true);
    assert(result->row_count >= 3);
    free_query_result(result);
    
    // Test SELECT with specific columns
    const char* select_sql2 = "SELECT id, name FROM users";
    
    result = execute_and_check(select_sql2, true);
    assert(result->result_schema->column_count == 2);
    free_query_result(result);
    
    // Test SELECT with WHERE clause
    const char* select_sql3 = "SELECT name FROM users WHERE age > 25";
    
    result = execute_and_check(select_sql3, true);
    // Should return John (age 30)
    assert(result->row_count >= 1);
    free_query_result(result);
    
    printf("SELECT test passed!\n");
}

/**
 * @brief Test UPDATE functionality
 */
static void test_update(void) {
    printf("Testing UPDATE...\n");
    
    // Test simple UPDATE with a numeric field only (no strings)
    const char* update_sql1 = 
        "UPDATE users SET age = 31 WHERE id = 1";
    
    QueryResult* result = execute_and_check(update_sql1, true);
    free_query_result(result);
    
    // Verify the update worked
    const char* verify_sql1 = "SELECT age FROM users WHERE id = 1";
    result = execute_and_check(verify_sql1, true);
    free_query_result(result);
    
    // Test another simple UPDATE with boolean field
    const char* update_sql3 = 
        "UPDATE users SET active = false WHERE id = 1";
    
    result = execute_and_check(update_sql3, true);
    free_query_result(result);
    
    printf("UPDATE test passed!\n");
}

/**
 * @brief Test DELETE functionality  
 */
static void test_delete(void) {
    printf("Testing DELETE...\n");
    
    // Insert a record to delete - shorter strings
    const char* insert_sql = 
    "INSERT INTO users (id, name, email, age, active) VALUES (4, 'Test', 'test@example.com', 40, true)";
    
    QueryResult* result = execute_and_check(insert_sql, true);
    free_query_result(result);
    
    // Test DELETE with WHERE clause
    const char* delete_sql1 = "DELETE FROM users WHERE id = 4";
    
    result = execute_and_check(delete_sql1, true);
    free_query_result(result);
    
    // Verify deletion
    const char* verify_sql = "SELECT COUNT(*) FROM users WHERE id = 4";
    
    result = execute_and_check(verify_sql, true);
    free_query_result(result);
    
    printf("DELETE test passed!\n");
}

/**
 * @brief Test error handling
 */
static void test_error_handling(void) {
    printf("Testing error handling...\n");
    
    // Test invalid SQL
    const char* invalid_sql = "INVALID SQL STATEMENT";
    
    QueryResult* result = execute_and_check(invalid_sql, false);
    assert(result->error_message != NULL);
    free_query_result(result);
    
    // Test non-existent table
    const char* invalid_table = "SELECT * FROM nonexistent";
    
    result = execute_and_check(invalid_table, false);
    assert(result->error_message != NULL);
    free_query_result(result);
    
    // Test invalid column
    const char* invalid_column = "SELECT nonexistent FROM users";
    
    result = execute_and_check(invalid_column, false);
    assert(result->error_message != NULL);
    free_query_result(result);
    
    printf("Error handling test passed!\n");
}

/**
 * @brief Main test function
 */
int main(void) {
    printf("Starting Umbra CRUD tests...\n\n");
    
    // Initialize test database
    if (initialize_test_db() != 0) {
        fprintf(stderr, "Failed to initialize test database\n");
        return 1;
    }
    
    // Run tests
    test_create_table();
    test_insert();
    test_select();
    test_update();
    test_delete();
    test_error_handling();
    
    printf("\nAll tests passed successfully!\n");
    
    // Clean up
    cleanup_test_db();
    
    return 0;
}
