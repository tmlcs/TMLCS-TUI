/**
 * Entry point for the demo application.
 * All application-specific code lives in examples/demo.c
 * This file only initializes notcurses and delegates.
 */

#include <notcurses/notcurses.h>
#include <locale.h>
#include <string.h>
#include <stdio.h>
#include "core/logger.h"

/* Forward declaration of the demo main (defined in examples/demo.c) */
int demo_main(struct notcurses* nc);

static void print_usage(const char* prog) {
    printf("TMLCS-TUI v0.6.0 - Terminal User Interface Framework\n");
    printf("Usage: %s [OPTIONS]\n", prog);
    printf("\nOptions:\n");
    printf("  --help, -h        Show this help message\n");
    printf("  --version, -V     Show version\n");
    printf("  --log-level LVL   Set log level (0=DEBUG, 1=INFO, 2=WARN, 3=ERROR)\n");
    printf("  --log-file PATH   Set log file path (default: /tmp/tmlcs_tui_debug.log)\n");
    printf("\nKeybindings:\n");
    printf("  q/Q          Quit\n");
    printf("  Alt+1..9     Switch workspace\n");
    printf("  Left/Right   Navigate tabs\n");
    printf("  Tab          Cycle window focus\n");
    printf("  x            Close focused window\n");
    printf("  w            Close active tab\n");
    printf("  d            Close active workspace\n");
}

static void print_version(void) {
    printf("tmlcs-tui version 0.6.0\n");
    printf("Copyright (c) 2026 TMLCS-TUI contributors. MIT License.\n");
}

int main(int argc, char* argv[]) {
    const char* log_file = "/tmp/tmlcs_tui_debug.log";
    LogLevel log_level = LOG_DEBUG;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-V") == 0) {
            print_version();
            return 0;
        }
        if (strcmp(argv[i], "--log-level") == 0 && i + 1 < argc) {
            char* endptr;
            long val = strtol(argv[++i], &endptr, 10);
            if (*endptr == '\0' && val >= LOG_DEBUG && val <= LOG_ERROR) {
                log_level = (LogLevel)val;
            } else {
                fprintf(stderr, "Invalid log level: %s (must be 0-3)\n", argv[i]);
                return 1;
            }
        }
        if (strcmp(argv[i], "--log-file") == 0 && i + 1 < argc) {
            log_file = argv[++i];
        }
    }

    if (!setlocale(LC_ALL, "")) {
        fprintf(stderr, "Error in setlocale\n");
        return 1;
    }

    struct notcurses_options n_opts = { .flags = 0 };
    struct notcurses* nc = notcurses_init(&n_opts, NULL);
    if (!nc) return 1;

    tui_logger_init(log_file);
    tui_logger_set_level(log_level);
    tui_log(LOG_INFO, "Starting TMLCS-TUI (log level: %d, file: %s)", log_level, log_file);

    notcurses_mice_enable(nc, NCMICE_ALL_EVENTS);

    int ret = demo_main(nc);

    tui_logger_destroy();
    notcurses_stop(nc);
    return ret;
}
