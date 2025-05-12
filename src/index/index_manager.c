/**
 * @file index_manager.c
 * @brief Manages database indexes
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>
#include <stdarg.h>
#include "index_manager.h"
#include "index_generator.h"
#include "index_compiler.h"
#include "../schema/directory_manager.h"
#include "../loader/so_loader.h"

/**
 * @brief Create a new CREATE INDEX result
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
    if (!result) {
        return;
    }
    
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
static int save_index_metadata(const IndexManager* manager, const char* base_dir) {
    if (!manager || !base_dir) {
        return -1;
    }
    
    // Create metadata directory
    char metadata_dir[2048];
    snprintf(metadata_dir, sizeof(metadata_dir), "%s/tables/%s/metadata", 
             base_dir, manager->table_name);
    
    struct stat st;
    if (stat(metadata_dir, &st) != 0) {
        if (mkdir(metadata_dir, 0755) != 0) {
            fprintf(stderr, "Failed to create metadata directory: %s\n", strerror(errno));
            return -1;
        }
    }
    
    // Create metadata file path
    char metadata_path[2048];
    snprintf(metadata_path, sizeof(metadata_path), "%s/indices.dat", metadata_dir);
    
    // Write indices to file
    FILE* file = fopen(metadata_path, "wb");
    if (!file) {
        fprintf(stderr, "Failed to open metadata file for writing: %s\n", strerror(errno));
        return -1;
    }
    
    // Write count first
    if (fwrite(&manager->index_count, sizeof(int), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    
    // Write indices
    if (manager->index_count > 0) {
        if (fwrite(manager->indices, sizeof(IndexDefinition), manager->index_count, file) 
            != (size_t)manager->index_count) {
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
static int load_index_metadata(IndexManager* manager, const char* base_dir) {
    if (!manager || !base_dir) {
        return -1;
    }
    
    // Create metadata file path
    char metadata_path[2048];
    snprintf(metadata_path, sizeof(metadata_path), "%s/tables/%s/metadata/indices.dat", 
             base_dir, manager->table_name);
    
    // Check if file exists
    struct stat st;
    if (stat(metadata_path, &st) != 0) {
        // No indices yet
        manager->indices = NULL;
        manager->index_count = 0;
        return 0;
    }
    
    // Read indices from file
    FILE* file = fopen(metadata_path, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open metadata file for reading: %s\n", strerror(errno));
        return -1;
    }
    
    // Read count first
    if (fread(&manager->index_count, sizeof(int), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    
    // Read indices
    if (manager->index_count > 0) {
        manager->indices = malloc(manager->index_count * sizeof(IndexDefinition));
        if (!manager->indices) {
            fclose(file);
            return -1;
        }
        
        if (fread(manager->indices, sizeof(IndexDefinition), manager->index_count, file) 
            != (size_t)manager->index_count) {
            free(manager->indices);
            manager->indices = NULL;
            manager->index_count = 0;
            fclose(file);
            return -1;
        }
    } else {
        manager->indices = NULL;
    }
    
    fclose(file);
    return 0;
}

/**
 * @brief Initialize index manager for a table
 */
int init_index_manager(IndexManager* manager, const char* table_name, const char* base_dir) {
    if (!manager || !table_name || !base_dir) {
        return -1;
    }
    
    strncpy(manager->table_name, table_name, sizeof(manager->table_name) - 1);
    manager->table_name[sizeof(manager->table_name) - 1] = '\0';
    
    manager->indices = NULL;
    manager->index_count = 0;
    
    return load_index_metadata(manager, base_dir);
}

/**
 * @brief Free resources used by index manager
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
 * @brief Check if column is already indexed
 */
static bool is_column_indexed(const IndexManager* manager, const char* column_name) {
    for (int i = 0; i < manager->index_count; i++) {
        if (strcmp(manager->indices[i].column_name, column_name) == 0) {
            return true;
        }
    }
    
    return false;
}

