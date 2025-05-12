/**
 * @file index_generator.c
 * @brief Generates index files
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include "index_generator.h"
#include "index_compiler.h"
#include "index_manager.h"
#include "../schema/directory_manager.h"

/**
 * @brief Ensure directory exists
 * @return 0 if directory exists or was created, -1 on error
 */
static int ensure_directory(const char* path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        if (S_ISDIR(st.st_mode)) {
            return 0; // Directory already exists
        }
        fprintf(stderr, "Path exists but is not a directory: %s\n", path);
        return -1;
    }
    
    if (mkdir(path, 0755) != 0) {
        fprintf(stderr, "Failed to create directory %s: %s\n", path, strerror(errno));
        return -1;
    }
    
    fprintf(stderr, "Created directory: %s\n", path);
    return 0;
}

/**
 * @brief Generate header file for an index
 */
int generate_index_header(const TableSchema* schema, const IndexDefinition* index_def,
                         const char* base_dir) {
    if (!schema || !index_def || !base_dir) {
        fprintf(stderr, "Invalid parameters for generate_index_header\n");
        return -1;
    }
    
    // Get index directory path
    char index_dir[1024];
    snprintf(index_dir, sizeof(index_dir), "%s/tables/%s/indices", 
             base_dir, schema->name);
    
    // Create directory if it doesn't exist
    if (ensure_directory(index_dir) != 0) {
        return -1;
    }
    
    // Create header file path - use consistent naming pattern
    const char* type_str = (index_def->type == INDEX_TYPE_BTREE) ? "btree" : "hash";
    char header_path[2048];
    snprintf(header_path, sizeof(header_path), "%s/%s_%s_index_%s.h", 
             index_dir, schema->name, type_str, index_def->column_name);
    
    fprintf(stderr, "Creating index header file: %s\n", header_path);
    
    // Open header file
    FILE* file = fopen(header_path, "w");
    if (!file) {
        fprintf(stderr, "Failed to open header file for writing: %s\n", strerror(errno));
        return -1;
    }
    
    // Write header guard - use consistent naming
    fprintf(file, "#ifndef UMBRA_INDEX_%s_%s_%s_H\n", 
            schema->name, type_str, index_def->column_name);
    fprintf(file, "#define UMBRA_INDEX_%s_%s_%s_H\n\n", 
            schema->name, type_str, index_def->column_name);
    
    // Write includes
    fprintf(file, "#include <stdbool.h>\n");
    fprintf(file, "#include \"../../%s.h\"\n\n", schema->name);
    
    // Write function declarations
    fprintf(file, "/**\n");
    fprintf(file, " * @brief Initialize the index\n");
    fprintf(file, " * @return 0 on success, -1 on error\n");
    fprintf(file, " */\n");
    fprintf(file, "int init_index(void);\n\n");
    
    fprintf(file, "/**\n");
    fprintf(file, " * @brief Add a record to the index\n");
    fprintf(file, " * @param record Record to add\n");
    fprintf(file, " * @param position Position of record in the data page\n");
    fprintf(file, " * @return 0 on success, -1 on error\n");
    fprintf(file, " */\n");
    fprintf(file, "int add_to_index(%s* record, int position);\n\n", schema->name);
    
    fprintf(file, "/**\n");
    fprintf(file, " * @brief Find records matching a key\n");
    fprintf(file, " * @param key Key to search for\n");
    fprintf(file, " * @param positions Output array for positions\n");
    fprintf(file, " * @param max_positions Maximum number of positions\n");
    fprintf(file, " * @return Number of matching records or -1 on error\n");
    fprintf(file, " */\n");
    
    // Get column index and type
    int col_idx = get_column_index(schema, index_def->column_name);
    if (col_idx < 0) {
        fprintf(stderr, "Column not found: %s\n", index_def->column_name);
        fclose(file);
        return -1;
    }
    
    // Use appropriate key type based on column type
    const char* key_type;
    switch (schema->columns[col_idx].type) {
        case TYPE_INT:
            key_type = "int";
            break;
        case TYPE_FLOAT:
            key_type = "double";
            break;
        case TYPE_VARCHAR:
        case TYPE_TEXT:
            key_type = "const char*";
            break;
        case TYPE_BOOLEAN:
            key_type = "bool";
            break;
        case TYPE_DATE:
            key_type = "time_t";
            break;
        default:
            key_type = "void*";
            break;
    }
    
    fprintf(file, "int find_by_index(%s key, int* positions, int max_positions);\n\n", key_type);
    
    // Close header guard
    fprintf(file, "#endif /* UMBRA_INDEX_%s_%s_%s_H */\n", 
            schema->name, type_str, index_def->column_name);
    
    fclose(file);
    fprintf(stderr, "Generated index header file: %s\n", header_path);
    return 0;
}

