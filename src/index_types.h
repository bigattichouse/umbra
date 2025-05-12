/**
 * @file index_types.h
 * @brief Index structure definitions
 */

#ifndef UMBRA_INDEX_TYPES_H
#define UMBRA_INDEX_TYPES_H

#include <stdbool.h>
#include "../schema/type_system.h"

/**
 * @brief Maximum order for B-tree (children per node)
 */
#define BTREE_ORDER 5

/**
 * @brief Default hash table size
 */
#define DEFAULT_HASH_SIZE 101

/**
 * @enum IndexType
 * @brief Types of indices
 */
typedef enum {
    INDEX_BTREE,
    INDEX_HASH
} IndexType;

/**
 * @struct IndexDefinition
 * @brief Defines an index on a table column
 */
typedef struct {
    char table_name[64];    /**< Table name */
    char column_name[64];   /**< Column name */
    char index_name[64];    /**< Index name */
    IndexType type;         /**< Index type */
    bool unique;            /**< Whether index enforces uniqueness */
    bool primary;           /**< Whether index is for primary key */
} IndexDefinition;

/**
 * @struct BTreeNode
 * @brief Node in a B-tree index
 */
typedef struct BTreeNode {
    int key_count;                      /**< Number of keys in node */
    DataType key_type;                  /**< Type of keys */
    void* keys[BTREE_ORDER - 1];        /**< Keys in node */
    int positions[BTREE_ORDER - 1];     /**< Record positions for each key */
    struct BTreeNode* children[BTREE_ORDER]; /**< Child nodes */
    bool is_leaf;                       /**< Whether node is a leaf */
} BTreeNode;

/**
 * @struct BTreeIndex
 * @brief B-tree index structure
 */
typedef struct {
    BTreeNode* root;       /**< Root node of B-tree */
    DataType key_type;     /**< Type of keys */
    int height;            /**< Height of the tree */
    int node_count;        /**< Number of nodes */
    char column_name[64];  /**< Indexed column name */
} BTreeIndex;

/**
 * @struct HashEntry
 * @brief Entry in a hash index
 */
typedef struct HashEntry {
    void* key;                 /**< Key value */
    int position;              /**< Record position */
    struct HashEntry* next;    /**< Next entry in chain (for collisions) */
} HashEntry;

/**
 * @struct HashIndex
 * @brief Hash table index
 */
typedef struct {
    HashEntry** table;      /**< Hash table array */
    int size;               /**< Size of hash table */
    DataType key_type;      /**< Type of keys */
    int entry_count;        /**< Number of entries */
    char column_name[64];   /**< Indexed column name */
} HashIndex;

/**
 * @struct KeyValuePair
 * @brief Used during index construction
 */
typedef struct {
    void* key;             /**< Key value */
    int position;          /**< Record position */
} KeyValuePair;

#endif /* UMBRA_INDEX_TYPES_H */
