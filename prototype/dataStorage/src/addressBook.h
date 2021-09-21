struct Customers {
    char* uuid;
    char* name;
    char* city;
    struct Permission* permissions;
    struct umbraMetadata metadata;
};

struct CustomersDB {
   long long unsigned int size;
   struct Customers* Customers;
};

struct CustomersDB _Customers(int page);
