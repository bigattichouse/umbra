/**
 * @file btree_index.c
 * @brief Implements B-tree for indexing
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include "btree_index.h"
#include "../schema/type_system.h"

/**
 * @brief Compare two keys of the same type
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
 * @brief Create a new node
 */
static BTreeNode* create_node(bool is_leaf, DataType key_type) {
    BTreeNode* node = malloc(sizeof(BTreeNode));
    if (!node) return NULL;
    
    node->is_leaf = is_leaf;
    node->key_count = 0;
    node->key_type = key_type;
    
    // Initialize keys, positions, and children
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
 * @brief Free a node and all its children recursively
 */
static void free_node(BTreeNode* node) {
    if (!node) return;
    
    // Free non-leaf children recursively
    if (!node->is_leaf) {
        for (int i = 0; i <= node->key_count; i++) {
            free_node(node->children[i]);
        }
    }
    
    // Free keys (depends on key type)
    for (int i = 0; i < node->key_count; i++) {
        if (node->keys[i]) {
            // Free allocated keys
            if (node->key_type == TYPE_VARCHAR || node->key_type == TYPE_TEXT) {
                free(node->keys[i]);
            }
            node->keys[i] = NULL;
        }
    }
    
    free(node);
}

/**
 * @brief Split a child node
 */
static int split_child(BTreeNode* parent, int index, DataType key_type) {
    BTreeNode* y = parent->children[index];
    BTreeNode* z = create_node(y->is_leaf, key_type);
    if (!z) return -1;
    
    // Set up the new node z
    z->key_count = BTREE_ORDER / 2 - 1;
    
    // Copy the second half of keys and positions to z
    for (int j = 0; j < BTREE_ORDER / 2 - 1; j++) {
        z->keys[j] = y->keys[j + BTREE_ORDER / 2];
        z->positions[j] = y->positions[j + BTREE_ORDER / 2];
        
        // Clear references in y
        y->keys[j + BTREE_ORDER / 2] = NULL;
        y->positions[j + BTREE_ORDER / 2] = -1;
    }
    
    // If not leaf, copy the children as well
    if (!y->is_leaf) {
        for (int j = 0; j < BTREE_ORDER / 2; j++) {
            z->children[j] = y->children[j + BTREE_ORDER / 2];
            y->children[j + BTREE_ORDER / 2] = NULL;
        }
    }
    
    // Update y's key count
    y->key_count = BTREE_ORDER / 2 - 1;
    
    // Shift parent's children to make room for z
    for (int j = parent->key_count + 1; j > index + 1; j--) {
        parent->children[j] = parent->children[j - 1];
    }
    
    // Set z as parent's child
    parent->children[index + 1] = z;
    
    // Shift parent's keys and positions to make room for median key
    for (int j = parent->key_count; j > index; j--) {
        parent->keys[j] = parent->keys[j - 1];
        parent->positions[j] = parent->positions[j - 1];
    }
    
    // Copy median key and position to parent
    parent->keys[index] = y->keys[BTREE_ORDER / 2 - 1];
    parent->positions[index] = y->positions[BTREE_ORDER / 2 - 1];
    
    // Clear reference in y
    y->keys[BTREE_ORDER / 2 - 1] = NULL;
    y->positions[BTREE_ORDER / 2 - 1] = -1;
    
    // Increment parent's key count
    parent->key_count++;
    
    return 0;
}

/**
 * @brief Insert into a non-full node
 */
static int insert_non_full(BTreeNode* node, void* key, int position, DataType key_type) {
    int i = node->key_count - 1;
    
    if (node->is_leaf) {
        // Shift keys and positions to make room for new key
        while (i >= 0 && compare_keys(node->keys[i], key, key_type) > 0) {
            node->keys[i + 1] = node->keys[i];
            node->positions[i + 1] = node->positions[i];
            i--;
        }
        
        // Insert the new key and position
        node->keys[i + 1] = key;
        node->positions[i + 1] = position;
        node->key_count++;
        
        return 0;
    }
    
    // Find the child which will contain the new key
    while (i >= 0 && compare_keys(node->keys[i], key, key_type) > 0) {
        i--;
    }
    i++;
    
    // If the child is full, split it
    if (node->children[i]->key_count == BTREE_ORDER - 1) {
        if (split_child(node, i, key_type) != 0) {
            return -1;
        }
        
        // After split, the median key from the child is now in the parent
        // Determine which child to go to
        if (compare_keys(node->keys[i], key, key_type) < 0) {
            i++;
        }
    }
    
    // Recursively insert into the appropriate child
    return insert_non_full(node->children[i], key, position, key_type);
}

/**
 * @brief Initialize a B-tree index
 */
BTreeIndex* btree_init(const char* column_name, DataType key_type) {
    // Allocate the index
    BTreeIndex* index = malloc(sizeof(BTreeIndex));
    if (!index) {
        return NULL;
    }
    
    // Initialize index fields
    index->root = create_node(true, key_type); // Create leaf root node
    if (!index->root) {
        free(index);
        return NULL;
    }
    
    index->key_type = key_type;
    index->height = 1;
    index->node_count = 1;
    
    strncpy(index->column_name, column_name, sizeof(index->column_name) - 1);
    index->column_name[sizeof(index->column_name) - 1] = '\0';
    
    return index;
}

/**
 * @brief Free a B-tree index
 */
void btree_free(BTreeIndex* index) {
    if (!index) {
        return;
    }
    
    // Free all nodes recursively starting from root
    free_node(index->root);
    index->root = NULL;
    
    // Reset index state
    index->height = 0;
    index->node_count = 0;
    
    // Free the index structure itself
    free(index);
}

/**
 * @brief Insert a key-position pair into the index
 */
int btree_insert(BTreeIndex* index, void* key, int position) {
    if (!index || !key) {
        return -1;
    }
    
    // Handle root split
    if (index->root->key_count == BTREE_ORDER - 1) {
        BTreeNode* new_root = create_node(false, index->key_type);
        if (!new_root) {
            return -1;
        }
        
        // Old root becomes first child of new root
        new_root->children[0] = index->root;
        index->root = new_root;
        
        // Update index metadata
        index->height++;
        index->node_count++;
        
        // Split the old root
        if (split_child(new_root, 0, index->key_type) != 0) {
            return -1;
        }
        
        // Insert into the new root
        return insert_non_full(new_root, key, position, index->key_type);
    }
    
    // Insert into the non-full root
    return insert_non_full(index->root, key, position, index->key_type);
}

/**
 * @brief Find exact matches for a key
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
                    // Reached maximum positions
                    return count;
                }
            } else if (cmp > 0) {
                break;
            }
        }
        
        if (node->is_leaf) {
            break;
        }
        
        node = node->children[i];
    }
    
    return count;
}

/**
 * @brief Collect all positions from a subtree
 */
static int collect_all_positions(BTreeNode* node, int* positions, 
                               int max_positions, int current_count) {
    if (!node || current_count >= max_positions) {
        return current_count;
    }
    
    if (node->is_leaf) {
        // Collect all positions in this leaf
        for (int i = 0; i < node->key_count && current_count < max_positions; i++) {
            positions[current_count++] = node->positions[i];
        }
    } else {
        // Process first child
        current_count = collect_all_positions(node->children[0], positions, 
                                            max_positions, current_count);
        
        // Process keys and remaining children
        for (int i = 0; i < node->key_count && current_count < max_positions; i++) {
            // Add position for current key
            positions[current_count++] = node->positions[i];
            
            // Process child to the right of this key
            current_count = collect_all_positions(node->children[i + 1], positions, 
                                                max_positions, current_count);
        }
    }
    
    return current_count;
}

/**
 * @brief Find keys in a range
 */
int btree_find_range(const BTreeIndex* index, const void* low_key, 
                    const void* high_key, int* positions, int max_positions) {
    if (!index || !positions || max_positions <= 0) {
        return -1;
    }
    
    if (!low_key && !high_key) {
        // Collect all positions in the index
        return collect_all_positions(index->root, positions, max_positions, 0);
    }
    
    int count = 0;
    BTreeNode* node = index->root;
    
    // Find the first leaf node that may contain values in range
    while (!node->is_leaf) {
        int i = 0;
        
        if (low_key) {
            // Find the child that may contain the low key
            while (i < node->key_count && 
                  compare_keys(node->keys[i], low_key, index->key_type) < 0) {
                i++;
            }
        }
        
        node = node->children[i];
    }
    
    // Starting position in the leaf
    int i = 0;
    
    // Skip keys less than low_key
    if (low_key) {
        while (i < node->key_count && 
              compare_keys(node->keys[i], low_key, index->key_type) < 0) {
            i++;
        }
    }
    
    // Collect keys in range
    for (; i < node->key_count && count < max_positions; i++) {
        // Check upper bound if provided
        if (high_key && compare_keys(node->keys[i], high_key, index->key_type) > 0) {
            break;
        }
        
        positions[count++] = node->positions[i];
    }
    
    // TODO: Traverse to next leaf nodes if needed
    
    return count;
}

/**
 * @brief Generate code for a B-tree node (placeholder implementation)
 */
static int generate_node_code(const BTreeNode* node, char* output, size_t output_size, 
                            int node_id) {
    if (!node || !output || output_size <= 0) {
        return -1;
    }
    
    int written = 0;
    
    // Generate node structure
    written += snprintf(output + written, output_size - written,
        "static BTreeNode node_%d = {\n"
        "    .is_leaf = %s,\n"
        "    .key_count = %d,\n"
        "    .keys = {",
        node_id, node->is_leaf ? "true" : "false", node->key_count);
    
    // Add keys
    for (int i = 0; i < BTREE_ORDER - 1; i++) {
        if (i > 0) {
            written += snprintf(output + written, output_size - written, ", ");
        }
        
        if (i < node->key_count && node->keys[i]) {
            written += snprintf(output + written, output_size - written, "&key_%d_%d", node_id, i);
        } else {
            written += snprintf(output + written, output_size - written, "NULL");
        }
    }
    
    written += snprintf(output + written, output_size - written, "},\n");
    
    // Add positions
    written += snprintf(output + written, output_size - written, "    .positions = {");
    
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
    
    written += snprintf(output + written, output_size - written, "},\n");
    
    // Add children
    written += snprintf(output + written, output_size - written, "    .children = {");
    
    for (int i = 0; i < BTREE_ORDER; i++) {
        if (i > 0) {
            written += snprintf(output + written, output_size - written, ", ");
        }
        
        if (i <= node->key_count && node->children[i]) {
            // Note: In a complete implementation, we would need a way to map 
            // node pointers to node IDs. For now, just use a placeholder.
            written += snprintf(output + written, output_size - written, "&node_0");
        } else {
            written += snprintf(output + written, output_size - written, "NULL");
        }
    }
    
    written += snprintf(output + written, output_size - written, "},\n");
    
    // Add key type
    written += snprintf(output + written, output_size - written, 
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
    
    // Add header comment
    written += snprintf(output + written, output_size - written,
        "/**\n"
        " * @brief Generated B-tree index for %s column\n"
        " */\n\n",
        index->column_name);
    
    // Add includes
    written += snprintf(output + written, output_size - written,
        "#include <stdbool.h>\n"
        "#include \"../schema/type_system.h\"\n"
        "#include \"btree_index.h\"\n\n");
    
    // TODO: Generate code for all nodes and keys
    
    // Generate index structure
    written += snprintf(output + written, output_size - written,
        "static BTreeIndex generated_index = {\n"
        "    .root = &node_0,\n"
        "    .column_name = \"%s\",\n"
        "    .key_type = %d,\n"
        "    .height = %d,\n"
        "    .node_count = %d\n"
        "};\n\n",
        index->column_name, index->key_type, index->height,
        index->node_count);
    
    // Generate accessor function
    written += snprintf(output + written, output_size - written,
        "BTreeIndex* get_index(void) {\n"
        "    return &generated_index;\n"
        "}\n");
    
    return 0;
}

/**
 * @brief Compare function for qsort (will be used in future)
 */
static int compare_pairs(const void* a, const void* b) {
    const KeyValuePair* pair_a = (const KeyValuePair*)a;
    const KeyValuePair* pair_b = (const KeyValuePair*)b;
    
    // Compare keys based on their type
    // For simplicity, we're using a fixed type here
    return compare_keys(pair_a->key, pair_b->key, TYPE_INT); // Simplified
    
    return 0; // To satisfy all control paths
}

/**
 * @brief Build a B-tree from sorted key-value pairs
 */
BTreeIndex* btree_build_from_sorted(const KeyValuePair* pairs, int pair_count,
                                   const char* column_name, DataType key_type) {
    if (!pairs || pair_count <= 0 || !column_name) {
        return NULL;
    }
    
    // Initialize empty index
    BTreeIndex* index = btree_init(column_name, key_type);
    if (!index) {
        return NULL;
    }
    
    // Insert each key-value pair into the index
    for (int i = 0; i < pair_count; i++) {
        if (btree_insert(index, pairs[i].key, pairs[i].position) != 0) {
            btree_free(index);
            return NULL;
        }
    }
    
    return index;
}
