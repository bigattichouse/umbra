# Umbra Next Steps Development Plan

## Overview

This document outlines the next phases of development for the Umbra compiled database system. With the core infrastructure complete (schema definition, data page generation, and dynamic loading), we can now focus on implementing query functionality and expanding the system capabilities.

## Phase 1: SELECT Query Implementation (Highest Priority)

### Overview
Implement SELECT query support to demonstrate Umbra's key innovation - compiling queries into native code for optimal performance.

### Components

#### 1.1 SQL Parser for SELECT
- **File**: `src/parser/select_parser.h/c`
- **Dependencies**: `ast.h`, `lexer.h`
- **Tasks**:
  - Parse SELECT statements with columns, FROM, WHERE clauses
  - Support basic operators (=, !=, <, >, <=, >=)
  - Handle AND/OR logic in WHERE clauses
  - Generate AST representation of queries

#### 1.2 Kernel Generator
- **Files**: 
  - `src/kernel/kernel_generator.h/c` - Main kernel generation
  - `src/kernel/filter_generator.h/c` - WHERE clause to C code
  - `src/kernel/projection_generator.h/c` - Column selection
- **Dependencies**: `ast.h`, `schema_parser.h`
- **Tasks**:
  - Convert AST WHERE clauses into C filter functions
  - Generate projection functions for selected columns
  - Create optimized data access patterns
  - Generate compilable C code for queries

#### 1.3 Query Executor
- **Files**:
  - `src/query/query_executor.h/c` - Main execution engine
  - `src/query/select_executor.h/c` - SELECT-specific execution
  - `src/kernel/kernel_compiler.h/c` - Compile generated kernels
  - `src/kernel/kernel_loader.h/c` - Load compiled kernels
- **Dependencies**: `kernel_generator.h`, `page_manager.h`
- **Tasks**:
  - Compile generated kernel code
  - Load compiled shared objects
  - Execute queries across data pages
  - Return result sets

### Example Implementation Flow
```
SELECT name, email FROM Customers WHERE age > 30 AND active = true
    ↓
Parser → AST → Kernel Generator → C Code → Compiler → .so file → Executor → Results
```

### Implementation Steps: 4 major steps

## Phase 2: CLI Framework

### Overview
Create a command-line interface for interacting with Umbra.

### Components

#### 2.1 Interactive Mode (REPL)
- **Files**:
  - `src/cli/cli_main.c` - Main entry point
  - `src/cli/interactive_mode.h/c` - REPL implementation
  - `src/cli/command_history.h/c` - Command history
- **Tasks**:
  - Accept SQL commands interactively
  - Parse and execute commands
  - Display results in formatted tables
  - Support command history and editing

#### 2.2 Result Formatting
- **File**: `src/cli/result_formatter.h/c`
- **Tasks**:
  - Format query results as ASCII tables
  - Support CSV and JSON output formats
  - Handle large result sets with pagination

#### 2.3 Special Commands
- **File**: `src/cli/cli_commands.h/c`
- **Tasks**:
  - Implement `.tables` to list tables
  - Implement `.schema` to show table structure
  - Implement `.exit` and `.help` commands

### Implementation Steps: 2 major steps

## Phase 3: Data Modification Operations

### Overview
Implement INSERT, UPDATE, and DELETE operations.

### Components

#### 3.1 INSERT Implementation
- **Files**:
  - `src/parser/insert_parser.h/c` - Parse INSERT statements
  - `src/query/insert_executor.h/c` - Execute INSERT operations
- **Tasks**:
  - Parse INSERT statements
  - Validate data against schema
  - Add records to appropriate pages
  - Handle page splits when full

#### 3.2 UPDATE Implementation
- **Files**:
  - `src/parser/update_parser.h/c` - Parse UPDATE statements
  - `src/query/update_executor.h/c` - Execute UPDATE operations
- **Tasks**:
  - Parse UPDATE statements with WHERE clauses
  - Generate update kernels
  - Modify records in-place
  - Recompile affected pages

#### 3.3 DELETE Implementation
- **Files**:
  - `src/parser/delete_parser.h/c` - Parse DELETE statements
  - `src/query/delete_executor.h/c` - Execute DELETE operations
- **Tasks**:
  - Parse DELETE statements with WHERE clauses
  - Mark records as deleted (soft delete)
  - Implement page compaction
  - Recompile affected pages

### Implementation Steps: 3 major steps

## Phase 4: Transaction Support

### Overview
Add transaction capabilities with git integration.

### Components

#### 4.1 Transaction Manager
- **Files**:
  - `src/transaction/transaction_manager.h/c` - Core transaction logic
  - `src/transaction/change_tracker.h/c` - Track modifications
- **Tasks**:
  - Implement BEGIN/COMMIT/ROLLBACK
  - Track changes during transactions
  - Maintain transaction isolation

#### 4.2 Git Integration
- **Files**:
  - `src/git/git_manager.h/c` - Git operations
  - `src/git/commit_integration.h/c` - Link transactions to git
- **Tasks**:
  - Map database commits to git commits
  - Store data pages in git repository
  - Enable rollback via git history
  - Support distributed replication

### Implementation Steps: 3 major steps

## Phase 5: Indexing System

### Overview
Implement indexing for improved query performance.

### Components

#### 5.1 Index Management
- **Files**:
  - `src/index/index_manager.h/c` - Index lifecycle management
  - `src/index/index_generator.h/c` - Generate index structures
- **Tasks**:
  - CREATE INDEX support
  - Maintain indexes during data modifications
  - Choose optimal indexes for queries

#### 5.2 Index Types
- **Files**:
  - `src/index/btree_index.h/c` - B-tree implementation
  - `src/index/hash_index.h/c` - Hash index implementation
- **Tasks**:
  - Implement B-tree for range queries
  - Implement hash indexes for equality
  - Compile indexes as shared objects

### Implementation Steps: 4 major steps

## Immediate Next Steps

1. **Start with Phase 1** - Implement SELECT query support
2. **Create test suite** - Build comprehensive tests for each component
3. **Document APIs** - Create documentation as features are built
4. **Performance benchmarks** - Measure compilation vs. execution time

## Success Metrics

- Query compilation time < 100ms for simple queries
- Query execution 10x faster than interpreted approach
- Support for standard SQL subset
- All tests passing with memory leak detection
- Documentation coverage > 80%

## Development Principles

1. **Maintain Compilation Model** - Everything is compiled C code
2. **Git-First Design** - Use git for storage and distribution
3. **Modular Architecture** - Keep files under 250 lines
4. **Test-Driven Development** - Write tests before implementation
5. **Performance Focus** - Measure and optimize critical paths

## Implementation Summary

- Phase 1: 4 major steps (SELECT queries)
- Phase 2: 2 major steps (CLI)
- Phase 3: 3 major steps (INSERT/UPDATE/DELETE)
- Phase 4: 3 major steps (Transactions)
- Phase 5: 4 major steps (Indexing)

**Total**: 16 major implementation steps

## Risks and Mitigations

| Risk | Impact | Mitigation |
|------|--------|------------|
| Compilation overhead | High | Cache compiled queries, optimize generator |
| Query complexity | Medium | Start with simple queries, iterate |
| Memory management | High | Use address sanitizer, careful testing |
| Git performance | Medium | Batch commits, optimize storage format |

## Conclusion

The next phase focuses on SELECT query implementation to demonstrate Umbra's core value proposition. This foundation will enable rapid development of other database features while maintaining the unique compilation-based architecture.
