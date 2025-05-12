/**
 * @file btree_index.h
 * @brief B-tree index implementation
 */

#ifndef UMBRA_BTREE_INDEX_H
#define UMBRA_BTREE_INDEX_H

#include <stdbool.h>
#include <time.h>
#include "../schema/type_system.h"

/**
 * @brief Default order for B-tree (maximum number of children per node)
 */
#define BTREE_DEFAULT_ORDER 5

/**
 * @struct BTreeEntry
 * @brief Entry in a B-tree node
 */
typedef struct {
    void* key;            /**< Key pointer */
    int position;         /**< Position in the data file */
} BTreeEntry;

/**
 * @struct BTreeNode
 * @brief Node in a B-tree
 */
typedef struct BTreeNode {
    int entry_count;              /**< Number of entries in this node */
    BTreeEntry* entries;          /**< Array of entries */
    struct BTreeNode** children;  /**< Array of child nodes */
    bool is_leaf;                 /**< Whether this is a leaf node */
    int order;                    /**< B-tree order (max children) */
} BTreeNode;

/**
 * @struct BTreeIndex
 * @brief B-tree index
 */
typedef struct {
    char column_name[64];   /**< Column name this index is for */
    BTreeNode* root;        /**< Root node of the B-tree */
    int order;              /**< B-tree order (max children) */
    int node_count;         /**< Number of nodes in the tree */
    int entry_count;        /**< Number of entries in the index */
    DataType key_type;      /**< Type of keys in this index */
} BTreeIndex;

/**
 * @brief Initialize a B-tree index
 * @param index B-tree index to initialize
 * @param column_name Column name this index is for
 * @param key_type Type of keys in this index
 * @param order B-tree order (max children, 0 for default)
 * @return 0 on success, -1 on error
 */
int btree_init(BTreeIndex* index, const char* column_name, DataType key_type, int order);

/**
 * @brief Free resources used by a B-tree index
 * @param index B-tree index to free
 */
void btree_free(BTreeIndex* index);

/**
 * @brief Insert a key-position pair into the B-tree index
 * @param index B-tree index to insert into
 * @param key Key pointer
 * @param position Position in the data file
 * @return 0 on success, -1 on error
 */
int btree_insert(BTreeIndex* index, const void* key, int position);

/**
 * @brief Find positions for a key in the B-tree index
 * @param index B-tree index to search
 * @param key Key to search for
 * @param positions Array to store found positions
 * @param max_positions Maximum number of positions to return
 * @return Number of positions found
 */
int btree_find(const BTreeIndex* index, const void* key, int* positions, int max_positions);

/**
 * @brief Generate C code for a B-tree index
 * @param index B-tree index to generate code for
 * @param output Buffer to store generated code
 * @param output_size Size of output buffer
 * @param function_name Name of generated lookup function
 * @return Number of bytes written, or -1 on error
 */
int btree_generate_code(const BTreeIndex* index, char* output, size_t output_size, 
                       const char* function_name);

/**
 * @brief Build a B-tree index from an array of key-value pairs
 * @param pairs Array of key-value pairs
 * @param pair_count Number of pairs
 * @param column_name Column name this index is for
 * @param key_type Type of keys in this index
 * @return New B-tree index, or NULL on error
 */
BTreeIndex* btree_build_from_pairs(const KeyValuePair* pairs, int pair_count,
                                 const char* column_name, DataType key_type);

#endif /* UMBRA_BTREE_INDEX_H */
