# Umbra Database Makefile

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g3 -O0 -I. -DDEBUG -fsanitize=address -fno-omit-frame-pointer
LDFLAGS = -ldl -lreadline -luuid -fsanitize=address
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
              $(SRC_DIR)/schema/directory_manager.c \
              $(SRC_DIR)/schema/metadata.c \
              $(SRC_DIR)/schema/permission.c

PAGES_SRCS = $(SRC_DIR)/pages/page_generator.c \
             $(SRC_DIR)/pages/accessor_generator.c \
             $(SRC_DIR)/pages/compile_scripts.c \
             $(SRC_DIR)/pages/page_splitter.c

LOADER_SRCS = $(SRC_DIR)/loader/so_loader.c \
              $(SRC_DIR)/loader/page_manager.c \
              $(SRC_DIR)/loader/record_access.c \
              $(SRC_DIR)/loader/error_handler.c

PARSER_SRCS = $(SRC_DIR)/parser/lexer.c \
              $(SRC_DIR)/parser/select_parser.c \
              $(SRC_DIR)/parser/insert_parser.c \
              $(SRC_DIR)/parser/update_parser.c \
              $(SRC_DIR)/parser/delete_parser.c \
              $(SRC_DIR)/parser/parser_common.c \
              $(SRC_DIR)/parser/ast.c

KERNEL_SRCS = $(SRC_DIR)/kernel/kernel_generator.c \
              $(SRC_DIR)/kernel/filter_generator.c \
              $(SRC_DIR)/kernel/projection_generator.c \
              $(SRC_DIR)/kernel/kernel_compiler.c \
              $(SRC_DIR)/kernel/kernel_loader.c

QUERY_SRCS = $(SRC_DIR)/query/query_executor.c \
             $(SRC_DIR)/query/select_executor.c \
             $(SRC_DIR)/query/insert_executor.c \
             $(SRC_DIR)/query/update_executor.c \
             $(SRC_DIR)/query/delete_executor.c \
             $(SRC_DIR)/query/create_table_executor.c \
             $(SRC_DIR)/query/table_scan.c

CLI_SRCS = $(SRC_DIR)/cli/cli_main.c \
           $(SRC_DIR)/cli/interactive_mode.c \
           $(SRC_DIR)/cli/command_mode.c \
           $(SRC_DIR)/cli/result_formatter.c \
           $(SRC_DIR)/cli/cli_commands.c \
           $(SRC_DIR)/cli/command_history.c
           
UTIL_SRCS = $(SRC_DIR)/util/uuid_utils.c

# New index module
INDEX_SRCS = $(SRC_DIR)/index/index_manager.c \
             $(SRC_DIR)/index/index_generator.c \
             $(SRC_DIR)/index/btree_index.c \
             $(SRC_DIR)/index/hash_index.c \
             $(SRC_DIR)/index/index_compiler.c

# All source files
ALL_SRCS = $(SCHEMA_SRCS) $(PAGES_SRCS) $(LOADER_SRCS) $(PARSER_SRCS) $(KERNEL_SRCS) \
           $(QUERY_SRCS) $(CLI_SRCS) $(UTIL_SRCS) $(INDEX_SRCS)

# Object files
SCHEMA_OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(SCHEMA_SRCS))
PAGES_OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(PAGES_SRCS))
LOADER_OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(LOADER_SRCS))
PARSER_OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(PARSER_SRCS))
KERNEL_OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(KERNEL_SRCS))
QUERY_OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(QUERY_SRCS))
CLI_OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(CLI_SRCS))
UTIL_OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(UTIL_SRCS))
INDEX_OBJS = $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(INDEX_SRCS))

ALL_OBJS = $(SCHEMA_OBJS) $(PAGES_OBJS) $(LOADER_OBJS) $(PARSER_OBJS) $(KERNEL_OBJS) \
           $(QUERY_OBJS) $(CLI_OBJS) $(UTIL_OBJS) $(INDEX_OBJS)

# Static library
UMBRA_LIB = $(LIB_DIR)/libumbra.a

# Test files and binaries
TEST_SRCS = $(wildcard $(TEST_DIR)/*.c)
TEST_BINS = $(patsubst $(TEST_DIR)/%.c,$(BIN_DIR)/%,$(TEST_SRCS))

# Main executable
UMBRA_CLI = $(BIN_DIR)/umbra

# Default target
all: $(UMBRA_LIB) $(UMBRA_CLI) tests

# Create necessary directories
$(BUILD_DIR) $(OBJ_DIR) $(LIB_DIR) $(BIN_DIR) $(OBJ_DIR)/schema $(OBJ_DIR)/pages \
$(OBJ_DIR)/loader $(OBJ_DIR)/parser $(OBJ_DIR)/kernel $(OBJ_DIR)/query $(OBJ_DIR)/cli \
$(OBJ_DIR)/util $(OBJ_DIR)/index:
	@mkdir -p $@

# Build object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR) $(OBJ_DIR)/schema $(OBJ_DIR)/pages $(OBJ_DIR)/loader \
$(OBJ_DIR)/parser $(OBJ_DIR)/kernel $(OBJ_DIR)/query $(OBJ_DIR)/cli $(OBJ_DIR)/util $(OBJ_DIR)/index
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

# Build main CLI executable - special handling to avoid conflict with test pattern
$(UMBRA_CLI): $(SRC_DIR)/cli/cli_main.c $(UMBRA_LIB) | $(BIN_DIR)
	@echo "CC $@"
	@$(CC) $(CFLAGS) $(SRC_DIR)/cli/cli_main.c -o $@ -L$(LIB_DIR) -lumbra $(LDFLAGS)

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

# Index specific tests
test_indices: $(BIN_DIR)/test_indices
	@echo "Running index tests..."
	@$< || exit 1

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
	@rm -rf test_db  # Clean test database directories

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
	@echo "  Parser: $(PARSER_SRCS)"
	@echo "  Kernel: $(KERNEL_SRCS)"
	@echo "  Query: $(QUERY_SRCS)"
	@echo "  CLI: $(CLI_SRCS)"
	@echo "  Index: $(INDEX_SRCS)"
	@echo ""
	@echo "Object files:"
	@echo "  Schema: $(SCHEMA_OBJS)"
	@echo "  Pages: $(PAGES_OBJS)"
	@echo "  Loader: $(LOADER_OBJS)"
	@echo "  Parser: $(PARSER_OBJS)"
	@echo "  Kernel: $(KERNEL_OBJS)"
	@echo "  Query: $(QUERY_OBJS)"
	@echo "  CLI: $(CLI_OBJS)"
	@echo "  Index: $(INDEX_OBJS)"
	@echo ""
	@echo "Test binaries: $(TEST_BINS)"
	@echo "Library: $(UMBRA_LIB)"
	@echo "CLI: $(UMBRA_CLI)"

# Phony targets
.PHONY: all clean tests test install uninstall info test_create_table test_indices debug_test

# Dependencies (automatically generated)
-include $(ALL_OBJS:.o=.d)

# Generate dependency files
$(OBJ_DIR)/%.d: $(SRC_DIR)/%.c | $(OBJ_DIR) $(OBJ_DIR)/schema $(OBJ_DIR)/pages $(OBJ_DIR)/loader \
$(OBJ_DIR)/parser $(OBJ_DIR)/kernel $(OBJ_DIR)/query $(OBJ_DIR)/cli $(OBJ_DIR)/util $(OBJ_DIR)/index
	@$(CC) $(CFLAGS) -MM -MT $(@:.d=.o) $< > $@
