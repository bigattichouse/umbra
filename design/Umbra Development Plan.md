# Umbra Development Plan

This document outlines the development plan for the Umbra compiled database system, breaking down components into files of manageable size (<250 lines where possible).

## Phase 1: Core Infrastructure

### Component: Schema Definition System

| File | Purpose | Dependencies | Est. Lines |
|------|---------|--------------|-----------|
| `src/schema/schema_parser.h` | Parser interface for schema definitions | None | 50 |
| `src/schema/schema_parser.c` | Parser implementation for CREATE TABLE statements | `schema_parser.h` | 200 |
| `src/schema/schema_generator.h` | Interface for C struct generation | None | 40 |
| `src/schema/schema_generator.c` | Generates C struct definitions from parsed schemas | `schema_parser.h`, `schema_generator.h` | 200 |
| `src/schema/type_system.h` | Defines supported data types | None | 100 |
| `src/schema/type_system.c` | Type conversion and validation logic | `type_system.h` | 150 |
| `src/schema/metadata.h` | Metadata structure definitions | None | 50 |
| `src/schema/permission.h` | Permission structure definitions | None | 50 |
| `src/schema/directory_manager.h` | Interface for directory structure management | None | 30 |
| `src/schema/directory_manager.c` | Creates and manages directory structures for tables | `directory_manager.h` | 150 |

### Component: Data Page Generation

| File | Purpose | Dependencies | Est. Lines |
|------|---------|--------------|-----------|
| `src/pages/page_template.h` | Page template definitions | None | 30 |
| `src/pages/page_generator.h` | Interface for page generation | None | 40 |
| `src/pages/page_generator.c` | Generates data files and page headers | `page_template.h`, `page_generator.h` | 200 |
| `src/pages/accessor_generator.h` | Interface for accessor function generation | None | 40 |
| `src/pages/accessor_generator.c` | Generates count() and read() functions | `accessor_generator.h` | 180 |
| `src/pages/compile_scripts.h` | Compilation script interface | None | 30 |
| `src/pages/compile_scripts.c` | Generates compilation scripts for pages | `compile_scripts.h` | 150 |
| `src/pages/page_splitter.h` | Interface for page splitting logic | None | 40 |
| `src/pages/page_splitter.c` | Handles page splitting when full | `page_splitter.h` | 180 |

### Component: Dynamic Loading Framework

| File | Purpose | Dependencies | Est. Lines |
|------|---------|--------------|-----------|
| `src/loader/so_loader.h` | Interface for shared object loading | None | 40 |
| `src/loader/so_loader.c` | Loads shared objects dynamically | `so_loader.h` | 150 |
| `src/loader/page_manager.h` | Interface for page management | None | 50 |
| `src/loader/page_manager.c` | Manages loaded pages and caching | `page_manager.h`, `so_loader.h` | 200 |
| `src/loader/record_access.h` | Interface for record access | None | 40 |
| `src/loader/record_access.c` | Provides access to records from loaded pages | `record_access.h`, `page_manager.h` | 180 |
| `src/loader/error_handler.h` | Error handling for loading operations | None | 40 |
| `src/loader/error_handler.c` | Implements error handling logic | `error_handler.h` | 100 |

## Phase 2: Query System

### Component: CLI Framework

| File | Purpose | Dependencies | Est. Lines |
|------|---------|--------------|-----------|
| `src/cli/cli_main.c` | Main entry point for CLI | Multiple | 200 |
| `src/cli/interactive_mode.h` | Interface for interactive mode | None | 30 |
| `src/cli/interactive_mode.c` | REPL for interactive SQL | `interactive_mode.h` | 200 |
| `src/cli/command_mode.h` | Interface for command mode | None | 30 |
| `src/cli/command_mode.c` | Handles command-line SQL execution | `command_mode.h` | 150 |
| `src/cli/result_formatter.h` | Interface for formatting results | None | 40 |
| `src/cli/result_formatter.c` | Formats query results as tables/CSV/JSON | `result_formatter.h` | 200 |
| `src/cli/cli_commands.h` | Special CLI command definitions | None | 50 |
| `src/cli/cli_commands.c` | Implements special CLI commands | `cli_commands.h` | 180 |
| `src/cli/command_history.h` | Interface for command history | None | 30 |
| `src/cli/command_history.c` | Manages command history for interactive mode | `command_history.h` | 120 |

