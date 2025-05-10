/**
 * @file schema_generator.c
 * @brief Generates C struct definitions from parsed schemas
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include "schema_generator.h"

/**
 * @brief Convert a data type to C type string
 */
static const char* data_type_to_c_type(DataType type, int length) {
    switch (type) {
        case TYPE_INT:
            return "int";
        case TYPE_VARCHAR:
            return "char";
        case TYPE_TEXT:
            return "char";
        case TYPE_FLOAT:
            return "double";
        case TYPE_DATE:
            return "time_t";
        case TYPE_BOOLEAN:
            return "bool";
        default:
            return "void";
    }
}

/**
 * @brief Generate C struct definition for a table schema
 */
int generate_struct_definition(const TableSchema* schema, char* output, size_t output_size) {
    if (!schema || !output || output_size <= 0) {
        return -1;
    }
    
    // Start with struct definition
    int written = snprintf(output, output_size,
        "/**\n"
        " * @struct %s\n"
        " * @brief Generated struct for %s table\n"
        " */\n"
        "typedef struct {\n", 
        schema->name, schema->name);
    
    if (written < 0 || written >= output_size) {
        return -1;
    }
    
    // Add each column
    for (int i = 0; i < schema->column_count; i++) {
        const ColumnDefinition* column = &schema->columns[i];
        
        // Add comment
        int col_written = snprintf(output + written, output_size - written,
            "    /* Column: %s */\n", column->name);
        
        if (col_written < 0 || col_written >= output_size - written) {
            return -1;
        }
        
        written += col_written;
        
        // Add field definition
        const char* c_type = data_type_to_c_type(column->type, column->length);
        
        if (column->type == TYPE_VARCHAR || column->type == TYPE_TEXT) {
            // String types need array size
            int len = (column->type == TYPE_VARCHAR) ? column->length : 4096;
            col_written = snprintf(output + written, output_size - written,
                "    %s %s[%d];\n", c_type, column->name, len + 1); // +1 for null terminator
        } else {
            col_written = snprintf(output + written, output_size - written,
                "    %s %s;\n", c_type, column->name);
        }
        
        if (col_written < 0 || col_written >= output_size - written) {
            return -1;
        }
        
        written += col_written;
    }
    
    // Close struct definition
    int end_written = snprintf(output + written, output_size - written,
        "} %s;\n", schema->name);
    
    if (end_written < 0 || end_written >= output_size - written) {
        return -1;
    }
    
    written += end_written;
    return written;
}

/**
 * @brief Ensure directory exists
 */
static int ensure_directory(const char* path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        // Check if it's a directory
        if (S_ISDIR(st.st_mode)) {
            return 0;
        }
        return -1;
    }
    
    // Directory doesn't exist, create it
    if (mkdir(path, 0755) != 0) {
        fprintf(stderr, "Failed to create directory %s: %s\n", path, strerror(errno));
        return -1;
    }
    
    return 0;
}

/**
 * @brief Generate header file for a table schema
 */
int generate_header_file(const TableSchema* schema, const char* directory) {
    if (!schema || !directory) {
        return -1;
    }
    
    // Ensure directory exists
    if (ensure_directory(directory) != 0) {
        return -1;
    }
    
    // Create header file path
    char header_path[1024];
    snprintf(header_path, sizeof(header_path), "%s/%s.h", directory, schema->name);
    
    // Open header file
    FILE* header_file = fopen(header_path, "w");
    if (!header_file) {
        fprintf(stderr, "Failed to open %s for writing: %s\n", header_path, strerror(errno));
        return -1;
    }
    
    // Write header guard
    fprintf(header_file, 
        "#ifndef UMBRA_STRUCT_%s_H\n"
        "#define UMBRA_STRUCT_%s_H\n\n"
        "#include <time.h>\n"
        "#include <stdbool.h>\n\n",
        schema->name, schema->name);
    
    // Write struct definition
    char struct_def[4096];
    int result = generate_struct_definition(schema, struct_def, sizeof(struct_def));
    if (result < 0) {
        fclose(header_file);
        return -1;
    }
    
    fprintf(header_file, "%s\n", struct_def);
    
    // Write accessor function prototypes
    fprintf(header_file,
        "/**\n"
        " * @brief Returns the number of records in the page\n"
        " * @return Number of records\n"
        " */\n"
        "int count(void);\n\n"
        "/**\n"
        " * @brief Returns a record at the specified position\n"
        " * @param pos Position of the record\n"
        " * @return Pointer to the record or NULL if out of bounds\n"
        " */\n"
        "%s* read(int pos);\n\n",
        schema->name);
    
    // Close header guard
    fprintf(header_file, "#endif /* UMBRA_STRUCT_%s_H */\n", schema->name);
    
    fclose(header_file);
    return 0;
}

/**
 * @brief Generate empty data page header file
 */
int generate_empty_data_page(const TableSchema* schema, const char* directory, int page_number) {
    if (!schema || !directory) {
        return -1;
    }
    
    // Create data directory
    char data_dir[1024];
    snprintf(data_dir, sizeof(data_dir), "%s/data", directory);
    if (ensure_directory(data_dir) != 0) {
        return -1;
    }
    
    // Create data file path
    char data_path[1024];
    snprintf(data_path, sizeof(data_path), "%s/%sData.%d.dat.h", 
             data_dir, schema->name, page_number);
    
    // Open data file
    FILE* data_file = fopen(data_path, "w");
    if (!data_file) {
        fprintf(stderr, "Failed to open %s for writing: %s\n", data_path, strerror(errno));
        return -1;
    }
    
    // Write header comment
    fprintf(data_file, "/*This file autogenerated, do not edit manually*/\n");
    
    // Close file
    fclose(data_file);
    return 0;
}

/**
 * @brief Generate accessor functions source file
 */
int generate_accessor_functions(const TableSchema* schema, const char* directory, int page_number) {
    if (!schema || !directory) {
        return -1;
    }
    
    // Create source directory
    char src_dir[1024];
    snprintf(src_dir, sizeof(src_dir), "%s/src", directory);
    if (ensure_directory(src_dir) != 0) {
        return -1;
    }
    
    // Create source file path
    char src_path[1024];
    snprintf(src_path, sizeof(src_path), "%s/%sData_%d.c", 
             src_dir, schema->name, page_number);
    
    // Open source file
    FILE* src_file = fopen(src_path, "w");
    if (!src_file) {
        fprintf(stderr, "Failed to open %s for writing: %s\n", src_path, strerror(errno));
        return -1;
    }
    
    // Write includes
    fprintf(src_file, 
        "#include <stdlib.h>\n"
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
    
    // Write count function
    fprintf(src_file,
        "/**\n"
        " * @brief Returns the number of records in the page\n"
        " * @return Number of records\n"
        " */\n"
        "int count(void) {\n"
        "    return sizeof(%sData_%d) / sizeof(%s);\n"
        "}\n\n",
        schema->name, page_number, schema->name);
    
    // Write read function
    fprintf(src_file,
        "/**\n"
        " * @brief Returns a record at the specified position\n"
        " * @param pos Position of the record\n"
        " * @return Pointer to the record or NULL if out of bounds\n"
        " */\n"
        "%s* read(int pos) {\n"
        "    if (pos < 0 || pos >= count()) {\n"
        "        return NULL;\n"
        "    }\n"
        "    return &%sData_%d[pos];\n"
        "}\n",
        schema->name,
        schema->name, page_number);
    
    // Close file
    fclose(src_file);
    return 0;
}
