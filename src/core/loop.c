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

int tui_manager_run(TuiManager* mgr, const TuiLoopConfig* config) {
    if (!mgr) return -1;

    int frame_time_ns = 16666666;  /* ~60fps default */
    if (config && config->target_fps > 0) {
        frame_time_ns = 1000000000 / config->target_fps;
    }

    struct ncinput ni;

    while (mgr->_running) {
        /* Pre-frame callback */
        if (config && config->on_frame) {
            if (!config->on_frame(mgr, config->userdata)) {
                struct timespec ts = {0, frame_time_ns};
                nanosleep(&ts, NULL);
                continue;
            }
        }

        /* Render */
        tui_manager_render(mgr);

        /* Post-render callback */
        if (config && config->after_render) {
            config->after_render(mgr, config->userdata);
        }

        /* Input */
        uint32_t key = notcurses_get_nblock(mgr->_nc, &ni);
        if (key == (uint32_t)-1) {
            struct timespec ts = {0, frame_time_ns};
            nanosleep(&ts, NULL);
            continue;
        }

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

        /* Frame rate limiting */
        struct timespec ts = {0, frame_time_ns};
        nanosleep(&ts, NULL);
    }

    return 0;
}

void tui_manager_quit(TuiManager* mgr) {
    if (mgr) mgr->_running = false;
}
