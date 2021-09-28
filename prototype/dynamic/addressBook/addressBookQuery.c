
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "schema/umbraMetadata.h"
#include "schema/addressBookPermission.h"
#include "schema/addressBook.h"
#include "pages/addressBookPages.h"

/*

This binary is a prototype that loads a query kernel module
and uses it to query against records, only outputting those that pass

The query will have a floating point "score", but in this case a "pass" will
only be 1 or 0. This will allow creating vectors and other query types that
might have a fractional result in the future.

*/

//gcc addressBookQuery.c -o addressBookQuery -ldl


int main(int argc, char *argv[] ) {
    struct Customers *data;
    struct addressBookDynamicPage page = loadPage(0);
    struct umbraQueryKernel query = loadKernel();
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
