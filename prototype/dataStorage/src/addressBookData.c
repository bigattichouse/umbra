#include "umbraMetadata.h"
#include "addressBook.h"
#include "addressBookPermission.h"
#include "addressBookDataPages.h"

long long unsigned int _CustomerPageCount(){
  return CustomerPageCount;
}

struct CustomersDB _Customers(int page){
   struct CustomersDB DB;
   DB.size = CustomersPages[page].size;
   DB.Customers = CustomersPages[page].Customers;
   return DB;
}
