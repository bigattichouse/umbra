/**
 * @file command_mode.h
 * @brief Interface for command mode
 */

#ifndef UMBRA_COMMAND_MODE_H
#define UMBRA_COMMAND_MODE_H

/**
 * @brief Execute command line SQL
 * @param database_path Path to database directory
 * @param command SQL command to execute (optional)
 * @param file SQL file to execute (optional)
 * @param output_format Output format (table, csv, json)
 * @return Exit code
 */
int execute_command_mode(const char* database_path, const char* command, 
                        const char* file, const char* output_format);

#endif /* UMBRA_COMMAND_MODE_H */
