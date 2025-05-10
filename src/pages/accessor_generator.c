/**
 * @file accessor_generator.c
 * @brief Generates count() and read() functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "accessor_generator.h"
#include "page_template.h"
#include "../schema/directory_manager.h"

/**
 * @brief Generate accessor function file for a table page
 */
int generate_accessor_file(const TableSchema* schema, const char* base_dir, int page_number) {
    if (!schema || !base_dir) {
        return -1;
    }
    
    // Get source directory path
    char src_dir[2048];
    if (get_source_directory(schema->name, base_dir, src_dir, sizeof(src_dir)) != 0) {
        return -1;
    }
    
    // Create source file path - increase buffer size
    char src_path[3072];
    snprintf(src_path, sizeof(src_path), "%s/%sData_%d.c", 
             src_dir, schema->name, page_number);
    
    // Open source file
    FILE* src_file = fopen(src_path, "w");
    if (!src_file) {
        fprintf(stderr, "Failed to open %s for writing\n", src_path);
        return -1;
    }
    
    // Write accessor source template
    fprintf(src_file, ACCESSOR_SOURCE_TEMPLATE,
            schema->name,
            schema->name, schema->name, page_number,
            schema->name,
            schema->name, page_number,
            schema->name,
            schema->name, page_number, schema->name,
            schema->name,
            schema->name, page_number);
    
    // Close file
    fclose(src_file);
    return 0;
}

/**
 * @brief Generate filter function for WHERE clause
 */
static int generate_filter_function(const TableSchema* schema, FILE* file, const char* where_clause) {
    if (!schema || !file || !where_clause) {
        return -1;
    }
    
    // For a real implementation, this would parse the WHERE clause and generate C code
    // Here we just generate a simple placeholder
    
    fprintf(file,
        "/**\n"
        " * @brief Filter function for records\n"
        " * @param record Record to filter\n"
        " * @return true if record matches filter, false otherwise\n"
        " */\n"
        "static bool filter_record(const %s* record) {\n"
        "    /* TODO: Implement actual filter based on WHERE clause: %s */\n"
        "    return true; /* Placeholder: accept all records */\n"
        "}\n\n",
        schema->name, where_clause);
    
    return 0;
}

/**
 * @brief Generate filtered accessor functions
 */
