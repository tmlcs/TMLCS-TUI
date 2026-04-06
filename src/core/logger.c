#include "core/logger.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

static FILE* s_log_file = NULL;
static TuiLogBuffer s_log_buffer;
static pthread_mutex_t s_log_mutex = PTHREAD_MUTEX_INITIALIZER;
static LogLevel s_log_level = LOG_DEBUG; /* Default: log everything */

void tui_logger_init(const char* filepath) {
    pthread_mutex_lock(&s_log_mutex);

    /* Close previous file if re-initialized without destroy (defensive) */
    if (s_log_file) {
        fclose(s_log_file);
        s_log_file = NULL;
    }

    if (filepath) {
        s_log_file = fopen(filepath, "a");
    }

    s_log_buffer.head = 0;
    s_log_buffer.count = 0;
    memset(s_log_buffer.lines, 0, sizeof(s_log_buffer.lines));
    s_log_level = LOG_DEBUG; /* Reset to default on each init */

    pthread_mutex_unlock(&s_log_mutex);

    tui_log(LOG_INFO, "Logging engine initialized. (Path: %s)", filepath ? filepath : "N/A");
}

void tui_logger_destroy(void) {
    tui_log(LOG_INFO, "Shutting down logging engine.");

    pthread_mutex_lock(&s_log_mutex);
    if (s_log_file) {
        fclose(s_log_file);
        s_log_file = NULL;
    }
    pthread_mutex_unlock(&s_log_mutex);
}

void tui_log(LogLevel level, const char* format, ...) {
    pthread_mutex_lock(&s_log_mutex);

    /* Filter by log level */
    if (level < s_log_level) {
        pthread_mutex_unlock(&s_log_mutex);
        return;
    }

    /* Time formatting */
    time_t rawtime;
    struct tm * timeinfo;
    char time_str[24];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(time_str, sizeof(time_str), "%H:%M:%S", timeinfo);
    
    const char* level_str;
    switch(level) {
        case LOG_INFO:  level_str = "INFO"; break;
        case LOG_WARN:  level_str = "WARN"; break;
        case LOG_ERROR: level_str = "ERR "; break;
        case LOG_DEBUG: level_str = "DBG "; break;
        default:        level_str = "UNK "; break;
    }
    
    /* Resolving variadic arguments */
    char msg_buf[MAX_LOG_LENGTH];
    va_list args;
    va_start(args, format);
    vsnprintf(msg_buf, sizeof(msg_buf), format, args);
    va_end(args);

    /* Write physical file if open */
    if (s_log_file) {
        fprintf(s_log_file, "[%s] [%s] %s\n", time_str, level_str, msg_buf);
        fflush(s_log_file); /* Force flush to guarantee audit logs persist */
    }

    /* Write to memory ring-buffer: "[HH:MM:SS] msg" fits in 256 chars */
    /* time_str is 8 chars max, prefix is 12 total, leaving 244 for msg */
    int h = s_log_buffer.head;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    snprintf(s_log_buffer.lines[h], MAX_LOG_LENGTH, "[%8s] %.*s",
             time_str, MAX_LOG_LENGTH - 13, msg_buf);
#pragma GCC diagnostic pop
    s_log_buffer.levels[h] = level;
    
    s_log_buffer.head = (h + 1) % MAX_LOG_LINES;
    if (s_log_buffer.count < MAX_LOG_LINES) {
        s_log_buffer.count++;
    }
    
    pthread_mutex_unlock(&s_log_mutex);
}

TuiLogBuffer* tui_logger_get_buffer(void) {
    return &s_log_buffer;
}

void tui_logger_set_level(LogLevel level) {
    pthread_mutex_lock(&s_log_mutex);
    s_log_level = level;
    pthread_mutex_unlock(&s_log_mutex);
}

LogLevel tui_logger_get_level(void) {
    pthread_mutex_lock(&s_log_mutex);
    LogLevel level = s_log_level;
    pthread_mutex_unlock(&s_log_mutex);
    return level;
}