/**
 * @brief Get column index from schema
 */
static int get_column_index(const TableSchema* schema, const char* column_name) {
    for (int i = 0; i < schema->column_count; i++) {
        if (strcmp(schema->columns[i].name, column_name) == 0) {
            return i;
        }
    }
    return -1;
}

/**
 * @brief Create an index for a column
 */
CreateIndexResult* create_index(IndexManager* manager, const char* column_name,
                               IndexType index_type, const char* base_dir) {
    CreateIndexResult* result = create_result();
    
    if (!manager || !column_name || !base_dir) {
        set_error(result, "Invalid parameters");
        return result;
    }
    
    // Check if column is already indexed
    if (is_column_indexed(manager, column_name)) {
        set_error(result, "Column '%s' is already indexed", column_name);
        return result;
    }
    
    // Load table schema
    TableSchema* schema = load_table_schema(manager->table_name, base_dir);
    if (!schema) {
        set_error(result, "Failed to load schema for table '%s'", manager->table_name);
        return result;
    }
    
    // Check if column exists
    int col_idx = get_column_index(schema, column_name);
    if (col_idx < 0) {
        set_error(result, "Column '%s' not found in table '%s'", column_name, manager->table_name);
        free_table_schema(schema);
        return result;
    }
    
    // Create index definition
    IndexDefinition index_def;
    strncpy(index_def.table_name, manager->table_name, sizeof(index_def.table_name) - 1);
    index_def.table_name[sizeof(index_def.table_name) - 1] = '\0';
    
    strncpy(index_def.column_name, column_name, sizeof(index_def.column_name) - 1);
    index_def.column_name[sizeof(index_def.column_name) - 1] = '\0';
    
    // Generate index name
    snprintf(index_def.index_name, sizeof(index_def.index_name), 
             "idx_%s_%s", manager->table_name, column_name);
    
    index_def.type = index_type;
    index_def.unique = false;
    index_def.primary = false;
    
    // Check if this is a primary key column
    for (int i = 0; i < schema->primary_key_column_count; i++) {
        int pk_col = schema->primary_key_columns[i];
        if (pk_col == col_idx) {
            index_def.primary = true;
            index_def.unique = true;
            break;
        }
    }
    
    // Generate index
    if (generate_index_for_column(schema, column_name, index_type, base_dir) != 0) {
        set_error(result, "Failed to generate index");
        free_table_schema(schema);
        return result;
    }
    
    // Add index to manager
    manager->indices = realloc(manager->indices, 
                              (manager->index_count + 1) * sizeof(IndexDefinition));
    
    if (!manager->indices) {
        set_error(result, "Memory allocation failed");
        free_table_schema(schema);
        return result;
    }
    
    manager->indices[manager->index_count] = index_def;
    manager->index_count++;
    
    // Save index metadata
    if (save_index_metadata(manager, base_dir) != 0) {
        set_error(result, "Failed to save index metadata");
        free_table_schema(schema);
        return result;
    }
    
    // Count pages for table
    int page_count = 0;
    char compiled_dir[2048];
    snprintf(compiled_dir, sizeof(compiled_dir), "%s/compiled", base_dir);
    
    DIR* dir = opendir(compiled_dir);
    if (dir) {
        struct dirent* entry;
        char pattern[256];
        snprintf(pattern, sizeof(pattern), "%sData_", schema->name);
        size_t pattern_len = strlen(pattern);
        
        while ((entry = readdir(dir)) != NULL) {
            if (strncmp(entry->d_name, pattern, pattern_len) == 0) {
                size_t name_len = strlen(entry->d_name);
                if (name_len > 3 && strcmp(entry->d_name + name_len - 3, ".so") == 0) {
                    page_count++;
                }
            }
        }
        
        closedir(dir);
    }
    
    // Compile index for each page
    for (int page_num = 0; page_num < page_count; page_num++) {
        if (compile_index(schema, &index_def, base_dir, page_num) != 0) {
            set_error(result, "Failed to compile index for page %d", page_num);
            free_table_schema(schema);
            return result;
        }
    }
    
    free_table_schema(schema);
    
    result->success = true;
    return result;
}

