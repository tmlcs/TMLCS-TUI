/**
 * @file loop.c
 * @brief Main event loop implementation.
 *
 * Drives the render/input cycle at a configurable frame rate,
 * dispatching keyboard and mouse events through the manager.
 */

#include "core/loop.h"
#include "core/manager.h"
#include "core/types_private.h"
#include <notcurses/notcurses.h>
#include <time.h>

/** Get current monotonic time in nanoseconds */
static int64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (int64_t)ts.tv_sec * 1000000000LL + (int64_t)ts.tv_nsec;
}

int tui_manager_run(TuiManager* mgr, const TuiLoopConfig* config) {
    if (!mgr) return -1;

    mgr->_loop_config = config;

    int64_t frame_time_ns = 16666666;  /* ~60fps default */
    if (config && config->target_fps > 0) {
        frame_time_ns = 1000000000LL / config->target_fps;
    }

    struct ncinput ni;

    while (mgr->_running) {
        int64_t frame_start = now_ns();

        /* Pre-frame callback */
        if (config && config->on_frame) {
            if (!config->on_frame(mgr, config->userdata)) {
                /* Exit requested via callback — skip to input check */
            }
        }

        /* Render */
        tui_manager_render(mgr);

        /* Post-render callback */
        if (config && config->after_render) {
            config->after_render(mgr, config->userdata);
        }

        /* Input — non-blocking poll */
        uint32_t key = notcurses_get_nblock(mgr->_nc, &ni);

        if (key != (uint32_t)-1) {
            bool handled = false;
            if (key >= NCKEY_BUTTON1 && key <= NCKEY_BUTTON11) {
                handled = tui_manager_process_mouse(mgr, key, &ni);
            } else if (key == NCKEY_MOTION) {
                handled = tui_manager_process_mouse(mgr, key, &ni);
            } else {
                handled = tui_manager_process_keyboard(mgr, key, &ni);
                if (!handled && config && config->on_unhandled_key) {
                    config->on_unhandled_key(mgr, key, &ni, config->userdata);
                }
            }
        }

        /* Adaptive frame rate limiting: subtract elapsed time from sleep */
        int64_t elapsed = now_ns() - frame_start;
        int64_t sleep_ns = frame_time_ns - elapsed;
        if (sleep_ns > 0) {
            struct timespec ts = {
                .tv_sec  = (time_t)(sleep_ns / 1000000000LL),
                .tv_nsec = (long)(sleep_ns % 1000000000LL)
            };
            nanosleep(&ts, NULL);
        }
    }

    return 0;
}

void tui_manager_quit(TuiManager* mgr) {
    if (mgr) mgr->_running = false;
}
