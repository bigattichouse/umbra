#!/bin/bash

# Compile data page 1 for table Customers

CC=${CC:-gcc}
CFLAGS="-fPIC -shared -O2 -g"

# Create compiled directory if it doesn't exist
mkdir -p ./test_db/compiled

# Compile the data page
$CC $CFLAGS -o ./test_db/compiled/CustomersData_1.so ./test_db/tables/Customers/src/CustomersData_1.c

echo "Compiled CustomersData_1.so"
