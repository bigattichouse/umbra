/**
 * @file index_types.h
 * @brief Common type definitions for indexing system
 */

#ifndef UMBRA_INDEX_TYPES_H
#define UMBRA_INDEX_TYPES_H

#include <stdbool.h>
#include "../schema/schema_parser.h"

/**
 * @enum IndexType
 * @brief Types of indexes
 */
typedef enum {
    INDEX_TYPE_BTREE,
    INDEX_TYPE_HASH
} IndexType;

/**
 * @struct IndexKeyInfo
 * @brief Information about an index key
 */
typedef struct {
    char column_name[64];      /**< Column name */
    DataType data_type;        /**< Data type of column */
    int column_offset;         /**< Offset of column in record */
    int column_size;           /**< Size of column in bytes */
} IndexKeyInfo;

/**
 * @struct IndexMetadata
 * @brief Metadata for an index
 */
typedef struct {
    char index_name[64];       /**< Index name */
    char table_name[64];       /**< Table name */
    IndexType type;            /**< Index type */
    IndexKeyInfo key;          /**< Key information */
    bool is_unique;            /**< Whether index enforces uniqueness */
    time_t created_at;         /**< Creation timestamp */
    time_t modified_at;        /**< Last modification timestamp */
} IndexMetadata;

/**
 * @struct IndexNode
 * @brief Generic index node
 */
typedef struct IndexNode {
    void* key;                 /**< Key value */
    int record_position;       /**< Position of record in data page */
    int page_number;           /**< Page number containing record */
    struct IndexNode* next;    /**< Next node in chain (for hash collisions) */
} IndexNode;

/**
 * @struct BTreeNode
 * @brief B-tree node structure
 */
typedef struct BTreeNode {
    int key_count;             /**< Number of keys in node */
    void** keys;               /**< Array of keys */
    int* record_positions;     /**< Array of record positions */
    int* page_numbers;         /**< Array of page numbers */
    struct BTreeNode** children; /**< Array of child node pointers */
    bool is_leaf;              /**< Whether node is a leaf */
} BTreeNode;

/**
 * @struct HashBucket
 * @brief Hash table bucket
 */
typedef struct {
    IndexNode* head;           /**< Head of chain */
} HashBucket;

/**
 * @struct BTreeIndex
 * @brief B-tree index structure
 */
typedef struct {
    BTreeNode* root;           /**< Root node of B-tree */
    IndexMetadata metadata;    /**< Index metadata */
    int (*compare)(const void*, const void*); /**< Comparison function */
} BTreeIndex;

/**
 * @struct HashIndex
 * @brief Hash index structure
 */
typedef struct {
    HashBucket* buckets;       /**< Array of buckets */
    int bucket_count;          /**< Number of buckets */
    IndexMetadata metadata;    /**< Index metadata */
    unsigned int (*hash)(const void*); /**< Hash function */
} HashIndex;

/**
 * @brief Search a BTree index
 * @param index BTree index to search
 * @param key Key to search for
 * @param results Array to store matching record positions
 * @param max_results Maximum number of results to return
 * @return Number of matching records found
 */
int search_btree_index(const BTreeIndex* index, const void* key, 
                      int* results, int max_results);

/**
 * @brief Search a Hash index
 * @param index Hash index to search
 * @param key Key to search for
 * @param results Array to store matching record positions
 * @param max_results Maximum number of results to return
 * @return Number of matching records found
 */
int search_hash_index(const HashIndex* index, const void* key, 
                     int* results, int max_results);

/**
 * @brief Generic index search function
 * @param index Index to search (cast to appropriate type inside)
 * @param key Key to search for
 * @param results Array to store matching record positions
 * @param max_results Maximum number of results to return
 * @return Number of matching records found
 */
int search_index(const void* index, const void* key, 
                int* results, int max_results);

#endif /* UMBRA_INDEX_TYPES_H */
