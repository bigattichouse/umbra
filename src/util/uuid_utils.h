/* In a new file: src/util/uuid_utils.h */

#ifndef UMBRA_UUID_UTILS_H
#define UMBRA_UUID_UTILS_H

/**
 * @brief Generate a UUID string
 * @param out Buffer to receive the UUID string (must be at least 37 bytes)
 * @return 0 on success, -1 on error
 */
int generate_uuid(char* out);

#endif /* UMBRA_UUID_UTILS_H */
