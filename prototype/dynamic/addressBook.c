#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "umbraMetadata.h"
#include "addressBook.h" 


int main(int argc, char *argv[] ) {
   void *handle;
   void (*load_page)();
   Customers *data;

   handle = dlopen("addressBook/bin/addressBook_0.so", RTLD_LAZY);

   if (!handle) {
        /* fail to load the library */
        fprintf(stderr, "Error: %s\n", dlerror());
        return EXIT_FAILURE;
   }
   
   *(void**)(&load_page) = dlsym(handle, "load_page");
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
