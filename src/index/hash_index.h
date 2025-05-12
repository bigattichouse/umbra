/**
 * @file hash_index.h
 * @brief Hash index implementation
 */

#ifndef UMBRA_HASH_INDEX_H
#define UMBRA_HASH_INDEX_H

#include "index_types.h"

/**
 * @brief Initialize a hash index
 * @param index Index to initialize
 * @param key_type Type of keys
 * @param column_name Name of indexed column
 * @param size Initial size of hash table
 * @return 0 on success, -1 on error
 */
int hash_init(HashIndex* index, DataType key_type, const char* column_name, int size);

/**
 * @brief Free resources used by a hash index
 * @param index Index to free
 * @return 0 on success, -1 on error
 */
int hash_free(HashIndex* index);

/**
 * @brief Insert a key-position pair into the index
 * @param index Index to insert into
 * @param key Key to insert
 * @param position Record position
 * @return 0 on success, -1 on error
 */
int hash_insert(HashIndex* index, void* key, int position);

/**
 * @brief Find positions for an exact key match
 * @param index Index to search
 * @param key Key to find
 * @param positions Output array for positions
 * @param max_positions Maximum positions to return
 * @return Number of positions found, or -1 on error
 */
int hash_find(const HashIndex* index, const void* key, 
             int* positions, int max_positions);

/**
 * @brief Generate C code for hash index
 * @param index Index to generate code for
 * @param output Output buffer
 * @param output_size Size of output buffer
 * @return Number of bytes written, or -1 on error
 */
int hash_generate_code(const HashIndex* index, char* output, size_t output_size);

/**
 * @brief Build a hash index from key-value pairs
 * @param pairs Key-value pairs
 * @param pair_count Number of pairs
 * @param key_type Type of keys
 * @param column_name Name of indexed column
 * @return Initialized hash index, or NULL on error
 */
HashIndex* hash_build_from_pairs(const KeyValuePair* pairs, int pair_count,
                               DataType key_type, const char* column_name);

#endif /* UMBRA_HASH_INDEX_H */
