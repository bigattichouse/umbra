/**
 * @file interactive_mode.h
 * @brief Interface for interactive mode
 */

#ifndef UMBRA_INTERACTIVE_MODE_H
#define UMBRA_INTERACTIVE_MODE_H

/**
 * @brief Run the interactive REPL
 * @param database_path Path to database directory
 * @return Exit code
 */
int run_interactive_mode(const char* database_path);

/**
 * @brief Process a single command in interactive mode
 * @param command Command to process
 * @param database_path Path to database directory
 * @return 0 on success, -1 on error, 1 to exit
 */
int process_interactive_command(const char* command, const char* database_path);

#endif /* UMBRA_INTERACTIVE_MODE_H */
