//gcc -ldl -o ../bin/addressBookData.0.so addressBookData.0.c  -shared -fPIC

#include <stdio.h>
#include "../schema/umbraMetadata.h"
#include "../schema/addressBookPermission.h"
#include "../schema/addressBook.h"
#include "../headers/addressBookData.0.h"

int count(void) {
  return sizeof(CustomersData_0)/sizeof(CustomersData_0[0]);
}

struct Customers *read(int pos){
 int size = count();
 if((pos<size) && (pos>=0)){
   return &CustomersData_0[pos];
 }
 return NULL;
}
