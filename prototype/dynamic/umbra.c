#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "umbraMetadata.h"
#include "addressBook/addressBook.h"

/*
So, each "table" has a standard interface:  pageCount, loadPage

Each of those modules has a recordCount(), record(position).
Internally those modules will report back a record or NULL to prevent positions beyond the count.

Later add an "insert" record.

SO for this iteration of umbra, we're dynamically loading the addressBook table


*/

int main(int argc, char *argv[] ) {
   void *handle;
   void (*load_page)();
   Customers *data;

   handle = dlopen("addressBook/bin/addressBook.so", RTLD_LAZY);



   if (!handle) {
        /* fail to load the library */
        fprintf(stderr, "Error: %s\n", dlerror());
        return EXIT_FAILURE;
   }

   *(void**)(&load_page) = dlsym(handle, "pageCount");
    if (!func_print_name) {
        /* no such symbol */
        fprintf(stderr, "Error: %s\n", dlerror());
        dlclose(handle);
        return EXIT_FAILURE;
    }

    data = loadpage();
    Customer *rec;
    pos = 0;

    while((rec = data[pos])!=NULL){
      printf("%d.Name = %s\n",pos,rec.name);
      pos++;
    }

    dlclose(handle);

    return EXIT_SUCCESS;

}