### Component: SQL Parser

| File | Purpose | Dependencies | Est. Lines |
|------|---------|--------------|-----------|
| `src/parser/lexer.h` | Lexer interface | None | 40 |
| `src/parser/lexer.c` | Tokenizes SQL input | `lexer.h` | 200 |
| `src/parser/parser.h` | Parser interface | None | 50 |
| `src/parser/parser.c` | Parses SQL statements into AST | `parser.h`, `lexer.h` | 250 |
| `src/parser/ast.h` | Abstract Syntax Tree definitions | None | 100 |
| `src/parser/select_parser.h` | Interface for SELECT parsing | None | 30 |
| `src/parser/select_parser.c` | Parses SELECT statements | `select_parser.h`, `ast.h` | 200 |
| `src/parser/insert_parser.h` | Interface for INSERT parsing | None | 30 |
| `src/parser/insert_parser.c` | Parses INSERT statements | `insert_parser.h`, `ast.h` | 150 |
| `src/parser/update_parser.h` | Interface for UPDATE parsing | None | 30 |
| `src/parser/update_parser.c` | Parses UPDATE statements | `update_parser.h`, `ast.h` | 150 |
| `src/parser/delete_parser.h` | Interface for DELETE parsing | None | 30 |
| `src/parser/delete_parser.c` | Parses DELETE statements | `delete_parser.h`, `ast.h` | 120 |
| `src/parser/where_parser.h` | Interface for WHERE clause parsing | None | 40 |
| `src/parser/where_parser.c` | Parses WHERE conditions | `where_parser.h`, `ast.h` | 200 |

### Component: Kernel Generation

| File | Purpose | Dependencies | Est. Lines |
|------|---------|--------------|-----------|
| `src/kernel/kernel_generator.h` | Interface for kernel generation | None | 50 |
| `src/kernel/kernel_generator.c` | Generates kernel code from AST | `kernel_generator.h`, `ast.h` | 200 |
| `src/kernel/filter_generator.h` | Interface for filter generation | None | 40 |
| `src/kernel/filter_generator.c` | Generates filter functions | `filter_generator.h` | 200 |
| `src/kernel/compare_generator.h` | Interface for compare function generation | None | 40 |
| `src/kernel/compare_generator.c` | Generates comparison functions | `compare_generator.h` | 180 |
| `src/kernel/projection_generator.h` | Interface for projection generation | None | 40 |
| `src/kernel/projection_generator.c` | Generates projection functions | `projection_generator.h` | 150 |
| `src/kernel/kernel_compiler.h` | Interface for kernel compilation | None | 40 |
| `src/kernel/kernel_compiler.c` | Compiles generated kernel code | `kernel_compiler.h` | 180 |
| `src/kernel/kernel_loader.h` | Interface for loading compiled kernels | None | 40 |
| `src/kernel/kernel_loader.c` | Loads and executes compiled kernels | `kernel_loader.h` | 150 |

### Component: Query Execution

| File | Purpose | Dependencies | Est. Lines |
|------|---------|--------------|-----------|
| `src/query/query_executor.h` | Interface for query execution | None | 50 |
| `src/query/query_executor.c` | Executes queries using kernels | `query_executor.h` | 200 |
| `src/query/result_set.h` | Result set structure definitions | None | 40 |
| `src/query/result_set.c` | Manages query results | `result_set.h` | 150 |
| `src/query/select_executor.h` | Interface for SELECT execution | None | 40 |
| `src/query/select_executor.c` | Executes SELECT queries | `select_executor.h`, `query_executor.h` | 200 |
| `src/query/insert_executor.h` | Interface for INSERT execution | None | 40 |
| `src/query/insert_executor.c` | Executes INSERT operations | `insert_executor.h`, `query_executor.h` | 180 |
| `src/query/update_executor.h` | Interface for UPDATE execution | None | 40 |
| `src/query/update_executor.c` | Executes UPDATE operations | `update_executor.h`, `query_executor.h` | 180 |
| `src/query/delete_executor.h` | Interface for DELETE execution | None | 40 |
| `src/query/delete_executor.c` | Executes DELETE operations | `delete_executor.h`, `query_executor.h` | 150 |

## Phase 3: Transaction System

