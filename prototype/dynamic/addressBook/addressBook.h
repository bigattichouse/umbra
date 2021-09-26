/*
CREATE TABLE CUSTOMERS {uuid:string,name:string,city:string,permissions:Permissions}
*/

struct Customers {
    char* uuid;
    char* name;
    char* city;
    struct Permission* permissions;
    struct umbraMetadata metadata;
};

