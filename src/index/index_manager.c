/**
 * @file index_manager.c
 * @brief Implements index management
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <dirent.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include "index_manager.h"
#include "index_generator.h"
#include "index_compiler.h"
#include "../query/query_executor.h"  /* For load_table_schema */

/**
 * @brief Create a new result structure
 */
static CreateIndexResult* create_result(void) {
    CreateIndexResult* result = malloc(sizeof(CreateIndexResult));
    result->success = false;
    result->error_message = NULL;
    return result;
}

/**
 * @brief Set error message in result
 */
static void set_error(CreateIndexResult* result, const char* format, ...) {
    result->success = false;
    
    va_list args;
    va_start(args, format);
    
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    
    result->error_message = strdup(buffer);
    
    va_end(args);
}

/**
 * @brief Save index metadata to file
 */
int save_index_metadata(const IndexManager* manager, const char* base_dir) {
    if (!manager || !base_dir) {
        return -1;
    }
    
    // Create index metadata directory
    char metadata_dir[1024];
    snprintf(metadata_dir, sizeof(metadata_dir), "%s/tables/%s/metadata", 
             base_dir, manager->table_name);
    
    struct stat st;
    if (stat(metadata_dir, &st) != 0) {
        if (mkdir(metadata_dir, 0755) != 0) {
            fprintf(stderr, "Failed to create metadata directory: %s\n", strerror(errno));
            return -1;
        }
    }
    
    // Create index metadata file path
    char metadata_path[2048];
    snprintf(metadata_path, sizeof(metadata_path), "%s/indices.dat", metadata_dir);
    
    // Write metadata to file
    FILE* file = fopen(metadata_path, "wb");
    if (!file) {
        fprintf(stderr, "Failed to open metadata file for writing: %s\n", strerror(errno));
        return -1;
    }
    
    // Write index count
    if (fwrite(&manager->index_count, sizeof(int), 1, file) != 1) {
        fprintf(stderr, "Failed to write index count\n");
        fclose(file);
        return -1;
    }
    
    // Write indices
    if (manager->index_count > 0) {
        if (fwrite(manager->indices, sizeof(IndexDefinition), manager->index_count, file) 
            != (size_t)manager->index_count) {
            fprintf(stderr, "Failed to write indices\n");
            fclose(file);
            return -1;
        }
    }
    
    fclose(file);
    return 0;
}

/**
 * @brief Load index metadata from file
 */
int load_index_metadata(IndexManager* manager, const char* base_dir) {
    if (!manager || !base_dir) {
        return -1;
    }
    
    // Create index metadata file path
    char metadata_path[2048];
    snprintf(metadata_path, sizeof(metadata_path), "%s/tables/%s/metadata/indices.dat", 
             base_dir, manager->table_name);
    
    // Check if file exists
    struct stat st;
    if (stat(metadata_path, &st) != 0) {
        // No indices - this is not an error
        manager->index_count = 0;
        manager->indices = NULL;
        return 0;
    }
    
    // Open metadata file
    FILE* file = fopen(metadata_path, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open metadata file for reading: %s\n", strerror(errno));
        return -1;
    }
    
    // Read index count
    if (fread(&manager->index_count, sizeof(int), 1, file) != 1) {
        fprintf(stderr, "Failed to read index count\n");
        fclose(file);
        return -1;
    }
    
    // Read indices
    if (manager->index_count > 0) {
        manager->indices = malloc(manager->index_count * sizeof(IndexDefinition));
        if (!manager->indices) {
            fprintf(stderr, "Failed to allocate memory for indices\n");
            fclose(file);
            return -1;
        }
        
        if (fread(manager->indices, sizeof(IndexDefinition), manager->index_count, file) 
            != (size_t)manager->index_count) {
            fprintf(stderr, "Failed to read indices\n");
            free(manager->indices);
            manager->indices = NULL;
            manager->index_count = 0;
            fclose(file);
            return -1;
        }
    }
    
    fclose(file);
    return 0;
}

/**
 * @brief Initialize an index manager
 */
int init_index_manager(IndexManager* manager, const char* table_name) {
    if (!manager || !table_name) {
        return -1;
    }
    
    memset(manager, 0, sizeof(IndexManager));
    
    strncpy(manager->table_name, table_name, sizeof(manager->table_name) - 1);
    manager->table_name[sizeof(manager->table_name) - 1] = '\0';
    
    manager->indices = NULL;
    manager->index_count = 0;
    
    return 0;
}

/**
 * @brief Free resources used by an index manager
 */
