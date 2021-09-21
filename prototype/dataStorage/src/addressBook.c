#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "umbraMetadata.h"
#include "addressBook.h"
#include "addressBookPermission.h"


int main(int argc, char *argv[] ) {
   // printf() displays the string inside quotation
   struct CustomersDB Customers = _Customers(0);
   int cust = 3;
   printf("3.Name = %s\n",Customers.Customers[cust].name);
   printf("3.permissions.%s=%d\n",Customers.Customers[cust].permissions[0].action, Customers.Customers[cust].permissions[0].permit);
   printf("3.permissions.%s=%d\n",Customers.Customers[cust].permissions[1].action, Customers.Customers[cust].permissions[1].permit);
   return 0;
}
