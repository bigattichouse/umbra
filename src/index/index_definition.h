/**
 * @file index_definition.h
 * @brief Index structure definitions
 */

#ifndef UMBRA_INDEX_DEFINITION_H
#define UMBRA_INDEX_DEFINITION_H

#include <stdbool.h>

/**
 * @enum IndexType
 * @brief Types of indices
 */
typedef enum {
    INDEX_TYPE_BTREE,  // B-tree index for range queries
    INDEX_TYPE_HASH    // Hash index for equality queries
} IndexType;

/**
 * @struct IndexDefinition
 * @brief Represents an index definition
 */
typedef struct {
    char table_name[64];    /**< Table name */
    char column_name[64];   /**< Column name */
    char index_name[128];   /**< Index name */
    IndexType type;         /**< Index type (BTREE, HASH) */
    bool unique;            /**< Whether the index enforces uniqueness */
    bool primary;           /**< Whether this is a primary key index */
} IndexDefinition;

#endif /* UMBRA_INDEX_DEFINITION_H */
