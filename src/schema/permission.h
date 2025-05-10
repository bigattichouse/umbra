/**
 * @file permission.h
 * @brief Permission structure definitions
 */

#ifndef UMBRA_PERMISSION_H
#define UMBRA_PERMISSION_H

#include <stdbool.h>

/**
 * @enum PermissionType
 * @brief Types of permissions
 */
typedef enum {
    PERM_READ = 0x01,
    PERM_WRITE = 0x02,
    PERM_CREATE = 0x04,
    PERM_DROP = 0x08,
    PERM_ADMIN = 0xFF
} PermissionType;

/**
 * @struct UserPermission
 * @brief Permission for a user on a table
 */
typedef struct {
    char user[64];                 /**< Username */
    char table[64];                /**< Table name */
    PermissionType permissions;    /**< Permission flags */
} UserPermission;

/**
 * @brief Check if a user has a specific permission
 * @param permission User permission
 * @param type Permission type to check
 * @return true if has permission, false otherwise
 */
bool has_permission(const UserPermission* permission, PermissionType type);

/**
 * @brief Set a permission for a user
 * @param permission User permission to modify
 * @param type Permission type to set
 */
void set_permission(UserPermission* permission, PermissionType type);

/**
 * @brief Remove a permission from a user
 * @param permission User permission to modify
 * @param type Permission type to remove
 */
void remove_permission(UserPermission* permission, PermissionType type);

/**
 * @brief Save permissions to file
 * @param permissions Array of permissions
 * @param count Number of permissions
 * @param directory Directory to save to
 * @return 0 on success, -1 on error
 */
int save_permissions(const UserPermission* permissions, int count, const char* directory);

/**
 * @brief Load permissions from file
 * @param directory Directory to load from
 * @param permissions Output parameter for permissions array
 * @param count Output parameter for number of permissions
 * @return 0 on success, -1 on error
 */
int load_permissions(const char* directory, UserPermission** permissions, int* count);

#endif /* UMBRA_PERMISSION_H */
