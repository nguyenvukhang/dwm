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

bool Client::applysizehints(Rect &rect, int interact) {
    int baseismin;
    Monitor *m = this->mon;

    /* set minimum possible */
    rect.w = MAX(1, rect.w);
    rect.h = MAX(1, rect.h);
    if (interact) {
        if (rect.x > sw) {
            rect.x = sw - this->effective_width();
        }
        if (rect.y > sh) {
            rect.y = sh - this->effective_height();
        }
        if (rect.x + rect.w + 2 * this->bw < 0) {
            rect.x = 0;
        }
        if (rect.y + rect.h + 2 * this->bw < 0) {
            rect.y = 0;
        }
    } else {
        if (rect.x >= m->w.x + m->w.w) {
            rect.x = m->w.x + m->w.w - this->effective_width();
        }
        if (rect.y >= m->w.y + m->w.h) {
            rect.y = m->w.y + m->w.h - this->effective_height();
        }
        if (rect.x + rect.w + 2 * this->bw <= m->w.x) {
            rect.x = m->w.x;
        }
        if (rect.y + rect.h + 2 * this->bw <= m->w.y) {
            rect.y = m->w.y;
        }
    }
    if (rect.h < bh) {
        rect.h = bh;
    }
    if (rect.w < bh) {
        rect.w = bh;
    }
    if (resizehints || this->isfloating ||
        !this->mon->lt[this->mon->sellt]->arrange) {
        if (!this->hintsvalid) {
            this->updatesizehints();
        }
        /* see last two sentences in ICCCM 4.1.2.3 */
        baseismin = this->base == this->min;
        if (!baseismin) { /* temporarily remove base dimensions */
            rect.w -= this->base.w;
            rect.h -= this->base.h;
        }
        /* adjust for aspect limits */
        if (this->mina > 0 && this->maxa > 0) {
            if (this->maxa < (float)rect.w / rect.h) {
                rect.w = rect.h * this->maxa + 0.5;
            } else if (this->mina < (float)rect.h / rect.w) {
                rect.h = rect.w * this->mina + 0.5;
            }
        }
        if (baseismin) { /* increment calculation requires this */
            rect.w -= this->base.w;
            rect.h -= this->base.h;
        }
        /* adjust for increment value */
        if (this->inc.w) {
            rect.w -= rect.w % this->inc.w;
        }
        if (this->inc.h) {
            rect.h -= rect.h % this->inc.h;
        }
        /* restore base dimensions */
        rect.w = MAX(rect.w + this->base.w, this->min.w);
        rect.h = MAX(rect.h + this->base.h, this->min.h);
        if (this->max.w) {
            rect.w = MIN(rect.w, this->max.w);
        }
        if (this->max.h) {
            rect.h = MIN(rect.h, this->max.h);
        }
    }
    return !rect.equals(this);
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

void Client::resize(Rect rect, bool interact) {
    if (this->applysizehints(rect, interact)) {
        this->resizeclient(&rect);
    }
}

void Client::resizeclient(const Rect *r) {
    this->old.take_rect_value(this);
    this->take_rect_value(r);
    XWindowChanges wc{
        .x = r->x,
        .y = r->y,
        .width = r->w,
        .height = r->h,
        .border_width = this->bw,
    };
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
    this->unfocus(1);
    this->detach();
    this->detachstack();
    this->mon = m;
    this->tags = m->tagset[m->seltags]; /* assign tags of target monitor */
    this->attach();
    this->attachstack();
    if (this->isfullscreen) {
        this->resizeclient(&m->m);
    }
    focus(NULL);
    arrange();
}

void Client::setclientstate(long state) const {
    long data[] = {state, None};

    XChangeProperty(dpy, this->win, wmatom[WMState], wmatom[WMState], 32,
                    PropModeReplace, (unsigned char *)data, 2);
}

void Client::setfocus() const {
    if (!this->neverfocus) {
        XSetInputFocus(dpy, this->win, RevertToPointerRoot, CurrentTime);
    }
    XChangeProperty(dpy, root, netatom[NetActiveWindow], XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *)&this->win, 1);
    this->sendevent(wmatom[WMTakeFocus]);
}

void Client::setfullscreen(int fullscreen) {
    if (fullscreen && !this->isfullscreen) {
        XChangeProperty(dpy, this->win, netatom[NetWMState], XA_ATOM, 32,
                        PropModeReplace,
                        (unsigned char *)&netatom[NetWMFullscreen], 1);
        this->isfullscreen = 1;
        this->oldstate = this->isfloating;
        this->oldbw = this->bw;
        this->bw = 0;
        this->isfloating = 1;
        this->resizeclient(&this->mon->m);
        XRaiseWindow(dpy, this->win);
    } else if (!fullscreen && this->isfullscreen) {
        XChangeProperty(dpy, this->win, netatom[NetWMState], XA_ATOM, 32,
                        PropModeReplace, (unsigned char *)0, 0);
        this->isfullscreen = 0;
        this->isfloating = this->oldstate;
        this->bw = this->oldbw;
        this->take_rect_value(&this->old);
        this->resizeclient(this);
        this->mon->arrange();
    }
}

