#include <stdio.h>
#include <stdlib.h>

#include "addressBook.h"

int main(int argc, char *argv[] ) {
   // printf() displays the string inside quotation
   struct leaf *Customers = _Customers(0);
   int cust = 3;
   printf("3.Name = %s\n",Customers[cust].record.name);
   printf("3.permissions.%s=%d\n",Customers[cust].record.permissions[0].action, Customers[cust].record.permissions[0].permit);
   printf("3.permissions.%s=%d\n",Customers[cust].record.permissions[1].action, Customers[cust].record.permissions[1].permit);
   return 0;
}
