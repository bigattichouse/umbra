/**
 * @file hash_index.c
 * @brief Hash index implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>  /* Add this for uintptr_t */
#include "hash_index.h"
#include "../schema/schema_parser.h"
#include "../schema/type_system.h"

/**
 * @brief Compute hash value for a key
 */
static unsigned int hash_key(const void* key, DataType key_type) {
    unsigned int hash = 5381; // djb2 hash function
    
    switch (key_type) {
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
            while (*str) {
                hash = ((hash << 5) + hash) + *str++;
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
            // Unknown type, use pointer value as hash
            uintptr_t key_val = (uintptr_t)key;
            hash = ((hash << 5) + hash) + key_val;
            break;
    }
    
    return hash;
}

/**
 * @brief Compare two keys
 */
static int compare_keys(const void* key1, const void* key2, DataType key_type) {
    switch (key_type) {
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
        case TYPE_TEXT: {
            return strcmp((const char*)key1, (const char*)key2);
        }
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
            // Unknown type, compare pointer values
            return ((key1 > key2) - (key1 < key2));
    }
}

/**
 * @brief Duplicate a key
 */
static void* duplicate_key(const void* key, DataType key_type) {
    if (!key) {
        return NULL;
    }
    
    switch (key_type) {
        case TYPE_INT: {
            int* dup = malloc(sizeof(int));
            if (dup) {
                *dup = *(const int*)key;
            }
            return dup;
        }
        case TYPE_FLOAT: {
            double* dup = malloc(sizeof(double));
            if (dup) {
                *dup = *(const double*)key;
            }
            return dup;
        }
        case TYPE_VARCHAR:
        case TYPE_TEXT: {
            return strdup((const char*)key);
        }
        case TYPE_BOOLEAN: {
            bool* dup = malloc(sizeof(bool));
            if (dup) {
                *dup = *(const bool*)key;
            }
            return dup;
        }
        case TYPE_DATE: {
            time_t* dup = malloc(sizeof(time_t));
            if (dup) {
                *dup = *(const time_t*)key;
            }
            return dup;
        }
        default:
            // Unknown type, return NULL
            return NULL;
    }
}

/**
 * @brief Initialize a hash index
 */
int hash_init(HashIndex* index, const char* column_name, DataType key_type, size_t size) {
    if (!index || !column_name) {
        return -1;
    }
    
    if (size == 0) {
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
void hash_free(HashIndex* index) {
    if (!index) {
        return;
    }
    
    for (size_t i = 0; i < index->size; i++) {
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
}

/**
 * @brief Insert a key-position pair into the hash index
 */
int hash_insert(HashIndex* index, const void* key, int position) {
    if (!index || !key) {
        return -1;
    }
    
    unsigned int h = hash_key(key, index->key_type) % index->size;
    
    // Create new entry
    HashEntry* entry = malloc(sizeof(HashEntry));
    if (!entry) {
        return -1;
    }
    
    // Duplicate the key
    entry->key = duplicate_key(key, index->key_type);
    if (!entry->key) {
        free(entry);
        return -1;
    }
    
    entry->position = position;
    
    // Insert at the front of the chain
    entry->next = index->table[h];
    index->table[h] = entry;
    index->entry_count++;
    
    return 0;
}

/**
 * @brief Find positions for a key in the hash index
 */
int hash_find(const HashIndex* index, const void* key, int* positions, int max_positions) {
    if (!index || !key || !positions || max_positions <= 0) {
        return 0;
    }
    
    unsigned int h = hash_key(key, index->key_type) % index->size;
    
    int count = 0;
    
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
 * @brief Generate C code for a hash index
 */
int hash_generate_code(const HashIndex* index, char* output, size_t output_size, 
                       const char* function_name) {
    if (!index || !output || output_size <= 0 || !function_name) {
        return -1;
    }
    
    int written = 0;
    
    // Generate hash entries
    written += snprintf(output + written, output_size - written,
        "/**\n"
        " * @brief Generated hash index entries\n"
        " */\n"
        "static struct {\n"
        "    void* key;\n"
        "    int position;\n"
        "    int next_entry;\n"
        "} hash_entries[] = {\n");
    
    int entry_idx = 0;
    
    for (size_t i = 0; i < index->size; i++) {
        HashEntry* entry = index->table[i];
        
        while (entry) {
            if (entry_idx > 0) {
                written += snprintf(output + written, output_size - written, ",\n");
            }
            
            written += snprintf(output + written, output_size - written,
                "    /* Entry %d */\n"
                "    {\n"
                "        .key = ", entry_idx);
            
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
                    "        .next_entry = %d\n", entry_idx + 1);
            } else {
                written += snprintf(output + written, output_size - written,
                    "        .next_entry = -1\n");
            }
            
            written += snprintf(output + written, output_size - written, "    }");
            
            entry = entry->next;
            entry_idx++;
        }
    }
    
    written += snprintf(output + written, output_size - written, "\n};\n\n");
    
    // Generate hash table
    written += snprintf(output + written, output_size - written,
        "/**\n"
        " * @brief Generated hash table\n"
        " */\n"
        "static int hash_table[] = {\n    ");
    
    for (size_t i = 0; i < index->size; i++) {
        if (i > 0) {
            if (i % 8 == 0) {
                written += snprintf(output + written, output_size - written, ",\n    ");
            } else {
                written += snprintf(output + written, output_size - written, ", ");
            }
        }
        
        if (index->table[i]) {
            // Find the first entry index for this bucket
            int first_idx = 0;
            HashEntry* entry = index->table[0];
            
            while (entry != index->table[i]) {
                first_idx++;
                entry = index->table[first_idx];
            }
            
            written += snprintf(output + written, output_size - written, "%d", first_idx);
        } else {
            written += snprintf(output + written, output_size - written, "-1");
        }
    }
    
    written += snprintf(output + written, output_size - written, "\n};\n\n");
    
    // Generate lookup function
    written += snprintf(output + written, output_size - written,
        "/**\n"
        " * @brief Generated hash index lookup function\n"
        " */\n"
        "int %s(const void* key, int* positions, int max_positions) {\n"
        "    /* Hash index for column %s */\n"
        "    /* Size: %zu, Type: %d, Entries: %d */\n"
        "    \n"
        "    /* Calculate hash */\n"
        "    unsigned int hash = 5381;\n"
        "    /* TODO: Generate hash calculation based on key type */\n"
        "    \n"
        "    /* Find matching entries */\n"
        "    int count = 0;\n"
        "    int entry_idx = hash_table[hash %% %zu];\n"
        "    \n"
        "    while (entry_idx >= 0 && count < max_positions) {\n"
        "        /* TODO: Compare keys based on type */\n"
        "        positions[count++] = hash_entries[entry_idx].position;\n"
        "        entry_idx = hash_entries[entry_idx].next_entry;\n"
        "    }\n"
        "    \n"
        "    return count;\n"
        "}\n",
        function_name,
        index->column_name, index->size, index->key_type,
        index->entry_count, index->size);
    
    return written;
}

/**
 * @brief Build a hash index from an array of key-value pairs
 */
HashIndex* hash_build_from_pairs(const KeyValuePair* pairs, int pair_count,
                               const char* column_name, DataType key_type) {
    if (!pairs || pair_count <= 0 || !column_name) {
        return NULL;
    }
    
    // Calculate appropriate size (at least 2x the number of pairs)
    size_t size = pair_count * 2;
    if (size < DEFAULT_HASH_SIZE) {
        size = DEFAULT_HASH_SIZE;
    }
    
    // Create and initialize index
    HashIndex* index = malloc(sizeof(HashIndex));
    if (!index) {
        return NULL;
    }
    
    if (hash_init(index, column_name, key_type, size) != 0) {
        free(index);
        return NULL;
    }
    
    // Insert pairs
    for (int i = 0; i < pair_count; i++) {
        if (hash_insert(index, pairs[i].key, pairs[i].position) != 0) {
            hash_free(index);
            free(index);
            return NULL;
        }
    }
    
    return index;
}