/**
 * @brief Generate source file for an index
 */
int generate_index_source(const TableSchema* schema, const IndexDefinition* index_def,
                         const char* base_dir, int page_number) {
    if (!schema || !index_def || !base_dir) {
        fprintf(stderr, "Invalid parameters for generate_index_source\n");
        return -1;
    }
    
    // Create necessary directories
    // 1. Tables directory
    char tables_dir[1024];
    int written = snprintf(tables_dir, sizeof(tables_dir), "%s/tables", base_dir);
    if (written < 0 || written >= (int)sizeof(tables_dir)) {
        fprintf(stderr, "Path too long for tables directory\n");
        return -1;
    }
    
    if (ensure_directory(tables_dir) != 0) {
        return -1;
    }
    
    // 2. Table directory
    char table_dir[2048];
    written = snprintf(table_dir, sizeof(table_dir), "%s/%s", tables_dir, schema->name);
    if (written < 0 || written >= (int)sizeof(table_dir)) {
        fprintf(stderr, "Path too long for table directory\n");
        return -1;
    }
    
    if (ensure_directory(table_dir) != 0) {
        return -1;
    }
    
    // 3. Source directory
    char src_dir[3072]; // Increased buffer size to avoid truncation
    written = snprintf(src_dir, sizeof(src_dir), "%s/src", table_dir);
    if (written < 0 || written >= (int)sizeof(src_dir)) {
        fprintf(stderr, "Path too long for source directory\n");
        return -1;
    }
    
    if (ensure_directory(src_dir) != 0) {
        return -1;
    }
    
    // 4. Indices directory
    char indices_dir[3072]; // Increased buffer size to avoid truncation
    written = snprintf(indices_dir, sizeof(indices_dir), "%s/indices", table_dir);
    if (written < 0 || written >= (int)sizeof(indices_dir)) {
        fprintf(stderr, "Path too long for indices directory\n");
        return -1;
    }
    
    if (ensure_directory(indices_dir) != 0) {
        return -1;
    }
    
    // 5. Compiled directory
    char compiled_dir[1024];
    written = snprintf(compiled_dir, sizeof(compiled_dir), "%s/compiled", base_dir);
    if (written < 0 || written >= (int)sizeof(compiled_dir)) {
        fprintf(stderr, "Path too long for compiled directory\n");
        return -1;
    }
    
    if (ensure_directory(compiled_dir) != 0) {
        return -1;
    }
    
    // 6. Scripts directory
    char scripts_dir[1024];
    written = snprintf(scripts_dir, sizeof(scripts_dir), "%s/scripts", base_dir);
    if (written < 0 || written >= (int)sizeof(scripts_dir)) {
        fprintf(stderr, "Path too long for scripts directory\n");
        return -1;
    }
    
    if (ensure_directory(scripts_dir) != 0) {
        return -1;
    }
    
    // Find the column in the schema to get its type
    int col_idx = -1;
    for (int i = 0; i < schema->column_count; i++) {
        if (strcasecmp(schema->columns[i].name, index_def->column_name) == 0) {
            col_idx = i;
            break;
        }
    }
    
    if (col_idx == -1) {
        fprintf(stderr, "Column %s not found in schema\n", index_def->column_name);
        return -1;
    }
    
    // Determine the C type for the column
    const char* key_type;
    switch (schema->columns[col_idx].type) {
        case TYPE_INT:
            key_type = "int";
            break;
        case TYPE_FLOAT:
            key_type = "double";
            break;
        case TYPE_VARCHAR:
        case TYPE_TEXT:
            key_type = "const char*";
            break;
        case TYPE_BOOLEAN:
            key_type = "bool";
            break;
        case TYPE_DATE:
            key_type = "time_t";
            break;
        default:
            fprintf(stderr, "Unsupported data type for index\n");
            return -1;
    }
    
    // Debug output about index type
    fprintf(stderr, "Generating index source for type: %s (%d) on column %s\n", 
            index_def->type == INDEX_TYPE_BTREE ? "BTREE" : "HASH", 
            index_def->type, 
            index_def->column_name);
    
    // Create the index source file - use correct index type and consistent naming pattern
    const char* type_str = (index_def->type == INDEX_TYPE_BTREE) ? "btree" : "hash";
    char src_path[4096]; // Increased buffer size
    
    // FIXED: Updated filename pattern to be consistent with compiler expectations
    // New pattern: {schema_name}_{type}_index_{column}_{page}.c
    if (page_number >= 0) {
        written = snprintf(src_path, sizeof(src_path), "%s/%s_%s_index_%s_%d.c", 
                src_dir, schema->name, type_str, index_def->column_name, page_number);
    } else {
        written = snprintf(src_path, sizeof(src_path), "%s/%s_%s_index_%s.c", 
                src_dir, schema->name, type_str, index_def->column_name);
    }
    
    if (written < 0 || written >= (int)sizeof(src_path)) {
        fprintf(stderr, "Path too long for source file\n");
        return -1;
    }
    
    fprintf(stderr, "Creating index source file: %s\n", src_path);
    
    // Open the source file for writing
    FILE* src_file = fopen(src_path, "w");
    if (!src_file) {
        fprintf(stderr, "Failed to create index source file: %s - %s\n", src_path, strerror(errno));
        return -1;
    }
    
    // Write index source code based on index type
    if (index_def->type == INDEX_TYPE_BTREE) {
        // Write B-tree index code
        fprintf(src_file, "/**\n");
        fprintf(src_file, " * Generated B-tree index for %s.%s\n", 
                schema->name, index_def->column_name);
        fprintf(src_file, " */\n\n");
        fprintf(src_file, "#include <stdlib.h>\n");
        fprintf(src_file, "#include <string.h>\n");
        fprintf(src_file, "#include <stdbool.h>\n");
        fprintf(src_file, "#include <time.h>\n");
        fprintf(src_file, "#include \"../%s.h\"\n\n", schema->name);
        
        // Add basic index function implementations
        fprintf(src_file, "int init_index(void) {\n");
        fprintf(src_file, "    return 0;\n");
        fprintf(src_file, "}\n\n");
        
        fprintf(src_file, "int add_to_index(%s* record, int position) {\n", schema->name);
        fprintf(src_file, "    return 0;\n");
        fprintf(src_file, "}\n\n");
        
        // Use the actual type instead of a placeholder comment
        fprintf(src_file, "int find_by_index(%s key, int* positions, int max_positions) {\n", key_type);
        fprintf(src_file, "    return 0;\n");
        fprintf(src_file, "}\n");
    } else if (index_def->type == INDEX_TYPE_HASH) {
        // Write Hash index code
        fprintf(src_file, "/**\n");
        fprintf(src_file, " * Generated Hash index for %s.%s\n", 
                schema->name, index_def->column_name);
        fprintf(src_file, " */\n\n");
        fprintf(src_file, "#include <stdlib.h>\n");
        fprintf(src_file, "#include <string.h>\n");
        fprintf(src_file, "#include <stdbool.h>\n");
        fprintf(src_file, "#include <time.h>\n");
        fprintf(src_file, "#include \"../%s.h\"\n\n", schema->name);
        
        // Add basic index function implementations
        fprintf(src_file, "int init_index(void) {\n");
        fprintf(src_file, "    return 0;\n");
        fprintf(src_file, "}\n\n");
        
        fprintf(src_file, "int add_to_index(%s* record, int position) {\n", schema->name);
        fprintf(src_file, "    return 0;\n");
        fprintf(src_file, "}\n\n");
        
        // Use the actual type instead of a placeholder comment
        fprintf(src_file, "int find_by_index(%s key, int* positions, int max_positions) {\n", key_type);
        fprintf(src_file, "    return 0;\n");
        fprintf(src_file, "}\n");
    } else {
        fprintf(stderr, "Unsupported index type: %d\n", index_def->type);
        fclose(src_file);
        return -1;
    }
    
    fclose(src_file);
    fprintf(stderr, "Generated index source file: %s\n", src_path);
    return 0;
}

