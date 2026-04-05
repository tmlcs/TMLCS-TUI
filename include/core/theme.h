#ifndef CORE_THEME_H
#define CORE_THEME_H

// ============================================================
// TMLCS-TUI Theme Constants
// Centraliza todos los magic numbers de color y layout
// ============================================================

// --- Colores Dracula (tema por defecto) ---
// Fondos
#define THEME_BG_DARKEST    0x111116   // Fondo terminal base
#define THEME_BG_DARK       0x1A1A24   // Barra de herramientas
#define THEME_BG_TASKBAR    0x21222c   // Taskbar inferior
#define THEME_BG_WINDOW     0x282a36   // Fondo ventanas
#define THEME_BG_TEXT_FOC   0x383a4f   // Fondo text input enfocado
#define THEME_BG_TITLE_ACT  0xf1fa8c   // Fondo titulo activo
#define THEME_BG_TITLE_INA  0xbd93f9   // Fondo titulo inactivo
#define THEME_BG_CLOSE_BTN  0xff5555   // Fondo boton cerrar
#define THEME_BG_TAB_ACTIVE 0x50fa7b   // Fondo tab seleccionado
#define THEME_BG_START_BTN  0xbd93f9   // Fondo boton START

// Foregrounds
#define THEME_FG_DEFAULT    0xf8f8f2   // Texto default
#define THEME_FG_CURSOR     0xffb86c   // Cursor text input
#define THEME_FG_TIMESTAMP  0xCCCCCC   // Timestamps
#define THEME_FG_SEPARATOR  0x333344   // Separador sutil / borde contenedor
#define THEME_FG_SUBTITLE   0x627211   // Subtitulo contenedor tab
#define THEME_FG_WS_LABEL   0xffb86c   // Label "Workspace:"
#define THEME_FG_TAB_ACTIVE 0x282a36   // Texto tab seleccionado (contraste)
#define THEME_FG_TAB_INA    0x6272a4   // Texto tab inactivo
#define THEME_FG_BORDER_ACT 0xf1fa8c   // Borde ventana activa
#define THEME_FG_BORDER_INA 0x44475a   // Borde ventana inactiva
#define THEME_FG_TITLE_ACT  0xf1fa8c   // Fondo titulo activo (mismo que border)
#define THEME_FG_TITLE_INA  0xbd93f9   // Fondo titulo inactivo
#define THEME_FG_CLOSE_TEXT 0xf8f8f2   // Texto boton [X]
#define THEME_FG_GRIP       0x111116   // Fondo grip zone
#define THEME_FG_INPUT_ICON 0x50fa7b   // Icono [ Input ]
#define THEME_FG_INPUT_BORD 0x6272a4   // Borde text input []
#define THEME_FG_LOG_INFO   0x50fa7b   // Log nivel INFO (verde)
#define THEME_FG_LOG_WARN   0xffb86c   // Log nivel WARN (naranja)
#define THEME_FG_LOG_ERROR  0xff5555   // Log nivel ERROR (rojo)
#define THEME_FG_LOG_DEBUG  0x6272a4   // Log nivel DEBUG (gris azulado)
#define THEME_FG_TIMEBAR    0xf8f8f2   // Hora en taskbar

// --- Layout y dimensiones ---
#define TAB_BAR_HEIGHT          2   // Lineas: toolbar + separador (0=y=0, 1=separador)
#define TASKBAR_HEIGHT          1   // Linea inferior
#define TAB_BAR_START_X         30  // Donde empieza el primer tab en toolbar
#define WINDOW_MIN_HEIGHT       4   // Altura minima redimensioable
#define WINDOW_MIN_WIDTH        15  // Ancho minimo redimensionable
#define GRIP_AREA_MIN_X         2   // Offset from right para zona de grip
#define CLOSE_BUTTON_AREA_MIN_X 4   // Offset from right para zona [X]
#define TITLE_PADDING_LEFT      2   // Espacio antes del titulo
#define LOG_LINE_LEFT_PAD       2   // Padding izquierdo en líneas de log
#define LOG_FRAME_TOP_OFFSET    1   // Offset vertical debajo del frame
#define INPUT_FRAME_Y           2   // Linea del texto [ Input ]
#define INPUT_TEXT_Y            3   // Linea del text widget
#define INPUT_LEFT_PAD          1   // Padding izquierdo en text input
#define INPUT_WIDTH_REDUCTION   3   // (ancho ventana - 3) = ancho text input
#define MIN_TEXT_INPUT_WIDTH    4   // Ancho minimo para text input
#define TAB_PLANE_TOP_PAD       2   // Offset Y donde inicia el plano del tab
#define WS_PLANE_BOTTOM_PAD     1   // Linea reservada para taskbar
#define TAB_HEADER_TOP          0   // Posicion del texto del tab header
#define WINDOW_PLANE_DEF_X      0   // Posicion X por defecto de ventana
#define WINDOW_PLANE_DEF_Y      0   // Posicion Y por defecto de ventana
#define WS_HIDDEN_X             -10000  // Off-screen parking X (alias de OFFSCREEN_Y)
#define OFFSCREEN_Y            -10000  // Off-screen parking Y
// WINDOW_HIDDEN_X eliminado — usar OFFSCREEN_Y directamente
#define TEXT_INPUT_MAX_LEN      512   // Max chars en buffer de texto (de widget/text_input.h, redundante pero util aqui para layout)

// --- Zonas de interaccion en ventana ---
#define GRIP_AREA_START_X       3     // Offset from right para zona de grip resize
#define TAB_LABEL_PADDING       6     // Chars extra para tab label (espacios + decoracion)

// --- Colores del cursor y bordes ---
#define CURSOR_BLOCK_COLOR  0xffb86c  // Color bloque cursor (naranja)
#define CURSOR_BLOCK_CHAR   ' '       // Carácter del cursor bloque

// --- Colores de fondo por defecto de componentes ---
#define INPUT_FG_NORMAL     0xf8f8f2  // Texto normal text input
#define INPUT_BG_NORMAL     0x21222c  // Fondo normal text input
#define INPUT_BG_FOCUSED    0x383a4f  // Fondo enfocado text input

// --- Colores de borde y fondo por defecto ventanas ---
#define WIN_BORDER_FG_NORMAL 0xbd93f9  // Lila (titulo ventana)
#define WIN_BG_NORMAL        0x282a36  // Fondo ventana

// --- Colores START button ---
#define START_BTN_BG  0xbd93f9  // Fondo lila
#define START_BTN_FG  0x282a36  // Texto oscuro

#endif // CORE_THEME_H
