/**
 * @file permission.c
 * @brief Permission structure implementations
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include "permission.h"

/**
 * @brief Check if a user has a specific permission
 */
bool has_permission(const UserPermission* permission, PermissionType type) {
    if (!permission) {
        return false;
    }
    
    return (permission->permissions & type) != 0;
}

/**
 * @brief Set a permission for a user
 */
void set_permission(UserPermission* permission, PermissionType type) {
    if (!permission) {
        return;
    }
    
    permission->permissions |= type;
}

/**
 * @brief Remove a permission from a user
 */
void remove_permission(UserPermission* permission, PermissionType type) {
    if (!permission) {
        return;
    }
    
    permission->permissions &= ~type;
}

/**
 * @brief Save permissions to file
 */
int save_permissions(const UserPermission* permissions, int count, const char* directory) {
    if (!permissions || count <= 0 || !directory) {
        return -1;
    }
    
    // Create permissions directory if it doesn't exist
    char perm_dir[1024];
    snprintf(perm_dir, sizeof(perm_dir), "%s/permissions", directory);
    
    struct stat st;
    if (stat(perm_dir, &st) != 0) {
        if (mkdir(perm_dir, 0755) != 0) {
            fprintf(stderr, "Failed to create permissions directory: %s\n", strerror(errno));
            return -1;
        }
    }
    
    // Create permissions file path
    char perm_path[2048];
    snprintf(perm_path, sizeof(perm_path), "%s/user_permissions.dat", perm_dir);
    
    // Write permissions to file
    FILE* file = fopen(perm_path, "wb");
    if (!file) {
        fprintf(stderr, "Failed to open permissions file for writing: %s\n", strerror(errno));
        return -1;
    }
    
    // Write count first
    if (fwrite(&count, sizeof(int), 1, file) != 1) {
        fclose(file);
        fprintf(stderr, "Failed to write permission count\n");
        return -1;
    }
    
    // Write permissions
    size_t written = fwrite(permissions, sizeof(UserPermission), count, file);
    fclose(file);
    
    if (written != (size_t)count) {
        fprintf(stderr, "Failed to write all permissions\n");
        return -1;
    }
    
    return 0;
}

/**
 * @brief Load permissions from file
 */
int load_permissions(const char* directory, UserPermission** permissions, int* count) {
    if (!directory || !permissions || !count) {
        return -1;
    }
    
    // Create permissions file path
    char perm_path[2048];
    snprintf(perm_path, sizeof(perm_path), "%s/permissions/user_permissions.dat", directory);
    
    // Read permissions from file
    FILE* file = fopen(perm_path, "rb");
    if (!file) {
        // File doesn't exist - no permissions
        *permissions = NULL;
        *count = 0;
        return 0;
    }
    
    // Read count first
    if (fread(count, sizeof(int), 1, file) != 1) {
        fclose(file);
        return -1;
    }
    
    if (*count <= 0) {
        fclose(file);
        *permissions = NULL;
        return 0;
    }
    
    // Allocate memory for permissions
    *permissions = malloc(*count * sizeof(UserPermission));
    if (!*permissions) {
        fclose(file);
        return -1;
    }
    
    // Read permissions
    size_t read = fread(*permissions, sizeof(UserPermission), *count, file);
    fclose(file);
    
    if (read != (size_t)*count) {
        free(*permissions);
        *permissions = NULL;
        *count = 0;
        return -1;
    }
    
    return 0;
}
