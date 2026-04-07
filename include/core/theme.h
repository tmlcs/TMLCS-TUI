/**
 * @file theme.h
 * @brief Color palette and layout constants for the tmlcs-tui framework.
 *
 * Defines the Dracula-based color theme and layout dimensions used
 * throughout the TUI. Colors can be queried at runtime via
 * tui_theme_get_color() for dynamic theming support.
 *
 * @defgroup theme_colors Theme Colors
 * @brief RGB color constants (0xRRGGBB format).
 *
 * @defgroup theme_layout Theme Layout
 * @brief Terminal cell dimensions and positioning constants.
 *
 * @defgroup theme_zones Interaction Zones
 * @brief Mouse interaction boundary constants.
 *
 * @defgroup theme_components Component Colors
 * @brief Color constants for specific UI components.
 */

#ifndef CORE_THEME_H
#define CORE_THEME_H

/* ============================================================
 * Dracula Color Palette (default theme)
 * ============================================================ */

/** @ingroup theme_colors Background colors */
/** @brief Darkest background — terminal base. */
#define THEME_BG_DARKEST    0x111116
/** @brief Dark background — toolbar bar. */
#define THEME_BG_DARK       0x1A1A24
/** @brief Taskbar bottom bar background. */
#define THEME_BG_TASKBAR    0x21222c
/** @brief Window background. */
#define THEME_BG_WINDOW     0x282a36
/** @brief Text input background when focused. */
#define THEME_BG_TEXT_FOC   0x383a4f
/** @brief Active window title bar background. */
#define THEME_BG_TITLE_ACT  0xf1fa8c
/** @brief Inactive window title bar background. */
#define THEME_BG_TITLE_INA  0xbd93f9
/** @brief Close button background. */
#define THEME_BG_CLOSE_BTN  0xff5555
/** @brief Active tab background. */
#define THEME_BG_TAB_ACTIVE 0x50fa7b
/** @brief Start button background. */
#define THEME_BG_START_BTN  0xbd93f9

/** @ingroup theme_colors Foreground colors */
/** @brief Default text foreground. */
#define THEME_FG_DEFAULT    0xf8f8f2
/** @brief Text input cursor color. */
#define THEME_FG_CURSOR     0xffb86c
/** @brief Log timestamp color. */
#define THEME_FG_TIMESTAMP  0xCCCCCC
/** @brief Separator/border divider color. */
#define THEME_FG_SEPARATOR  0x333344
/** @brief Tab subtitle/subtitle text color. */
#define THEME_FG_SUBTITLE   0x627211
/** @brief Workspace label text color. */
#define THEME_FG_WS_LABEL   0xffb86c
/** @brief Active tab text (contrast on green bg). */
#define THEME_FG_TAB_ACTIVE 0x282a36
/** @brief Inactive tab text color. */
#define THEME_FG_TAB_INA    0x6272a4
/** @brief Active window border color. */
#define THEME_FG_BORDER_ACT 0xf1fa8c
/** @brief Inactive window border color. */
#define THEME_FG_BORDER_INA 0x44475a
/** @brief Active title text color. */
#define THEME_FG_TITLE_ACT  0xf1fa8c
/** @brief Inactive title text color. */
#define THEME_FG_TITLE_INA  0xbd93f9
/** @brief Close button text color. */
#define THEME_FG_CLOSE_TEXT 0xf8f8f2
/** @brief Resize grip zone background. */
#define THEME_FG_GRIP       0x111116
/** @brief Text input icon color. */
#define THEME_FG_INPUT_ICON 0x50fa7b
/** @brief Text input border color. */
#define THEME_FG_INPUT_BORD 0x6272a4

/** @ingroup theme_colors Log level colors */
/** @brief Log INFO level color (green). */
#define THEME_FG_LOG_INFO   0x50fa7b
/** @brief Log WARN level color (orange). */
#define THEME_FG_LOG_WARN   0xffb86c
/** @brief Log ERROR level color (red). */
#define THEME_FG_LOG_ERROR  0xff5555
/** @brief Log DEBUG level color (blue-gray). */
#define THEME_FG_LOG_DEBUG  0x6272a4

/** @brief Taskbar clock text color. */
#define THEME_FG_TIMEBAR    0xf8f8f2

