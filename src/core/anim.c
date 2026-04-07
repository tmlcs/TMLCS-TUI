#include "core/anim.h"

float tui_ease(float t, TuiEasing easing) {
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    switch (easing) {
        case EASE_LINEAR: return t;
        case EASE_IN_OUT: return t < 0.5f ? 2.0f*t*t : -1.0f+(4.0f-2.0f*t)*t;
        case EASE_OUT:    return 1.0f - (1.0f-t)*(1.0f-t);
        case EASE_IN:     return t * t;
    }
    return t;
}
