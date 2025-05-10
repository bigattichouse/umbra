#!/bin/bash

# Debug helper script for Umbra database tests

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Umbra Debug Helper${NC}"
echo "-------------------"

# Check if we need to rebuild
if [ ! -f "build/bin/test_create_table" ] || [ "$1" == "rebuild" ]; then
    echo -e "${YELLOW}Rebuilding with debug flags...${NC}"
    make clean
    make
fi

# Create clean test environment
rm -rf test_db

case "$1" in
    "gdb")
        echo -e "${YELLOW}Running with GDB...${NC}"
        echo "Useful commands:"
        echo "  run                  - start the program"
        echo "  bt                   - show backtrace when crashed"
        echo "  frame <n>           - select stack frame"
        echo "  print <variable>    - print variable value"
        echo "  break <function>    - set breakpoint"
        echo ""
        make debug_test
        ;;
    
    "valgrind")
        echo -e "${YELLOW}Running with Valgrind...${NC}"
        make valgrind_test
        ;;
    
    "asan")
        echo -e "${YELLOW}Running with Address Sanitizer...${NC}"
        ./build/bin/test_create_table
        ;;
    
    "strace")
        echo -e "${YELLOW}Running with strace...${NC}"
        strace -f ./build/bin/test_create_table
        ;;
    
    "core")
        echo -e "${YELLOW}Enabling core dumps and running...${NC}"
        ulimit -c unlimited
        ./build/bin/test_create_table
        if [ -f core ]; then
            echo -e "${YELLOW}Core dump found. Analyzing with GDB...${NC}"
            gdb ./build/bin/test_create_table core -ex "bt" -ex "quit"
        fi
        ;;
    
    "rebuild")
        echo -e "${GREEN}Rebuild complete. Run without arguments to execute test.${NC}"
        ;;
    
    *)
        echo -e "${YELLOW}Running test normally...${NC}"
        ./build/bin/test_create_table
        ;;
esac

echo ""
echo "Debug options:"
echo "  ./debug.sh          - run normally"
echo "  ./debug.sh gdb      - run with GDB"
echo "  ./debug.sh valgrind - run with Valgrind"
echo "  ./debug.sh asan     - run with Address Sanitizer"
echo "  ./debug.sh strace   - run with strace"
echo "  ./debug.sh core     - enable core dumps"
echo "  ./debug.sh rebuild  - force rebuild with debug flags"
