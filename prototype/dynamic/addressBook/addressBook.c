
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "schema/umbraMetadata.h"
#include "schema/addressBookPermission.h"
#include "schema/addressBook.h"
#include "pages/addressBookPages.h"

/*
This is the shared library that counts and dynamically loads data pages,
it provides the API used by apps to access information.

Later this will include inserts and updates, for now just read only.

For now, this will just be a simple C program to load the relevant pages, then
we'll go up one level

*/

//gcc addressBook.c -o addressBook -ldl


int main(int argc, char *argv[] ) {
    struct Customers *data;
    struct addressBookDynamicPage page = loadPage(0);
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

    unload_page(page);

    return EXIT_SUCCESS;

}
