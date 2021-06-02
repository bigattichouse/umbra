/*
creates a sample c file with one million records

1 millions records in C was 158Meg compiled, but preety much instant by index
emcc kille dthe machine trying to compile 1 million

emcc 10k records : initial memory too small, 21473840 bytes needed
-s TOTAL_MEMORY=16MB  to set memory
node loadTest.js > lotsa_data.c
emcc lotsa_data.c  -s ALLOW_MEMORY_GROWTH=1 -s STANDALONE_WASM -s TOTAL_MEMORY=1GB

1M records kills the machine with swapping, and hae to reboot.
10k works, compiles in about 5 sec   512MB
100k works, compiles in about 12-16 sec 512MB
250k works, compiles in about 25 sec 1GB
1M dies

Raw SQL is slightly larger than wasm version

gcc:
50k records compiles in 2 s  = 12500 rec/sec
250k records compiles in 15 s = 16666 rec/sec  (62.5 Meg)  second time @ 13s -O3 => 60 Meg
500k records compiles in 31 s = 16129 rec/sec  (93 Meg)
1M timed out.. there's probably a memory cap there.

*/
console.log("");
console.log("#include <stdio.h>");
console.log("#include <stdlib.h>");
console.log("");
console.log("struct Permission {");
console.log("   char* action;");
console.log("   int permit;");
console.log("};");
console.log("");
console.log("struct Customer {");
console.log("    char* uuid;");
console.log("    char* name;");
console.log("    char* city;");
console.log("    struct Permission* permissions;");
console.log("};");
console.log("");
console.log("struct leaf {");
console.log("   struct Customer record;");
console.log("};");
console.log("");
console.log("");
console.log("/* all lookup-style values stored as explicit vals, normally a whole other file*/");
console.log("struct Permission readOnly[] = {{\"read\",1},{\"write\",0}};");
console.log("struct Permission readWrite[] = {{\"read\",1},{\"write\",1}};");

console.log("");
console.log("struct leaf Customers[1000000] = {");
for(var i=0;i<250000;i++){
    comma="";
    if (i<249999) comma=",";
    console.log("{{\""+i+"\",\"John Q Public"+i+"\",\"Chenoa\", readOnly }}"+comma);
}
console.log("};");
console.log("int main(int argc, char *argv[] ) {");
   console.log("// printf() displays the string inside quotation");
   console.log("int cust = 7500;");
   console.log("printf(\"7500.Name = %s\\n\",Customers[cust].record.name);");
   console.log("printf(\"7500.permissions.%s=%d\\n\",Customers[cust].record.permissions[0].action, Customers[cust].record.permissions[0].permit);");
   console.log("printf(\"7500.permissions.%s=%d\\n\",Customers[cust].record.permissions[1].action, Customers[cust].record.permissions[1].permit);");
   console.log("return 0;");
console.log("}");
