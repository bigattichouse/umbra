/**
 * @file btree_index.c
 * @brief Implements B-tree for indexing
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "btree_index.h"
#include "../schema/type_system.h"

/**
 * @brief Compare keys based on data type
 */
static int compare_keys(const void* key1, const void* key2, DataType type) {
    switch (type) {
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
        case TYPE_TEXT:
            return strcmp((const char*)key1, (const char*)key2);
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
            return 0;
    }
}

/**
 * @brief Create a new B-tree node
 */
static BTreeNode* create_node(bool is_leaf, DataType key_type) {
    BTreeNode* node = malloc(sizeof(BTreeNode));
    if (!node) return NULL;
    
    node->key_count = 0;
    node->key_type = key_type;
    node->is_leaf = is_leaf;
    
    for (int i = 0; i < BTREE_ORDER - 1; i++) {
        node->keys[i] = NULL;
        node->positions[i] = -1;
    }
    
    for (int i = 0; i < BTREE_ORDER; i++) {
        node->children[i] = NULL;
    }
    
    return node;
}

/**
 * @brief Duplicate a key value
 */
static void* duplicate_key(const void* key, DataType type) {
    size_t size;
    
    switch (type) {
        case TYPE_INT:
            size = sizeof(int);
            break;
        case TYPE_FLOAT:
            size = sizeof(double);
            break;
        case TYPE_BOOLEAN:
            size = sizeof(bool);
            break;
        case TYPE_DATE:
            size = sizeof(time_t);
            break;
        case TYPE_VARCHAR:
        case TYPE_TEXT:
            size = strlen((const char*)key) + 1;
            break;
        default:
            return NULL;
    }
    
    void* new_key = malloc(size);
    if (!new_key) return NULL;
    
    memcpy(new_key, key, size);
    return new_key;
}

/**
 * @brief Free a B-tree node and all children
 */
static void free_node(BTreeNode* node) {
    if (!node) return;
    
    // Free all keys
    for (int i = 0; i < node->key_count; i++) {
        free(node->keys[i]);
    }
    
    // Free all children recursively
    if (!node->is_leaf) {
        for (int i = 0; i <= node->key_count; i++) {
            free_node(node->children[i]);
        }
    }
    
    free(node);
}

/**
 * @brief Insert a key into a non-full node
 */
static void insert_non_full(BTreeNode* node, void* key, int position, DataType key_type) {
    int i = node->key_count - 1;
    
    if (node->is_leaf) {
        // Find position to insert
        while (i >= 0 && compare_keys(node->keys[i], key, key_type) > 0) {
            node->keys[i + 1] = node->keys[i];
            node->positions[i + 1] = node->positions[i];
            i--;
        }
        
        // Insert key and position
        node->keys[i + 1] = duplicate_key(key, key_type);
        node->positions[i + 1] = position;
        node->key_count++;
    } else {
        // Find child to insert in
        while (i >= 0 && compare_keys(node->keys[i], key, key_type) > 0) {
            i--;
        }
        
        i++;
        
        // If child is full, split it
        if (node->children[i]->key_count == BTREE_ORDER - 1) {
            // TODO: Implement split_child
            // split_child(node, i, key_type);
            
            // After split, figure out which child to go to
            if (compare_keys(node->keys[i], key, key_type) < 0) {
                i++;
            }
        }
        
        // Recursively insert into child
        insert_non_full(node->children[i], key, position, key_type);
    }
}

/**
 * @brief Split a full child node
 */
static void split_child(BTreeNode* parent, int index, DataType key_type) {
    BTreeNode* y = parent->children[index];
    BTreeNode* z = create_node(y->is_leaf, key_type);
    
    // Move half the keys to the new node
    for (int j = 0; j < BTREE_ORDER / 2 - 1; j++) {
        z->keys[j] = y->keys[j + BTREE_ORDER / 2];
        z->positions[j] = y->positions[j + BTREE_ORDER / 2];
        y->keys[j + BTREE_ORDER / 2] = NULL;
        y->positions[j + BTREE_ORDER / 2] = -1;
    }
    
    // Move half the children if not a leaf
    if (!y->is_leaf) {
        for (int j = 0; j < BTREE_ORDER / 2; j++) {
            z->children[j] = y->children[j + BTREE_ORDER / 2];
            y->children[j + BTREE_ORDER / 2] = NULL;
        }
    }
    
    // Set key counts
    z->key_count = BTREE_ORDER / 2 - 1;
    y->key_count = BTREE_ORDER / 2 - 1;
    
    // Move keys in parent to make room
    for (int j = parent->key_count; j > index; j--) {
        parent->keys[j] = parent->keys[j - 1];
        parent->positions[j] = parent->positions[j - 1];
        parent->children[j + 1] = parent->children[j];
    }
    
    // Insert median key into parent
    parent->keys[index] = y->keys[BTREE_ORDER / 2 - 1];
    parent->positions[index] = y->positions[BTREE_ORDER / 2 - 1];
    y->keys[BTREE_ORDER / 2 - 1] = NULL;
    y->positions[BTREE_ORDER / 2 - 1] = -1;
    
    // Link new child
    parent->children[index + 1] = z;
    parent->key_count++;
}