void Client::seturgent(int urg) {
    XWMHints *wmh;

    this->isurgent = urg;
    if (!(wmh = XGetWMHints(dpy, this->win))) {
        return;
    }
    wmh->flags =
        urg ? (wmh->flags | XUrgencyHint) : (wmh->flags & ~XUrgencyHint);
    XSetWMHints(dpy, this->win, wmh);
    XFree(wmh);
}

void Client::showhide() {
    if (ISVISIBLE(this)) {
        /* show clients top down */
        XMoveWindow(dpy, this->win, this->x, this->y);
        if ((!this->mon->lt[this->mon->sellt]->arrange || this->isfloating) &&
            !this->isfullscreen) {
            this->resize(*this, false);
        }
        if (this->snext) {
            this->snext->showhide();
        }
    } else {
        /* hide clients bottom up */
        if (this->snext) {
            this->snext->showhide();
        }
        XMoveWindow(dpy, this->win, this->effective_width() * -2, this->y);
    }
}

void Client::unfocus(int setfocus) const {
    this->grabbuttons(0);
    XSetWindowBorder(dpy, this->win, scheme[SchemeNorm][ColBorder].pixel);
    if (setfocus) {
        XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
        XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
    }
}

void Client::unmanage(int destroyed) {
    Monitor *m = this->mon;
    XWindowChanges wc;

    this->detach();
    this->detachstack();
    if (!destroyed) {
        wc.border_width = this->oldbw;
        XGrabServer(dpy); /* avoid race conditions */
        XSetErrorHandler(xerrordummy);
        XSelectInput(dpy, this->win, NoEventMask);
        XConfigureWindow(dpy, this->win, CWBorderWidth,
                         &wc); /* restore border */
        XUngrabButton(dpy, AnyButton, AnyModifier, this->win);
        this->setclientstate(WithdrawnState);
        XSync(dpy, False);
        XSetErrorHandler(xerror);
        XUngrabServer(dpy);
    }
    free(this);
    focus(NULL);
    updateclientlist();
    m->arrange();
}

void Client::updatesizehints() {
    long msize;
    XSizeHints size;

    if (!XGetWMNormalHints(dpy, this->win, &size, &msize)) {
        /* size is uninitialized, ensure that size.flags aren't used */
        size.flags = PSize;
    }
    if (size.flags & PBaseSize) {
        this->base.w = size.base_width;
        this->base.h = size.base_height;
    } else if (size.flags & PMinSize) {
        this->base.w = size.min_width;
        this->base.h = size.min_height;
    } else {
        this->base.w = this->base.h = 0;
    }
    if (size.flags & PResizeInc) {
        this->inc.w = size.width_inc;
        this->inc.h = size.height_inc;
    } else {
        this->inc.w = this->inc.h = 0;
    }
    if (size.flags & PMaxSize) {
        this->max.w = size.max_width;
        this->max.h = size.max_height;
    } else {
        this->max.w = this->max.h = 0;
    }
    if (size.flags & PMinSize) {
        this->min.w = size.min_width;
        this->min.h = size.min_height;
    } else if (size.flags & PBaseSize) {
        this->min.w = size.base_width;
        this->min.h = size.base_height;
    } else {
        this->min.w = this->min.h = 0;
    }
    if (size.flags & PAspect) {
        this->mina = (float)size.min_aspect.y / size.min_aspect.x;
        this->maxa = (float)size.max_aspect.x / size.max_aspect.y;
    } else {
        this->maxa = this->mina = 0.0;
    }
    this->isfixed = (this->max.w && this->max.h && this->max.w == this->min.w &&
                     this->max.h == this->min.h);
    this->hintsvalid = 1;
}

void Client::updatetitle() {
    if (!gettextprop(this->win, netatom[NetWMName], this->name,
                     sizeof this->name)) {
        gettextprop(this->win, XA_WM_NAME, this->name, sizeof this->name);
    }
    if (this->name[0] == '\0') { /* hack to mark broken clients */
        strcpy(this->name, broken);
    }
}

void Client::updatewindowtype() {
    Atom state = this->getatomprop(netatom[NetWMState]);
    Atom wtype = this->getatomprop(netatom[NetWMWindowType]);

    if (state == netatom[NetWMFullscreen]) {
        this->setfullscreen(1);
    }
    if (wtype == netatom[NetWMWindowTypeDialog]) {
        this->isfloating = 1;
    }
}

void Client::updatewmhints() {
    XWMHints *wmh;

    if ((wmh = XGetWMHints(dpy, this->win))) {
        if (this == selmon->sel && wmh->flags & XUrgencyHint) {
            wmh->flags &= ~XUrgencyHint;
            XSetWMHints(dpy, this->win, wmh);
        } else {
            this->isurgent = (wmh->flags & XUrgencyHint) ? 1 : 0;
        }
        if (wmh->flags & InputHint) {
            this->neverfocus = !wmh->input;
        } else {
            this->neverfocus = 0;
        }
        XFree(wmh);
    }
}

int Client::effective_width() const { return this->w + 2 * this->bw; }

int Client::effective_height() const { return this->h + 2 * this->bw; }