int free_index_manager(IndexManager* manager) {
    if (!manager) {
        return -1;
    }
    
    free(manager->indices);
    manager->indices = NULL;
    manager->index_count = 0;
    
    return 0;
}

/**
 * @brief Check if a column is indexed
 */
bool is_column_indexed(const IndexManager* manager, const char* column_name) {
    if (!manager || !column_name) {
        return false;
    }
    
    for (int i = 0; i < manager->index_count; i++) {
        if (strcmp(manager->indices[i].column_name, column_name) == 0) {
            return true;
        }
    }
    
    return false;
}

/**
 * @brief Get column index in the schema
 */
int get_column_index(const TableSchema* schema, const char* column_name) {
    if (!schema || !column_name) {
        return -1;
    }
    
    for (int i = 0; i < schema->column_count; i++) {
        if (strcmp(schema->columns[i].name, column_name) == 0) {
            return i;
        }
    }
    
    return -1;
}

/**
 * @brief Create a new index
 */
int create_index(IndexManager* manager, const char* column_name, 
                IndexType index_type, const char* base_dir) {
    CreateIndexResult* result = create_result();
    
    if (!manager || !column_name || !base_dir) {
        set_error(result, "Invalid parameters");
        return -1;
    }
    
    // Check if column is already indexed
    if (is_column_indexed(manager, column_name)) {
        set_error(result, "Column already indexed");
        return -1;
    }
    
    // Load table schema
    TableSchema* schema = load_table_schema(manager->table_name, base_dir);
    if (!schema) {
        set_error(result, "Table not found: %s", manager->table_name);
        return -1;
    }
    
    // Check if column exists
    int col_idx = get_column_index(schema, column_name);
    if (col_idx < 0) {
        set_error(result, "Column not found: %s", column_name);
        free_table_schema(schema);
        return -1;
    }
    
    // Create index definition
    IndexDefinition index_def;
    strncpy(index_def.table_name, manager->table_name, sizeof(index_def.table_name) - 1);
    index_def.table_name[sizeof(index_def.table_name) - 1] = '\0';
    
    strncpy(index_def.column_name, column_name, sizeof(index_def.column_name) - 1);
    index_def.column_name[sizeof(index_def.column_name) - 1] = '\0';
    
    // Create index name: {table}_{column}_{type}
    snprintf(index_def.index_name, sizeof(index_def.index_name),
             "%s_%s_%s", manager->table_name, column_name,
             index_type == INDEX_TYPE_BTREE ? "btree" : "hash");
    
    index_def.type = index_type;
    index_def.unique = false;
    index_def.primary = false;
    
    // Check if column is part of primary key
    for (int i = 0; i < schema->primary_key_column_count; i++) {
        if (schema->primary_key_columns[i] == col_idx) {
            index_def.primary = true;
            index_def.unique = true;
            break;
        }
    }
    
    // Generate index files and build index
    if (generate_index_for_column(schema, column_name, index_type, base_dir) != 0) {
        set_error(result, "Failed to generate index");
        free_table_schema(schema);
        return -1;
    }
    
    // Add index definition to manager
    IndexDefinition* new_indices = realloc(manager->indices, 
                               (manager->index_count + 1) * sizeof(IndexDefinition));
    if (!new_indices) {
        set_error(result, "Memory allocation failed");
        free_table_schema(schema);
        return -1;
    }
    
    manager->indices = new_indices;
    manager->indices[manager->index_count] = index_def;
    manager->index_count++;
    
    // Save index metadata
    if (save_index_metadata(manager, base_dir) != 0) {
        set_error(result, "Failed to save index metadata");
        free_table_schema(schema);
        return -1;
    }
    
    free_table_schema(schema);
    result->success = true;
    return 0;
}

/* Remaining implementation not shown for brevity */

/**
 * @brief Execute a CREATE INDEX statement
 */
CreateIndexResult* execute_create_index(const char* create_statement, const char* base_dir) {
    CreateIndexResult* result = create_result();
    
    if (!create_statement || !base_dir) {
        set_error(result, "Invalid parameters");
        return result;
    }
    
    // Parse CREATE INDEX statement
    char table_name[64];
    char column_name[64];
    IndexType index_type;
    
    if (parse_create_index(create_statement, table_name, sizeof(table_name),
                          column_name, sizeof(column_name), &index_type) != 0) {
        set_error(result, "Failed to parse CREATE INDEX statement");
        return result;
    }
    
    // Initialize index manager
    IndexManager manager;
    if (init_index_manager(&manager, table_name) != 0) {
        set_error(result, "Failed to initialize index manager");
        free_index_manager(&manager);
        return result;
    }
    
    // Load existing indices
    if (load_index_metadata(&manager, base_dir) != 0) {
        set_error(result, "Failed to load index metadata");
        free_index_manager(&manager);
        return result;
    }
    
    // Create index
    if (create_index(&manager, column_name, index_type, base_dir) != 0) {
        set_error(result, "Failed to create index");
        free_index_manager(&manager);
        return result;
    }
    
    // Cleanup
    free_index_manager(&manager);
    
    result->success = true;
    return result;
}

