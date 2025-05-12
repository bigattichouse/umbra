/**
 * @file hash_index.c
 * @brief Implements hash tables for indexing
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hash_index.h"
#include "../schema/type_system.h"

/**
 * @brief Hash function for different key types
 */
static unsigned int hash_key(const void* key, DataType type) {
    unsigned int hash = 5381;
    
    switch (type) {
        case TYPE_INT: {
            int val = *(const int*)key;
            hash = ((hash << 5) + hash) + val;
            break;
        }
        case TYPE_FLOAT: {
            double val = *(const double*)key;
            unsigned char* bytes = (unsigned char*)&val;
            for (size_t i = 0; i < sizeof(double); i++) {
                hash = ((hash << 5) + hash) + bytes[i];
            }
            break;
        }
        case TYPE_VARCHAR:
        case TYPE_TEXT: {
            const char* str = (const char*)key;
            unsigned char c;
            while ((c = *str++)) {
                hash = ((hash << 5) + hash) + c;
            }
            break;
        }
        case TYPE_BOOLEAN: {
            bool val = *(const bool*)key;
            hash = ((hash << 5) + hash) + (val ? 1 : 0);
            break;
        }
        case TYPE_DATE: {
            time_t val = *(const time_t*)key;
            unsigned char* bytes = (unsigned char*)&val;
            for (size_t i = 0; i < sizeof(time_t); i++) {
                hash = ((hash << 5) + hash) + bytes[i];
            }
            break;
        }
        default:
            break;
    }
    
    return hash;
}

/**
 * @brief Compare keys based on data type
 */
static int compare_keys(const void* key1, const void* key2, DataType type) {
    switch (type) {
        case TYPE_INT: {
            int a = *(const int*)key1;
            int b = *(const int*)key2;
            return (a > b) - (a < b);
        }
        case TYPE_FLOAT: {
            double a = *(const double*)key1;
            double b = *(const double*)key2;
            return (a > b) - (a < b);
        }
        case TYPE_VARCHAR:
        case TYPE_TEXT:
            return strcmp((const char*)key1, (const char*)key2);
        case TYPE_BOOLEAN: {
            bool a = *(const bool*)key1;
            bool b = *(const bool*)key2;
            return (a > b) - (a < b);
        }
        case TYPE_DATE: {
            time_t a = *(const time_t*)key1;
            time_t b = *(const time_t*)key2;
            return (a > b) - (a < b);
        }
        default:
            return 0;
    }
}

/**
 * @brief Duplicate a key value
 */
static void* duplicate_key(const void* key, DataType type) {
    size_t size;
    
    switch (type) {
        case TYPE_INT:
            size = sizeof(int);
            break;
        case TYPE_FLOAT:
            size = sizeof(double);
            break;
        case TYPE_BOOLEAN:
            size = sizeof(bool);
            break;
        case TYPE_DATE:
            size = sizeof(time_t);
            break;
        case TYPE_VARCHAR:
        case TYPE_TEXT:
            size = strlen((const char*)key) + 1;
            break;
        default:
            return NULL;
    }
    
    void* new_key = malloc(size);
    if (!new_key) return NULL;
    
    memcpy(new_key, key, size);
    return new_key;
}

/**
 * @brief Initialize a hash index
 */
int hash_init(HashIndex* index, DataType key_type, const char* column_name, int size) {
    if (!index || !column_name) {
        return -1;
    }
    
    if (size <= 0) {
        size = DEFAULT_HASH_SIZE;
    }
    
    index->table = calloc(size, sizeof(HashEntry*));
    if (!index->table) {
        return -1;
    }
    
    index->size = size;
    index->key_type = key_type;
    index->entry_count = 0;
    
    strncpy(index->column_name, column_name, sizeof(index->column_name) - 1);
    index->column_name[sizeof(index->column_name) - 1] = '\0';
    
    return 0;
}

/**
 * @brief Free resources used by a hash index
 */
int hash_free(HashIndex* index) {
    if (!index) {
        return -1;
    }
    
    // Free all entries and chains
    for (int i = 0; i < index->size; i++) {
        HashEntry* entry = index->table[i];
        
        while (entry) {
            HashEntry* next = entry->next;
            free(entry->key);
            free(entry);
            entry = next;
        }
    }
    
    free(index->table);
    index->table = NULL;
    index->size = 0;
    index->entry_count = 0;
    
    return 0;
}

/**
 * @brief Insert a key-position pair into the index
 */
int hash_insert(HashIndex* index, void* key, int position) {
    if (!index || !key) {
        return -1;
    }
    
    unsigned int h = hash_key(key, index->key_type) % index->size;
    
    // Create new entry
    HashEntry* entry = malloc(sizeof(HashEntry));
    if (!entry) {
        return -1;
    }
    
    entry->key = duplicate_key(key, index->key_type);
    if (!entry->key) {
        free(entry);
        return -1;
    }
    
    entry->position = position;
    
    // Insert at head of chain
    entry->next = index->table[h];
    index->table[h] = entry;
    index->entry_count++;
    
    return 0;
}

