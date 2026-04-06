/**
 * @file error.h
 * @brief Thread-local error reporting system.
 *
 * Provides a global (thread-local) last-error accessor for all TUI operations.
 */

#ifndef CORE_ERROR_H
#define CORE_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TUI_OK = 0,
    TUI_ERROR_NULL_ARGUMENT,
    TUI_ERROR_OUT_OF_MEMORY,
    TUI_ERROR_PLANE_CREATE_FAILED,
    TUI_ERROR_PLANE_REPARENT_FAILED,
    TUI_ERROR_WIDGET_LIMIT_REACHED,
    TUI_ERROR_INVALID_INDEX,
    TUI_ERROR_INVALID_THEME,
    TUI_ERROR_FILE_NOT_FOUND,
    TUI_ERROR_LAST
} TuiError;

/** Returns the last error code set by any TUI operation. */
TuiError tui_last_error(void);

/** Returns a human-readable message for the last error, or "No error" if TUI_OK. */
const char* tui_last_error_msg(void);

/** Clears the last error state. */
void tui_clear_error(void);

/** Internal: set an error with a printf-style detail message. */
void _tui_set_error(TuiError err, const char* fmt, ...);

#ifdef __cplusplus
}
#endif
#endif /* CORE_ERROR_H */
