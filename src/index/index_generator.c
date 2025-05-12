/**
 * @file index_generator.c
 * @brief Generates index structures
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include "index_generator.h"
#include "btree_index.h"
#include "hash_index.h"
#include "../schema/directory_manager.h"
#include "../loader/page_manager.h"
#include "../loader/record_access.h"

/**
 * @brief Create directory if it doesn't exist
 */
static int ensure_directory(const char* path) {
    struct stat st;
    
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return 0;
        }
        return -1;
    }
    
    if (mkdir(path, 0755) != 0) {
        fprintf(stderr, "Failed to create directory: %s\n", strerror(errno));
        return -1;
    }
    
    return 0;
}

/**
 * @brief Get the column index for a column name
 */
int get_column_index(const TableSchema* schema, const char* column_name) {
    for (int i = 0; i < schema->column_count; i++) {
        if (strcmp(schema->columns[i].name, column_name) == 0) {
            return i;
        }
    }
    
    return -1;
}

/**
 * @brief Extract column value from record
 */
static void* get_column_value(const TableSchema* schema, void* record, int col_idx) {
    if (!schema || !record || col_idx < 0 || col_idx >= schema->column_count) {
        return NULL;
    }
    
    // Calculate offset to column in record
    // This is a simplified approach - real implementation would need
    // to handle various types and alignment issues
    
    // For simplicity, we'll assume all columns are at fixed offsets
    // In a real implementation, you'd need to calculate proper struct layout
    
    // Example offset calculation for demonstration only
    size_t offset = 0;
    for (int i = 0; i < col_idx; i++) {
        switch (schema->columns[i].type) {
            case TYPE_INT:
                offset += sizeof(int);
                break;
            case TYPE_FLOAT:
                offset += sizeof(double);
                break;
            case TYPE_BOOLEAN:
                offset += sizeof(bool);
                break;
            case TYPE_DATE:
                offset += sizeof(time_t);
                break;
            case TYPE_VARCHAR:
                offset += schema->columns[i].length + 1;
                break;
            case TYPE_TEXT:
                offset += 4096; // Fixed size for TEXT
                break;
            default:
                offset += 8; // Default size
                break;
        }
        
        // Add padding for alignment
        // This is a simplification
        offset = (offset + 7) & ~7;
    }
    
    return (char*)record + offset;
}

/**
 * @brief Generate index header file for a table
 */
int generate_index_header(const TableSchema* schema, const IndexDefinition* index_def, 
                         const char* directory) {
    if (!schema || !index_def || !directory) {
        return -1;
    }
    
    // Create directory path
    char header_path[2048];
    snprintf(header_path, sizeof(header_path), "%s/%s_index.h", 
             directory, index_def->column_name);
    
    // Open header file
    FILE* file = fopen(header_path, "w");
    if (!file) {
        fprintf(stderr, "Failed to open header file for writing: %s\n", strerror(errno));
        return -1;
    }
    
    // Write header
    fprintf(file, 
        "/**\n"
        " * @file %s_index.h\n"
        " * @brief Index for %s.%s\n"
        " * @note Generated file - do not edit\n"
        " */\n\n"
        "#ifndef UMBRA_INDEX_%s_%s_H\n"
        "#define UMBRA_INDEX_%s_%s_H\n\n"
        "#include \"../index/index_types.h\"\n\n",
        index_def->column_name, schema->name, index_def->column_name,
        schema->name, index_def->column_name, schema->name, index_def->column_name);
    
    // Write index accessor function prototypes
    if (index_def->type == INDEX_BTREE) {
        fprintf(file,
            "/**\n"
            " * @brief Find records by indexed field (exact match)\n"
            " * @param key Key value to find\n"
            " * @param results Buffer to store result positions\n"
            " * @param max_results Maximum number of results to return\n"
            " * @return Number of matching records found\n"
            " */\n"
            "int find_by_%s_exact(const void* key, int* results, int max_results);\n\n"
            
            "/**\n"
            " * @brief Find records by indexed field (range match)\n"
            " * @param low_key Lower bound (inclusive, NULL for unbounded)\n"
            " * @param high_key Upper bound (inclusive, NULL for unbounded)\n"
            " * @param results Buffer to store result positions\n"
            " * @param max_results Maximum number of results to return\n"
            " * @return Number of matching records found\n"
            " */\n"
            "int find_by_%s_range(const void* low_key, const void* high_key, "
            "int* results, int max_results);\n\n",
            index_def->column_name, index_def->column_name);
    } else {
        fprintf(file,
            "/**\n"
            " * @brief Find records by indexed field\n"
            " * @param key Key value to find\n"
            " * @param results Buffer to store result positions\n"
            " * @param max_results Maximum number of results to return\n"
            " * @return Number of matching records found\n"
            " */\n"
            "int find_by_%s(const void* key, int* results, int max_results);\n\n",
            index_def->column_name);
    }
    
    // Close header
    fprintf(file, "#endif /* UMBRA_INDEX_%s_%s_H */\n", 
            schema->name, index_def->column_name);
    
    fclose(file);
    return 0;
}

