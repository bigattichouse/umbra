//gcc -ldl -o ../bin/addressBookData.1.so addressBookData.1.c  -shared -fPIC

#include <stdio.h>
#include "../schema/umbraMetadata.h"
#include "../schema/addressBookPermission.h"
#include "../schema/addressBook.h"
#include "../headers/addressBookData.1.h"

int count(void) {
  return sizeof(CustomersData_1)/sizeof(CustomersData_1[0]);
}

struct Customers *read(int pos){
 int size = count();
 if((pos<size) && (pos>=0)){
   return &CustomersData_1[pos];
 }
 return NULL;
}