/**
 * @brief Initialize a B-tree index
 */
int btree_init(BTreeIndex* index, DataType key_type, const char* column_name) {
    if (!index || !column_name) {
        return -1;
    }
    
    index->root = create_node(true, key_type);
    if (!index->root) {
        return -1;
    }
    
    index->key_type = key_type;
    index->height = 1;
    index->node_count = 1;
    
    strncpy(index->column_name, column_name, sizeof(index->column_name) - 1);
    index->column_name[sizeof(index->column_name) - 1] = '\0';
    
    return 0;
}

/**
 * @brief Free resources used by a B-tree index
 */
int btree_free(BTreeIndex* index) {
    if (!index) {
        return -1;
    }
    
    free_node(index->root);
    index->root = NULL;
    index->height = 0;
    index->node_count = 0;
    
    return 0;
}

/**
 * @brief Insert a key-position pair into the index
 */
int btree_insert(BTreeIndex* index, void* key, int position) {
    if (!index || !key) {
        return -1;
    }
    
    // If root is full, split it
    if (index->root->key_count == BTREE_ORDER - 1) {
        BTreeNode* new_root = create_node(false, index->key_type);
        if (!new_root) {
            return -1;
        }
        
        new_root->children[0] = index->root;
        index->root = new_root;
        index->height++;
        index->node_count++;
        
        split_child(new_root, 0, index->key_type);
    }
    
    // Insert into non-full root
    insert_non_full(index->root, key, position, index->key_type);
    return 0;
}

/**
 * @brief Find positions for an exact key match
 */
int btree_find_exact(const BTreeIndex* index, const void* key, 
                    int* positions, int max_positions) {
    if (!index || !key || !positions || max_positions <= 0) {
        return -1;
    }
    
    int count = 0;
    BTreeNode* node = index->root;
    
    while (node) {
        int i;
        
        // Find key in current node
        for (i = 0; i < node->key_count; i++) {
            int cmp = compare_keys(node->keys[i], key, index->key_type);
            
            if (cmp == 0) {
                // Found key
                positions[count++] = node->positions[i];
                if (count >= max_positions) {
                    return count;
                }
            } else if (cmp > 0) {
                // Key is smaller, go to left child
                break;
            }
        }
        
        // If leaf, we're done
        if (node->is_leaf) {
            break;
        }
        
        // Go to appropriate child
        node = node->children[i];
    }
    
    return count;
}

/**
 * @brief Find all positions in subtree and add to results
 */
static int collect_all_positions(BTreeNode* node, int* positions, 
                                int max_positions, int current_count) {
    if (!node || current_count >= max_positions) {
        return current_count;
    }
    
    if (node->is_leaf) {
        // Add all positions in leaf
        for (int i = 0; i < node->key_count && current_count < max_positions; i++) {
            positions[current_count++] = node->positions[i];
        }
    } else {
        // Traverse all children
        for (int i = 0; i <= node->key_count && current_count < max_positions; i++) {
            // Add key if not the last child
            if (i < node->key_count) {
                positions[current_count++] = node->positions[i];
            }
            
            // Recursively collect from child
            current_count = collect_all_positions(node->children[i], 
                                               positions, max_positions, 
                                               current_count);
        }
    }
    
    return current_count;
}

/**
 * @brief Find positions for a range of keys
 */
int btree_find_range(const BTreeIndex* index, const void* low_key, 
                    const void* high_key, int* positions, int max_positions) {
    if (!index || !positions || max_positions <= 0) {
        return -1;
    }
    
    // If no bounds, collect all
    if (!low_key && !high_key) {
        return collect_all_positions(index->root, positions, max_positions, 0);
    }
    
    int count = 0;
    BTreeNode* node = index->root;
    
    // TODO: Implement range search
    // This is a simplified version that doesn't properly handle ranges
    // A real implementation would traverse the tree once to find the start node,
    // then collect all positions in range
    
    // For now, just find the lower bound and collect everything after it
    while (node && !node->is_leaf) {
        int i;
        
        if (!low_key) {
            // No lower bound, go to leftmost leaf
            node = node->children[0];
            continue;
        }
        
        // Find position for low key
        for (i = 0; i < node->key_count; i++) {
            if (compare_keys(node->keys[i], low_key, index->key_type) >= 0) {
                break;
            }
        }
        
        node = node->children[i];
    }
    
    // Now collect all positions in range
    if (node && node->is_leaf) {
        int i = 0;
        
        // Skip keys below low_key
        if (low_key) {
            while (i < node->key_count && compare_keys(node->keys[i], low_key, index->key_type) < 0) {
                i++;
            }
        }
        
        // Collect keys until high_key
        for (; i < node->key_count && count < max_positions; i++) {
            if (high_key && compare_keys(node->keys[i], high_key, index->key_type) > 0) {
                break;
            }
            
            positions[count++] = node->positions[i];
        }
    }
    
    return count;
}

/**
 * @brief Generate C code for a B-tree node
 */
