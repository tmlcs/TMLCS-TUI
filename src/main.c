/**
 * Entry point mínimo para la aplicación demo.
 * Todo el código específico de la aplicación vive en examples/demo.c
 * Este archivo solo inicializa notcurses y delega.
 */

#include <notcurses/notcurses.h>
#include <locale.h>
#include <string.h>
#include "core/logger.h"

// Forward declaration del main del demo (definido en examples/demo.c)
int demo_main(struct notcurses* nc);

int main(void) {
    if (!setlocale(LC_ALL, "")) {
        fprintf(stderr, "Error en setlocale\n");
        return 1;
    }

    struct notcurses_options n_opts = { .flags = 0 };
    struct notcurses* nc = notcurses_init(&n_opts, NULL);
    if (!nc) return 1;

    // Inicializar logger ANTES de cualquier otra cosa del core
    tui_logger_init("/tmp/tmlcs_tui_debug.log");
    tui_log(LOG_INFO, "Iniciando TMLCS-TUI (core library)");

    // Habilitar mouse events para el demo
    notcurses_mice_enable(nc, NCMICE_ALL_EVENTS);

    // Delega al demo
    int ret = demo_main(nc);

    // Cleanup
    tui_logger_destroy();
    notcurses_stop(nc);
    return ret;
}
