#ifndef WIDGET_SLIDER_H
#define WIDGET_SLIDER_H

#include <notcurses/notcurses.h>
#include <stdbool.h>

typedef void (*TuiSliderCb)(float value, void* userdata);

/**
 * @brief A range-based value selection slider.
 */
typedef struct TuiSlider {
    struct ncplane* plane;
    float min;
    float max;
    float value;
    float step;
    int width;
    bool focused;
    TuiSliderCb cb;
    void* userdata;
    unsigned fg_track;
    unsigned fg_fill;
    unsigned fg_thumb;
    unsigned bg_normal;
    unsigned bg_focused;
    int _type_id;
} TuiSlider;

/**
 * @brief Create a slider widget.
 * @param parent Parent ncplane.
 * @param y Y position.
 * @param x X position.
 * @param width Width in columns (>= 4).
 * @param min Minimum value.
 * @param max Maximum value (must be > min).
 * @param step Step size for keyboard navigation.
 * @param cb Callback on value change. May be NULL.
 * @param userdata Passed to callback.
 * @return New slider, or NULL on failure.
 */
TuiSlider* tui_slider_create(struct ncplane* parent, int y, int x, int width,
                               float min, float max, float step,
                               TuiSliderCb cb, void* userdata);

/**
 * @brief Destroy the slider.
 * @param slider Slider, or NULL.
 */
void tui_slider_destroy(TuiSlider* slider);

/**
 * @brief Get the current value.
 * @param slider Slider.
 * @return Current value.
 */
float tui_slider_get_value(const TuiSlider* slider);

/**
 * @brief Set the current value (clamped to range).
 * @param slider Slider.
 * @param value New value.
 */
void tui_slider_set_value(TuiSlider* slider, float value);

/**
 * @brief Handle a key event.
 * @param slider Slider.
 * @param key Notcurses key code.
 * @param ni Input details.
 * @return true if consumed.
 */
bool tui_slider_handle_key(TuiSlider* slider, uint32_t key, const struct ncinput* ni);

/**
 * @brief Handle a mouse event.
 * @param slider Slider.
 * @param key Notcurses key code.
 * @param ni Input details.
 * @return true if consumed.
 */
bool tui_slider_handle_mouse(TuiSlider* slider, uint32_t key, const struct ncinput* ni);

/**
 * @brief Render the slider.
 * @param slider Slider, or NULL.
 */
void tui_slider_render(TuiSlider* slider);

/**
 * @brief Set focus state.
 * @param slider Slider.
 * @param focused Focus state.
 */
void tui_slider_set_focused(TuiSlider* slider, bool focused);

#endif /* WIDGET_SLIDER_H */
