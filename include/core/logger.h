#ifndef CORE_LOGGER_H
#define CORE_LOGGER_H

#include <stdbool.h>

typedef enum {
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR
} LogLevel;

#define MAX_LOG_LINES 100
#define MAX_LOG_LENGTH 256

typedef struct {
    char lines[MAX_LOG_LINES][MAX_LOG_LENGTH];
    LogLevel levels[MAX_LOG_LINES];
    int head;
    int count;
} TuiLogBuffer;

/**
 * @brief Initialize the logging system. Opens the log file and resets the ring buffer.
 * @param filepath Path to the log file. If NULL, logging is memory-only.
 */
void tui_logger_init(const char* filepath);

/**
 * @brief Shut down the logging system. Closes the log file.
 */
void tui_logger_destroy(void);

/**
 * @brief Log a message at the given level.
 * @param level Log severity level.
 * @param format printf-style format string.
 */
void tui_log(LogLevel level, const char* format, ...);

/**
 * @brief Get the internal ring buffer for reading recent log entries.
 * @return Pointer to the ring buffer. Never NULL.
 */
TuiLogBuffer* tui_logger_get_buffer(void);

/**
 * @brief Set the minimum log level. Messages below this level are filtered.
 * @param level Minimum log level (LOG_DEBUG logs everything).
 */
void tui_logger_set_level(LogLevel level);

/**
 * @brief Get the current minimum log level.
 * @return Current log level.
 */
LogLevel tui_logger_get_level(void);

#endif // CORE_LOGGER_H
