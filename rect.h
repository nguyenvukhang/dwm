#pragma once
#include "util.h"

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

    /// Get the area of intersection. Always returns a non-negative value.
    int intersect(const Rect &o) const {
        const int width = MIN(x + w, o.x + o.w) - MAX(x, o.x);
        const int height = MIN(y + h, o.y + o.h) - MAX(y, o.y);
        // At this point, both `width` and `height` are guaranteed to be
        // non-negative.
        return MAX(0, width) * MAX(0, height);
    }

    bool equals(const Rect *o) const {
        return x == o->x && y == o->y && w == o->w && h == o->h;
    }
};
