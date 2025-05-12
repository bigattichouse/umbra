/**
 * @file umbra_shell.c
 * @brief Interactive shell for Umbra database
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "src/cli/interactive_mode.h"
#include "src/cli/command_history.h"

int main(int argc, char *argv[]) {
    const char* database_path = "umbra_db";
    
    // Check if database path was provided as argument
    if (argc > 1) {
        database_path = argv[1];
    }
    
    printf("Umbra Database Shell\n");
    printf("Using database: %s\n", database_path);
    
    // Initialize command history
    init_command_history();
    
    // Run interactive shell
    return run_interactive_mode(database_path);
}
