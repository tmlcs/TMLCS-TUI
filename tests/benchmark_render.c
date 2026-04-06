/**
 * @file benchmark_render.c
 * @brief Simple render loop performance benchmark.
 *
 * Measures the time to execute render iterations simulating
 * the dirty tracking logic overhead.
 *
 * Usage: ./build/benchmark_render_runner
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include "core/logger.h"

static double now_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1e6;
}

/* Simulate window render work */
static void simulate_window_render(void) {
    volatile char buf[256];
    for (int i = 0; i < 256; i++) buf[i] = (char)(i & 0xFF);
    (void)buf;
}

/* Simulate toolbar render work */
static void simulate_toolbar_render(void) {
    volatile char buf[128];
    for (int i = 0; i < 128; i++) buf[i] = (char)(i & 0xFF);
    (void)buf;
}

/* Simulate taskbar render work */
static void simulate_taskbar_render(void) {
    volatile char buf[64];
    for (int i = 0; i < 64; i++) buf[i] = (char)(i & 0xFF);
    (void)buf;
}

/* Benchmark: full redraw (no dirty tracking) */
static void benchmark_full_redraw(int iterations, int num_windows) {
    printf("\n--- Full Redraw Benchmark (no dirty tracking, %d windows) ---\n", num_windows);

    double start = now_ms();
    for (int iter = 0; iter < iterations; iter++) {
        /* Always render everything */
        simulate_toolbar_render();
        simulate_taskbar_render();
        for (int w = 0; w < num_windows; w++) {
            simulate_window_render();
        }
    }
    double elapsed = now_ms() - start;

    printf("  Total time:          %.2f ms\n", elapsed);
    printf("  Time per iteration:  %.2f us\n", (elapsed * 1000.0) / iterations);
    printf("  Effective FPS:       %.0f\n", 1000.0 / (elapsed / iterations));
}

/* Benchmark: dirty tracking (idle frames) */
static void benchmark_dirty_tracking(int iterations, int num_windows) {
    printf("\n--- Dirty Tracking Benchmark (idle frames, %d windows) ---\n", num_windows);

    /* Simulate dirty flags and state tracking */
    bool toolbar_dirty = false;
    bool taskbar_dirty = false;
    bool* window_dirty = (bool*)calloc(num_windows, sizeof(bool));
    int last_active_window = 0;
    int current_active_window = 0;
    char last_clock[10] = "12:00";
    char current_clock[10];

    double start = now_ms();
    for (int iter = 0; iter < iterations; iter++) {
        /* No state changes - simulate idle frames */
        bool focus_changed = (current_active_window != last_active_window);
        bool clock_changed = false; /* Clock stays the same */

        if (focus_changed) {
            toolbar_dirty = true;
            for (int w = 0; w < num_windows; w++) {
                window_dirty[w] = true;
            }
        }

        /* Only render dirty zones */
        if (toolbar_dirty) {
            simulate_toolbar_render();
        }
        if (taskbar_dirty || clock_changed) {
            simulate_taskbar_render();
            strcpy(last_clock, current_clock);
        }
        for (int w = 0; w < num_windows; w++) {
            if (window_dirty[w]) {
                simulate_window_render();
                window_dirty[w] = false;
            }
        }

        /* Update cached state */
        last_active_window = current_active_window;

        /* Clear dirty flags */
        toolbar_dirty = false;
        taskbar_dirty = false;
    }
    double elapsed = now_ms() - start;

    printf("  Total time:          %.2f ms\n", elapsed);
    printf("  Time per iteration:  %.2f us\n", (elapsed * 1000.0) / iterations);
    printf("  Effective FPS:       %.0f\n", 1000.0 / (elapsed / iterations));

    free(window_dirty);
}

/* Benchmark: dirty tracking with occasional updates (10% window changes) */
static void benchmark_mixed(int iterations, int num_windows) {
    printf("\n--- Mixed Benchmark (90%% idle, 10%% window update, %d windows) ---\n", num_windows);

    bool toolbar_dirty = false;
    bool taskbar_dirty = false;
    bool* window_dirty = (bool*)calloc(num_windows, sizeof(bool));
    int last_active_window = 0;
    int current_active_window = 0;
    char last_clock[10] = "12:00";
    char current_clock[10];

    double start = now_ms();
    for (int iter = 0; iter < iterations; iter++) {
        /* 10% of frames have a window update */
        if (iter % 10 == 0) {
            window_dirty[0] = true;
        }

        bool focus_changed = (current_active_window != last_active_window);
        bool clock_changed = false;

        if (focus_changed) {
            toolbar_dirty = true;
            for (int w = 0; w < num_windows; w++) {
                window_dirty[w] = true;
            }
        }

        if (toolbar_dirty) {
            simulate_toolbar_render();
        }
        if (taskbar_dirty || clock_changed) {
            simulate_taskbar_render();
            strcpy(last_clock, current_clock);
        }
        for (int w = 0; w < num_windows; w++) {
            if (window_dirty[w]) {
                simulate_window_render();
                window_dirty[w] = false;
            }
        }

        last_active_window = current_active_window;
        toolbar_dirty = false;
        taskbar_dirty = false;
    }
    double elapsed = now_ms() - start;

    printf("  Total time:          %.2f ms\n", elapsed);
    printf("  Time per iteration:  %.2f us\n", (elapsed * 1000.0) / iterations);
    printf("  Effective FPS:       %.0f\n", 1000.0 / (elapsed / iterations));

    free(window_dirty);
}

int main(void) {
    const int iterations = 10000;
    const int windows = 5;

    printf("Render Benchmark: %d iterations x %d windows\n", iterations, windows);
    printf("================================================\n");

    tui_logger_init("/tmp/benchmark.log");
    tui_logger_set_level(LOG_WARN);

    benchmark_full_redraw(iterations, windows);
    benchmark_dirty_tracking(iterations, windows);
    benchmark_mixed(iterations, windows);

    printf("\n================================================\n");
    printf("Benchmark complete. Check /tmp/benchmark.log for details.\n");

    tui_logger_destroy();
    return 0;
}
