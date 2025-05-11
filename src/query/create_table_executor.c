/**
 * @file create_table_executor.c
 * @brief Executes CREATE TABLE operations
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include "create_table_executor.h"
#include "../schema/directory_manager.h"
#include "../schema/schema_generator.h"
#include "../schema/metadata.h"
#include "../pages/page_generator.h"  // Add this include

/**
 * @brief Create result structure
 */
static CreateTableResult* create_result(void) {
    CreateTableResult* result = malloc(sizeof(CreateTableResult));
    result->success = false;
    result->error_message = NULL;
    return result;
}

/**
 * @brief Set error in result
 */
static void set_error(CreateTableResult* result, const char* format, ...) {
    result->success = false;
    
    va_list args;
    va_start(args, format);
    
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    
    result->error_message = strdup(buffer);
    
    va_end(args);
}

/**
 * @brief Execute a CREATE TABLE statement
 */
CreateTableResult* execute_create_table(const char* create_statement, const char* base_dir) {
    CreateTableResult* result = create_result();
    
    if (!create_statement || !base_dir) {
        set_error(result, "Invalid parameters");
        return result;
    }
    
    // Parse the CREATE TABLE statement
    TableSchema* schema = parse_create_table(create_statement);
    if (!schema) {
        set_error(result, "Failed to parse CREATE TABLE statement");
        return result;
    }
    
    // Validate the schema
    if (!validate_schema(schema)) {
        set_error(result, "Invalid schema");
        free_table_schema(schema);
        return result;
    }
    
    // Check if table already exists
    if (table_directory_exists(schema->name, base_dir)) {
        set_error(result, "Table '%s' already exists", schema->name);
        free_table_schema(schema);
        return result;
    }
    
    // Create table directory structure
    if (create_table_directory(schema->name, base_dir) != 0) {
        set_error(result, "Failed to create table directory structure");
        free_table_schema(schema);
        return result;
    }
    
    // Save schema metadata
    if (save_schema_metadata(schema, base_dir) != 0) {
        set_error(result, "Failed to save schema metadata");
        free_table_schema(schema);
        return result;
    }
    
    // Generate header file
    char table_dir[2048];
    if (get_table_directory(schema->name, base_dir, table_dir, sizeof(table_dir)) != 0) {
        set_error(result, "Failed to get table directory");
        free_table_schema(schema);
        return result;
    }
    
    if (generate_header_file(schema, table_dir) != 0) {
        set_error(result, "Failed to generate header file");
        free_table_schema(schema);
        return result;
    }
    
    // Create table metadata
    TableMetadata metadata = create_table_metadata(schema, "system", 1000);
    if (save_table_metadata(&metadata, base_dir) != 0) {
        set_error(result, "Failed to save table metadata");
        free_table_schema(schema);
        return result;
    }
    
    // Generate initial empty page
    if (generate_data_page(schema, base_dir, 0) != 0) {
        set_error(result, "Failed to generate initial data page");
        free_table_schema(schema);
        return result;
    }
    
    result->success = true;
    free_table_schema(schema);
    return result;
}

/**
 * @brief Free create table result
 */
void free_create_table_result(CreateTableResult* result) {
    if (result) {
        free(result->error_message);
        free(result);
    }
}
