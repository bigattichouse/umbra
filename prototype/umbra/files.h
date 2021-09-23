struct umbraFieldDefs {
     char *defType;
     char *field;
     char *fieldType;
};

struct umbraParsedRecord {
  char *defType;
  char *field;
  char *fieldType;
  char *string;
  int int;
  float float;
  long int timestamp;
};

struct umbraTableDef {
   char *directory;
   char *tablename;
   struct umbraFieldDefs *Fields;
   int fieldCount;
};

int create_directory(char *path);
struct umbraTableDef create_table_definition(char *directory,char *tablename,struct umbraFieldDefs *fieldDefs,int fieldCount);
int create_main_header(struct umbraTableDef TableDef); //AddressBook.h
int create_data_reader(struct umbraTableDef TableDef); //AddressBookData.h
int create_data_page_header(struct umbraTableDef TableDef); //AddressBookDataPages.h
int create_first_data_page(struct umbraTableDef TableDef); //AddressBookData_0.h
int create_boilerplate (struct umbraTableDef TableDef); // main() to allow commandline reads

int append_record(struct umbraTableDef TableDef,struct umbraParsedRecord ParsedRecord); //insert rec into current datapage
