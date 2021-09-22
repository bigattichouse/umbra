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

 create_directory("./database");
 create_directory("./database/AddressBook/");
 create_boilerplate ("./database/AddressBook/","AddressBook",AddressBookDef, sizeof(AddressBookDef)/sizeof(AddressBookDef[0]));

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
