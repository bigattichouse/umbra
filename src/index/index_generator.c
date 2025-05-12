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
#include "../schema/directory_manager.h"

/**
 * @brief Generate header file for an index
 */
int generate_index_header(const TableSchema* schema, const IndexDefinition* index_def,
                         const char* base_dir) {
    if (!schema || !index_def || !base_dir) {
        return -1;
    }
    
    // Get index directory path
    char index_dir[1024];
    snprintf(index_dir, sizeof(index_dir), "%s/tables/%s/indices", 
             base_dir, schema->name);
    
    // Create directory if it doesn't exist
    struct stat st;
    if (stat(index_dir, &st) != 0) {
        if (mkdir(index_dir, 0755) != 0) {
            fprintf(stderr, "Failed to create index directory: %s\n", strerror(errno));
            return -1;
        }
    }
    
    // Create header file path
    char header_path[2048];
    snprintf(header_path, sizeof(header_path), "%s/%s.h", 
             index_dir, index_def->index_name);
    
    // Open header file
    FILE* file = fopen(header_path, "w");
    if (!file) {
        fprintf(stderr, "Failed to open header file for writing: %s\n", strerror(errno));
        return -1;
    }
    
    // Write header guard
    fprintf(file, "#ifndef UMBRA_INDEX_%s_H\n", index_def->index_name);
    fprintf(file, "#define UMBRA_INDEX_%s_H\n\n", index_def->index_name);
    
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
        default:
            key_type = "void*";
            break;
    }
    
    fprintf(file, "int find_by_index(%s key, int* positions, int max_positions);\n\n", key_type);
    
    // Close header guard
    fprintf(file, "#endif /* UMBRA_INDEX_%s_H */\n", index_def->index_name);
    
    fclose(file);
    return 0;
}

/**
 * @brief Generate source file for an index
 */
int generate_index_source(const TableSchema* schema, const IndexDefinition* index_def,
                         const char* base_dir, int page_number) {
    if (!schema || !index_def || !base_dir) {
        return -1;
    }
    
    // Create necessary directories first
    char src_dir[2048];
    snprintf(src_dir, sizeof(src_dir), "%s/tables/%s/src", base_dir, schema->name);
    
    struct stat st;
    if (stat(src_dir, &st) != 0) {
        // Directory doesn't exist, create it
        if (mkdir(src_dir, 0755) != 0) {
            fprintf(stderr, "Failed to create source directory: %s\n", strerror(errno));
            return -1;
        }
    }
    
    // Create indices directory if needed
    char indices_dir[2048];
    snprintf(indices_dir, sizeof(indices_dir), "%s/tables/%s/indices", base_dir, schema->name);
    
    if (stat(indices_dir, &st) != 0) {
        // Directory doesn't exist, create it
        if (mkdir(indices_dir, 0755) != 0) {
            fprintf(stderr, "Failed to create indices directory: %s\n", strerror(errno));
            return -1;
        }
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
    
    // Create the index source file
    char src_path[3072];
    const char* type_str = (index_def->type == INDEX_TYPE_BTREE) ? "btree" : "hash";
    
    if (page_number >= 0) {
        snprintf(src_path, sizeof(src_path), "%s/%s_%s_index_%d.c", 
                src_dir, type_str, index_def->column_name, page_number);
    } else {
        snprintf(src_path, sizeof(src_path), "%s/%s_%s_index.c", 
                src_dir, type_str, index_def->column_name);
    }
    
    // Open the source file for writing
    FILE* src_file = fopen(src_path, "w");
    if (!src_file) {
        fprintf(stderr, "Failed to create index source file: %s\n", strerror(errno));
        return -1;
    }
    
    // Write index source code - this will depend on index type
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
    } else {
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
    }
    
    fclose(src_file);
    return 0;
}
/**
 * @brief Build index from a data page
 */
int build_index_from_page(const TableSchema* schema, const IndexDefinition* index_def,
                         const char* base_dir, int page_number) {
    if (!schema || !index_def || !base_dir || page_number < 0) {
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
        return -1;
    }
    
    // Create index definition
    IndexDefinition index_def;
    strncpy(index_def.table_name, schema->name, sizeof(index_def.table_name) - 1);
    index_def.table_name[sizeof(index_def.table_name) - 1] = '\0';
    
    strncpy(index_def.column_name, column_name, sizeof(index_def.column_name) - 1);
    index_def.column_name[sizeof(index_def.column_name) - 1] = '\0';
    
    // Create index name: {table}_{column}_{type}
    snprintf(index_def.index_name, sizeof(index_def.index_name),
             "%s_%s_%s", schema->name, column_name,
             index_type == INDEX_TYPE_BTREE ? "btree" : "hash");
    
    index_def.type = index_type;
    index_def.unique = false;
    index_def.primary = false;
    
    // Check if column is part of primary key
    int col_idx = get_column_index(schema, column_name);
    if (col_idx < 0) {
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
        return -1;
    }
    
    // Count data pages
    int page_count = 1; // For simplicity, assume at least one page
    
    // Generate source and build index for each page
    for (int i = 0; i < page_count; i++) {
        if (generate_index_source(schema, &index_def, base_dir, i) != 0) {
            return -1;
        }
        
        if (build_index_from_page(schema, &index_def, base_dir, i) != 0) {
            return -1;
        }
        
        if (compile_index(schema, &index_def, base_dir, i) != 0) {
            return -1;
        }
    }
    
    return 0;
}
