/**
 * @file command_history.h
 * @brief Interface for command history
 */

#ifndef UMBRA_COMMAND_HISTORY_H
#define UMBRA_COMMAND_HISTORY_H

/**
 * @brief Initialize command history
 * @return 0 on success, -1 on error
 */
int init_command_history(void);

/**
 * @brief Save command history to file
 * @return 0 on success, -1 on error
 */
int save_command_history(void);

/**
 * @brief Get history file path
 * @param buffer Buffer to store path
 * @param size Size of buffer
 * @return 0 on success, -1 on error
 */
int get_history_file_path(char* buffer, size_t size);

#endif /* UMBRA_COMMAND_HISTORY_H */
