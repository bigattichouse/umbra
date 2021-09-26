struct Permission {
   char* action;
   int permit;
};

// CREATE FLAGS Permissions ( ["read","write","exec"], int );  so three possible flags, all int values
int PermissionSize = 3;

// CREATE FLAG ALIAS readOnly({read:1,write:0,exec:0});
struct Permission readOnly[] = {{"read",1},{"write",0},{"exec",0}};
struct Permission readWrite[] = {{"read",1},{"write",1},{"exec",0}};
struct Permission readWriteExec[] = {{"read",1},{"write",1},{"exec",1}};
