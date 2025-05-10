/**
 * @file interactive_mode.c
 * @brief REPL for interactive SQL
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "interactive_mode.h"
#include "command_history.h"
#include "cli_commands.h"
#include "result_formatter.h"
#include "../query/query_executor.h"

/**
 * @brief Signal handling for Ctrl+C
 */
static sigjmp_buf ctrl_c_jmp_buf;

static void handle_ctrl_c(int sig) {
    (void)sig;
    printf("\n");
    siglongjmp(ctrl_c_jmp_buf, 1);
}

/**
 * @brief Print welcome message
 */
static void print_welcome(void) {
    printf("Umbra Database CLI v0.1.0\n");
    printf("Type '.help' for help, '.exit' to quit\n");
    printf("\n");
}

/**
 * @brief Check if command is a special CLI command
 */
static bool is_cli_command(const char* command) {
    return command && command[0] == '.';
}

/**
 * @brief Process a SQL query
 */
static int process_sql_query(const char* query, const char* database_path, OutputFormat format) {
    QueryResult* result = execute_query(query, database_path);
    
    if (!result) {
        printf("Error: Query execution failed\n");
        return -1;
    }
    
    if (!result->success) {
        printf("Error: %s\n", result->error_message);
        free_query_result(result);
        return -1;
    }
    
    // Format and display results
    format_query_result(result, format);
    
    free_query_result(result);
    return 0;
}

/**
 * @brief Process a single command in interactive mode
 */
int process_interactive_command(const char* command, const char* database_path) {
    if (!command || strlen(command) == 0) {
        return 0;
    }
    
    // Check for CLI commands
    if (is_cli_command(command)) {
        return execute_cli_command(command, database_path);
    }
    
    // Process as SQL query
    return process_sql_query(command, database_path, FORMAT_TABLE);
}

/**
 * @brief Build multi-line SQL statement
 */
static char* build_multiline_statement(void) {
    char* statement = NULL;
    size_t statement_size = 0;
    int line_count = 0;
    
    while (1) {
        char* prompt = line_count == 0 ? "umbra> " : "   ...> ";
        char* line = readline(prompt);
        
        if (!line) {
            // EOF or error
            free(statement);
            return NULL;
        }
        
        // Check for empty line
        if (strlen(line) == 0) {
            free(line);
            continue;
        }
        
        // Add to history
        add_history(line);
        
        // Append line to statement
        size_t line_len = strlen(line);
        statement = realloc(statement, statement_size + line_len + 2);
        
        if (statement_size == 0) {
            statement[0] = '\0';
        }
        
        strcat(statement, line);
        strcat(statement, " ");
        statement_size += line_len + 1;
        
        // Check if statement is complete (ends with semicolon)
        char* trimmed = line;
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
        
        size_t trimmed_len = strlen(trimmed);
        while (trimmed_len > 0 && (trimmed[trimmed_len-1] == ' ' || trimmed[trimmed_len-1] == '\t')) {
            trimmed_len--;
        }
        
        if (trimmed_len > 0 && trimmed[trimmed_len-1] == ';') {
            free(line);
            break;
        }
        
        free(line);
        line_count++;
    }
    
    return statement;
}

/**
 * @brief Run the interactive REPL
 */
int run_interactive_mode(const char* database_path) {
    print_welcome();
    
    // Set up signal handler for Ctrl+C
    signal(SIGINT, handle_ctrl_c);
    
    // Initialize command history
    init_command_history();
    
    // Main REPL loop
    while (1) {
        // Set jump point for Ctrl+C
        if (sigsetjmp(ctrl_c_jmp_buf, 1) != 0) {
            // Ctrl+C was pressed, continue loop
            continue;
        }
        
        // Read command (possibly multi-line)
        char* command = build_multiline_statement();
        
        if (!command) {
            // EOF reached
            printf("\n");
            break;
        }
        
        // Process command
        int result = process_interactive_command(command, database_path);
        
        free(command);
        
        // Check if we should exit
        if (result > 0) {
            break;
        }
    }
    
    // Save command history
    save_command_history();
    
    return 0;
}