/**
 * @brief Find positions for an exact key match
 */
int hash_find(const HashIndex* index, const void* key, 
             int* positions, int max_positions) {
    if (!index || !key || !positions || max_positions <= 0) {
        return -1;
    }
    
    unsigned int h = hash_key(key, index->key_type) % index->size;
    int count = 0;
    
    // Traverse chain
    HashEntry* entry = index->table[h];
    
    while (entry && count < max_positions) {
        if (compare_keys(entry->key, key, index->key_type) == 0) {
            positions[count++] = entry->position;
        }
        
        entry = entry->next;
    }
    
    return count;
}

/**
 * @brief Generate C code for hash index
 */
int hash_generate_code(const HashIndex* index, char* output, size_t output_size) {
    if (!index || !output || output_size <= 0) {
        return -1;
    }
    
    int written = 0;
    
    // Generate hash entries
    for (int i = 0; i < index->size; i++) {
        HashEntry* entry = index->table[i];
        int entry_id = 0;
        
        while (entry) {
            written += snprintf(output + written, output_size - written,
                "static HashEntry hash_entry_%d_%d = {\n"
                "    .key = ",
                i, entry_id);
            
            // Generate key based on type
            switch (index->key_type) {
                case TYPE_INT:
                    written += snprintf(output + written, output_size - written, 
                                     "&(int){%d}", *(int*)entry->key);
                    break;
                case TYPE_FLOAT:
                    written += snprintf(output + written, output_size - written, 
                                     "&(double){%f}", *(double*)entry->key);
                    break;
                case TYPE_VARCHAR:
                case TYPE_TEXT:
                    written += snprintf(output + written, output_size - written, 
                                     "\"%s\"", (char*)entry->key);
                    break;
                case TYPE_BOOLEAN:
                    written += snprintf(output + written, output_size - written, 
                                     "&(bool){%s}", *(bool*)entry->key ? "true" : "false");
                    break;
                case TYPE_DATE:
                    written += snprintf(output + written, output_size - written, 
                                     "&(time_t){%ld}", *(time_t*)entry->key);
                    break;
                default:
                    written += snprintf(output + written, output_size - written, "NULL");
                    break;
            }
            
            written += snprintf(output + written, output_size - written,
                ",\n    .position = %d,\n", entry->position);
            
            // Link to next entry
            if (entry->next) {
                written += snprintf(output + written, output_size - written,
                    "    .next = &hash_entry_%d_%d\n", i, entry_id + 1);
            } else {
                written += snprintf(output + written, output_size - written,
                    "    .next = NULL\n");
            }
            
            written += snprintf(output + written, output_size - written, "};\n\n");
            
            entry = entry->next;
            entry_id++;
        }
    }
    
    // Generate table array
    written += snprintf(output + written, output_size - written,
        "static HashEntry* hash_table[] = {\n");
    
    for (int i = 0; i < index->size; i++) {
        if (i > 0) {
            written += snprintf(output + written, output_size - written, ",\n");
        }
        
        if (index->table[i]) {
            written += snprintf(output + written, output_size - written, 
                              "    &hash_entry_%d_0", i);
        } else {
            written += snprintf(output + written, output_size - written, "    NULL");
        }
    }
    
    written += snprintf(output + written, output_size - written, "\n};\n\n");
    
    // Generate index structure
    written += snprintf(output + written, output_size - written,
        "HashIndex %s_hash_index = {\n"
        "    .table = hash_table,\n"
        "    .size = %d,\n"
        "    .key_type = %d,\n"
        "    .entry_count = %d,\n"
        "    .column_name = \"%s\"\n"
        "};\n",
        index->column_name, index->size, index->key_type, 
        index->entry_count, index->column_name);
    
    return written;
}

/**
 * @brief Build a hash index from key-value pairs
 */
HashIndex* hash_build_from_pairs(const KeyValuePair* pairs, int pair_count,
                               DataType key_type, const char* column_name) {
    if (!pairs || pair_count <= 0 || !column_name) {
        return NULL;
    }
    
    // Calculate a reasonable hash size based on pair count
    int size = pair_count * 2;
    if (size < DEFAULT_HASH_SIZE) {
        size = DEFAULT_HASH_SIZE;
    }
    
    HashIndex* index = malloc(sizeof(HashIndex));
    if (!index) {
        return NULL;
    }
    
    if (hash_init(index, key_type, column_name, size) != 0) {
        free(index);
        return NULL;
    }
    
    // Insert all pairs into the hash table
    for (int i = 0; i < pair_count; i++) {
        if (hash_insert(index, pairs[i].key, pairs[i].position) != 0) {
            hash_free(index);
            free(index);
            return NULL;
        }
    }
    
    return index;
}
