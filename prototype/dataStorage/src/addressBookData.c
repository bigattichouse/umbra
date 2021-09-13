#include "addressBook.h"
#include "addressBookPermission.h"

struct leaf Customers[1000000] = {
{{"0","John Q Public0","Chenoa", readOnly }},
{{"1","John Q Public1","Chenoa", readOnly }},
{{"2","John Q Public2","Chenoa", readOnly }},
{{"3","John Q Public3","Chenoa", readOnly }},
{{"4","John Q Public3","Chenoa", readOnly }}
};

struct leaf *_Customers(int page){
   return Customers;
}
