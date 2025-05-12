/**
 * @file btree_index.h
 * @brief B-tree index implementation
 */

#ifndef UMBRA_BTREE_INDEX_H
#define UMBRA_BTREE_INDEX_H

#include <stdbool.h>
#include <stddef.h>
#include "../schema/type_system.h"

/**
 * @brief Order of the B-tree (max children per node)
 */
#define BTREE_ORDER 5

/**
 * @brief Key-value pair for building indexes
 */
typedef struct {
    void* key;
    int position;
} KeyValuePair;

/**
 * @brief B-tree node
 */
typedef struct BTreeNode {
    bool is_leaf;
    int key_count;              /* Number of keys in node */
    DataType key_type;          /* Type of keys stored */
    void* keys[BTREE_ORDER - 1]; /* Array of keys */
    int positions[BTREE_ORDER - 1]; /* Positions of records */
    struct BTreeNode* children[BTREE_ORDER]; /* Child nodes */
} BTreeNode;

/**
 * @brief B-tree index
 */
typedef struct {
    char column_name[64];       /* Column name for the index */
    DataType key_type;          /* Type of keys stored */
    int height;                 /* Height of the tree */
    BTreeNode* root;            /* Root node */
} BTreeIndex;

/**
 * @brief Initialize a B-tree index
 * @param column_name Name of the column being indexed
 * @param key_type Type of keys to store
 * @return Initialized B-tree index
 */
BTreeIndex* btree_init(const char* column_name, DataType key_type);

/**
 * @brief Free a B-tree index
 * @param index B-tree index to free
 */
void btree_free(BTreeIndex* index);

/**
 * @brief Insert a key-position pair into the B-tree
 * @param index B-tree index
 * @param key Key to insert
 * @param position Position of the record
 * @return 0 on success, -1 on error
 */
int btree_insert(BTreeIndex* index, void* key, int position);

/**
 * @brief Find positions of records with keys matching the search key
 * @param index B-tree index
 * @param key Search key
 * @param positions Output array for positions
 * @param max_positions Maximum number of positions to return
 * @return Number of positions found, or -1 on error
 */
int btree_find_exact(const BTreeIndex* index, const void* key, 
                     int* positions, int max_positions);

/**
 * @brief Find positions of records with keys in the given range
 * @param index B-tree index
 * @param low_key Lower bound key (NULL for no lower bound)
 * @param high_key Upper bound key (NULL for no upper bound)
 * @param positions Output array for positions
 * @param max_positions Maximum number of positions to return
 * @return Number of positions found, or -1 on error
 */
int btree_find_range(const BTreeIndex* index, const void* low_key, const void* high_key,
                     int* positions, int max_positions);

/**
 * @brief Generate C code for a B-tree index
 * @param index B-tree index
 * @param output Output buffer for code
 * @param output_size Size of output buffer
 * @return Number of bytes written, or -1 on error
 */
int btree_generate_code(const BTreeIndex* index, char* output, size_t output_size);

/**
 * @brief Build a B-tree index from key-value pairs
 * @param pairs Array of key-value pairs
 * @param pair_count Number of pairs
 * @param column_name Name of the column being indexed
 * @param key_type Type of keys to store
 * @return Initialized B-tree index, or NULL on error
 */
BTreeIndex* btree_build_from_pairs(const KeyValuePair* pairs, int pair_count,
                                 const char* column_name, DataType key_type);

#endif /* UMBRA_BTREE_INDEX_H */
