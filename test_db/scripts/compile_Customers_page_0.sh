#!/bin/bash

# Compile data page 0 for table Customers

CC=${CC:-gcc}
CFLAGS="-fPIC -shared -O2 -g"

# Create compiled directory if it doesn't exist
mkdir -p ./test_db/compiled

# Compile the data page
$CC $CFLAGS -o ./test_db/compiled/CustomersData_0.so ./test_db/tables/Customers/src/CustomersData_0.c

echo "Compiled CustomersData_0.so"
