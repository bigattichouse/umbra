#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "umbraMetadata.h"
#include "addressBook.h"
#include "addressBookPermission.h"


int main(int argc, char *argv[] ) {
   // printf() displays the string inside quotation
   for(int page = 0; page < _CustomerPageCount(); page++){
   struct CustomersDB Customers = _Customers(page);
   for(int cust = 0; cust<Customers.size; cust++){
     printf("%d.Name = %s\n",cust,Customers.Customers[cust].name);
     printf("%d.permissions.%s=%d\n",cust,Customers.Customers[cust].permissions[0].action, Customers.Customers[cust].permissions[0].permit);
     printf("%d.permissions.%s=%d\n",cust,Customers.Customers[cust].permissions[1].action, Customers.Customers[cust].permissions[1].permit);
     printf("%d.permissions.%s=%d\n",cust,Customers.Customers[cust].permissions[2].action, Customers.Customers[cust].permissions[2].permit);
   }
   }
   return 0;
}
