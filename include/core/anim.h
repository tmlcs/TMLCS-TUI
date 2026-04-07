#ifndef CORE_ANIM_H
#define CORE_ANIM_H

/**
 * @brief Easing function types for animations.
 */
typedef enum {
    EASE_LINEAR,    /**< No easing, linear progress */
    EASE_IN_OUT,    /**< Slow start and end, fast middle */
    EASE_OUT,       /**< Fast start, slow end */
    EASE_IN         /**< Slow start, fast end */
} TuiEasing;

/**
 * @brief Apply an easing function to a progress value.
 * @param progress Normalized progress value [0.0, 1.0].
 * @param easing The easing function to apply.
 * @return Eased progress value in [0.0, 1.0].
 */
float tui_ease(float progress, TuiEasing easing);

#endif /* CORE_ANIM_H */