/**
 * @brief Generate index source file for a page
 */
int generate_index_source(const TableSchema* schema, const IndexDefinition* index_def,
                         const char* base_dir, int page_number) {
    if (!schema || !index_def || !base_dir) {
        return -1;
    }
    
    // Create source directory if needed
    char src_dir[2048];
    snprintf(src_dir, sizeof(src_dir), "%s/tables/%s/src", base_dir, schema->name);
    
    if (ensure_directory(src_dir) != 0) {
        return -1;
    }
    
    // Create source file path
    char src_path[2048];
    snprintf(src_path, sizeof(src_path), "%s/%s_index_%d.c", 
             src_dir, index_def->column_name, page_number);
    
    // Open source file
    FILE* file = fopen(src_path, "w");
    if (!file) {
        fprintf(stderr, "Failed to open source file for writing: %s\n", strerror(errno));
        return -1;
    }
    
    // Write source file header
    fprintf(file,
        "/**\n"
        " * @file %s_index_%d.c\n"
        " * @brief Index for %s.%s (page %d)\n"
        " * @note Generated file - do not edit\n"
        " */\n\n"
        "#include <stdlib.h>\n"
        "#include <string.h>\n"
        "#include \"../%s.h\"\n"
        "#include \"../index/index_types.h\"\n\n",
        index_def->column_name, page_number,
        schema->name, index_def->column_name, page_number,
        schema->name);
    
    // Generate index structure code
    char* index_code = malloc(1024 * 1024); // Allocate 1MB for index code
    if (!index_code) {
        fclose(file);
        return -1;
    }
    
    int bytes_written = build_index_from_page(schema, index_def, base_dir, 
                                            page_number, index_code, 1024 * 1024);
    
    if (bytes_written <= 0) {
        free(index_code);
        fclose(file);
        return -1;
    }
    
    // Write index code to file
    fprintf(file, "%s\n\n", index_code);
    
    // Write accessor functions
    if (index_def->type == INDEX_BTREE) {
        fprintf(file,
            "/**\n"
            " * @brief Find records by indexed field (exact match)\n"
            " */\n"
            "int find_by_%s_exact(const void* key, int* results, int max_results) {\n"
            "    return btree_find_exact(&%s_btree_index, key, results, max_results);\n"
            "}\n\n"
            
            "/**\n"
            " * @brief Find records by indexed field (range match)\n"
            " */\n"
            "int find_by_%s_range(const void* low_key, const void* high_key, "
            "int* results, int max_results) {\n"
            "    return btree_find_range(&%s_btree_index, low_key, high_key, "
            "results, max_results);\n"
            "}\n",
            index_def->column_name, index_def->column_name,
            index_def->column_name, index_def->column_name);
    } else {
        fprintf(file,
            "/**\n"
            " * @brief Find records by indexed field\n"
            " */\n"
            "int find_by_%s(const void* key, int* results, int max_results) {\n"
            "    return hash_find(&%s_hash_index, key, results, max_results);\n"
            "}\n",
            index_def->column_name, index_def->column_name);
    }
    
    free(index_code);
    fclose(file);
    return 0;
}

/**
 * @brief Build index from page data
 */
