struct umbraFieldDefs {
     char *defType;
     char *field;
     char *fieldType;
};

int create_directory(char *path);
int create_boilerplate (char *path, char *tableName, struct umbraFieldDefs definitions[], int size);
