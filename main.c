#include <notcurses/notcurses.h>
#include <locale.h>
#include <stdlib.h>
#include <stdio.h>

int main(void) {
    fprintf(stderr, "[DEBUG] Iniciando programa...\n");
    
    if (!setlocale(LC_ALL, "")) {
        fprintf(stderr, "[DEBUG] Error en setlocale\n");
    }

    struct notcurses_options n_opts = { .flags = NCOPTION_INHIBIT_SETLOCALE };
    
    fprintf(stderr, "[DEBUG] Intentando notcurses_init...\n");
    struct notcurses* nc = notcurses_init(&n_opts, NULL);
    
    if (!nc) {
        fprintf(stderr, "[DEBUG] notcurses_init falló. Terminal incompatible.\n");
        return 1;
    }
    fprintf(stderr, "[DEBUG] Notcurses iniciado correctamente.\n");

    struct ncplane* stdp = notcurses_stdplane(nc);
    if (!stdp) {
        fprintf(stderr, "[DEBUG] Error: stdplane es NULL\n");
        notcurses_stop(nc);
        return 1;
    }

    fprintf(stderr, "[DEBUG] Intentando crear primer plano (Desktop)...\n");
    struct ncplane_options opts = { .y = 0, .x = 0, .rows = 10, .cols = 10 };
    struct ncplane* p1 = ncplane_create(stdp, &opts);
    
    if (!p1) {
        fprintf(stderr, "[DEBUG] ncplane_create devolvió NULL. ¿Tamaño terminal?\n");
    } else {
        fprintf(stderr, "[DEBUG] Plano creado. Renderizando...\n");
        notcurses_render(nc);
    }

    fprintf(stderr, "[DEBUG] Esperando tecla para cerrar...\n");
    struct ncinput ni;
    notcurses_get_blocking(nc, &ni);

    notcurses_stop(nc);
    fprintf(stderr, "[DEBUG] Finalizado sin errores.\n");
    return 0;
}