int build_index_from_page(const TableSchema* schema, const IndexDefinition* index_def,
                         const char* base_dir, int page_number,
                         char* output, size_t output_size) {
    if (!schema || !index_def || !base_dir || !output || output_size <= 0) {
        return -1;
    }
    
    // Find column in schema
    int col_idx = get_column_index(schema, index_def->column_name);
    if (col_idx < 0) {
        fprintf(stderr, "Column '%s' not found in table '%s'\n", 
               index_def->column_name, schema->name);
        return -1;
    }
    
    // Load page
    LoadedPage page;
    if (load_page(base_dir, schema->name, page_number, &page) != 0) {
        fprintf(stderr, "Failed to load page %d\n", page_number);
        return -1;
    }
    
    // Get record count
    int record_count;
    if (get_page_count(&page, &record_count) != 0 || record_count == 0) {
        unload_page(&page);
        // Empty page, generate empty index
        if (index_def->type == INDEX_BTREE) {
            BTreeIndex empty_index;
            if (btree_init(&empty_index, schema->columns[col_idx].type, 
                           index_def->column_name) != 0) {
                return -1;
            }
            
            int result = btree_generate_code(&empty_index, output, output_size);
            btree_free(&empty_index);
            return result;
        } else {
            HashIndex empty_index;
            if (hash_init(&empty_index, schema->columns[col_idx].type, 
                          index_def->column_name, DEFAULT_HASH_SIZE) != 0) {
                return -1;
            }
            
            int result = hash_generate_code(&empty_index, output, output_size);
            hash_free(&empty_index);
            return result;
        }
    }
    
    // Collect key-value pairs from page
    KeyValuePair* pairs = malloc(record_count * sizeof(KeyValuePair));
    if (!pairs) {
        unload_page(&page);
        return -1;
    }
    
    int valid_pairs = 0;
    
    for (int i = 0; i < record_count; i++) {
        void* record;
        if (read_record(&page, i, &record) != 0) {
            continue;
        }
        
        void* value = get_column_value(schema, record, col_idx);
        if (!value) {
            continue;
        }
        
        // Duplicate value
        pairs[valid_pairs].key = duplicate_key(value, 
                                             schema->columns[col_idx].type);
        pairs[valid_pairs].position = i;
        valid_pairs++;
    }
    
    unload_page(&page);
    
    // Build index from pairs
    int result = -1;
    
    if (index_def->type == INDEX_BTREE) {
        // Sort pairs
        qsort(pairs, valid_pairs, sizeof(KeyValuePair), compare_pairs);
        
        // Build B-tree
        BTreeIndex* index = btree_build_from_sorted(pairs, valid_pairs,
                                                  schema->columns[col_idx].type,
                                                  index_def->column_name);
        
        if (index) {
            result = btree_generate_code(index, output, output_size);
            btree_free(index);
            free(index);
        }
    } else {
        // Build hash index
        HashIndex* index = hash_build_from_pairs(pairs, valid_pairs,
                                               schema->columns[col_idx].type,
                                               index_def->column_name);
        
        if (index) {
            result = hash_generate_code(index, output, output_size);
            hash_free(index);
            free(index);
        }
    }
    
    // Free pairs
    for (int i = 0; i < valid_pairs; i++) {
        free(pairs[i].key);
    }
    free(pairs);
    
    return result;
}

/**
 * @brief Generate index for column in table
 */
int generate_index_for_column(const TableSchema* schema, const char* column_name,
                             IndexType index_type, const char* base_dir) {
    if (!schema || !column_name || !base_dir) {
        return -1;
    }
    
    // Create index definition
    IndexDefinition index_def;
    strncpy(index_def.table_name, schema->name, sizeof(index_def.table_name) - 1);
    index_def.table_name[sizeof(index_def.table_name) - 1] = '\0';
    
    strncpy(index_def.column_name, column_name, sizeof(index_def.column_name) - 1);
    index_def.column_name[sizeof(index_def.column_name) - 1] = '\0';
    
    // Generate index name
    snprintf(index_def.index_name, sizeof(index_def.index_name), 
             "idx_%s_%s", schema->name, column_name);
    
    index_def.type = index_type;
    index_def.unique = false;
    index_def.primary = false;
    
    // Check if this is a primary key column
    for (int i = 0; i < schema->primary_key_column_count; i++) {
        int pk_col = schema->primary_key_columns[i];
        if (strcmp(schema->columns[pk_col].name, column_name) == 0) {
            index_def.primary = true;
            index_def.unique = true;
            break;
        }
    }
    
    // Generate index header
    char include_dir[2048];
    snprintf(include_dir, sizeof(include_dir), "%s/tables/%s", base_dir, schema->name);
    
    if (generate_index_header(schema, &index_def, include_dir) != 0) {
        return -1;
    }
    
    // Count pages for table
    int page_count = 0;
    char compiled_dir[2048];
    snprintf(compiled_dir, sizeof(compiled_dir), "%s/compiled", base_dir);
    
    DIR* dir = opendir(compiled_dir);
    if (!dir) {
        return -1;
    }
    
    // Look for page .so files for this table
    struct dirent* entry;
    char pattern[256];
    snprintf(pattern, sizeof(pattern), "%sData_", schema->name);
    size_t pattern_len = strlen(pattern);
    
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, pattern, pattern_len) == 0) {
            // Check if filename ends with .so
            size_t name_len = strlen(entry->d_name);
            if (name_len > 3 && strcmp(entry->d_name + name_len - 3, ".so") == 0) {
                page_count++;
            }
        }
    }
    
    closedir(dir);
    
    // Generate index for each page
    for (int page_num = 0; page_num < page_count; page_num++) {
        if (generate_index_source(schema, &index_def, base_dir, page_num) != 0) {
            return -1;
        }
    }
    
    return 0;
}
