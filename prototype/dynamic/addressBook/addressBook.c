
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "schema/umbraMetadata.h"
#include "schema/addressBookPermission.h"
#include "schema/addressBook.h"
#include "headers/addressBookPages.h"

/*
This is the shared library that counts and dynamically loads data pages,
it provides the API used by apps to access information.

Later this will include inserts and updates, for now just read only.

For now, this will just be a simple C program to load the relevant pages, then
we'll go up one level

*/

//gcc addressBook.c -o addressBook -ldl

char *addressBookPageFileName(int page){
  size_t nbytes = snprintf(NULL, 0, "bin/addressBookData.%d.so", page) + 1; /* +1 for the '\0' */
  char *str = malloc(nbytes);
  snprintf(str, nbytes, "bin/addressBookData.%d.so", page);
  return str;
}

struct addressBookDynamicPage loadPage(int page){
     struct addressBookDynamicPage loaded;
     loaded.page = page;
     char *filename = addressBookPageFileName(page);
     loaded.handle = dlopen(filename, RTLD_LAZY);
     free(filename);
     if (!loaded.handle) {
          /* fail to load the library */
          fprintf(stderr, "Error: %s\n", dlerror());
          loaded.error = EXIT_FAILURE;
          return loaded;
     }

     *(int **)(&loaded.count) = dlsym(loaded.handle, "count");
      if (!loaded.count) {
          /* no such symbol */
          fprintf(stderr, "Error: %s\n", dlerror());
          dlclose(loaded.handle);
          loaded.error = EXIT_FAILURE;
          return loaded;
      }

      *(struct Customers **)(&loaded.read) = dlsym(loaded.handle, "read");
       if (!loaded.read) {
           /* no such symbol */
           fprintf(stderr, "Error: %s\n", dlerror());
           dlclose(loaded.handle);
           loaded.error = EXIT_FAILURE;
           return loaded;
       }
  return loaded;
}

int main(int argc, char *argv[] ) {
    struct Customers *data;
    struct addressBookDynamicPage page = loadPage(1);
    if(page.error>0){
      return page.error;
    }
    int records = page.count();
    data = page.read(0);
    struct Customers *rec;
    int pos = 0;

    while((rec = page.read(pos))!=NULL){
      printf("%d.Name = %s\n",pos,rec->name);
      pos++;
    }

    dlclose(page.handle);

    return EXIT_SUCCESS;

}
