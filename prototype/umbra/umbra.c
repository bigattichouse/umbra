#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "files.h"
#include "verbs.h"

int main(int argc, char *argv[] ) {

  struct umbraFieldDefs AddressBookDef[] = {
     {"field","uuid","string"},
     {"field","name","string"},
     {"field","company","string"},
     {"field","email","string"},
     {"field","address1","string"},
     {"field","address2","string"},
     {"field","city","string"},
     {"field","state","string"},
     {"field","zip","string"},
     {"index","uuid","unique"}, //TODO
     {"index","email",""} //TODO
  };

  struct umbraParsedRecord AddressBookRec[] = {
     {"field","uuid","string","someUUID",0,0,0},
     {"field","name","string","Michael Johnson",0,0,0},
     {"field","company","string","BigAtticHouse",0,0,0},
     {"field","email","string","father@bigattichouse.com",0,0,0},
     {"field","address1","string","424 S Division",0,0,0},
     {"field","address2","string","--",0,0,0},
     {"field","city","string","Chenoa",0,0,0},
     {"field","state","string","IL",0,0,0},
     {"field","zip","string","61726",0,0,0}
  };


 create_directory("./database");
 create_directory("./database/AddressBook/");
 struct umbraTableDef AddressBookTable = create_table_definition("./database/AddressBook/","AddressBook",AddressBookDef, sizeof(AddressBookDef)/sizeof(AddressBookDef[0]));
 create_main_header(AddressBookTable); //AddressBook.h
 create_data_reader(AddressBookTable); //AddressBookData.h
 create_data_page_header(AddressBookTable); //AddressBookDataPages.h
 create_first_data_page(AddressBookTable); //AddressBookData_0.h
 create_boilerplate (AddressBookTable); // main() to allow commandline reads

 append_record(AddressBookTable,AddressBookRec); //insert rec into current datapage

 exit(0);
}

/*

CREATE DATABASE TestDB;
USE DATABASE TestDB;

CREATE ENUMERATION StatusType (["active","inactive"]);

CREATE FLAGS Permissions ( ["read","write","exec"], int );  so three possible flags, all int values
CREATE FLAGS TEMPLATE readOnly ({read:1,write:0,exec:0});
CREATE TABLE CUSTOMERS {uuid:string,name:string,city:string,permissions:Permissions}

CREATE [UNIQUE] INDEX CUSTOMERS (UUID);

*/
