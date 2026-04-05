#ifndef WIDGET_TEXT_INPUT_H
#define WIDGET_TEXT_INPUT_H

#include <notcurses/notcurses.h>
#include <stdbool.h>

#define TEXT_INPUT_MAX_LEN 512

// Callback invocado al pulsar Enter con el contenido actual del buffer
typedef void (*TextInputSubmitCb)(const char* text, void* userdata);

typedef struct TuiTextInput {
    struct ncplane*    plane;       // Plano propio del widget
    char               buffer[TEXT_INPUT_MAX_LEN];
    int                cursor;      // Posición del cursor dentro del buffer
    int                len;         // Longitud actual del texto
    int                scroll_off;  // Desplazamiento horizontal para textos largos
    bool               focused;     // Recibe inputs de teclado si es true

    TextInputSubmitCb  on_submit;
    void*              userdata;

    // Colores
    unsigned int bg_normal;         // p.ej. 0x282a36
    unsigned int fg_normal;         // p.ej. 0xf8f8f2
    unsigned int bg_focused;        // p.ej. 0x44475a
    unsigned int fg_cursor;         // p.ej. 0xbd93f9
} TuiTextInput;

// Ciclo de vida
TuiTextInput* tui_text_input_create(struct ncplane* parent,
                                     int y, int x, int width,
                                     TextInputSubmitCb on_submit,
                                     void* userdata);
void tui_text_input_destroy(TuiTextInput* ti);

// Interacción
bool tui_text_input_handle_key(TuiTextInput* ti, uint32_t key, const struct ncinput* ni);
void tui_text_input_render(TuiTextInput* ti);
void tui_text_input_clear(TuiTextInput* ti);
const char* tui_text_input_get(TuiTextInput* ti);

#endif // WIDGET_TEXT_INPUT_H