/**
 * @brief Drop an index
 */
int drop_index(IndexManager* manager, const char* index_name, const char* base_dir) {
    if (!manager || !index_name || !base_dir) {
        return -1;
    }
    
    // Find index
    int index_idx = -1;
    for (int i = 0; i < manager->index_count; i++) {
        if (strcmp(manager->indices[i].index_name, index_name) == 0) {
            index_idx = i;
            break;
        }
    }
    
    if (index_idx < 0) {
        fprintf(stderr, "Index '%s' not found\n", index_name);
        return -1;
    }
    
    // Remove index files
    // TODO: Implement index file removal
    
    // Remove index from list
    for (int i = index_idx; i < manager->index_count - 1; i++) {
        manager->indices[i] = manager->indices[i + 1];
    }
    
    manager->index_count--;
    
    if (manager->index_count > 0) {
        manager->indices = realloc(manager->indices, manager->index_count * sizeof(IndexDefinition));
    } else {
        free(manager->indices);
        manager->indices = NULL;
    }
    
    // Save index metadata
    return save_index_metadata(manager, base_dir);
}

/**
 * @brief Get indices for a table
 */
int get_table_indices(const char* table_name, const char* base_dir,
                     IndexDefinition** indices, int* count) {
    if (!table_name || !base_dir || !indices || !count) {
        return -1;
    }
    
    // Initialize index manager
    IndexManager manager;
    if (init_index_manager(&manager, table_name, base_dir) != 0) {
        return -1;
    }
    
    // Copy indices
    if (manager.index_count > 0) {
        *indices = malloc(manager.index_count * sizeof(IndexDefinition));
        if (!*indices) {
            free_index_manager(&manager);
            return -1;
        }
        
        memcpy(*indices, manager.indices, manager.index_count * sizeof(IndexDefinition));
    } else {
        *indices = NULL;
    }
    
    *count = manager.index_count;
    
    free_index_manager(&manager);
    return 0;
}

/**
 * @brief Load a compiled index
 */
void* load_index(const TableSchema* schema, const char* column_name,
                const char* base_dir, int page_number, IndexType index_type) {
    if (!schema || !column_name || !base_dir) {
        return NULL;
    }
    
    // Get index .so path
    char so_path[2048];
    if (get_index_so_path(schema, column_name, base_dir, page_number, index_type, so_path, sizeof(so_path)) < 0) {
        return NULL;
    }
    
    // Check if file exists
    struct stat st;
    if (stat(so_path, &st) != 0) {
        fprintf(stderr, "Index file not found: %s\n", so_path);
        return NULL;
    }
    
    // Load index library
    LoadedLibrary* lib = malloc(sizeof(LoadedLibrary));
    if (!lib) {
        return NULL;
    }
    
    if (load_library(so_path, lib) != 0) {
        free(lib);
        return NULL;
    }
    
    // Get function pointers based on index type
    if (index_type == INDEX_BTREE) {
        // Get exact match function
        char func_name[256];
        snprintf(func_name, sizeof(func_name), "find_by_%s_exact", column_name);
        
        void* exact_fn;
        if (get_function(lib, func_name, &exact_fn) != 0) {
            unload_library(lib);
            free(lib);
            return NULL;
        }
        
        // Get range function
        snprintf(func_name, sizeof(func_name), "find_by_%s_range", column_name);
        
        void* range_fn;
        if (get_function(lib, func_name, &range_fn) != 0) {
            unload_library(lib);
            free(lib);
            return NULL;
        }
    } else {
        // Get hash function
        char func_name[256];
        snprintf(func_name, sizeof(func_name), "find_by_%s", column_name);
        
        void* find_fn;
        if (get_function(lib, func_name, &find_fn) != 0) {
            unload_library(lib);
            free(lib);
            return NULL;
        }
    }
    
    return lib;
}

