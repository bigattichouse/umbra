struct Customers_Page {
    int size;
    struct Customers *Customers;
};

long long unsigned int CustomerPageCount = 2;
long long unsigned int CustomerPageMaxSize = 65535; //only allow 64k records per page

#include "headers/addressBookData_0.h"
#include "headers/addressBookData_1.h"

struct Customers_Page CustomersPages[] = {
  {CustomersSize_0,CustomersData_0},
  {CustomersSize_1,CustomersData_1}
};
