#include "core/error.h"
#include "core/logger.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static __thread TuiError s_last_error = TUI_OK;
static __thread char s_error_msg[256] = {0};

TuiError tui_last_error(void) {
    return s_last_error;
}

const char* tui_last_error_msg(void) {
    if (s_last_error == TUI_OK) return "No error";
    if (s_error_msg[0] == '\0') return "Unknown error";
    return s_error_msg;
}

void tui_clear_error(void) {
    s_last_error = TUI_OK;
    s_error_msg[0] = '\0';
}

void _tui_set_error(TuiError err, const char* fmt, ...) {
    s_last_error = err;

    const char* prefix = "Unknown error";
    switch (err) {
        case TUI_ERROR_NULL_ARGUMENT:       prefix = "Null argument"; break;
        case TUI_ERROR_OUT_OF_MEMORY:        prefix = "Out of memory"; break;
        case TUI_ERROR_PLANE_CREATE_FAILED:  prefix = "Cannot create plane"; break;
        case TUI_ERROR_PLANE_REPARENT_FAILED: prefix = "Cannot reparent plane"; break;
        case TUI_ERROR_WIDGET_LIMIT_REACHED: prefix = "Widget limit reached"; break;
        case TUI_ERROR_INVALID_INDEX:        prefix = "Invalid index"; break;
        case TUI_ERROR_INVALID_THEME:        prefix = "Invalid theme"; break;
        case TUI_ERROR_FILE_NOT_FOUND:       prefix = "File not found"; break;
        default: break;
    }

    int n = snprintf(s_error_msg, sizeof(s_error_msg), "%s", prefix);
    if (fmt && n > 0 && n < (int)sizeof(s_error_msg)) {
        va_list args;
        va_start(args, fmt);
        vsnprintf(s_error_msg + n, sizeof(s_error_msg) - n, fmt, args);
        va_end(args);
    }

    tui_log(LOG_ERROR, "[ERROR] %s", s_error_msg);
}
