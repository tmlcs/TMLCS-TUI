#include "core/help.h"
#include "core/theme.h"
#include <string.h>

static bool s_help_visible = false;

void tui_help_show(struct ncplane* plane) {
    if (!plane) return;
    s_help_visible = true;

    uint64_t bg = 0;
    ncchannels_set_fg_rgb(&bg, THEME_FG_DEFAULT);
    ncchannels_set_bg_rgb(&bg, THEME_BG_DARKEST);
    ncchannels_set_bg_alpha(&bg, NCALPHA_OPAQUE);
    ncplane_set_base(plane, " ", 0, bg);
    ncplane_erase(plane);

    /* Title */
    ncplane_set_fg_rgb(plane, THEME_FG_WS_LABEL);
    ncplane_printf_yx(plane, 1, 4, "=== TMLCS-TUI Help ===");

    /* Keybindings table */
    ncplane_set_fg_rgb(plane, THEME_FG_DEFAULT);
    int row = 3;
    const char* lines[] = {
        " Key          Action",
        " q / Q        Quit application",
        " Alt+1..9     Switch workspace",
        " Left / Right Navigate tabs",
        " Tab          Cycle window focus",
        " x            Close focused window",
        " w            Close active tab",
        " d            Close active workspace",
        " F1 / ?       Toggle this help screen",
        "",
        " Mouse:",
        " Click tab    Switch tab",
        " Click window Focus window",
        " Drag title   Move window",
        " Drag grip    Resize window",
        " Click [X]    Close window",
        "",
        " Press any key to dismiss.",
        NULL
    };
    for (int i = 0; lines[i] != NULL; i++) {
        ncplane_putstr_yx(plane, row + i, 4, lines[i]);
    }

    notcurses_render(ncplane_notcurses(plane));
}

void tui_help_hide(void) {
    s_help_visible = false;
}

bool tui_help_is_visible(void) {
    return s_help_visible;
}
