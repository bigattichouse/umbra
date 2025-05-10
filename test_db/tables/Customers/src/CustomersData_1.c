#include <stdlib.h>
#include "../Customers.h"

/* Data array containing records */
static Customers CustomersData_1[] = {
    /*BEGIN Customers DATA*/
#include "../data/CustomersData.1.dat.h"
    /*END Customers DATA*/
};

/**
 * @brief Returns the number of records in the page
 * @return Number of records
 */
int count(void) {
    return sizeof(CustomersData_1) / sizeof(Customers);
}

/**
 * @brief Returns a record at the specified position
 * @param pos Position of the record
 * @return Pointer to the record or NULL if out of bounds
 */
Customers* read(int pos) {
    if (pos < 0 || pos >= count()) {
        return NULL;
    }
    return &CustomersData_1[pos];
}
