#include "core/theme_loader.h"
#include "core/logger.h"
#include <string.h>
#include <stdio.h>

/* Runtime-overridable color storage (sparse, only changed colors) */
#define MAX_THEME_OVERRIDES 64
static struct {
    char name[32];
    unsigned rgb;
} s_theme_overrides[MAX_THEME_OVERRIDES];
static int s_override_count = 0;

/* Helper: find override index */
static int find_override(const char* name) {
    for (int i = 0; i < s_override_count; i++) {
        if (strcmp(s_theme_overrides[i].name, name) == 0) return i;
    }
    return -1;
}

bool tui_theme_set_color(const char* name, unsigned rgb) {
    unsigned dummy;
    if (!tui_theme_get_color(name, &dummy) && find_override(name) < 0) {
        tui_log(LOG_WARN, "tui_theme_set_color: unknown color '%s'", name);
        return false;
    }
    int idx = find_override(name);
    if (idx >= 0) {
        s_theme_overrides[idx].rgb = rgb;
    } else if (s_override_count < MAX_THEME_OVERRIDES) {
        strncpy(s_theme_overrides[s_override_count].name, name, 31);
        s_theme_overrides[s_override_count].name[31] = '\0';
        s_theme_overrides[s_override_count].rgb = rgb;
        s_override_count++;
    }
    return true;
}

bool tui_theme_get_color(const char* name, unsigned* rgb) {
    int idx = find_override(name);
    if (idx >= 0) {
        if (rgb) *rgb = s_theme_overrides[idx].rgb;
        return true;
    }
    #define CHECK_THEME(c) if (strcmp(name, #c) == 0) { if (rgb) *rgb = c; return true; }
    CHECK_THEME(THEME_BG_DARKEST)
    CHECK_THEME(THEME_BG_DARK)
    CHECK_THEME(THEME_BG_TASKBAR)
    CHECK_THEME(THEME_BG_WINDOW)
    CHECK_THEME(THEME_BG_TEXT_FOC)
    CHECK_THEME(THEME_BG_TITLE_ACT)
    CHECK_THEME(THEME_BG_TITLE_INA)
    CHECK_THEME(THEME_BG_CLOSE_BTN)
    CHECK_THEME(THEME_BG_TAB_ACTIVE)
    CHECK_THEME(THEME_BG_START_BTN)
    CHECK_THEME(THEME_FG_DEFAULT)
    CHECK_THEME(THEME_FG_CURSOR)
    CHECK_THEME(THEME_FG_TIMESTAMP)
    CHECK_THEME(THEME_FG_SEPARATOR)
    CHECK_THEME(THEME_FG_SUBTITLE)
    CHECK_THEME(THEME_FG_WS_LABEL)
    CHECK_THEME(THEME_FG_TAB_ACTIVE)
    CHECK_THEME(THEME_FG_TAB_INA)
    CHECK_THEME(THEME_FG_BORDER_ACT)
    CHECK_THEME(THEME_FG_BORDER_INA)
    CHECK_THEME(THEME_FG_TITLE_ACT)
    CHECK_THEME(THEME_FG_TITLE_INA)
    CHECK_THEME(THEME_FG_CLOSE_TEXT)
    CHECK_THEME(THEME_FG_GRIP)
    CHECK_THEME(THEME_FG_INPUT_ICON)
    CHECK_THEME(THEME_FG_INPUT_BORD)
    CHECK_THEME(THEME_FG_LOG_INFO)
    CHECK_THEME(THEME_FG_LOG_WARN)
    CHECK_THEME(THEME_FG_LOG_ERROR)
    CHECK_THEME(THEME_FG_LOG_DEBUG)
    CHECK_THEME(THEME_FG_TIMEBAR)
    #undef CHECK_THEME
    return false;
}

bool tui_theme_load_file(const char* filepath) {
    FILE* f = fopen(filepath, "r");
    if (!f) {
        tui_log(LOG_ERROR, "tui_theme_load_file: cannot open '%s'", filepath);
        return false;
    }
    char line[128];
    int loaded = 0;
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') continue;
        char name[64];
        unsigned rgb;
        if (sscanf(line, "%63s %x", name, &rgb) == 2) {
            tui_theme_set_color(name, rgb);
            loaded++;
        }
    }
    fclose(f);
    tui_log(LOG_INFO, "tui_theme_load_file: loaded %d colors from '%s'", loaded, filepath);
    return true;
}

/* Built-in themes */
typedef struct {
    const char* color_name;
    unsigned rgb;
} ThemeEntry;

static const ThemeEntry theme_solarized[] = {
    {"THEME_BG_DARKEST",    0x002b36},
    {"THEME_BG_DARK",       0x073642},
    {"THEME_BG_TASKBAR",    0x073642},
    {"THEME_BG_WINDOW",     0x002b36},
    {"THEME_FG_DEFAULT",    0x839496},
    {"THEME_FG_TIMESTAMP",  0x586e75},
    {"THEME_FG_SEPARATOR",  0x586e75},
    {"THEME_FG_TAB_ACTIVE", 0x002b36},
    {"THEME_BG_TAB_ACTIVE", 0x859900},
    {"THEME_FG_TAB_INA",    0x586e75},
    {"THEME_BG_TITLE_ACT",  0x859900},
    {"THEME_BG_TITLE_INA",  0x268bd2},
    {NULL, 0}
};

static const ThemeEntry theme_highcontrast[] = {
    {"THEME_BG_DARKEST",    0x000000},
    {"THEME_BG_DARK",       0x1a1a1a},
    {"THEME_BG_TASKBAR",    0x1a1a1a},
    {"THEME_BG_WINDOW",     0x0a0a0a},
    {"THEME_FG_DEFAULT",    0xffffff},
    {"THEME_FG_TIMESTAMP",  0xcccccc},
    {"THEME_FG_SEPARATOR",  0x666666},
    {"THEME_FG_TAB_ACTIVE", 0x000000},
    {"THEME_BG_TAB_ACTIVE", 0xffff00},
    {"THEME_FG_TAB_INA",    0x999999},
    {"THEME_BG_TITLE_ACT",  0xffff00},
    {"THEME_BG_TITLE_INA",  0x444444},
    {"THEME_FG_LOG_INFO",   0x00ff00},
    {"THEME_FG_LOG_WARN",   0xffff00},
    {"THEME_FG_LOG_ERROR",  0xff0000},
    {NULL, 0}
};

static bool apply_theme_entries(const ThemeEntry* entries) {
    for (int i = 0; entries[i].color_name != NULL; i++) {
        if (!tui_theme_set_color(entries[i].color_name, entries[i].rgb)) return false;
    }
    return true;
}

bool tui_theme_apply(const char* name) {
    if (strcmp(name, "dracula") == 0) {
        s_override_count = 0;
        tui_log(LOG_INFO, "tui_theme_apply: dracula (defaults)");
        return true;
    }
    if (strcmp(name, "solarized") == 0) {
        s_override_count = 0;
        bool ok = apply_theme_entries(theme_solarized);
        if (ok) tui_log(LOG_INFO, "tui_theme_apply: solarized");
        return ok;
    }
    if (strcmp(name, "highcontrast") == 0) {
        s_override_count = 0;
        bool ok = apply_theme_entries(theme_highcontrast);
        if (ok) tui_log(LOG_INFO, "tui_theme_apply: highcontrast");
        return ok;
    }
    tui_log(LOG_ERROR, "tui_theme_apply: unknown theme '%s'", name);
    return false;
}