int generate_filtered_accessor(const TableSchema* schema, const char* base_dir, 
                              int page_number, const char* where_clause) {
    if (!schema || !base_dir || !where_clause) {
        return -1;
    }
    
    // Get source directory path
    char src_dir[2048];
    if (get_source_directory(schema->name, base_dir, src_dir, sizeof(src_dir)) != 0) {
        return -1;
    }
    
    // Create source file path - increase buffer size
    char src_path[3072];
    snprintf(src_path, sizeof(src_path), "%s/%sData_%d_filtered.c", 
             src_dir, schema->name, page_number);
    
    // Open source file
    FILE* src_file = fopen(src_path, "w");
    if (!src_file) {
        fprintf(stderr, "Failed to open %s for writing\n", src_path);
        return -1;
    }
    
    // Write includes
    fprintf(src_file,
        "#include <stdlib.h>\n"
        "#include <stdbool.h>\n"
        "#include \"../%s.h\"\n\n",
        schema->name);
    
    // Write data array declaration
    fprintf(src_file,
        "/* Data array containing records */\n"
        "static %s %sData_%d[] = {\n"
        "    /*BEGIN %s DATA*/\n"
        "#include \"../data/%sData.%d.dat.h\"\n"
        "    /*END %s DATA*/\n"
        "};\n\n",
        schema->name, schema->name, page_number,
        schema->name,
        schema->name, page_number,
        schema->name);
    
    // Generate filter function
    if (generate_filter_function(schema, src_file, where_clause) != 0) {
        fclose(src_file);
        return -1;
    }
    
    // Write filtered count function
    fprintf(src_file,
        "/**\n"
        " * @brief Returns the number of records that match the filter\n"
        " * @return Number of matching records\n"
        " */\n"
        "int count(void) {\n"
        "    int total = sizeof(%sData_%d) / sizeof(%s);\n"
        "    int matching = 0;\n"
        "    \n"
        "    for (int i = 0; i < total; i++) {\n"
        "        if (filter_record(&%sData_%d[i])) {\n"
        "            matching++;\n"
        "        }\n"
        "    }\n"
        "    \n"
        "    return matching;\n"
        "}\n\n",
        schema->name, page_number, schema->name,
        schema->name, page_number);
    
    // Write filtered read function
    fprintf(src_file,
        "/**\n"
        " * @brief Returns a record at the specified position that matches the filter\n"
        " * @param pos Position in the filtered result set\n"
        " * @return Pointer to the record or NULL if out of bounds\n"
        " */\n"
        "%s* read(int pos) {\n"
        "    if (pos < 0) {\n"
        "        return NULL;\n"
        "    }\n"
        "    \n"
        "    int total = sizeof(%sData_%d) / sizeof(%s);\n"
        "    int matching_idx = 0;\n"
        "    \n"
        "    for (int i = 0; i < total; i++) {\n"
        "        if (filter_record(&%sData_%d[i])) {\n"
        "            if (matching_idx == pos) {\n"
        "                return &%sData_%d[i];\n"
        "            }\n"
        "            matching_idx++;\n"
        "        }\n"
        "    }\n"
        "    \n"
        "    return NULL; /* Position out of bounds */\n"
        "}\n",
        schema->name,
        schema->name, page_number, schema->name,
        schema->name, page_number,
        schema->name, page_number);
    
    // Close file
    fclose(src_file);
    return 0;
}

/**
 * @brief Generate projection structure
 */
static int generate_projection_struct(const TableSchema* schema, FILE* file, 
                                    const int* columns, int column_count) {
    if (!schema || !file || !columns || column_count <= 0) {
        return -1;
    }
    
    // Generate projection struct
    fprintf(file,
        "/**\n"
        " * @struct %s_Projection\n"
        " * @brief Projection of selected columns\n"
        " */\n"
        "typedef struct {\n",
        schema->name);
    
    // Add each selected column
    for (int i = 0; i < column_count; i++) {
        int col_idx = columns[i];
        if (col_idx < 0 || col_idx >= schema->column_count) {
            fprintf(stderr, "Invalid column index: %d\n", col_idx);
            return -1;
        }
        
        const ColumnDefinition* col = &schema->columns[col_idx];
        
        if (col->type == TYPE_VARCHAR || col->type == TYPE_TEXT) {
            // String types need array size
            int len = (col->type == TYPE_VARCHAR) ? col->length : 4096;
            fprintf(file, "    char %s[%d];\n", col->name, len + 1);
        } else {
            // Get C type
            const char* c_type;
            switch (col->type) {
                case TYPE_INT:
                    c_type = "int";
                    break;
                case TYPE_FLOAT:
                    c_type = "double";
                    break;
                case TYPE_DATE:
                    c_type = "time_t";
                    break;
                case TYPE_BOOLEAN:
                    c_type = "bool";
                    break;
                default:
                    c_type = "void";
                    break;
            }
            
            fprintf(file, "    %s %s;\n", c_type, col->name);
        }
    }
    
    // Close struct definition
    fprintf(file, "} %s_Projection;\n\n", schema->name);
    
    return 0;
}

/**
 * @brief Generate projection accessor for selected columns
 */
