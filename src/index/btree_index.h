/**
 * @file btree_index.h
 * @brief B-tree index implementation
 */

#ifndef UMBRA_BTREE_INDEX_H
#define UMBRA_BTREE_INDEX_H

#include <stdbool.h>
#include "../schema/type_system.h"

/**
 * @brief Order of the B-tree (max children per node)
 */
#define BTREE_ORDER 5

/**
 * @struct KeyValuePair
 * @brief Key-value pair for index entries
 */
typedef struct {
    void* key;          /**< Key value */
    int position;       /**< Record position */
} KeyValuePair;

/**
 * @struct BTreeNode
 * @brief Node in a B-tree index
 */
typedef struct BTreeNode {
    bool is_leaf;                           /**< Whether this is a leaf node */
    int key_count;                          /**< Number of keys in node */
    void* keys[BTREE_ORDER - 1];            /**< Keys in node */
    int positions[BTREE_ORDER - 1];         /**< Record positions for keys */
    struct BTreeNode* children[BTREE_ORDER]; /**< Child nodes */
    DataType key_type;                      /**< Type of keys */
} BTreeNode;

/**
 * @struct BTreeIndex
 * @brief B-tree index for a table column
 */
typedef struct {
    BTreeNode* root;            /**< Root node of the B-tree */
    char column_name[64];       /**< Column being indexed */
    DataType key_type;          /**< Type of keys */
    int height;                 /**< Height of the tree */
    int node_count;             /**< Number of nodes in the tree */
} BTreeIndex;

/**
 * @brief Initialize a B-tree index
 * @param column_name Column to index
 * @param key_type Type of keys
 * @return Initialized B-tree index or NULL on error
 */
BTreeIndex* btree_init(const char* column_name, DataType key_type);

/**
 * @brief Free a B-tree index
 * @param index Index to free
 */
void btree_free(BTreeIndex* index);

/**
 * @brief Insert a key-position pair into the index
 * @param index B-tree index
 * @param key Key to insert
 * @param position Position of record with key
 * @return 0 on success, -1 on error
 */
int btree_insert(BTreeIndex* index, void* key, int position);

/**
 * @brief Find exact matches for a key
 * @param index B-tree index
 * @param key Key to search for
 * @param positions Array to store record positions
 * @param max_positions Maximum number of positions
 * @return Number of matching positions or -1 on error
 */
int btree_find_exact(const BTreeIndex* index, const void* key, 
                    int* positions, int max_positions);

/**
 * @brief Find keys in a range
 * @param index B-tree index
 * @param low_key Lower bound (inclusive)
 * @param high_key Upper bound (inclusive)
 * @param positions Array to store record positions
 * @param max_positions Maximum number of positions
 * @return Number of matching positions or -1 on error
 */
int btree_find_range(const BTreeIndex* index, const void* low_key, 
                    const void* high_key, int* positions, int max_positions);

/**
 * @brief Generate C code for a B-tree index
 * @param index B-tree index
 * @param output Buffer for C code
 * @param output_size Size of output buffer
 * @return 0 on success, -1 on error
 */
int btree_generate_code(const BTreeIndex* index, char* output, size_t output_size);

/**
 * @brief Build a B-tree from sorted key-value pairs
 * @param pairs Array of key-value pairs
 * @param pair_count Number of pairs
 * @param column_name Column being indexed
 * @param key_type Type of keys
 * @return Initialized B-tree index or NULL on error
 */
BTreeIndex* btree_build_from_sorted(const KeyValuePair* pairs, int pair_count,
                                   const char* column_name, DataType key_type);

#endif /* UMBRA_BTREE_INDEX_H */
