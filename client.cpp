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

void Client::detachstack() const {
    Client **tc, *t;

    for (tc = &this->mon->stack; *tc && *tc != this; tc = &(*tc)->snext);
    *tc = this->snext;

    if (this == this->mon->sel) {
        for (t = this->mon->stack; t && !ISVISIBLE(t); t = t->snext);
        this->mon->sel = t;
    }
}
Atom Client::getatomprop(Atom prop) const {
    int format;
    unsigned long nitems, dl;
    unsigned char *p = NULL;
    Atom da, atom = None;

    if (XGetWindowProperty(dpy, this->win, prop, 0L, sizeof atom, False,
                           XA_ATOM, &da, &format, &nitems, &dl,
                           &p) == Success &&
        p) {
        if (nitems > 0 && format == 32) {
            atom = *(long *)p;
        }
        XFree(p);
    }
    return atom;
}

void Client::grabbuttons(int focused) const {
    updatenumlockmask();
    {
        unsigned int i, j;
        unsigned int modifiers[] = {0, LockMask, numlockmask,
                                    numlockmask | LockMask};
        XUngrabButton(dpy, AnyButton, AnyModifier, this->win);
        if (!focused) {
            XGrabButton(dpy, AnyButton, AnyModifier, this->win, False,
                        BUTTONMASK, GrabModeSync, GrabModeSync, None, None);
        }
        for (i = 0; i < LENGTH(BUTTONS); i++) {
            if (BUTTONS[i].click == ClkClientWin) {
                for (j = 0; j < LENGTH(modifiers); j++) {
                    XGrabButton(dpy, BUTTONS[i].button,
                                BUTTONS[i].mask | modifiers[j], this->win,
                                False, BUTTONMASK, GrabModeAsync, GrabModeSync,
                                None, None);
                }
            }
        }
    }
}

Client *Client::nexttiled() {
    Client *c = this;
    for (; c && (c->isfloating || !ISVISIBLE(c)); c = c->next);
    return c;
}

void Client::pop() {
    this->detach();
    this->attach();
    focus(this);
    this->mon->arrange();
}

void Client::resize(int x, int y, int w, int h, int interact) {
    if (this->applysizehints(&x, &y, &w, &h, interact)) {
        this->resizeclient(x, y, w, h);
    }
}

void Client::resizeclient(int x, int y, int w, int h) {
    XWindowChanges wc;

    this->oldx = this->x;
    this->x = wc.x = x;
    this->oldy = this->y;
    this->y = wc.y = y;
    this->oldw = this->w;
    this->w = wc.width = w;
    this->oldh = this->h;
    this->h = wc.height = h;
    wc.border_width = this->bw;
    XConfigureWindow(dpy, this->win,
                     CWX | CWY | CWWidth | CWHeight | CWBorderWidth, &wc);
    this->configure();
    XSync(dpy, False);
}

int Client::sendevent(Atom proto) const {
    int n;
    Atom *protocols;
    int exists = 0;
    XEvent ev;

    if (XGetWMProtocols(dpy, this->win, &protocols, &n)) {
        while (!exists && n--) {
            exists = protocols[n] == proto;
        }
        XFree(protocols);
    }
    if (exists) {
        ev.type = ClientMessage;
        ev.xclient.window = this->win;
        ev.xclient.message_type = wmatom[WMProtocols];
        ev.xclient.format = 32;
        ev.xclient.data.l[0] = proto;
        ev.xclient.data.l[1] = CurrentTime;
        XSendEvent(dpy, this->win, False, NoEventMask, &ev);
    }
    return exists;
}

void Client::sendmon(Monitor *m) {
    if (this->mon == m) {
        return;
    }
    unfocus(this, 1);
    this->detach();
    this->detachstack();
    this->mon = m;
    this->tags = m->tagset[m->seltags]; /* assign tags of target monitor */
    this->attach();
    this->attachstack();
    if (this->isfullscreen) {
        this->resizeclient(m->mx, m->my, m->mw, m->mh);
    }
    focus(NULL);
    arrange();
}
