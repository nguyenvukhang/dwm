#pragma once

#include "util.h"

#include <X11/Xlib.h>

struct Size {
    int w, h;

    bool operator==(const Size &rhs) const {
        return this->w == rhs.w && this->h == rhs.h;
    }
};

struct Rect {
    int x, y;
    // Width. Guaranteed to be non-negative.
    int w;
    // Height. Guaranteed to be non-negative.
    int h;

    bool equals(const Rect *o) const;
    /// Get the area of intersection. Always returns a non-negative value.
    int intersect(const Rect &o) const;
    void take_rect_value(const Rect *other);
};
