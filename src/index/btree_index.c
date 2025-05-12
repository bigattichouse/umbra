/**
 * @file btree_index.c
 * @brief B-tree index implementation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>
#include <time.h>

#include "btree_index.h"
#include "../schema/type_system.h"

/**
 * @brief Compare two keys based on data type
 */
static int compare_keys(const void* key1, const void* key2, DataType key_type) {
    if (!key1 && !key2) return 0;
    if (!key1) return -1;
    if (!key2) return 1;
    
    switch (key_type) {
        case TYPE_INT: {
            int val1 = *(const int*)key1;
            int val2 = *(const int*)key2;
            return (val1 > val2) - (val1 < val2);
        }
        case TYPE_FLOAT: {
            double val1 = *(const double*)key1;
            double val2 = *(const double*)key2;
            return (val1 > val2) - (val1 < val2);
        }
        case TYPE_VARCHAR:
        case TYPE_TEXT: {
            return strcmp((const char*)key1, (const char*)key2);
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
    
    node->is_leaf = is_leaf;
    node->key_count = 0;
    node->key_type = key_type;
    
    // Initialize keys and pointers
    for (int i = 0; i < BTREE_ORDER - 1; i++) {
        node->keys[i] = NULL;
        node->positions[i] = -1;
    }
    
    // Initialize children
    for (int i = 0; i < BTREE_ORDER; i++) {
        node->children[i] = NULL;
    }
    
    return node;
}

/**
 * @brief Free a B-tree node and all its children
 */
static void free_node(BTreeNode* node) {
    if (!node) return;
    
    // Free children first
    if (!node->is_leaf) {
        for (int i = 0; i <= node->key_count; i++) {
            if (node->children[i]) {
                free_node(node->children[i]);
                node->children[i] = NULL;
            }
        }
    }
    
    // Free keys
    for (int i = 0; i < node->key_count; i++) {
        if (node->keys[i]) {
            // Free string keys
            if (node->key_type == TYPE_VARCHAR || node->key_type == TYPE_TEXT) {
                free(node->keys[i]);
            }
            node->keys[i] = NULL;
        }
    }
    
    free(node);
}

/**
 * @brief Split a child node during insertion
 */
static void split_child(BTreeNode* parent, int index, BTreeNode* y) {
    // Create a new node
    BTreeNode* z = create_node(y->is_leaf, y->key_type);
    
    // Set key count for new node
    z->key_count = BTREE_ORDER / 2 - 1;
    
    // Copy the second half of y's keys to z
    for (int j = 0; j < BTREE_ORDER / 2 - 1; j++) {
        z->keys[j] = y->keys[j + BTREE_ORDER / 2];
        z->positions[j] = y->positions[j + BTREE_ORDER / 2];
        
        // Clear the original key in y
        y->keys[j + BTREE_ORDER / 2] = NULL;
        y->positions[j + BTREE_ORDER / 2] = -1;
    }
    
    // If y is not a leaf, copy the appropriate children to z
    if (!y->is_leaf) {
        for (int j = 0; j < BTREE_ORDER / 2; j++) {
            z->children[j] = y->children[j + BTREE_ORDER / 2];
            y->children[j + BTREE_ORDER / 2] = NULL;
        }
    }
    
    // Update y's key count
    y->key_count = BTREE_ORDER / 2 - 1;
    
    // Move parent's keys and children to make room for the new key and child
    for (int j = parent->key_count + 1; j > index + 1; j--) {
        parent->children[j] = parent->children[j - 1];
    }
    
    // Link the new child to parent
    parent->children[index + 1] = z;
    
    // Move parent's keys to make room for the middle key from y
    for (int j = parent->key_count; j > index; j--) {
        parent->keys[j] = parent->keys[j - 1];
        parent->positions[j] = parent->positions[j - 1];
    }
    
    // Copy the middle key from y to parent
    parent->keys[index] = y->keys[BTREE_ORDER / 2 - 1];
    parent->positions[index] = y->positions[BTREE_ORDER / 2 - 1];
    
    // Clear the middle key in y
    y->keys[BTREE_ORDER / 2 - 1] = NULL;
    y->positions[BTREE_ORDER / 2 - 1] = -1;
    
    // Increment parent's key count
    parent->key_count++;
}

/**
 * @brief Insert a key-position pair into a non-full node
 */
static void insert_non_full(BTreeNode* node, void* key, int position, DataType key_type) {
    // Start from the rightmost element
    int i = node->key_count - 1;
    
    // If node is a leaf, insert the key directly
    if (node->is_leaf) {
        // Find position to insert the new key
        while (i >= 0 && compare_keys(node->keys[i], key, key_type) > 0) {
            node->keys[i + 1] = node->keys[i];
            node->positions[i + 1] = node->positions[i];
            i--;
        }
        
        // Insert the new key
        node->keys[i + 1] = key;
        node->positions[i + 1] = position;
        node->key_count++;
    } else {
        // If node is not a leaf, find the child to insert into
        
        // Find the appropriate child
        while (i >= 0 && compare_keys(node->keys[i], key, key_type) > 0) {
            i--;
        }
        
        i++;
        
        // Check if the child is full
        if (node->children[i]->key_count == BTREE_ORDER - 1) {
            // Split the child if it's full
            split_child(node, i, node->children[i]);
            
            // After splitting, determine which child to go into
            if (compare_keys(node->keys[i], key, key_type) < 0) {
                i++;
            }
        }
        
        // Insert into the appropriate child
        insert_non_full(node->children[i], key, position, key_type);
    }
}

/**
 * @brief Initialize a B-tree index
 */
BTreeIndex* btree_init(const char* column_name, DataType key_type) {
    BTreeIndex* index = malloc(sizeof(BTreeIndex));
    if (!index) return NULL;
    
    strncpy(index->column_name, column_name, sizeof(index->column_name) - 1);
    index->column_name[sizeof(index->column_name) - 1] = '\0';
    
    index->key_type = key_type;
    index->root = create_node(true, key_type);
    index->height = 1;
    
    if (!index->root) {
        free(index);
        return NULL;
    }
    
    return index;
}

/**
 * @brief Free a B-tree index
 */
void btree_free(BTreeIndex* index) {
    if (!index) return;
    
    if (index->root) {
        free_node(index->root);
        index->root = NULL;
    }
    
    index->height = 0;
    free(index);
}

/**
 * @brief Insert a key-position pair into the B-tree
 */
int btree_insert(BTreeIndex* index, void* key, int position) {
    if (!index || !key) return -1;
    
    // Handle root split case
    if (index->root->key_count == BTREE_ORDER - 1) {
        // Create a new root
        BTreeNode* new_root = create_node(false, index->key_type);
        if (!new_root) return -1;
        
        // Make old root the first child of new root
        new_root->children[0] = index->root;
        
        // Split the old root
        split_child(new_root, 0, index->root);
        
        // Replace the root
        index->root = new_root;
        index->height++;
        
        // Insert into the appropriate child
        int i = 0;
        if (compare_keys(new_root->keys[0], key, index->key_type) < 0) {
            i++;
        }
        
        insert_non_full(new_root->children[i], key, position, index->key_type);
    } else {
        // Root is not full, insert directly
        insert_non_full(index->root, key, position, index->key_type);
    }
    
    return 0;
}

/**
 * @brief Find positions of records with keys matching the search key
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
        for (i = 0; i < node->key_count; i++) {
            int cmp = compare_keys(node->keys[i], key, index->key_type);
            
            if (cmp == 0) {
                // Found a match
                if (count < max_positions) {
                    positions[count++] = node->positions[i];
                } else {
                    return count; // Buffer full
                }
            } else if (cmp > 0) {
                break; // Keys are sorted, so stop if we find a larger key
            }
        }
        
        // If we're at a leaf, we're done searching
        if (node->is_leaf) {
            break;
        }
        
        // Continue search in the appropriate child
        node = node->children[i];
    }
    
    return count;
}

/**
 * @brief Recursively collect all positions from a subtree
 */
static int collect_all_positions(BTreeNode* node, int* positions, 
                               int max_positions, int current_count) {
    if (!node || current_count >= max_positions) {
        return current_count;
    }
    
    if (node->is_leaf) {
        // Collect positions from leaf
        for (int i = 0; i < node->key_count && current_count < max_positions; i++) {
            positions[current_count++] = node->positions[i];
        }
    } else {
        // For non-leaf nodes, traverse children and collect positions
        for (int i = 0; i < node->key_count && current_count < max_positions; i++) {
            // Collect from left child
            current_count = collect_all_positions(node->children[i], 
                                               positions, max_positions, current_count);
            
            // Add current position
            positions[current_count++] = node->positions[i];
        }
        
        // Collect from rightmost child
        if (current_count < max_positions) {
            current_count = collect_all_positions(node->children[node->key_count], 
                                               positions, max_positions, current_count);
        }
    }
    
    return current_count;
}

/**
 * @brief Find positions of records with keys in the given range
 */
int btree_find_range(const BTreeIndex* index, const void* low_key, const void* high_key,
                    int* positions, int max_positions) {
    if (!index || !positions || max_positions <= 0) {
        return -1;
    }
    
    // If both bounds are NULL, collect all positions
    if (!low_key && !high_key) {
        return collect_all_positions(index->root, positions, max_positions, 0);
    }
    
    BTreeNode* node = index->root;
    int count = 0;
    
    // Find the lowest key in range
    while (!node->is_leaf) {
        int i = 0;
        if (low_key) {
            while (i < node->key_count &&
                   compare_keys(node->keys[i], low_key, index->key_type) < 0) {
                i++;
            }
        }
        
        node = node->children[i];
    }
    
    // Find starting position in leaf
    int i = 0;
    if (low_key) {
        while (i < node->key_count &&
               compare_keys(node->keys[i], low_key, index->key_type) < 0) {
            i++;
        }
    }
    
    // Collect positions in range
    for (; i < node->key_count && count < max_positions; i++) {
        // Check upper bound
        if (high_key && compare_keys(node->keys[i], high_key, index->key_type) > 0) {
            break;
        }
        
        positions[count++] = node->positions[i];
    }
    
    return count;
}

/**
 * @brief Generate C code for a B-tree node
 */
static int generate_node_code(const BTreeNode* node, char* output, size_t output_size,
                            int node_id, int* next_id) {
    int written = 0;
    
    // Generate node structure
    written += snprintf(output + written, output_size - written,
        "static BTreeNode node_%d = {\n"
        "    .is_leaf = %s,\n"
        "    .key_count = %d,\n"
        "    .keys = {",
        node_id, node->is_leaf ? "true" : "false", node->key_count);
    
    // Generate key array
    for (int i = 0; i < BTREE_ORDER - 1; i++) {
        if (i > 0) {
            written += snprintf(output + written, output_size - written, ", ");
        }
        
        if (i < node->key_count && node->keys[i]) {
            // Serialize key based on type
            switch (node->key_type) {
                case TYPE_INT:
                    written += snprintf(output + written, output_size - written,
                                      "&(int){%d}", *(int*)node->keys[i]);
                    break;
                case TYPE_VARCHAR:
                case TYPE_TEXT:
                    written += snprintf(output + written, output_size - written,
                                      "\"%s\"", (char*)node->keys[i]);
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
    
    // Generate positions array
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
    
    // Generate children array with recursive calls
    for (int i = 0; i <= BTREE_ORDER - 1; i++) {
        if (i > 0) {
            written += snprintf(output + written, output_size - written, ", ");
        }
        
        if (i <= node->key_count && node->children[i]) {
            int child_id = (*next_id)++;
            written += snprintf(output + written, output_size - written, "&node_%d", child_id);
            
            // Generate code for child node (will be placed before this node in the output)
            char child_code[8192];
            int child_written = generate_node_code(node->children[i], child_code, sizeof(child_code),
                                               child_id, next_id);
            
            // Insert child code before current node
            memmove(output + child_written, output, written);
            memcpy(output, child_code, child_written);
            
            written += child_written;
        } else {
            written += snprintf(output + written, output_size - written, "NULL");
        }
    }
    
    written += snprintf(output + written, output_size - written,
                      "},\n"
                      "    .key_type = %d\n};\n\n", node->key_type);
    
    return written;
}

/**
 * @brief Generate C code for a B-tree index
 */
int btree_generate_code(const BTreeIndex* index, char* output, size_t output_size) {
    if (!index || !output || output_size <= 0) {
        return -1;
    }
    
    int written = 0;
    int next_id = 1;
    
    // Generate root node
    int root_written = generate_node_code(index->root, output, output_size, 0, &next_id);
    if (root_written < 0) {
        return -1;
    }
    
    written += root_written;
    
    // Generate index structure
    written += snprintf(output + written, output_size - written,
        "BTreeIndex btree_index = {\n"
        "    .column_name = \"%s\",\n"
        "    .key_type = %d,\n"
        "    .height = %d,\n"
        "    .root = &node_0\n"
        "};\n",
        index->column_name, index->key_type, index->height);
    
    return written;
}

/**
 * @brief Compare function for sorting key-value pairs
 */
static int compare_pairs(const void* a, const void* b) {
    const KeyValuePair* pair_a = (const KeyValuePair*)a;
    const KeyValuePair* pair_b = (const KeyValuePair*)b;
    
    // If positions are equal, return 0
    if (pair_a->position == pair_b->position) {
        return 0;
    }
    
    return compare_keys(pair_a->key, pair_b->key, TYPE_INT); // Simplified
}

/**
 * @brief Build a B-tree index from key-value pairs
 */
BTreeIndex* btree_build_from_pairs(const KeyValuePair* pairs, int pair_count,
                                const char* column_name, DataType key_type) {
    if (!pairs || pair_count <= 0 || !column_name) {
        return NULL;
    }
    
    // Initialize the B-tree
    BTreeIndex* index = btree_init(column_name, key_type);
    if (!index) {
        return NULL;
    }
    
    // Insert all pairs into the B-tree
    for (int i = 0; i < pair_count; i++) {
        if (btree_insert(index, pairs[i].key, pairs[i].position) != 0) {
            btree_free(index);
            return NULL;
        }
    }
    
    return index;
}