### Component: Transaction Management

| File | Purpose | Dependencies | Est. Lines |
|------|---------|--------------|-----------|
| `src/transaction/transaction_manager.h` | Interface for transaction management | None | 50 |
| `src/transaction/transaction_manager.c` | Manages transactions | `transaction_manager.h` | 200 |
| `src/transaction/change_tracker.h` | Interface for change tracking | None | 40 |
| `src/transaction/change_tracker.c` | Tracks changes during transactions | `change_tracker.h` | 180 |
| `src/transaction/commit_handler.h` | Interface for commit operations | None | 40 |
| `src/transaction/commit_handler.c` | Handles COMMIT operations | `commit_handler.h`, `transaction_manager.h` | 200 |
| `src/transaction/rollback_handler.h` | Interface for rollback operations | None | 40 |
| `src/transaction/rollback_handler.c` | Handles ROLLBACK operations | `rollback_handler.h`, `transaction_manager.h` | 180 |
| `src/transaction/isolation.h` | Isolation level definitions | None | 30 |
| `src/transaction/isolation.c` | Implements isolation behaviors | `isolation.h` | 150 |

### Component: Git Integration

| File | Purpose | Dependencies | Est. Lines |
|------|---------|--------------|-----------|
| `src/git/git_manager.h` | Interface for git operations | None | 40 |
| `src/git/git_manager.c` | Manages git repository | `git_manager.h` | 180 |
| `src/git/commit_integration.h` | Interface for integrating with commit system | None | 40 |
| `src/git/commit_integration.c` | Links transaction commits to git commits | `commit_integration.h`, `git_manager.h` | 150 |
| `src/git/diff_analyzer.h` | Interface for analyzing changes | None | 40 |
| `src/git/diff_analyzer.c` | Analyzes git diffs to optimize recompilation | `diff_analyzer.h` | 180 |
| `src/git/replication.h` | Interface for git-based replication | None | 40 |
| `src/git/replication.c` | Manages push/pull operations | `replication.h`, `git_manager.h` | 200 |

## Phase 4: Optimization & Performance

### Component: Indexing System

| File | Purpose | Dependencies | Est. Lines |
|------|---------|--------------|-----------|
| `src/index/index_manager.h` | Interface for index management | None | 50 |
| `src/index/index_manager.c` | Manages database indexes | `index_manager.h` | 200 |
| `src/index/index_generator.h` | Interface for index generation | None | 40 |
| `src/index/index_generator.c` | Generates index structures | `index_generator.h` | 200 |
| `src/index/btree_index.h` | B-tree index implementation | None | 50 |
| `src/index/btree_index.c` | Implements B-tree for indexing | `btree_index.h` | 220 |
| `src/index/hash_index.h` | Hash index implementation | None | 40 |
| `src/index/hash_index.c` | Implements hash tables for indexing | `hash_index.h` | 180 |
| `src/index/index_compiler.h` | Interface for index compilation | None | 40 |
| `src/index/index_compiler.c` | Compiles index structures | `index_compiler.h` | 180 |

### Component: Query Optimization

| File | Purpose | Dependencies | Est. Lines |
|------|---------|--------------|-----------|
| `src/optimizer/query_optimizer.h` | Interface for query optimization | None | 50 |
| `src/optimizer/query_optimizer.c` | Optimizes queries before execution | `query_optimizer.h` | 200 |
| `src/optimizer/index_selector.h` | Interface for index selection | None | 40 |
| `src/optimizer/index_selector.c` | Selects appropriate indexes for queries | `index_selector.h` | 180 |
| `src/optimizer/kernel_optimizer.h` | Interface for kernel optimization | None | 40 |
| `src/optimizer/kernel_optimizer.c` | Optimizes generated kernel code | `kernel_optimizer.h` | 200 |
| `src/optimizer/parallel_executor.h` | Interface for parallel execution | None | 50 |
| `src/optimizer/parallel_executor.c` | Executes queries in parallel | `parallel_executor.h` | 200 |

## Phase 5: Distribution & Multi-Node

### Component: Distribution Configuration

