#ifndef UMBRA_STRUCT_Customers_H
#define UMBRA_STRUCT_Customers_H

#include <time.h>
#include <stdbool.h>

/**
 * @struct Customers
 * @brief Generated struct for Customers table
 */
typedef struct {
    /* Column: id */
    int id;
    /* Column: name */
    char name[101];
    /* Column: email */
    char email[101];
    /* Column: age */
    int age;
    /* Column: active */
    bool active;
} Customers;

/**
 * @brief Returns the number of records in the page
 * @return Number of records
 */
int count(void);

/**
 * @brief Returns a record at the specified position
 * @param pos Position of the record
 * @return Pointer to the record or NULL if out of bounds
 */
Customers* read(int pos);

#endif /* UMBRA_STRUCT_Customers_H */
