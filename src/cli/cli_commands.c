/**
 * @file cli_commands.c
 * @brief Implements special CLI commands
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>  // For NAME_MAX
#include "cli_commands.h"

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

/**
 * @brief Show help for CLI commands
 */
/**
 * @brief Show help for CLI commands
 */
void show_help(void) {
    printf("CLI Commands:\n");
    printf("  .help           Show this help message\n");
    printf("  .exit           Exit the CLI\n");
    printf("  .quit           Exit the CLI\n");
    printf("  EXIT            Exit the CLI (SQL command)\n");  // Add this line
    printf("  .tables         List all tables\n");
    printf("  .schema <table> Show schema for a table\n");
    printf("  .format <type>  Set output format (table, csv, json)\n");
    printf("\n");
    printf("SQL Commands:\n");
    printf("  SELECT ...      Query data from tables\n");
    printf("  CREATE TABLE    Create a new table\n");
    printf("  INSERT INTO     Insert data into a table\n");
    printf("  UPDATE          Update data in a table\n");
    printf("  DELETE FROM     Delete data from a table\n");
    printf("\n");
}

/**
 * @brief List all tables in database
 */
void list_tables(const char* database_path) {
    char tables_dir[2048];
    snprintf(tables_dir, sizeof(tables_dir), "%s/tables", database_path);
    
    DIR* dir = opendir(tables_dir);
    if (!dir) {
        printf("No tables found\n");
        return;
    }
    
    printf("Tables:\n");
    
    struct dirent* entry;
    int count = 0;
    
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Check if it's a directory
        // Allocate enough space for the worst case: tables_dir + "/" + NAME_MAX + null terminator
        char path[2048 + 1 + NAME_MAX + 1];
        snprintf(path, sizeof(path), "%s/%s", tables_dir, entry->d_name);
        
        struct stat st;
        if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
            printf("  %s\n", entry->d_name);
            count++;
        }
    }
    
    closedir(dir);
    
    if (count == 0) {
        printf("  (none)\n");
    }
}

/**
 * @brief Show schema for a table
 */
void show_table_schema(const char* table_name, const char* database_path) {
    // Suppress unused parameter warning
    (void)database_path;
    
    if (!table_name || strlen(table_name) == 0) {
        printf("Usage: .schema <table_name>\n");
        return;
    }
    
    // TODO: Actually load schema from metadata files
    // For now, show a placeholder
    
    printf("Schema for table '%s':\n", table_name);
    printf("  (Schema loading not implemented yet)\n");
}

/**
 * @brief Execute a CLI command
 */
int execute_cli_command(const char* command, const char* database_path) {
    if (!command || command[0] != '.') {
        return -1;
    }
    
    // Parse command and arguments
    char cmd_copy[256];
    strncpy(cmd_copy, command, sizeof(cmd_copy) - 1);
    cmd_copy[sizeof(cmd_copy) - 1] = '\0';
    
    char* cmd = strtok(cmd_copy, " \t");
    char* arg = strtok(NULL, " \t");
    
    if (strcmp(cmd, ".help") == 0) {
        show_help();
        return 0;
    } else if (strcmp(cmd, ".exit") == 0 || strcmp(cmd, ".quit") == 0) {
        return 1; // Signal to exit
    } else if (strcmp(cmd, ".tables") == 0) {
        list_tables(database_path);
        return 0;
    } else if (strcmp(cmd, ".schema") == 0) {
        show_table_schema(arg, database_path);
        return 0;
    } else {
        printf("Unknown command: %s\n", cmd);
        printf("Type .help for help\n");
        return -1;
    }
}
