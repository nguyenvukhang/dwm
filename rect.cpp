#include "rect.h"

bool Rect::equals(const Rect *o) const {
    return x == o->x && y == o->y && w == o->w && h == o->h;
}

int Rect::intersect(const Rect &o) const {
    const int width = MIN(x + w, o.x + o.w) - MAX(x, o.x);
    const int height = MIN(y + h, o.y + o.h) - MAX(y, o.y);
    // At this point, both `width` and `height` are guaranteed to be
    // non-negative.
    return MAX(0, width) * MAX(0, height);
}

void Rect::take_rect_value(const Rect *other) {
    this->x = other->x;
    this->y = other->y;
    this->w = other->w;
    this->h = other->h;
}
