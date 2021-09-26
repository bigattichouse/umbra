
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "umbraMetadata.h"
#include "addressBook.h"

/*
This is the shared library that counts and dynamically loads data pages,
it provides the API used by apps to access information.

Later this will include inserts and updates, for now just read only.

For now, this will just be a simple C program to load the relevant pages, then
we'll go up one level

*/



int main(int argc, char *argv[] ) {
   void *handle;
   void (*count)();
   void (*read)();
   Customers *data;

   handle = dlopen("bin/addressBookData.0.so", RTLD_LAZY);



   if (!handle) {
        /* fail to load the library */
        fprintf(stderr, "Error: %s\n", dlerror());
        return EXIT_FAILURE;
   }

   *(void**)(&count) = dlsym(handle, "count");
    if (!func_print_name) {
        /* no such symbol */
        fprintf(stderr, "Error: %s\n", dlerror());
        dlclose(handle);
        return EXIT_FAILURE;
    }

    *(void**)(&read) = dlsym(handle, "read");
     if (!func_print_name) {
         /* no such symbol */
         fprintf(stderr, "Error: %s\n", dlerror());
         dlclose(handle);
         return EXIT_FAILURE;
     }

    int records = count();
    data = read(0);
    Customer *rec;
    pos = 0;

    while((rec = read(pos))!=NULL){
      printf("%d.Name = %s\n",pos,rec.name);
      pos++;
    }

    dlclose(handle);

    return EXIT_SUCCESS;

}
