/**
 * @file metadata.c
 * @brief Metadata structure implementations
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <errno.h>
#include "metadata.h"

/**
 * @brief Create metadata for a new table
 */
TableMetadata create_table_metadata(const TableSchema* schema, const char* creator, int page_size) {
    TableMetadata metadata;
    
    strncpy(metadata.name, schema->name, sizeof(metadata.name) - 1);
    metadata.name[sizeof(metadata.name) - 1] = '\0';
    
    time(&metadata.created_at);
    metadata.modified_at = metadata.created_at;
    
    strncpy(metadata.creator, creator, sizeof(metadata.creator) - 1);
    metadata.creator[sizeof(metadata.creator) - 1] = '\0';
    
    metadata.page_count = 0;
    metadata.record_count = 0;
    metadata.page_size = page_size;
    
    return metadata;
}

/**
 * @brief Save table metadata to file
 */
int save_table_metadata(const TableMetadata* metadata, const char* directory) {
    if (!metadata || !directory) {
        return -1;
    }
    
    // Create metadata directory if it doesn't exist
    char metadata_dir[1024];
    snprintf(metadata_dir, sizeof(metadata_dir), "%s/tables/%s/metadata", 
             directory, metadata->name);
    
    struct stat st;
    if (stat(metadata_dir, &st) != 0) {
        if (mkdir(metadata_dir, 0755) != 0) {
            fprintf(stderr, "Failed to create metadata directory: %s\n", strerror(errno));
            return -1;
        }
    }
    
    // Create metadata file path
    char metadata_path[2048];
    snprintf(metadata_path, sizeof(metadata_path), "%s/table_metadata.dat", metadata_dir);
    
    // Write metadata to file
    FILE* file = fopen(metadata_path, "wb");
    if (!file) {
        fprintf(stderr, "Failed to open metadata file for writing: %s\n", strerror(errno));
        return -1;
    }
    
    size_t written = fwrite(metadata, sizeof(TableMetadata), 1, file);
    fclose(file);
    
    if (written != 1) {
        fprintf(stderr, "Failed to write metadata\n");
        return -1;
    }
    
    return 0;
}

/**
 * @brief Load table metadata from file
 */
int load_table_metadata(const char* table_name, const char* directory, TableMetadata* metadata) {
    if (!table_name || !directory || !metadata) {
        return -1;
    }
    
    // Create metadata file path
    char metadata_path[2048];
    snprintf(metadata_path, sizeof(metadata_path), "%s/tables/%s/metadata/table_metadata.dat", 
             directory, table_name);
    
    // Read metadata from file
    FILE* file = fopen(metadata_path, "rb");
    if (!file) {
        // File doesn't exist
        return -1;
    }
    
    size_t read = fread(metadata, sizeof(TableMetadata), 1, file);
    fclose(file);
    
    if (read != 1) {
        return -1;
    }
    
    return 0;
}

/**
 * @brief Update table metadata
 */
int update_table_metadata(TableMetadata* metadata, const char* directory) {
    if (!metadata || !directory) {
        return -1;
    }
    
    // Update modification time
    time(&metadata->modified_at);
    
    // Save updated metadata
    return save_table_metadata(metadata, directory);
}
