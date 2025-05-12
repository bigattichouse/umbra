/**
 * @file hash_index.h
 * @brief Hash index implementation
 */

#ifndef UMBRA_HASH_INDEX_H
#define UMBRA_HASH_INDEX_H

#include <stdbool.h>
#include <time.h>
#include "../schema/type_system.h"

/**
 * @brief Default size for hash tables
 */
#define DEFAULT_HASH_SIZE 1024

/**
 * @struct KeyValuePair
 * @brief Represents a key-value pair for index building
 */
typedef struct {
    void* key;            /**< Key pointer */
    int position;         /**< Position in the data file */
} KeyValuePair;

/**
 * @struct HashEntry
 * @brief Entry in a hash table
 */
typedef struct HashEntry {
    void* key;                /**< Key pointer */
    int position;             /**< Position in the data file */
    struct HashEntry* next;   /**< Next entry in the chain (for collision resolution) */
} HashEntry;

/**
 * @struct HashIndex
 * @brief Hash-based index
 */
typedef struct {
    char column_name[64];   /**< Column name this index is for */
    HashEntry** table;      /**< Hash table (array of entry chains) */
    size_t size;            /**< Size of hash table */
    int entry_count;        /**< Number of entries in the index */
    DataType key_type;      /**< Type of keys in this index */
} HashIndex;

/**
 * @brief Initialize a hash index
 * @param index Hash index to initialize
 * @param column_name Column name this index is for
 * @param key_type Type of keys in this index
 * @param size Initial size of hash table (0 for default)
 * @return 0 on success, -1 on error
 */
int hash_init(HashIndex* index, const char* column_name, DataType key_type, size_t size);

/**
 * @brief Free resources used by a hash index
 * @param index Hash index to free
 */
void hash_free(HashIndex* index);

/**
 * @brief Insert a key-position pair into the hash index
 * @param index Hash index to insert into
 * @param key Key pointer
 * @param position Position in the data file
 * @return 0 on success, -1 on error
 */
int hash_insert(HashIndex* index, const void* key, int position);

/**
 * @brief Find positions for a key in the hash index
 * @param index Hash index to search
 * @param key Key to search for
 * @param positions Array to store found positions
 * @param max_positions Maximum number of positions to return
 * @return Number of positions found
 */
int hash_find(const HashIndex* index, const void* key, int* positions, int max_positions);

/**
 * @brief Generate C code for a hash index
 * @param index Hash index to generate code for
 * @param output Buffer to store generated code
 * @param output_size Size of output buffer
 * @param function_name Name of generated lookup function
 * @return Number of bytes written, or -1 on error
 */
int hash_generate_code(const HashIndex* index, char* output, size_t output_size, 
                       const char* function_name);

/**
 * @brief Build a hash index from an array of key-value pairs
 * @param pairs Array of key-value pairs
 * @param pair_count Number of pairs
 * @param column_name Column name this index is for
 * @param key_type Type of keys in this index
 * @return New hash index, or NULL on error
 */
HashIndex* hash_build_from_pairs(const KeyValuePair* pairs, int pair_count,
                               const char* column_name, DataType key_type);

#endif /* UMBRA_HASH_INDEX_H */