static int generate_node_code(const BTreeNode* node, char* output, size_t output_size, 
                             int node_id, DataType key_type) {
    int written = 0;
    
    written += snprintf(output + written, output_size - written,
        "static BTreeNode node_%d = {\n"
        "    .key_count = %d,\n"
        "    .key_type = %d,\n"
        "    .is_leaf = %s,\n"
        "    .keys = {",
        node_id, node->key_count, key_type, node->is_leaf ? "true" : "false");
    
    // Write keys
    for (int i = 0; i < BTREE_ORDER - 1; i++) {
        if (i > 0) {
            written += snprintf(output + written, output_size - written, ", ");
        }
        
        if (i < node->key_count) {
            switch (key_type) {
                case TYPE_INT:
                    written += snprintf(output + written, output_size - written, 
                                     "&(int){%d}", *(int*)node->keys[i]);
                    break;
                case TYPE_FLOAT:
                    written += snprintf(output + written, output_size - written, 
                                     "&(double){%f}", *(double*)node->keys[i]);
                    break;
                case TYPE_VARCHAR:
                case TYPE_TEXT:
                    written += snprintf(output + written, output_size - written, 
                                     "\"%s\"", (char*)node->keys[i]);
                    break;
                case TYPE_BOOLEAN:
                    written += snprintf(output + written, output_size - written, 
                                     "&(bool){%s}", *(bool*)node->keys[i] ? "true" : "false");
                    break;
                case TYPE_DATE:
                    written += snprintf(output + written, output_size - written, 
                                     "&(time_t){%ld}", *(time_t*)node->keys[i]);
                    break;
                default:
                    written += snprintf(output + written, output_size - written, "NULL");
                    break;
            }
        } else {
            written += snprintf(output + written, output_size - written, "NULL");
        }
    }
    
    written += snprintf(output + written, output_size - written, "},\n    .positions = {");
    
    // Write positions
    for (int i = 0; i < BTREE_ORDER - 1; i++) {
        if (i > 0) {
            written += snprintf(output + written, output_size - written, ", ");
        }
        
        if (i < node->key_count) {
            written += snprintf(output + written, output_size - written, "%d", node->positions[i]);
        } else {
            written += snprintf(output + written, output_size - written, "-1");
        }
    }
    
    written += snprintf(output + written, output_size - written, "},\n    .children = {");
    
    // Write children
    for (int i = 0; i < BTREE_ORDER; i++) {
        if (i > 0) {
            written += snprintf(output + written, output_size - written, ", ");
        }
        
        if (!node->is_leaf && i <= node->key_count && node->children[i]) {
            written += snprintf(output + written, output_size - written, "&node_%d", node_id + i + 1);
            // TODO: This is incorrect, would need to track actual node IDs
        } else {
            written += snprintf(output + written, output_size - written, "NULL");
        }
    }
    
    written += snprintf(output + written, output_size - written, "}\n};\n\n");
    
    return written;
}

/**
 * @brief Generate C code for B-tree index
 */
int btree_generate_code(const BTreeIndex* index, char* output, size_t output_size) {
    if (!index || !output || output_size <= 0) {
        return -1;
    }
    
    int written = 0;
    
    // Generate code for each node
    // This is a simplified version - a real implementation would traverse the tree
    // and generate code for each node
    written += generate_node_code(index->root, output + written, output_size - written, 
                                 0, index->key_type);
    
    // Generate index structure
    written += snprintf(output + written, output_size - written,
        "BTreeIndex %s_btree_index = {\n"
        "    .root = &node_0,\n"
        "    .key_type = %d,\n"
        "    .height = %d,\n"
        "    .node_count = %d,\n"
        "    .column_name = \"%s\"\n"
        "};\n",
        index->column_name, index->key_type, index->height, 
        index->node_count, index->column_name);
    
    return written;
}

/**
 * @brief Compare function for qsort
 */
static int compare_pairs(const void* a, const void* b) {
    const KeyValuePair* pair_a = (const KeyValuePair*)a;
    const KeyValuePair* pair_b = (const KeyValuePair*)b;
    
    // This assumes both pairs have the same key type
    // In a real implementation, you would need to pass the key type
    return compare_keys(pair_a->key, pair_b->key, TYPE_INT); // Simplified
}

/**
 * @brief Build a B-tree from sorted key-value pairs
 */
BTreeIndex* btree_build_from_sorted(const KeyValuePair* pairs, int pair_count,
                                   DataType key_type, const char* column_name) {
    if (!pairs || pair_count <= 0 || !column_name) {
        return NULL;
    }
    
    BTreeIndex* index = malloc(sizeof(BTreeIndex));
    if (!index) {
        return NULL;
    }
    
    if (btree_init(index, key_type, column_name) != 0) {
        free(index);
        return NULL;
    }
    
    // Insert all pairs into the tree
    for (int i = 0; i < pair_count; i++) {
        if (btree_insert(index, pairs[i].key, pairs[i].position) != 0) {
            btree_free(index);
            free(index);
            return NULL;
        }
    }
    
    return index;
}
