/**
 * @file cli_main.c
 * @brief Main entry point for CLI
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "interactive_mode.h"
#include "command_mode.h"

/**
 * @brief Print usage information
 */
static void print_usage(const char* program_name) {
    printf("Usage: %s [options] [database_path]\n", program_name);
    printf("\nOptions:\n");
    printf("  -c, --command <sql>    Execute SQL command and exit\n");
    printf("  -f, --file <file>      Execute SQL from file and exit\n");
    printf("  -o, --output <format>  Output format (table, csv, json)\n");
    printf("  -h, --help             Show this help message\n");
    printf("  -v, --version          Show version information\n");
    printf("\nIf no command or file is specified, start interactive mode.\n");
}

/**
 * @brief Print version information
 */
static void print_version(void) {
    printf("Umbra Database CLI v0.1.0\n");
    printf("A compiled database system\n");
}

/**
 * @brief Main entry point
 */
int main(int argc, char* argv[]) {
    const char* database_path = NULL;
    const char* command = NULL;
    const char* file = NULL;
    const char* output_format = "table";
    
    // Parse command line options
    static struct option long_options[] = {
        {"command", required_argument, 0, 'c'},
        {"file", required_argument, 0, 'f'},
        {"output", required_argument, 0, 'o'},
        {"help", no_argument, 0, 'h'},
        {"version", no_argument, 0, 'v'},
        {0, 0, 0, 0}
    };
    
    int option_index = 0;
    int c;
    
    while ((c = getopt_long(argc, argv, "c:f:o:hv", long_options, &option_index)) != -1) {
        switch (c) {
            case 'c':
                command = optarg;
                break;
            case 'f':
                file = optarg;
                break;
            case 'o':
                output_format = optarg;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            case 'v':
                print_version();
                return 0;
            case '?':
                return 1;
            default:
                break;
        }
    }
    
    // Get database path from remaining arguments
    if (optind < argc) {
        database_path = argv[optind];
    } else {
        database_path = "umbra_db"; // Default database directory
    }
    
    // Initialize database directory if needed
    if (!database_path || strlen(database_path) == 0) {
        fprintf(stderr, "Error: Database path not specified\n");
        return 1;
    }
    
    // Execute command or file if specified
    if (command || file) {
        return execute_command_mode(database_path, command, file, output_format);
    } else {
        // Start interactive mode
        return run_interactive_mode(database_path);
    }
}