| File | Purpose | Dependencies | Est. Lines |
|------|---------|--------------|-----------|
| `src/dist/distribution_config.h` | Interface for distribution configuration | None | 40 |
| `src/dist/distribution_config.c` | Manages distribution settings | `distribution_config.h` | 180 |
| `src/dist/node_manager.h` | Interface for node management | None | 50 |
| `src/dist/node_manager.c` | Manages multiple database nodes | `node_manager.h` | 200 |
| `src/dist/shard_manager.h` | Interface for data sharding | None | 50 |
| `src/dist/shard_manager.c` | Manages data sharding across nodes | `shard_manager.h` | 220 |
| `src/dist/conflict_detector.h` | Interface for conflict detection | None | 40 |
| `src/dist/conflict_detector.c` | Detects conflicts in distributed operations | `conflict_detector.h` | 180 |
| `src/dist/conflict_resolver.h` | Interface for conflict resolution | None | 40 |
| `src/dist/conflict_resolver.c` | Resolves detected conflicts | `conflict_resolver.h` | 180 |

### Component: Distributed Queries

| File | Purpose | Dependencies | Est. Lines |
|------|---------|--------------|-----------|
| `src/dist_query/dist_query_planner.h` | Interface for distributed query planning | None | 50 |
| `src/dist_query/dist_query_planner.c` | Plans queries across nodes | `dist_query_planner.h` | 200 |
| `src/dist_query/query_router.h` | Interface for query routing | None | 40 |
| `src/dist_query/query_router.c` | Routes queries to appropriate nodes | `query_router.h` | 180 |
| `src/dist_query/result_aggregator.h` | Interface for result aggregation | None | 40 |
| `src/dist_query/result_aggregator.c` | Aggregates results from multiple nodes | `result_aggregator.h` | 200 |
| `src/dist_query/network_optimizer.h` | Interface for network optimization | None | 40 |
| `src/dist_query/network_optimizer.c` | Optimizes network communication | `network_optimizer.h` | 180 |

## Integration Testing

| File | Purpose | Dependencies | Est. Lines |
|------|---------|--------------|-----------|
| `tests/integration/schema_test.c` | Tests schema creation | Multiple | 150 |
| `tests/integration/query_test.c` | Tests query functionality | Multiple | 200 |
| `tests/integration/transaction_test.c` | Tests transaction operations | Multiple | 180 |
| `tests/integration/distribution_test.c` | Tests distributed operations | Multiple | 200 |
| `tests/integration/performance_test.c` | Tests system performance | Multiple | 180 |

## Build System

| File | Purpose | Dependencies | Est. Lines |
|------|---------|--------------|-----------|
| `Makefile` | Main build file | None | 200 |
| `cmake/FindDependencies.cmake` | Finds required dependencies | None | 100 |
| `cmake/CompilerSettings.cmake` | Sets compiler flags and options | None | 80 |
| `scripts/build.sh` | Build script | None | 100 |
| `scripts/test.sh` | Test script | None | 100 |
| `scripts/install.sh` | Installation script | None | 120 |

## Configuration and Documentation

| File | Purpose | Dependencies | Est. Lines |
|------|---------|--------------|-----------|
| `config/umbra.conf` | Configuration file template | None | 100 |
| `docs/user_manual.md` | User manual | None | 200 |
| `docs/developer_guide.md` | Developer guide | None | 200 |
| `docs/sql_reference.md` | SQL reference | None | 200 |
| `examples/basic_usage.sql` | Basic usage examples | None | 100 |
| `examples/advanced_queries.sql` | Advanced query examples | None | 150 |

## Implementation Timeline

### Phase 1: Core Infrastructure (3 Months)
- Month 1: Schema Definition System
- Month 2: Data Page Generation
- Month 3: Dynamic Loading Framework

### Phase 2: Query System (3 Months)
- Month 1: CLI Framework & SQL Parser
- Month 2: Kernel Generation
- Month 3: Query Execution

### Phase 3: Transaction System (2 Months)
- Month 1: Transaction Management
- Month 2: Git Integration

### Phase 4: Optimization & Performance (2 Months)
- Month 1: Indexing System
- Month 2: Query Optimization

### Phase 5: Distribution & Multi-Node (2 Months)
- Month 1: Distribution Configuration
- Month 2: Distributed Queries

### Final Phase: Integration, Testing, Documentation (1 Month)
- Week 1-2: Integration Testing
- Week 3: Performance Tuning
- Week 4: Documentation Finalization

