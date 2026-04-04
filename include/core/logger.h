#ifndef CORE_LOGGER_H
#define CORE_LOGGER_H

#include <stdbool.h>

typedef enum {
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_DEBUG
} LogLevel;

#define MAX_LOG_LINES 100
#define MAX_LOG_LENGTH 256

typedef struct {
    char lines[MAX_LOG_LINES][MAX_LOG_LENGTH];
    LogLevel levels[MAX_LOG_LINES];
    int head;
    int count;
} TuiLogBuffer;

void tui_logger_init(const char* filepath);
void tui_logger_destroy(void);
void tui_log(LogLevel level, const char* format, ...);

TuiLogBuffer* tui_logger_get_buffer(void);

#endif // CORE_LOGGER_H