/**
 * @brief Parse a CREATE INDEX statement
 */
static bool parse_create_index(const char* create_statement, char* table_name, size_t table_size,
                              char* column_name, size_t column_size, IndexType* index_type) {
    // Simple parser for: CREATE [BTREE|HASH] INDEX ON table_name(column_name)
    
    const char* ptr = create_statement;
    
    // Skip whitespace
    while (*ptr && isspace(*ptr)) ptr++;
    
    // Check for CREATE
    if (strncasecmp(ptr, "CREATE", 6) != 0) return false;
    ptr += 6;
    
    // Skip whitespace
    while (*ptr && isspace(*ptr)) ptr++;
    
    // Check for index type
    if (strncasecmp(ptr, "BTREE", 5) == 0) {
        *index_type = INDEX_BTREE;
        ptr += 5;
    } else if (strncasecmp(ptr, "HASH", 4) == 0) {
        *index_type = INDEX_HASH;
        ptr += 4;
    } else {
        // Default to B-tree
        *index_type = INDEX_BTREE;
    }
    
    // Skip whitespace
    while (*ptr && isspace(*ptr)) ptr++;
    
    // Check for INDEX
    if (strncasecmp(ptr, "INDEX", 5) != 0) return false;
    ptr += 5;
    
    // Skip whitespace
    while (*ptr && isspace(*ptr)) ptr++;
    
    // Check for ON
    if (strncasecmp(ptr, "ON", 2) != 0) return false;
    ptr += 2;
    
    // Skip whitespace
    while (*ptr && isspace(*ptr)) ptr++;
    
    // Extract table name
    const char* table_start = ptr;
    while (*ptr && !isspace(*ptr) && *ptr != '(') ptr++;
    
    size_t table_len = ptr - table_start;
    if (table_len == 0 || table_len >= table_size) return false;
    
    strncpy(table_name, table_start, table_len);
    table_name[table_len] = '\0';
    
    // Skip whitespace
    while (*ptr && isspace(*ptr)) ptr++;
    
    // Check for opening parenthesis
    if (*ptr != '(') return false;
    ptr++;
    
    // Skip whitespace
    while (*ptr && isspace(*ptr)) ptr++;
    
    // Extract column name
    const char* column_start = ptr;
    while (*ptr && *ptr != ')' && !isspace(*ptr)) ptr++;
    
    size_t column_len = ptr - column_start;
    if (column_len == 0 || column_len >= column_size) return false;
    
    strncpy(column_name, column_start, column_len);
    column_name[column_len] = '\0';
    
    // Skip whitespace
    while (*ptr && isspace(*ptr)) ptr++;
    
    // Check for closing parenthesis
    if (*ptr != ')') return false;
    
    return true;
}

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
    
    if (!parse_create_index(create_statement, table_name, sizeof(table_name),
                          column_name, sizeof(column_name), &index_type)) {
        set_error(result, "Failed to parse CREATE INDEX statement");
        return result;
    }
    
    // Check if table exists
    if (!table_directory_exists(table_name, base_dir)) {
        set_error(result, "Table '%s' not found", table_name);
        return result;
    }
    
    // Initialize index manager
    IndexManager manager;
    if (init_index_manager(&manager, table_name, base_dir) != 0) {
        set_error(result, "Failed to initialize index manager");
        return result;
    }
    
    // Create index
    CreateIndexResult* create_result = create_index(&manager, column_name, index_type, base_dir);
    
    // Copy result
    result->success = create_result->success;
    if (!create_result->success) {
        result->error_message = strdup(create_result->error_message);
    }
    
    free_create_index_result(create_result);
    free_index_manager(&manager);
    
    return result;
}

/**
 * @brief Free CREATE INDEX result
 */
void free_create_index_result(CreateIndexResult* result) {
    if (result) {
        free(result->error_message);
        free(result);
    }
}
