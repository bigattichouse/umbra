#include <stddef.h>
#include "umbraMetadata.h"
#include "addressBook.h"
#include "addressBookPermission.h"
#include "addressBook/addressBookDataPages.h"
#include <dirent.h>

long long unsigned int _CustomerPageCount(){
  int file_count = 0;
  DIR * dirp;
  struct dirent * entry;

  dirp = opendir("./addressBook/headers"); /* There should be error handling after this */
  while ((entry = readdir(dirp)) != NULL) {
      if (entry->d_type == DT_REG) { /* If the entry is a regular file */
           file_count++;
      }
  }
  closedir(dirp);
  return file_count;
}

struct CustomersDB _Customers(int page){
   struct CustomersDB DB;
   DB.size = CustomersPages[page].size;
   DB.Customers = CustomersPages[page].Customers;
   return DB;
}


// END with NULL to know where to stop, also dynamically load the pages instead of compiling them into the app.
