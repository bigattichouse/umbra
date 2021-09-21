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

long long unsigned int _CustomerPageCount();
struct CustomersDB _Customers(int page);
