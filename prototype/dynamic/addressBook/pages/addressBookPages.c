/*This file autogenerated, do not edit*/
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "../schema/umbraMetadata.h"
#include "../schema/addressBook.h"
#include "addressBookPages.h"

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

int unload_page(struct addressBookDynamicPage page){
      return dlclose(page.handle);
}