int generate_projection_accessor(const TableSchema* schema, const char* base_dir, 
                               int page_number, const int* columns, int column_count) {
    if (!schema || !base_dir || !columns || column_count <= 0) {
        return -1;
    }
    
    // Get source directory path
    char src_dir[2048];
    if (get_source_directory(schema->name, base_dir, src_dir, sizeof(src_dir)) != 0) {
        return -1;
    }
    
    // Create source file path - increase buffer size
    char src_path[3072];
    snprintf(src_path, sizeof(src_path), "%s/%sData_%d_projection.c", 
             src_dir, schema->name, page_number);
    
    // Open source file
    FILE* src_file = fopen(src_path, "w");
    if (!src_file) {
        fprintf(stderr, "Failed to open %s for writing\n", src_path);
        return -1;
    }
    
    // Write includes
    fprintf(src_file,
        "#include <stdlib.h>\n"
        "#include <string.h>\n"
        "#include \"../%s.h\"\n\n",
        schema->name);
    
    // Write data array declaration
    fprintf(src_file,
        "/* Data array containing records */\n"
        "static %s %sData_%d[] = {\n"
        "    /*BEGIN %s DATA*/\n"
        "#include \"../data/%sData.%d.dat.h\"\n"
        "    /*END %s DATA*/\n"
        "};\n\n",
        schema->name, schema->name, page_number,
        schema->name,
        schema->name, page_number,
        schema->name);
    
    // Generate projection struct
    if (generate_projection_struct(schema, src_file, columns, column_count) != 0) {
        fclose(src_file);
        return -1;
    }
    
    // Create projection array
    fprintf(src_file,
        "/* Projection array */\n"
        "static %s_Projection projections[sizeof(%sData_%d) / sizeof(%s)];\n"
        "static int projection_initialized = 0;\n\n",
        schema->name, schema->name, page_number, schema->name);
    
    // Write initialization function
    fprintf(src_file,
        "/**\n"
        " * @brief Initialize projections\n"
        " */\n"
        "static void initialize_projections(void) {\n"
        "    if (projection_initialized) {\n"
        "        return;\n"
        "    }\n"
        "    \n"
        "    int total = sizeof(%sData_%d) / sizeof(%s);\n"
        "    \n"
        "    for (int i = 0; i < total; i++) {\n",
        schema->name, page_number, schema->name);
    
    // Add field copying for each projected column
    for (int i = 0; i < column_count; i++) {
        int col_idx = columns[i];
        const ColumnDefinition* col = &schema->columns[col_idx];
        
        if (col->type == TYPE_VARCHAR || col->type == TYPE_TEXT) {
            // String types need strcpy
            fprintf(src_file,
                "        strcpy(projections[i].%s, %sData_%d[i].%s);\n",
                col->name, schema->name, page_number, col->name);
        } else {
            // Direct assignment for other types
            fprintf(src_file,
                "        projections[i].%s = %sData_%d[i].%s;\n",
                col->name, schema->name, page_number, col->name);
        }
    }
    
    // Close initialization function
    fprintf(src_file,
        "    }\n"
        "    \n"
        "    projection_initialized = 1;\n"
        "}\n\n");
    
    // Write count function
    fprintf(src_file,
        "/**\n"
        " * @brief Returns the number of records\n"
        " * @return Number of records\n"
        " */\n"
        "int count(void) {\n"
        "    return sizeof(%sData_%d) / sizeof(%s);\n"
        "}\n\n",
        schema->name, page_number, schema->name);
    
    // Write read function
    fprintf(src_file,
        "/**\n"
        " * @brief Returns a projected record at the specified position\n"
        " * @param pos Position of the record\n"
        " * @return Pointer to the projected record or NULL if out of bounds\n"
        " */\n"
        "%s_Projection* read(int pos) {\n"
        "    if (pos < 0 || pos >= count()) {\n"
        "        return NULL;\n"
        "    }\n"
        "    \n"
        "    if (!projection_initialized) {\n"
        "        initialize_projections();\n"
        "    }\n"
        "    \n"
        "    return &projections[pos];\n"
        "}\n",
        schema->name);
    
    // Close file
    fclose(src_file);
    return 0;
}
