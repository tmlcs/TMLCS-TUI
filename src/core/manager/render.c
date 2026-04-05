#include "core/manager.h"
#include "core/workspace.h"
#include "core/tab.h"
#include "core/window.h"
#include "core/theme.h"
#include <string.h>
#include <time.h>

void tui_manager_render(TuiManager* manager) {
    // Fondo base del UI global real
    uint64_t bg_channels = 0;
    ncchannels_set_fg_rgb(&bg_channels, THEME_FG_TIMESTAMP);
    ncchannels_set_bg_rgb(&bg_channels, THEME_BG_DARKEST);
    ncchannels_set_bg_alpha(&bg_channels, NCALPHA_OPAQUE); // FUERZA opacidad para evitar terminal host
    ncplane_set_base(manager->stdplane, " ", 0, bg_channels);
    ncplane_erase(manager->stdplane);
    unsigned dimy, dimx;
    ncplane_dim_yx(manager->stdplane, &dimy, &dimx);

    if (manager->active_workspace_index != -1) {
        TuiWorkspace* ws = manager->workspaces[manager->active_workspace_index];
        
        // 1. Barra de Herramientas Estilizada (Más Delgada)
        ncplane_set_bg_rgb(manager->stdplane, THEME_BG_DARK);
        for (unsigned i = 0; i < dimx; i++) ncplane_putstr_yx(manager->stdplane, 0, i, " ");

        // Separador sutil horizontal
        ncplane_set_fg_rgb(manager->stdplane, THEME_FG_SEPARATOR);
        for (unsigned i = 0; i < dimx; i++) ncplane_putstr_yx(manager->stdplane, 1, i, "━");

        // Info de Workspace
        ncplane_set_fg_rgb(manager->stdplane, THEME_FG_WS_LABEL);
        ncplane_printf_yx(manager->stdplane, 0, 2, "◆ Workspace: %s", ws->name);

        int offset_x = TAB_BAR_START_X;
        for(int i=0; i<ws->tab_count; i++){
            // Tab seleccionado con contraste alto
            if (i == ws->active_tab_index) {
                 ncplane_set_bg_rgb(manager->stdplane, THEME_BG_TAB_ACTIVE);
                 ncplane_set_fg_rgb(manager->stdplane, THEME_FG_TAB_ACTIVE);
                 ncplane_putstr_yx(manager->stdplane, 0, offset_x, " ");
                 ncplane_printf_yx(manager->stdplane, 0, offset_x + 1, " %s ", ws->tabs[i]->name);
                 ncplane_putstr_yx(manager->stdplane, 0, offset_x + 3 + strlen(ws->tabs[i]->name), " ");
            } else {
                 ncplane_set_bg_rgb(manager->stdplane, THEME_BG_DARK);
                 ncplane_set_fg_rgb(manager->stdplane, THEME_FG_TAB_INA);
                 ncplane_printf_yx(manager->stdplane, 0, offset_x + 2, " %s ", ws->tabs[i]->name);
            }
            offset_x += strlen(ws->tabs[i]->name) + 6;
        }

        // Dejar listo el stdplane con fondo base de nuevo
        ncplane_set_bg_rgb(manager->stdplane, THEME_BG_DARKEST);
        
        // 2. Procesar los renders dentro del Tab 
        if (ws->active_tab_index != -1) {
            TuiTab* tab = ws->tabs[ws->active_tab_index];
            
            // RENDER: Borde externo decorativo para el "Viewport de la pestaña"
            unsigned t_dimy, t_dimx;
            ncplane_dim_yx(tab->tab_plane, &t_dimy, &t_dimx);
            
            ncplane_set_bg_rgb(tab->tab_plane, THEME_BG_DARKEST);
            ncplane_set_fg_rgb(tab->tab_plane, THEME_FG_SEPARATOR); // Gris tenue para no molestar a las ventanas
            ncplane_erase(tab->tab_plane); // Limpiar rastros iterativos

            // Dibujar box-drawing redondeado con API Nativo de contingencia
            uint64_t border_channels = 0;
            ncchannels_set_fg_rgb(&border_channels, THEME_FG_SEPARATOR);
            ncchannels_set_bg_rgb(&border_channels, THEME_BG_DARKEST);
            ncplane_perimeter_rounded(tab->tab_plane, 0, border_channels, 0);

            // Subtítulo decorativo del Contenedor
            ncplane_set_fg_rgb(tab->tab_plane, THEME_FG_SUBTITLE); // Color secundario
            ncplane_printf_yx(tab->tab_plane, 0, 2, " ◳ Contenedor: %s ", tab->name);
            
            // RENDER: Ventanas hijas — foco sincronizado, siempre repintar
            for (int i = 0; i < tab->window_count; i++) {
                TuiWindow* win = tab->windows[i];
                
                // Sincronizar estado de foco (activa cambio de color de borde)
                win->focused = (i == tab->active_window_index);
                
                // Siempre llamar render_cb para mantener el contenido actualizado
                if (win->render_cb) {
                    win->render_cb(win);
                }
            }
        }
    }
    
    // --- RENDER TASKBAR INFERIOR ---
    ncplane_set_bg_rgb(manager->stdplane, THEME_BG_TASKBAR); // Fondo taskbar independiente
    for (unsigned i = 0; i < dimx; i++) ncplane_putstr_yx(manager->stdplane, dimy - 1, i, " ");

    ncplane_set_bg_rgb(manager->stdplane, THEME_BG_TITLE_INA); // Boton Lila centralizado
    ncplane_set_fg_rgb(manager->stdplane, THEME_FG_TAB_ACTIVE);
    ncplane_printf_yx(manager->stdplane, dimy - 1, 0, " ❖ START ");
    
    time_t rawtime;
    struct tm * timeinfo;
    char time_str[10];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(time_str, sizeof(time_str), "%H:%M", timeinfo);
    
    ncplane_set_bg_rgb(manager->stdplane, THEME_BG_WINDOW);
    ncplane_set_fg_rgb(manager->stdplane, THEME_FG_DEFAULT);
    ncplane_printf_yx(manager->stdplane, dimy - 1, dimx - 10, " %s ", time_str);
    
    notcurses_render(manager->nc);
}
