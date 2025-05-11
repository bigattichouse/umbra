/* In a new file: src/util/uuid_utils.c */

#include <stdlib.h>
#include <stdio.h>
#include <uuid/uuid.h>
#include "uuid_utils.h"

/**
 * @brief Generate a UUID string
 */
int generate_uuid(char* out) {
    if (!out) {
        return -1;
    }
    
    uuid_t uuid;
    uuid_generate(uuid);
    uuid_unparse_lower(uuid, out);
    return 0;
}
