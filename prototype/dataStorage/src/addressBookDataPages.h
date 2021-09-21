struct Customers_Page {
    int size;
    struct Customers *Customers;
};

#include "addressBookData_0.h"
#include "addressBookData_1.h"

long long unsigned int CustomerPageCount = 2;
long long unsigned int CustomerPageMaxSize = 65535; //only allow 64k records per page

struct Customers_Page CustomersPages[] = {
  {CustomersSize_0,CustomersData_0},
  {CustomersSize_1,CustomersData_1}
};
