# Umbra Database Makefile

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g3 -O0 -I. -DDEBUG -fsanitize=address -fno-omit-frame-pointer
LDFLAGS = -ldl -fsanitize=address
AR = ar
ARFLAGS = rcs

# Directories
SRC_DIR = src
TEST_DIR = tests
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
LIB_DIR = $(BUILD_DIR)/lib
BIN_DIR = $(BUILD_DIR)/bin

# Source files organized by module
SCHEMA_SRCS = $(SRC_DIR)/schema/schema_parser.c \
              $(SRC_DIR)/schema/schema_generator.c \
              $(SRC_DIR)/schema/type_system.c \
              $(SRC_DIR)/schema/directory_manager.c

PAGES_SRCS = $(SRC_DIR)/pages/page_generator.c \
             $(SRC_DIR)/pages/accessor_generator.c \
             $(SRC_DIR)/pages/compile_scripts.c \
             $(SRC_DIR)/pages/page_splitter.c

LOADER_SRCS = $(SRC_DIR)/loader/so_loader.c \
              $(SRC_DIR)/loader/page_manager.c \
              $(SRC_DIR)/loader/record_access.c \
              $(SRC_DIR)/loader/error_handler.c

# All source files
ALL_SRCS = $(SCHEMA_SRCS) $(PAGES_SRCS) $(LOADER_SRCS)

# Object files
SCHEMA_OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SCHEMA_SRCS))
PAGES_OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(PAGES_SRCS))
LOADER_OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(LOADER_SRCS))
ALL_OBJS = $(SCHEMA_OBJS) $(PAGES_OBJS) $(LOADER_OBJS)

# Static library
UMBRA_LIB = $(LIB_DIR)/libumbra.a

# Test files and binaries
TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)
TEST_BINS = $(patsubst $(TEST_DIR)/%.c,$(BIN_DIR)/%,$(TEST_SRCS))

# Default target
all: $(UMBRA_LIB) tests

# Create necessary directories
$(BUILD_DIR) $(OBJ_DIR) $(LIB_DIR) $(BIN_DIR) $(OBJ_DIR)/schema $(OBJ_DIR)/pages $(OBJ_DIR)/loader:
	@mkdir -p $@

# Build object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR) $(OBJ_DIR)/schema $(OBJ_DIR)/pages $(OBJ_DIR)/loader
	@echo "CC $<"
	@$(CC) $(CFLAGS) -c $< -o $@

# Build static library
$(UMBRA_LIB): $(ALL_OBJS) | $(LIB_DIR)
	@echo "AR $@"
	@$(AR) $(ARFLAGS) $@ $^

# Build tests
$(BIN_DIR)/%: $(TEST_DIR)/%.c $(UMBRA_LIB) | $(BIN_DIR)
	@echo "CC $@"
	@$(CC) $(CFLAGS) $< -o $@ -L$(LIB_DIR) -lumbra $(LDFLAGS)

# Build all tests
tests: $(TEST_BINS)

# Run all tests
test: tests
	@echo "Running tests..."
	@for test in $(TEST_BINS); do \
		echo "Running $$(basename $$test)..."; \
		$$test || exit 1; \
		echo ""; \
	done
	@echo "All tests passed!"

# Debug test with gdb
debug_test: tests
	@echo "Debugging test_create_table with gdb..."
	@gdb --args $(BIN_DIR)/test_create_table

# Debug test with valgrind
valgrind_test: tests
	@echo "Running test_create_table with valgrind..."
	@valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes $(BIN_DIR)/test_create_table

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
	@rm -rf test_db  # Clean test database directories

# Specific test targets
test_create_table: $(BIN_DIR)/test_create_table
	@echo "Running test_create_table..."
	@$< || exit 1

# Install target (optional)
PREFIX ?= /usr/local
install: $(UMBRA_LIB)
	@echo "Installing Umbra..."
	@install -d $(PREFIX)/lib
	@install -m 644 $(UMBRA_LIB) $(PREFIX)/lib/
	@install -d $(PREFIX)/include/umbra
	@find $(SRC_DIR) -name '*.h' -exec install -D -m 644 {} $(PREFIX)/include/umbra/{} \;

# Uninstall target
uninstall:
	@echo "Uninstalling Umbra..."
	@rm -f $(PREFIX)/lib/libumbra.a
	@rm -rf $(PREFIX)/include/umbra

# Debug info
info:
	@echo "Source files:"
	@echo "  Schema: $(SCHEMA_SRCS)"
	@echo "  Pages: $(PAGES_SRCS)"
	@echo "  Loader: $(LOADER_SRCS)"
	@echo ""
	@echo "Object files:"
	@echo "  Schema: $(SCHEMA_OBJS)"
	@echo "  Pages: $(PAGES_OBJS)"
	@echo "  Loader: $(LOADER_OBJS)"
	@echo ""
	@echo "Test binaries: $(TEST_BINS)"
	@echo "Library: $(UMBRA_LIB)"

# Phony targets
.PHONY: all clean tests test install uninstall info test_create_table debug_test valgrind_test

# Dependencies (automatically generated)
-include $(ALL_OBJS:.o=.d)

# Generate dependency files
$(OBJ_DIR)/%.d: $(SRC_DIR)/%.c | $(OBJ_DIR) $(OBJ_DIR)/schema $(OBJ_DIR)/pages $(OBJ_DIR)/loader
	@$(CC) $(CFLAGS) -MM -MT $(@:.d=.o) $< > $@