/* ============================================================
 * Layout and Dimensions
 * ============================================================ */

/** @ingroup theme_layout */
/** @brief Height of tab bar in terminal lines (toolbar + separator). */
#define TAB_BAR_HEIGHT          2
/** @brief Height of bottom taskbar in terminal lines. */
#define TASKBAR_HEIGHT          1
/** @brief X position where the first tab label starts in toolbar. */
#define TAB_BAR_START_X         30
/** @brief Minimum window height for resizing. */
#define WINDOW_MIN_HEIGHT       4
/** @brief Minimum window width for resizing. */
#define WINDOW_MIN_WIDTH        15
/** @brief Minimum X offset from right edge for grip zone. */
#define GRIP_AREA_MIN_X         2
/** @brief Minimum X offset from right edge for close button zone. */
#define CLOSE_BUTTON_AREA_MIN_X 4
/** @brief Left padding before title text. */
#define TITLE_PADDING_LEFT      2
/** @brief Left padding on log lines. */
#define LOG_LINE_LEFT_PAD       2
/** @brief Vertical offset below frame for log rendering. */
#define LOG_FRAME_TOP_OFFSET    1
/** @brief Y line for the [ Input ] text label. */
#define INPUT_FRAME_Y           2
/** @brief Y line for the text input widget content. */
#define INPUT_TEXT_Y            3
/** @brief Left padding inside text input area. */
#define INPUT_LEFT_PAD          1
/** @brief Width reduction from window width for text input area. */
#define INPUT_WIDTH_REDUCTION   3
/** @brief Minimum width for a text input widget. */
#define MIN_TEXT_INPUT_WIDTH    4
/** @brief Y offset where tab content plane starts. */
#define TAB_PLANE_TOP_PAD       2
/** @brief Lines reserved at bottom for taskbar. */
#define WS_PLANE_BOTTOM_PAD     1
/** @brief Y position of tab header text. */
#define TAB_HEADER_TOP          0
/** @brief Default X position for new windows. */
#define WINDOW_PLANE_DEF_X      0
/** @brief Default Y position for new windows. */
#define WINDOW_PLANE_DEF_Y      0
/** @brief Maximum height for window resize via keyboard. */
#define WINDOW_MAX_HEIGHT       50
/** @brief Maximum width for window resize via keyboard. */
#define WINDOW_MAX_WIDTH        100

/** @ingroup theme_layout Off-screen parking positions */
/** @brief Off-screen X coordinate for hidden elements. */
#define WS_HIDDEN_X             -10000
/** @brief Off-screen Y coordinate for hidden elements. */
#define OFFSCREEN_Y            -10000

/* ============================================================
 * Interaction Zones
 * ============================================================ */

/** @ingroup theme_zones */
/** @brief X offset from right edge for resize grip area. */
#define GRIP_AREA_START_X       3
/** @brief Extra characters for tab label (spaces + decoration). */
#define TAB_LABEL_PADDING       6
/** @brief Maximum Notcurses mouse button code. */
#define MAX_MOUSE_BUTTON        NCKEY_BUTTON11

/* ============================================================
 * Component-Specific Colors
 * ============================================================ */

/** @ingroup theme_components Cursor */
/** @brief Block cursor color. */
#define CURSOR_BLOCK_COLOR  0xffb86c
/** @brief Block cursor character. */
#define CURSOR_BLOCK_CHAR   ' '

/** @ingroup theme_components Text Input */
/** @brief Text input normal foreground. */
#define INPUT_FG_NORMAL     0xf8f8f2
/** @brief Text input normal background. */
#define INPUT_BG_NORMAL     0x21222c
/** @brief Text input focused background. */
#define INPUT_BG_FOCUSED    0x383a4f

/** @ingroup theme_components Window */
/** @brief Window normal border color. */
#define WIN_BORDER_FG_NORMAL 0xbd93f9
/** @brief Window normal background. */
#define WIN_BG_NORMAL        0x282a36

/** @ingroup theme_components Start Button */
/** @brief Start button background. */
#define START_BTN_BG  0xbd93f9
/** @brief Start button foreground. */
#define START_BTN_FG  0x282a36

#endif /* CORE_THEME_H */