/**
 * @brief Free create index result
 */
void free_create_index_result(CreateIndexResult* result) {
    if (result) {
        free(result->error_message);
        result->error_message = NULL;
        free(result);
    }
}

/**
 * @brief Parse a CREATE INDEX statement
 */
int parse_create_index(const char* sql, char* table_name, size_t table_name_size,
                    char* column_name, size_t column_name_size, IndexType* index_type) {
    if (!sql || !table_name || !column_name || !index_type) {
        return -1;
    }
    
    // Default to BTREE index
    *index_type = INDEX_TYPE_BTREE;
    
    // Create an uppercase copy for keyword matching
    char sql_upper[1024];
    strncpy(sql_upper, sql, sizeof(sql_upper) - 1);
    sql_upper[sizeof(sql_upper) - 1] = '\0';
    
    for (int i = 0; sql_upper[i]; i++) {
        sql_upper[i] = toupper(sql_upper[i]);
    }
    
    // Check if it starts with CREATE INDEX
    if (strncmp(sql_upper, "CREATE INDEX", 12) != 0) {
        return -1;
    }
    
    // Find ON keyword in uppercase string
    char* on_ptr_upper = strstr(sql_upper, " ON ");
    if (!on_ptr_upper) {
        return -1;
    }
    
    // Find USING keyword
    char* using_ptr_upper = strstr(sql_upper, " USING ");
    if (using_ptr_upper) {
        // Check index type
        if (strstr(using_ptr_upper, " BTREE") || strstr(using_ptr_upper, "(BTREE)")) {
            *index_type = INDEX_TYPE_BTREE;
        } else if (strstr(using_ptr_upper, " HASH") || strstr(using_ptr_upper, "(HASH)")) {
            *index_type = INDEX_TYPE_HASH;
        } else {
            // Unknown index type
            return -1;
        }
    }
    
    // Find positions in uppercase string
    char* table_start_upper = on_ptr_upper + 4; // Skip " ON "
    char* table_end_upper = strstr(table_start_upper, " (");
    if (!table_end_upper) {
        return -1;
    }
    
    // Calculate offsets in original string
    size_t table_start_offset = table_start_upper - sql_upper;
    size_t table_end_offset = table_end_upper - sql_upper;
    
    const char* table_start = sql + table_start_offset;
    const char* table_end = sql + table_end_offset;
    
    int table_len = table_end - table_start;
    if (table_len <= 0 || (size_t)table_len >= table_name_size) {
        return -1;
    }
    
    // Copy table name from the original SQL, preserving case
    strncpy(table_name, table_start, table_len);
    table_name[table_len] = '\0';
    
    // Find column positions in uppercase string
    char* column_start_upper = table_end_upper + 2; // Skip " ("
    char* column_end_upper = strstr(column_start_upper, ")");
    if (!column_end_upper) {
        return -1;
    }
    
    // Calculate offsets in original string
    size_t column_start_offset = column_start_upper - sql_upper;
    size_t column_end_offset = column_end_upper - sql_upper;
    
    const char* column_start = sql + column_start_offset;
    const char* column_end = sql + column_end_offset;
    
    int column_len = column_end - column_start;
    if (column_len <= 0 || (size_t)column_len >= column_name_size) {
        return -1;
    }
    
    // Copy column name from the original SQL, preserving case
    strncpy(column_name, column_start, column_len);
    column_name[column_len] = '\0';
    
    // Trim leading and trailing whitespace from table and column names
    // Trim table name
    while (*table_name == ' ' && *table_name) {
        memmove(table_name, table_name + 1, strlen(table_name));
    }
    for (int i = strlen(table_name) - 1; i >= 0 && table_name[i] == ' '; i--) {
        table_name[i] = '\0';
    }
    
    // Trim column name
    while (*column_name == ' ' && *column_name) {
        memmove(column_name, column_name + 1, strlen(column_name));
    }
    for (int i = strlen(column_name) - 1; i >= 0 && column_name[i] == ' '; i--) {
        column_name[i] = '\0';
    }
    
    return 0;
}
