struct Customer {
    char* uuid;
    char* name;
    char* city;
    struct Permission* permissions;
};

struct leaf {
   struct Customer record;
};


struct leaf *_Customers(int page);
