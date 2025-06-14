/**
 * @file page_template.h
 * @brief Page template definitions
 */

#ifndef UMBRA_PAGE_TEMPLATE_H
#define UMBRA_PAGE_TEMPLATE_H

/**
 * @brief Template for data page header file
 */
#define DATA_PAGE_HEADER_TEMPLATE \
    "/*This file autogenerated, do not edit manually*/\n"

/**
 * @brief Template for C source file with accessor functions
 */
#define ACCESSOR_SOURCE_TEMPLATE \
    "#include <stdlib.h>\n" \
    "#include \"../%s.h\"\n\n" \
    "/* Data array containing records */\n" \
    "static %s %sData_%d[] = {\n" \
    "    /*BEGIN %s DATA*/\n" \
    "#include \"../data/%sData.%d.dat.h\"\n" \
    "    /*END %s DATA*/\n" \
    "};\n\n" \
    "/**\n" \
    " * @brief Returns the number of records in the page\n" \
    " * @return Number of records\n" \
    " */\n" \
    "int count(void) {\n" \
    "    return sizeof(%sData_%d) / sizeof(%s);\n" \
    "}\n\n" \
    "/**\n" \
    " * @brief Returns a record at the specified position\n" \
    " * @param pos Position of the record\n" \
    " * @return Pointer to the record or NULL if out of bounds\n" \
    " */\n" \
    "%s* read(int pos) {\n" \
    "    if (pos < 0 || pos >= count()) {\n" \
    "        return NULL;\n" \
    "    }\n" \
    "    return &%sData_%d[pos];\n" \
    "}\n"

/**
 * @brief Template for compilation script
 */
#define COMPILE_SCRIPT_TEMPLATE \
    "#!/bin/bash\n\n" \
    "# Compile data page %d for table %s\n\n" \
    "CC=${CC:-gcc}\n" \
    "CFLAGS=\"-fPIC -shared -O2 -g\"\n\n" \
    "# Create compiled directory if it doesn't exist\n" \
    "mkdir -p %s/compiled\n\n" \
    "# Compile the data page\n" \
    "$CC $CFLAGS -o %s/compiled/%sData_%d.so %s/tables/%s/src/%sData_%d.c\n\n" 

#endif /* UMBRA_PAGE_TEMPLATE_H */
