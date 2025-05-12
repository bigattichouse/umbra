/**
 * @file btree_index.h
 * @brief B-tree index implementation
 */

#ifndef UMBRA_BTREE_INDEX_H
#define UMBRA_BTREE_INDEX_H

#include "index_types.h"

/**
 * @brief Initialize a B-tree index
 * @param index Index to initialize
 * @param key_type Type of keys
 * @param column_name Name of indexed column
 * @return 0 on success, -1 on error
 */
int btree_init(BTreeIndex* index, DataType key_type, const char* column_name);

/**
 * @brief Free resources used by a B-tree index
 * @param index Index to free
 * @return 0 on success, -1 on error
 */
int btree_free(BTreeIndex* index);

/**
 * @brief Insert a key-position pair into the index
 * @param index Index to insert into
 * @param key Key to insert
 * @param position Record position
 * @return 0 on success, -1 on error
 */
int btree_insert(BTreeIndex* index, void* key, int position);

/**
 * @brief Find positions for an exact key match
 * @param index Index to search
 * @param key Key to find
 * @param positions Output array for positions
 * @param max_positions Maximum positions to return
 * @return Number of positions found, or -1 on error
 */
int btree_find_exact(const BTreeIndex* index, const void* key, 
                    int* positions, int max_positions);

/**
 * @brief Find positions for a range of keys
 * @param index Index to search
 * @param low_key Lower bound (inclusive, NULL for unbounded)
 * @param high_key Upper bound (inclusive, NULL for unbounded)
 * @param positions Output array for positions
 * @param max_positions Maximum positions to return
 * @return Number of positions found, or -1 on error
 */
int btree_find_range(const BTreeIndex* index, const void* low_key, 
                    const void* high_key, int* positions, int max_positions);

/**
 * @brief Generate C code for B-tree index
 * @param index Index to generate code for
 * @param output Output buffer
 * @param output_size Size of output buffer
 * @return Number of bytes written, or -1 on error
 */
int btree_generate_code(const BTreeIndex* index, char* output, size_t output_size);

/**
 * @brief Build a B-tree index from sorted key-value pairs
 * @param pairs Sorted key-value pairs
 * @param pair_count Number of pairs
 * @param key_type Type of keys
 * @param column_name Name of indexed column
 * @return Initialized B-tree index, or NULL on error
 */
BTreeIndex* btree_build_from_sorted(const KeyValuePair* pairs, int pair_count,
                                   DataType key_type, const char* column_name);

#endif /* UMBRA_BTREE_INDEX_H */