/**
 * @brief Build index from a data page
 */
int build_index_from_page(const TableSchema* schema, const IndexDefinition* index_def,
                         const char* base_dir, int page_number) {
    if (!schema || !index_def || !base_dir || page_number < 0) {
        fprintf(stderr, "Invalid parameters for build_index_from_page\n");
        return -1;
    }
    
    // First make sure the index source file exists
    int result = generate_index_source(schema, index_def, base_dir, page_number);
    if (result != 0) {
        fprintf(stderr, "Failed to generate index source file\n");
        return -1;
    }
    
    // Generate compilation script for the index
    result = generate_index_compile_script(schema, index_def, base_dir, page_number);
    if (result != 0) {
        fprintf(stderr, "Failed to generate index compilation script\n");
        return -1;
    }
    
    return 0;
}

/**
 * @brief Generate index for a column
 */
int generate_index_for_column(const TableSchema* schema, const char* column_name,
                           IndexType index_type, const char* base_dir) {
    if (!schema || !column_name || !base_dir) {
        fprintf(stderr, "Invalid parameters for generate_index_for_column\n");
        return -1;
    }
    
    fprintf(stderr, "Generating index for column %s with type %s (%d)\n", 
            column_name, 
            index_type == INDEX_TYPE_BTREE ? "BTREE" : "HASH",
            index_type);
    
    // Create index definition
    IndexDefinition index_def;
    strncpy(index_def.table_name, schema->name, sizeof(index_def.table_name) - 1);
    index_def.table_name[sizeof(index_def.table_name) - 1] = '\0';
    
    strncpy(index_def.column_name, column_name, sizeof(index_def.column_name) - 1);
    index_def.column_name[sizeof(index_def.column_name) - 1] = '\0';
    
    // Create index name: {table}_{column}_{type} - follow same consistent pattern
    const char* type_str = (index_type == INDEX_TYPE_BTREE) ? "btree" : "hash";
    snprintf(index_def.index_name, sizeof(index_def.index_name),
             "%s_%s_index_%s", schema->name, type_str, column_name);
    
    index_def.type = index_type;
    index_def.unique = false;
    index_def.primary = false;
    
    // Check if column is part of primary key
    int col_idx = get_column_index(schema, column_name);
    if (col_idx < 0) {
        fprintf(stderr, "Column not found: %s\n", column_name);
        return -1;
    }
    
    for (int i = 0; i < schema->primary_key_column_count; i++) {
        if (schema->primary_key_columns[i] == col_idx) {
            index_def.primary = true;
            index_def.unique = true;
            break;
        }
    }
    
    // Generate header file
    if (generate_index_header(schema, &index_def, base_dir) != 0) {
        fprintf(stderr, "Failed to generate index header\n");
        return -1;
    }
    
    // Count data pages - for now, assume at least one page
    int page_count = 1;
    
    // Generate source and build index for each page
    for (int i = 0; i < page_count; i++) {
        if (generate_index_source(schema, &index_def, base_dir, i) != 0) {
            fprintf(stderr, "Failed to generate index source for page %d\n", i);
            return -1;
        }
        
        if (build_index_from_page(schema, &index_def, base_dir, i) != 0) {
            fprintf(stderr, "Failed to build index for page %d\n", i);
            return -1;
        }
        
        if (compile_index(schema, &index_def, base_dir, i) != 0) {
            fprintf(stderr, "Failed to compile index for page %d\n", i);
            return -1;
        }
    }
    
    fprintf(stderr, "Successfully generated index for column %s\n", column_name);
    return 0;
}
