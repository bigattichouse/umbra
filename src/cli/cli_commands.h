/**
 * @file cli_commands.h
 * @brief Special CLI command definitions
 */

#ifndef UMBRA_CLI_COMMANDS_H
#define UMBRA_CLI_COMMANDS_H

/**
 * @brief Execute a CLI command
 * @param command Command string (starting with '.')
 * @param database_path Path to database directory
 * @return 0 on success, -1 on error, 1 to exit
 */
int execute_cli_command(const char* command, const char* database_path);

/**
 * @brief Show help for CLI commands
 */
void show_help(void);

/**
 * @brief List all tables in database
 * @param database_path Path to database directory
 */
void list_tables(const char* database_path);

/**
 * @brief Show schema for a table
 * @param table_name Table name
 * @param database_path Path to database directory
 */
void show_table_schema(const char* table_name, const char* database_path);

#endif /* UMBRA_CLI_COMMANDS_H */
