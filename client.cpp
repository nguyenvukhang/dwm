#include "config.h"
#include "monitor.h"

void Client::applyrules() {
    const char *cls, *instance;
    unsigned int i;
    const Rule *r;
    Monitor *m;
    XClassHint ch = {NULL, NULL};

    /* rule matching */
    isfloating = 0;
    tags = 0;
    XGetClassHint(dpy, win, &ch);
    cls = ch.res_class ? ch.res_class : broken;
    instance = ch.res_name ? ch.res_name : broken;

    for (i = 0; i < LENGTH(RULES); i++) {
        r = &RULES[i];
        if ((!r->title || strstr(name, r->title)) &&
            (!r->cls || strstr(cls, r->cls)) &&
            (!r->instance || strstr(instance, r->instance))) {
            isfloating = r->isfloating;
            tags |= r->tags;
            for (m = mons; m && m->num != r->monitor; m = m->next);
            if (m) {
                mon = m;
            }
        }
    }
    if (ch.res_class) {
        XFree(ch.res_class);
    }
    if (ch.res_name) {
        XFree(ch.res_name);
    }
    tags = tags & TAGMASK ? tags & TAGMASK : mon->tagset[mon->seltags];
}

int Client::applysizehints(int *x, int *y, int *w, int *h, int interact) {
    int baseismin;
    Monitor *m = this->mon;

    /* set minimum possible */
    *w = MAX(1, *w);
    *h = MAX(1, *h);
    if (interact) {
        if (*x > sw) {
            *x = sw - WIDTH(this);
        }
        if (*y > sh) {
            *y = sh - HEIGHT(this);
        }
        if (*x + *w + 2 * this->bw < 0) {
            *x = 0;
        }
        if (*y + *h + 2 * this->bw < 0) {
            *y = 0;
        }
    } else {
        if (*x >= m->wx + m->ww) {
            *x = m->wx + m->ww - WIDTH(this);
        }
        if (*y >= m->wy + m->wh) {
            *y = m->wy + m->wh - HEIGHT(this);
        }
        if (*x + *w + 2 * this->bw <= m->wx) {
            *x = m->wx;
        }
        if (*y + *h + 2 * this->bw <= m->wy) {
            *y = m->wy;
        }
    }
    if (*h < bh) {
        *h = bh;
    }
    if (*w < bh) {
        *w = bh;
    }
    if (resizehints || this->isfloating ||
        !this->mon->lt[this->mon->sellt]->arrange) {
        if (!this->hintsvalid) {
            updatesizehints(this);
        }
        /* see last two sentences in ICCCM 4.1.2.3 */
        baseismin = this->basew == this->minw && this->baseh == this->minh;
        if (!baseismin) { /* temporarily remove base dimensions */
            *w -= this->basew;
            *h -= this->baseh;
        }
        /* adjust for aspect limits */
        if (this->mina > 0 && this->maxa > 0) {
            if (this->maxa < (float)*w / *h) {
                *w = *h * this->maxa + 0.5;
            } else if (this->mina < (float)*h / *w) {
                *h = *w * this->mina + 0.5;
            }
        }
        if (baseismin) { /* increment calculation requires this */
            *w -= this->basew;
            *h -= this->baseh;
        }
        /* adjust for increment value */
        if (this->incw) {
            *w -= *w % this->incw;
        }
        if (this->inch) {
            *h -= *h % this->inch;
        }
        /* restore base dimensions */
        *w = MAX(*w + this->basew, this->minw);
        *h = MAX(*h + this->baseh, this->minh);
        if (this->maxw) {
            *w = MIN(*w, this->maxw);
        }
        if (this->maxh) {
            *h = MIN(*h, this->maxh);
        }
    }
    return *x != this->x || *y != this->y || *w != this->w || *h != this->h;
}

void Client::attach() {
    this->next = this->mon->clients;
    this->mon->clients = this;
}

void Client::attachstack() {
    this->snext = this->mon->stack;
    this->mon->stack = this;
}

void Client::configure() const {
    XConfigureEvent ce{.type = ConfigureNotify,
                       .display = dpy,
                       .event = this->win,
                       .window = this->win,
                       .x = this->x,
                       .y = this->y,
                       .width = this->w,
                       .height = this->h,
                       .border_width = this->bw,
                       .above = None,
                       .override_redirect = False};
    XSendEvent(dpy, this->win, False, StructureNotifyMask, (XEvent *)&ce);
}

void Client::detach() const {
    Client **tc;

    for (tc = &this->mon->clients; *tc && *tc != this; tc = &(*tc)->next);
    *tc = this->next;
}
