#include "umbraMetadata.h"
#include "addressBook.h"
#include "addressBookPermission.h"

long long unsigned int CustomersSize = 5;

struct Customers CustomersData[1000000] =  {
{"0","John Q Public0","Chenoa", readOnly ,{0.1,0,0,0,0}},
{"1","John Q Public1","Chenoa", readOnly ,{0.1,0,0,0,0}},
{"2","John Q Public2","Chenoa", readOnly ,{0.1,0,0,0,0}},
{"3","John Q Public3","Chenoa", readOnly ,{0.1,0,0,0,0}},
{"4","John Q Public3","Chenoa", readOnly ,{0.1,0,0,0,0}}
};

struct CustomersDB _Customers(int page){
   struct CustomersDB DB;
   DB.size = CustomersSize;
   DB.Customers = CustomersData;
   return DB;
}
