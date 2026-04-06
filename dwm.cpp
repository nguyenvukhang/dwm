/* configuration, allows nested code to access above variables */
#include "config.h"

#include "client.cpp"
#include "monitor.cpp"

/* compile-time check if all tags fit into an unsigned int bit array. */
struct NumTags {
    char limitexceeded[LENGTH(TAGS) > 31 ? -1 : 1];
};

void arrange() {
    Monitor *m;
    for (m = mons; m; m = m->next) {
        if (m->stack) {
            m->stack->showhide();
        }
    }
    for (m = mons; m; m = m->next) {
        m->arrangemon();
    }
}

void buttonpress(XEvent *e) {
    unsigned int i, x, click;
    Arg arg = {0};
    Client *c;
    Monitor *m;
    XButtonPressedEvent *ev = &e->xbutton;

    click = ClkRootWin;
    /* focus monitor if necessary */
    if ((m = wintomon(ev->window)) && m != selmon) {
        if (selmon->sel) {
            selmon->sel->unfocus(1);
        }
        selmon = m;
        focus(NULL);
    }
    if (ev->window == selmon->barwin) {
        i = x = 0;
        do {
            x += TEXTW(TAGS[i]);
        } while (ev->x >= x && ++i < LENGTH(TAGS));
        if (i < LENGTH(TAGS)) {
            click = ClkTagBar;
            arg.ui = 1 << i;
        } else if (ev->x < x + TEXTW(selmon->ltsymbol)) {
            click = ClkLtSymbol;
        } else if (ev->x > selmon->ww - (int)TEXTW(stext) + lrpad - 2) {
            click = ClkStatusText;
        } else {
            click = ClkWinTitle;
        }
    } else if ((c = wintoclient(ev->window))) {
        focus(c);
        selmon->restack();
        XAllowEvents(dpy, ReplayPointer, CurrentTime);
        click = ClkClientWin;
    }
    for (i = 0; i < LENGTH(BUTTONS); i++) {
        if (click == BUTTONS[i].click && BUTTONS[i].func &&
            BUTTONS[i].button == ev->button &&
            CLEANMASK(BUTTONS[i].mask) == CLEANMASK(ev->state)) {
            BUTTONS[i].func(click == ClkTagBar && BUTTONS[i].arg.i == 0
                                ? &arg
                                : &BUTTONS[i].arg);
        }
    }
}

void checkotherwm(void) {
    xerrorxlib = XSetErrorHandler(xerrorstart);
    /* this causes an error if some other window manager is running */
    XSelectInput(dpy, DefaultRootWindow(dpy), SubstructureRedirectMask);
    XSync(dpy, False);
    XSetErrorHandler(xerror);
    XSync(dpy, False);
}

void cleanup(void) {
    Arg a = {.ui = ~0u};
    Layout foo = {"", NULL};
    Monitor *m;
    size_t i;

    view(&a);
    selmon->lt[selmon->sellt] = &foo;
    for (m = mons; m; m = m->next) {
        while (m->stack) {
            m->stack->unmanage(0);
        }
    }
    XUngrabKey(dpy, AnyKey, AnyModifier, root);
    while (mons) {
        cleanupmon(mons);
    }
    for (i = 0; i < CurLast; i++) {
        drw_cur_free(drw, cursor[i]);
    }
    for (i = 0; i < LENGTH(COLORS); i++) {
        drw_scm_free(drw, scheme[i], 3);
    }
    free(scheme);
    XDestroyWindow(dpy, wmcheckwin);
    drw_free(drw);
    XSync(dpy, False);
    XSetInputFocus(dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
    XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
}

void cleanupmon(Monitor *mon) {
    Monitor *m;

    if (mon == mons) {
        mons = mons->next;
    } else {
        for (m = mons; m && m->next != mon; m = m->next);
        m->next = mon->next;
    }
    XUnmapWindow(dpy, mon->barwin);
    XDestroyWindow(dpy, mon->barwin);
    free(mon);
}

void clientmessage(XEvent *e) {
    XClientMessageEvent *cme = &e->xclient;
    Client *c = wintoclient(cme->window);

    if (!c) {
        return;
    }
    if (cme->message_type == netatom[NetWMState]) {
        if (cme->data.l[1] == netatom[NetWMFullscreen] ||
            cme->data.l[2] == netatom[NetWMFullscreen]) {
            c->setfullscreen((cme->data.l[0] == 1     /* _NET_WM_STATE_ADD    */
                              || (cme->data.l[0] == 2 /* _NET_WM_STATE_TOGGLE */
                                  && !c->isfullscreen)));
        }
    } else if (cme->message_type == netatom[NetActiveWindow]) {
        if (c != selmon->sel && !c->isurgent) {
            c->seturgent(1);
        }
    }
}

void configurenotify(XEvent *e) {
    Monitor *m;
    Client *c;
    XConfigureEvent *ev = &e->xconfigure;
    int dirty;

    /* TODO: updategeom handling sucks, needs to be simplified */
    if (ev->window == root) {
        dirty = (sw != ev->width || sh != ev->height);
        sw = ev->width;
        sh = ev->height;
        if (updategeom() || dirty) {
            drw_resize(drw, sw, bh);
            updatebars();
            for (m = mons; m; m = m->next) {
                for (c = m->clients; c; c = c->next) {
                    if (c->isfullscreen) {
                        c->resizeclient(m->mx, m->my, m->mw, m->mh);
                    }
                }
                XMoveResizeWindow(dpy, m->barwin, m->wx, m->by, m->ww, bh);
            }
            focus(NULL);
            arrange();
        }
    }
}

void configurerequest(XEvent *e) {
    Client *c;
    Monitor *m;
    XConfigureRequestEvent *ev = &e->xconfigurerequest;
    XWindowChanges wc;

    if ((c = wintoclient(ev->window))) {
        if (ev->value_mask & CWBorderWidth) {
            c->bw = ev->border_width;
        } else if (c->isfloating || !selmon->lt[selmon->sellt]->arrange) {
            m = c->mon;
            if (ev->value_mask & CWX) {
                c->oldx = c->x;
                c->x = m->mx + ev->x;
            }
            if (ev->value_mask & CWY) {
                c->oldy = c->y;
                c->y = m->my + ev->y;
            }
            if (ev->value_mask & CWWidth) {
                c->oldw = c->w;
                c->w = ev->width;
            }
            if (ev->value_mask & CWHeight) {
                c->oldh = c->h;
                c->h = ev->height;
            }
            if ((c->x + c->w) > m->mx + m->mw && c->isfloating) {
                c->x = m->mx +
                       (m->mw / 2 - WIDTH(c) / 2); /* center in x direction */
            }
            if ((c->y + c->h) > m->my + m->mh && c->isfloating) {
                c->y = m->my +
                       (m->mh / 2 - HEIGHT(c) / 2); /* center in y direction */
            }
            if ((ev->value_mask & (CWX | CWY)) &&
                !(ev->value_mask & (CWWidth | CWHeight))) {
                c->configure();
            }
            if (ISVISIBLE(c)) {
                XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
            }
        } else {
            c->configure();
        }
    } else {
        wc.x = ev->x;
        wc.y = ev->y;
        wc.width = ev->width;
        wc.height = ev->height;
        wc.border_width = ev->border_width;
        wc.sibling = ev->above;
        wc.stack_mode = ev->detail;
        XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
    }
    XSync(dpy, False);
}

Monitor *createmon(void) {
    Monitor *m;

    m = (Monitor *)ecalloc(1, sizeof(Monitor));
    m->tagset[0] = m->tagset[1] = 1;
    m->mfact = mfact;
    m->nmaster = nmaster;
    m->showbar = showbar;
    m->topbar = topbar;
    m->lt[0] = &LAYOUTS[0];
    m->lt[1] = &LAYOUTS[1 % LENGTH(LAYOUTS)];
    strncpy(m->ltsymbol, LAYOUTS[0].symbol, sizeof m->ltsymbol);
    return m;
}

void destroynotify(XEvent *e) {
    Client *c;
    XDestroyWindowEvent *ev = &e->xdestroywindow;

    if ((c = wintoclient(ev->window))) {
        c->unmanage(1);
    }
}

Monitor *dirtomon(int dir) {
    Monitor *m = NULL;

    if (dir > 0) {
        if (!(m = selmon->next)) {
            m = mons;
        }
    } else if (selmon == mons) {
        for (m = mons; m->next; m = m->next);
    } else {
        for (m = mons; m->next != selmon; m = m->next);
    }
    return m;
}

void drawbars(void) {
    Monitor *m;

    for (m = mons; m; m = m->next) {
        m->drawbar();
    }
}

void enternotify(XEvent *e) {
    Client *c;
    Monitor *m;
    XCrossingEvent *ev = &e->xcrossing;

    if ((ev->mode != NotifyNormal || ev->detail == NotifyInferior) &&
        ev->window != root) {
        return;
    }
    c = wintoclient(ev->window);
    m = c ? c->mon : wintomon(ev->window);
    if (m != selmon) {
        if (selmon->sel) {
            selmon->sel->unfocus(1);
        }
        selmon = m;
    } else if (!c || c == selmon->sel) {
        return;
    }
    focus(c);
}

void expose(XEvent *e) {
    Monitor *m;
    XExposeEvent *ev = &e->xexpose;

    if (ev->count == 0 && (m = wintomon(ev->window))) {
        m->drawbar();
    }
}

void focus(Client *c) {
    if (!c || !ISVISIBLE(c)) {
        for (c = selmon->stack; c && !ISVISIBLE(c); c = c->snext);
    }
    if (selmon->sel && selmon->sel != c) {
        if (selmon->sel) {
            selmon->sel->unfocus(0);
        }
    }
    if (c) {
        if (c->mon != selmon) {
            selmon = c->mon;
        }
        if (c->isurgent) {
            c->seturgent(0);
        }
        c->detachstack();
        c->attachstack();
        c->grabbuttons(1);
        XSetWindowBorder(dpy, c->win, scheme[SchemeSel][ColBorder].pixel);
        c->setfocus();
    } else {
        XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
        XDeleteProperty(dpy, root, netatom[NetActiveWindow]);
    }
    selmon->sel = c;
    drawbars();
}

/* there are some broken focus acquiring clients needing extra handling */
void focusin(XEvent *e) {
    XFocusChangeEvent *ev = &e->xfocus;

    if (selmon->sel && ev->window != selmon->sel->win) {
        selmon->sel->setfocus();
    }
}

void focusmon(const Arg *arg) {
    Monitor *m;

    if (!mons->next) {
        return;
    }
    if ((m = dirtomon(arg->i)) == selmon) {
        return;
    }
    if (selmon->sel) {
        selmon->sel->unfocus(0);
    }
    selmon = m;
    focus(NULL);
}

void focusstack(const Arg *arg) {
    Client *c = NULL, *i;

    if (!selmon->sel || (selmon->sel->isfullscreen && lockfullscreen)) {
        return;
    }
    if (arg->i > 0) {
        for (c = selmon->sel->next; c && !ISVISIBLE(c); c = c->next);
        if (!c) {
            for (c = selmon->clients; c && !ISVISIBLE(c); c = c->next);
        }
    } else {
        for (i = selmon->clients; i != selmon->sel; i = i->next) {
            if (ISVISIBLE(i)) {
                c = i;
            }
        }
        if (!c) {
            for (; i; i = i->next) {
                if (ISVISIBLE(i)) {
                    c = i;
                }
            }
        }
    }
    if (c) {
        focus(c);
        selmon->restack();
    }
}

int getrootptr(int *x, int *y) {
    int di;
    unsigned int dui;
    Window dummy;

    return XQueryPointer(dpy, root, &dummy, &dummy, x, y, &di, &di, &dui);
}

long getstate(Window w) {
    int format;
    long result = -1;
    unsigned char *p = NULL;
    unsigned long n, extra;
    Atom real;

    if (XGetWindowProperty(dpy, w, wmatom[WMState], 0L, 2L, False,
                           wmatom[WMState], &real, &format, &n, &extra,
                           &p) != Success) {
        return -1;
    }
    if (n != 0 && format == 32) {
        result = *(long *)p;
    }
    XFree(p);
    return result;
}

int gettextprop(Window w, Atom atom, char *text, unsigned int size) {
    char **list = NULL;
    int n;
    XTextProperty name;

    if (!text || size == 0) {
        return 0;
    }
    text[0] = '\0';
    if (!XGetTextProperty(dpy, w, &name, atom) || !name.nitems) {
        return 0;
    }
    if (name.encoding == XA_STRING) {
        strncpy(text, (char *)name.value, size - 1);
    } else if (XmbTextPropertyToTextList(dpy, &name, &list, &n) >= Success &&
               n > 0 && *list) {
        strncpy(text, *list, size - 1);
        XFreeStringList(list);
    }
    text[size - 1] = '\0';
    XFree(name.value);
    return 1;
}

void grabkeys(void) {
    updatenumlockmask();
    {
        unsigned int i, j, k;
        unsigned int modifiers[] = {0, LockMask, numlockmask,
                                    numlockmask | LockMask};
        int start, end, skip;
        KeySym *syms;

        XUngrabKey(dpy, AnyKey, AnyModifier, root);
        XDisplayKeycodes(dpy, &start, &end);
        syms = XGetKeyboardMapping(dpy, start, end - start + 1, &skip);
        if (!syms) {
            return;
        }
        for (k = start; k <= end; k++) {
            for (i = 0; i < LENGTH(KEYS); i++) {
                /* skip modifier codes, we do that ourselves */
                if (KEYS[i].keysym == syms[(k - start) * skip]) {
                    for (j = 0; j < LENGTH(modifiers); j++) {
                        XGrabKey(dpy, k, KEYS[i].mod | modifiers[j], root, True,
                                 GrabModeAsync, GrabModeAsync);
                    }
                }
            }
        }
        XFree(syms);
    }
}

void incnmaster(const Arg *arg) {
    selmon->nmaster = MAX(selmon->nmaster + arg->i, 0);
    selmon->arrange();
}

#ifdef XINERAMA
static int isuniquegeom(XineramaScreenInfo *unique, size_t n,
                        XineramaScreenInfo *info) {
    while (n--) {
        if (unique[n].x_org == info->x_org && unique[n].y_org == info->y_org &&
            unique[n].width == info->width &&
            unique[n].height == info->height) {
            return 0;
        }
    }
    return 1;
}
#endif /* XINERAMA */

void keypress(XEvent *e) {
    unsigned int i;
    KeySym keysym;
    XKeyEvent *ev;

    ev = &e->xkey;
    keysym = XkbKeycodeToKeysym(dpy, (KeyCode)ev->keycode, 0, 0);
    for (i = 0; i < LENGTH(KEYS); i++) {
        if (keysym == KEYS[i].keysym &&
            CLEANMASK(KEYS[i].mod) == CLEANMASK(ev->state) && KEYS[i].func) {
            KEYS[i].func(&(KEYS[i].arg));
        }
    }
}

void killclient(const Arg *arg) {
    if (!selmon->sel) {
        return;
    }
    if (!selmon->sel->sendevent(wmatom[WMDelete])) {
        XGrabServer(dpy);
        XSetErrorHandler(xerrordummy);
        XSetCloseDownMode(dpy, DestroyAll);
        XKillClient(dpy, selmon->sel->win);
        XSync(dpy, False);
        XSetErrorHandler(xerror);
        XUngrabServer(dpy);
    }
}

void manage(Window w, XWindowAttributes *wa) {
    Client *c, *t = NULL;
    Window trans = None;
    XWindowChanges wc;

    c = (Client *)ecalloc(1, sizeof(Client));
    c->win = w;
    /* geometry */
    c->x = c->oldx = wa->x;
    c->y = c->oldy = wa->y;
    c->w = c->oldw = wa->width;
    c->h = c->oldh = wa->height;
    c->oldbw = wa->border_width;

    updatetitle(c);
    if (XGetTransientForHint(dpy, w, &trans) && (t = wintoclient(trans))) {
        c->mon = t->mon;
        c->tags = t->tags;
    } else {
        c->mon = selmon;
        c->applyrules();
    }

    if (c->x + WIDTH(c) > c->mon->wx + c->mon->ww) {
        c->x = c->mon->wx + c->mon->ww - WIDTH(c);
    }
    if (c->y + HEIGHT(c) > c->mon->wy + c->mon->wh) {
        c->y = c->mon->wy + c->mon->wh - HEIGHT(c);
    }
    c->x = MAX(c->x, c->mon->wx);
    c->y = MAX(c->y, c->mon->wy);
    c->bw = borderpx;

    wc.border_width = c->bw;
    XConfigureWindow(dpy, w, CWBorderWidth, &wc);
    XSetWindowBorder(dpy, w, scheme[SchemeNorm][ColBorder].pixel);
    c->configure(); /* propagates border_width, if size doesn't change */
    updatewindowtype(c);
    updatesizehints(c);
    updatewmhints(c);
    XSelectInput(dpy, w,
                 EnterWindowMask | FocusChangeMask | PropertyChangeMask |
                     StructureNotifyMask);
    c->grabbuttons(0);
    if (!c->isfloating) {
        c->isfloating = c->oldstate = trans != None || c->isfixed;
    }
    if (c->isfloating) {
        XRaiseWindow(dpy, c->win);
    }
    c->attach();
    c->attachstack();
    XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32,
                    PropModeAppend, (unsigned char *)&(c->win), 1);
    XMoveResizeWindow(dpy, c->win, c->x + 2 * sw, c->y, c->w,
                      c->h); /* some windows require this */
    c->setclientstate(NormalState);
    if (c->mon == selmon) {
        if (selmon->sel) {
            selmon->sel->unfocus(0);
        }
    }
    c->mon->sel = c;
    c->mon->arrange();
    XMapWindow(dpy, c->win);
    focus(NULL);
}

void mappingnotify(XEvent *e) {
    XMappingEvent *ev = &e->xmapping;

    XRefreshKeyboardMapping(ev);
    if (ev->request == MappingKeyboard) {
        grabkeys();
    }
}

void maprequest(XEvent *e) {
    static XWindowAttributes wa;
    XMapRequestEvent *ev = &e->xmaprequest;

    if (!XGetWindowAttributes(dpy, ev->window, &wa) || wa.override_redirect) {
        return;
    }
    if (!wintoclient(ev->window)) {
        manage(ev->window, &wa);
    }
}

void monocle(Monitor *m) {
    unsigned int n = 0;
    Client *c;

    for (c = m->clients; c; c = c->next) {
        if (ISVISIBLE(c)) {
            n++;
        }
    }
    if (n > 0) { /* override layout symbol */
        snprintf(m->ltsymbol, sizeof m->ltsymbol, "[%d]", n);
    }
    for (c = m->clients->nexttiled(); c; c = c->next->nexttiled()) {
        c->resize(m->wx, m->wy, m->ww - 2 * c->bw, m->wh - 2 * c->bw, 0);
    }
}

void motionnotify(XEvent *e) {
    static Monitor *mon = NULL;
    Monitor *m;
    XMotionEvent *ev = &e->xmotion;

    if (ev->window != root) {
        return;
    }
    if ((m = recttomon(ev->x_root, ev->y_root, 1, 1)) != mon && mon) {
        if (selmon->sel) {
            selmon->sel->unfocus(1);
        }
        selmon = m;
        focus(NULL);
    }
    mon = m;
}

void movemouse(const Arg *arg) {
    int x, y, ocx, ocy, nx, ny;
    Client *c;
    Monitor *m;
    XEvent ev;
    Time lasttime = 0;

    if (!(c = selmon->sel)) {
        return;
    }
    if (c->isfullscreen) { /* no support moving fullscreen windows by mouse */
        return;
    }
    selmon->restack();
    ocx = c->x;
    ocy = c->y;
    if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
                     None, *cursor[CurMove], CurrentTime) != GrabSuccess) {
        return;
    }
    if (!getrootptr(&x, &y)) {
        return;
    }
    do {
        XMaskEvent(dpy, MOUSEMASK | ExposureMask | SubstructureRedirectMask,
                   &ev);
        switch (ev.type) {
        case ConfigureRequest:
        case Expose:
        case MapRequest:
            handler[ev.type](&ev);
            break;
        case MotionNotify:
            if ((ev.xmotion.time - lasttime) <= (1000 / refreshrate)) {
                continue;
            }
            lasttime = ev.xmotion.time;

            nx = ocx + (ev.xmotion.x - x);
            ny = ocy + (ev.xmotion.y - y);
            if (abs(selmon->wx - nx) < snap) {
                nx = selmon->wx;
            } else if (abs((selmon->wx + selmon->ww) - (nx + WIDTH(c))) <
                       snap) {
                nx = selmon->wx + selmon->ww - WIDTH(c);
            }
            if (abs(selmon->wy - ny) < snap) {
                ny = selmon->wy;
            } else if (abs((selmon->wy + selmon->wh) - (ny + HEIGHT(c))) <
                       snap) {
                ny = selmon->wy + selmon->wh - HEIGHT(c);
            }
            if (!c->isfloating && selmon->lt[selmon->sellt]->arrange &&
                (abs(nx - c->x) > snap || abs(ny - c->y) > snap)) {
                togglefloating(NULL);
            }
            if (!selmon->lt[selmon->sellt]->arrange || c->isfloating) {
                c->resize(nx, ny, c->w, c->h, 1);
            }
            break;
        }
    } while (ev.type != ButtonRelease);
    XUngrabPointer(dpy, CurrentTime);
    if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
        c->sendmon(m);
        selmon = m;
        focus(NULL);
    }
}

void propertynotify(XEvent *e) {
    Client *c;
    Window trans;
    XPropertyEvent *ev = &e->xproperty;

    if ((ev->window == root) && (ev->atom == XA_WM_NAME)) {
        updatestatus();
    } else if (ev->state == PropertyDelete) {
        return; /* ignore */
    } else if ((c = wintoclient(ev->window))) {
        switch (ev->atom) {
        default:
            break;
        case XA_WM_TRANSIENT_FOR:
            if (!c->isfloating && (XGetTransientForHint(dpy, c->win, &trans)) &&
                (c->isfloating = (wintoclient(trans)) != NULL)) {
                c->mon->arrange();
            }
            break;
        case XA_WM_NORMAL_HINTS:
            c->hintsvalid = 0;
            break;
        case XA_WM_HINTS:
            updatewmhints(c);
            drawbars();
            break;
        }
        if (ev->atom == XA_WM_NAME || ev->atom == netatom[NetWMName]) {
            updatetitle(c);
            if (c == c->mon->sel) {
                c->mon->drawbar();
            }
        }
        if (ev->atom == netatom[NetWMWindowType]) {
            updatewindowtype(c);
        }
    }
}

void quit(const Arg *arg) { running = 0; }

Monitor *recttomon(int x, int y, int w, int h) {
    Monitor *m, *r = selmon;
    int a, area = 0;

    for (m = mons; m; m = m->next) {
        if ((a = INTERSECT(x, y, w, h, m)) > area) {
            area = a;
            r = m;
        }
    }
    return r;
}

void resizemouse(const Arg *arg) {
    int ocx, ocy, nw, nh;
    Client *c;
    Monitor *m;
    XEvent ev;
    Time lasttime = 0;

    if (!(c = selmon->sel)) {
        return;
    }
    if (c->isfullscreen) { /* no support resizing fullscreen windows by mouse */
        return;
    }
    selmon->restack();
    ocx = c->x;
    ocy = c->y;
    if (XGrabPointer(dpy, root, False, MOUSEMASK, GrabModeAsync, GrabModeAsync,
                     None, *cursor[CurResize], CurrentTime) != GrabSuccess) {
        return;
    }
    XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1,
                 c->h + c->bw - 1);
    do {
        XMaskEvent(dpy, MOUSEMASK | ExposureMask | SubstructureRedirectMask,
                   &ev);
        switch (ev.type) {
        case ConfigureRequest:
        case Expose:
        case MapRequest:
            handler[ev.type](&ev);
            break;
        case MotionNotify:
            if ((ev.xmotion.time - lasttime) <= (1000 / refreshrate)) {
                continue;
            }
            lasttime = ev.xmotion.time;

            nw = MAX(ev.xmotion.x - ocx - 2 * c->bw + 1, 1);
            nh = MAX(ev.xmotion.y - ocy - 2 * c->bw + 1, 1);
            if (c->mon->wx + nw >= selmon->wx &&
                c->mon->wx + nw <= selmon->wx + selmon->ww &&
                c->mon->wy + nh >= selmon->wy &&
                c->mon->wy + nh <= selmon->wy + selmon->wh) {
                if (!c->isfloating && selmon->lt[selmon->sellt]->arrange &&
                    (abs(nw - c->w) > snap || abs(nh - c->h) > snap)) {
                    togglefloating(NULL);
                }
            }
            if (!selmon->lt[selmon->sellt]->arrange || c->isfloating) {
                c->resize(c->x, c->y, nw, nh, 1);
            }
            break;
        }
    } while (ev.type != ButtonRelease);
    XWarpPointer(dpy, None, c->win, 0, 0, 0, 0, c->w + c->bw - 1,
                 c->h + c->bw - 1);
    XUngrabPointer(dpy, CurrentTime);
    while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
    if ((m = recttomon(c->x, c->y, c->w, c->h)) != selmon) {
        c->sendmon(m);
        selmon = m;
        focus(NULL);
    }
}

void run(void) {
    XEvent ev;
    /* main event loop */
    XSync(dpy, False);
    while (running && !XNextEvent(dpy, &ev)) {
        if (handler[ev.type]) {
            handler[ev.type](&ev); /* call handler */
        }
    }
}

void scan(void) {
    unsigned int i, num;
    Window d1, d2, *wins = NULL;
    XWindowAttributes wa;

    if (XQueryTree(dpy, root, &d1, &d2, &wins, &num)) {
        for (i = 0; i < num; i++) {
            if (!XGetWindowAttributes(dpy, wins[i], &wa) ||
                wa.override_redirect ||
                XGetTransientForHint(dpy, wins[i], &d1)) {
                continue;
            }
            if (wa.map_state == IsViewable ||
                getstate(wins[i]) == IconicState) {
                manage(wins[i], &wa);
            }
        }
        for (i = 0; i < num; i++) { /* now the transients */
            if (!XGetWindowAttributes(dpy, wins[i], &wa)) {
                continue;
            }
            if (XGetTransientForHint(dpy, wins[i], &d1) &&
                (wa.map_state == IsViewable ||
                 getstate(wins[i]) == IconicState)) {
                manage(wins[i], &wa);
            }
        }
        if (wins) {
            XFree(wins);
        }
    }
}

void setlayout(const Arg *arg) {
    if (!arg || !arg->v || arg->v != selmon->lt[selmon->sellt]) {
        selmon->sellt ^= 1;
    }
    if (arg && arg->v) {
        selmon->lt[selmon->sellt] = (Layout *)arg->v;
    }
    strncpy(selmon->ltsymbol, selmon->lt[selmon->sellt]->symbol,
            sizeof selmon->ltsymbol);
    if (selmon->sel) {
        selmon->arrange();
    } else {
        selmon->drawbar();
    }
}

/* arg > 1.0 will set mfact absolutely */
void setmfact(const Arg *arg) {
    float f;

    if (!arg || !selmon->lt[selmon->sellt]->arrange) {
        return;
    }
    f = arg->f < 1.0 ? arg->f + selmon->mfact : arg->f - 1.0;
    if (f < 0.05 || f > 0.95) {
        return;
    }
    selmon->mfact = f;
    selmon->arrange();
}

void setup(void) {
    int i;
    XSetWindowAttributes wa;
    Atom utf8string;
    struct sigaction sa;

    handler[ButtonPress] = buttonpress;
    handler[ClientMessage] = clientmessage;
    handler[ConfigureRequest] = configurerequest;
    handler[ConfigureNotify] = configurenotify;
    handler[DestroyNotify] = destroynotify;
    handler[EnterNotify] = enternotify;
    handler[Expose] = expose;
    handler[FocusIn] = focusin;
    handler[KeyPress] = keypress;
    handler[MappingNotify] = mappingnotify;
    handler[MapRequest] = maprequest;
    handler[MotionNotify] = motionnotify;
    handler[PropertyNotify] = propertynotify;
    handler[UnmapNotify] = unmapnotify;

    /* do not transform children into zombies when they terminate */
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_NOCLDSTOP | SA_NOCLDWAIT | SA_RESTART;
    sa.sa_handler = SIG_IGN;
    sigaction(SIGCHLD, &sa, NULL);

    /* clean up any zombies (inherited from .xinitrc etc) immediately */
    while (waitpid(-1, NULL, WNOHANG) > 0);

    /* init screen */
    screen = DefaultScreen(dpy);
    sw = DisplayWidth(dpy, screen);
    sh = DisplayHeight(dpy, screen);
    root = RootWindow(dpy, screen);
    drw = drw_create(dpy, screen, root, sw, sh);
    if (!drw_fontset_create(drw, fonts, LENGTH(fonts))) {
        die("no fonts could be loaded.");
    }
    lrpad = drw->fonts->h;
    bh = drw->fonts->h + 2;
    updategeom();
    /* init atoms */
    utf8string = XInternAtom(dpy, "UTF8_STRING", False);
    wmatom[WMProtocols] = XInternAtom(dpy, "WM_PROTOCOLS", False);
    wmatom[WMDelete] = XInternAtom(dpy, "WM_DELETE_WINDOW", False);
    wmatom[WMState] = XInternAtom(dpy, "WM_STATE", False);
    wmatom[WMTakeFocus] = XInternAtom(dpy, "WM_TAKE_FOCUS", False);
    netatom[NetActiveWindow] = XInternAtom(dpy, "_NET_ACTIVE_WINDOW", False);
    netatom[NetSupported] = XInternAtom(dpy, "_NET_SUPPORTED", False);
    netatom[NetWMName] = XInternAtom(dpy, "_NET_WM_NAME", False);
    netatom[NetWMState] = XInternAtom(dpy, "_NET_WM_STATE", False);
    netatom[NetWMCheck] = XInternAtom(dpy, "_NET_SUPPORTING_WM_CHECK", False);
    netatom[NetWMFullscreen] =
        XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", False);
    netatom[NetWMWindowType] = XInternAtom(dpy, "_NET_WM_WINDOW_TYPE", False);
    netatom[NetWMWindowTypeDialog] =
        XInternAtom(dpy, "_NET_WM_WINDOW_TYPE_DIALOG", False);
    netatom[NetClientList] = XInternAtom(dpy, "_NET_CLIENT_LIST", False);
    /* init cursors */
    cursor[CurNormal] = drw_cur_create(drw, XC_left_ptr);
    cursor[CurResize] = drw_cur_create(drw, XC_sizing);
    cursor[CurMove] = drw_cur_create(drw, XC_fleur);
    /* init appearance */
    scheme = (XftColor **)ecalloc(LENGTH(COLORS), sizeof(XftColor *));
    for (i = 0; i < LENGTH(COLORS); i++) {
        scheme[i] = drw_scm_create(drw, COLORS[i], 3);
    }
    /* init bars */
    updatebars();
    updatestatus();
    /* supporting window for NetWMCheck */
    wmcheckwin = XCreateSimpleWindow(dpy, root, 0, 0, 1, 1, 0, 0, 0);
    XChangeProperty(dpy, wmcheckwin, netatom[NetWMCheck], XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *)&wmcheckwin, 1);
    XChangeProperty(dpy, wmcheckwin, netatom[NetWMName], utf8string, 8,
                    PropModeReplace, (unsigned char *)"dwm", 3);
    XChangeProperty(dpy, root, netatom[NetWMCheck], XA_WINDOW, 32,
                    PropModeReplace, (unsigned char *)&wmcheckwin, 1);
    /* EWMH support per view */
    XChangeProperty(dpy, root, netatom[NetSupported], XA_ATOM, 32,
                    PropModeReplace, (unsigned char *)netatom, NetLast);
    XDeleteProperty(dpy, root, netatom[NetClientList]);
    /* select events */
    wa.cursor = *cursor[CurNormal];
    wa.event_mask = SubstructureRedirectMask | SubstructureNotifyMask |
                    ButtonPressMask | PointerMotionMask | EnterWindowMask |
                    LeaveWindowMask | StructureNotifyMask | PropertyChangeMask;
    XChangeWindowAttributes(dpy, root, CWEventMask | CWCursor, &wa);
    XSelectInput(dpy, root, wa.event_mask);
    grabkeys();
    focus(NULL);
}

void spawn(const Arg *arg) {
    struct sigaction sa;

    if (arg->v == dmenucmd) {
        dmenumon[0] = '0' + selmon->num;
    }
    if (fork() == 0) {
        if (dpy) {
            close(ConnectionNumber(dpy));
        }
        setsid();

        sigemptyset(&sa.sa_mask);
        sa.sa_flags = 0;
        sa.sa_handler = SIG_DFL;
        sigaction(SIGCHLD, &sa, NULL);

        execvp(((char **)arg->v)[0], (char **)arg->v);
        die("dwm: execvp '%s' failed:", ((char **)arg->v)[0]);
    }
}

void tag(const Arg *arg) {
    if (selmon->sel && arg->ui & TAGMASK) {
        selmon->sel->tags = arg->ui & TAGMASK;
        focus(NULL);
        selmon->arrange();
    }
}

void tagmon(const Arg *arg) {
    if (!selmon->sel || !mons->next) {
        return;
    }
    selmon->sel->sendmon(dirtomon(arg->i));
}

void tile(Monitor *m) {
    unsigned int i, n, h, mw, my, ty;
    Client *c;

    for (n = 0, c = m->clients->nexttiled(); c; c = c->next->nexttiled(), n++);
    if (n == 0) {
        return;
    }

    if (n > m->nmaster) {
        mw = m->nmaster ? m->ww * m->mfact : 0;
    } else {
        mw = m->ww;
    }
    for (i = my = ty = 0, c = m->clients->nexttiled(); c;
         c = c->next->nexttiled(), i++) {
        if (i < m->nmaster) {
            h = (m->wh - my) / (MIN(n, m->nmaster) - i);
            c->resize(m->wx, m->wy + my, mw - (2 * c->bw), h - (2 * c->bw), 0);
            if (my + HEIGHT(c) < m->wh) {
                my += HEIGHT(c);
            }
        } else {
            h = (m->wh - ty) / (n - i);
            c->resize(m->wx + mw, m->wy + ty, m->ww - mw - (2 * c->bw),
                      h - (2 * c->bw), 0);
            if (ty + HEIGHT(c) < m->wh) {
                ty += HEIGHT(c);
            }
        }
    }
}

void togglebar(const Arg *arg) {
    selmon->showbar = !selmon->showbar;
    updatebarpos(selmon);
    XMoveResizeWindow(dpy, selmon->barwin, selmon->wx, selmon->by, selmon->ww,
                      bh);
    selmon->arrange();
}

void togglefloating(const Arg *arg) {
    if (!selmon->sel) {
        return;
    }
    if (selmon->sel->isfullscreen) { /* no support for fullscreen windows */
        return;
    }
    selmon->sel->isfloating = !selmon->sel->isfloating || selmon->sel->isfixed;
    if (selmon->sel->isfloating) {
        selmon->sel->resize(selmon->sel->x, selmon->sel->y, selmon->sel->w,
                            selmon->sel->h, 0);
    }
    selmon->arrange();
}

void toggletag(const Arg *arg) {
    unsigned int newtags;

    if (!selmon->sel) {
        return;
    }
    newtags = selmon->sel->tags ^ (arg->ui & TAGMASK);
    if (newtags) {
        selmon->sel->tags = newtags;
        focus(NULL);
        selmon->arrange();
    }
}

void toggleview(const Arg *arg) {
    unsigned int newtagset =
        selmon->tagset[selmon->seltags] ^ (arg->ui & TAGMASK);

    if (newtagset) {
        selmon->tagset[selmon->seltags] = newtagset;
        focus(NULL);
        selmon->arrange();
    }
}

void unmapnotify(XEvent *e) {
    Client *c;
    XUnmapEvent *ev = &e->xunmap;

    if ((c = wintoclient(ev->window))) {
        if (ev->send_event) {
            c->setclientstate(WithdrawnState);
        } else {
            c->unmanage(0);
        }
    }
}

void updatebars(void) {
    Monitor *m;
    XSetWindowAttributes wa = {.background_pixmap = ParentRelative,
                               .event_mask = ButtonPressMask | ExposureMask,
                               .override_redirect = True};
    char dwm[4] = "dwm";
    XClassHint ch = {dwm, dwm};
    for (m = mons; m; m = m->next) {
        if (m->barwin) {
            continue;
        }
        m->barwin = XCreateWindow(
            dpy, root, m->wx, m->by, m->ww, bh, 0, DefaultDepth(dpy, screen),
            CopyFromParent, DefaultVisual(dpy, screen),
            CWOverrideRedirect | CWBackPixmap | CWEventMask, &wa);
        XDefineCursor(dpy, m->barwin, *cursor[CurNormal]);
        XMapRaised(dpy, m->barwin);
        XSetClassHint(dpy, m->barwin, &ch);
    }
}

void updatebarpos(Monitor *m) {
    m->wy = m->my;
    m->wh = m->mh;
    if (m->showbar) {
        m->wh -= bh;
        m->by = m->topbar ? m->wy : m->wy + m->wh;
        m->wy = m->topbar ? m->wy + bh : m->wy;
    } else {
        m->by = -bh;
    }
}

void updateclientlist(void) {
    Client *c;
    Monitor *m;

    XDeleteProperty(dpy, root, netatom[NetClientList]);
    for (m = mons; m; m = m->next) {
        for (c = m->clients; c; c = c->next) {
            XChangeProperty(dpy, root, netatom[NetClientList], XA_WINDOW, 32,
                            PropModeAppend, (unsigned char *)&(c->win), 1);
        }
    }
}

int updategeom(void) {
    int dirty = 0;

#ifdef XINERAMA
    if (XineramaIsActive(dpy)) {
        int i, j, n, nn;
        Client *c;
        Monitor *m;
        XineramaScreenInfo *info = XineramaQueryScreens(dpy, &nn);
        XineramaScreenInfo *unique = NULL;

        for (n = 0, m = mons; m; m = m->next, n++);
        /* only consider unique geometries as separate screens */
        unique = (XineramaScreenInfo *)ecalloc(nn, sizeof(XineramaScreenInfo));
        for (i = 0, j = 0; i < nn; i++) {
            if (isuniquegeom(unique, j, &info[i])) {
                memcpy(&unique[j++], &info[i], sizeof(XineramaScreenInfo));
            }
        }
        XFree(info);
        nn = j;

        /* new monitors if nn > n */
        for (i = n; i < nn; i++) {
            for (m = mons; m && m->next; m = m->next);
            if (m) {
                m->next = createmon();
            } else {
                mons = createmon();
            }
        }
        for (i = 0, m = mons; i < nn && m; m = m->next, i++) {
            if (i >= n || unique[i].x_org != m->mx ||
                unique[i].y_org != m->my || unique[i].width != m->mw ||
                unique[i].height != m->mh) {
                dirty = 1;
                m->num = i;
                m->mx = m->wx = unique[i].x_org;
                m->my = m->wy = unique[i].y_org;
                m->mw = m->ww = unique[i].width;
                m->mh = m->wh = unique[i].height;
                updatebarpos(m);
            }
        }
        /* removed monitors if n > nn */
        for (i = nn; i < n; i++) {
            for (m = mons; m && m->next; m = m->next);
            while ((c = m->clients)) {
                dirty = 1;
                m->clients = c->next;
                c->detachstack();
                c->mon = mons;
                c->attach();
                c->attachstack();
            }
            if (m == selmon) {
                selmon = mons;
            }
            cleanupmon(m);
        }
        free(unique);
    } else
#endif /* XINERAMA */
    {  /* default monitor setup */
        if (!mons) {
            mons = createmon();
        }
        if (mons->mw != sw || mons->mh != sh) {
            dirty = 1;
            mons->mw = mons->ww = sw;
            mons->mh = mons->wh = sh;
            updatebarpos(mons);
        }
    }
    if (dirty) {
        selmon = mons;
        selmon = wintomon(root);
    }
    return dirty;
}

void updatenumlockmask(void) {
    unsigned int i, j;
    XModifierKeymap *modmap;

    numlockmask = 0;
    modmap = XGetModifierMapping(dpy);
    for (i = 0; i < 8; i++) {
        for (j = 0; j < modmap->max_keypermod; j++) {
            if (modmap->modifiermap[i * modmap->max_keypermod + j] ==
                XKeysymToKeycode(dpy, XK_Num_Lock)) {
                numlockmask = (1 << i);
            }
        }
    }
    XFreeModifiermap(modmap);
}

void updatesizehints(Client *c) {
    long msize;
    XSizeHints size;

    if (!XGetWMNormalHints(dpy, c->win, &size, &msize)) {
        /* size is uninitialized, ensure that size.flags aren't used */
        size.flags = PSize;
    }
    if (size.flags & PBaseSize) {
        c->basew = size.base_width;
        c->baseh = size.base_height;
    } else if (size.flags & PMinSize) {
        c->basew = size.min_width;
        c->baseh = size.min_height;
    } else {
        c->basew = c->baseh = 0;
    }
    if (size.flags & PResizeInc) {
        c->incw = size.width_inc;
        c->inch = size.height_inc;
    } else {
        c->incw = c->inch = 0;
    }
    if (size.flags & PMaxSize) {
        c->maxw = size.max_width;
        c->maxh = size.max_height;
    } else {
        c->maxw = c->maxh = 0;
    }
    if (size.flags & PMinSize) {
        c->minw = size.min_width;
        c->minh = size.min_height;
    } else if (size.flags & PBaseSize) {
        c->minw = size.base_width;
        c->minh = size.base_height;
    } else {
        c->minw = c->minh = 0;
    }
    if (size.flags & PAspect) {
        c->mina = (float)size.min_aspect.y / size.min_aspect.x;
        c->maxa = (float)size.max_aspect.x / size.max_aspect.y;
    } else {
        c->maxa = c->mina = 0.0;
    }
    c->isfixed =
        (c->maxw && c->maxh && c->maxw == c->minw && c->maxh == c->minh);
    c->hintsvalid = 1;
}

void updatestatus(void) {
    if (!gettextprop(root, XA_WM_NAME, stext, sizeof(stext))) {
        strcpy(stext, "dwm-" VERSION);
    }
    selmon->drawbar();
}

void updatetitle(Client *c) {
    if (!gettextprop(c->win, netatom[NetWMName], c->name, sizeof c->name)) {
        gettextprop(c->win, XA_WM_NAME, c->name, sizeof c->name);
    }
    if (c->name[0] == '\0') { /* hack to mark broken clients */
        strcpy(c->name, broken);
    }
}

void updatewindowtype(Client *c) {
    Atom state = c->getatomprop(netatom[NetWMState]);
    Atom wtype = c->getatomprop(netatom[NetWMWindowType]);

    if (state == netatom[NetWMFullscreen]) {
        c->setfullscreen(1);
    }
    if (wtype == netatom[NetWMWindowTypeDialog]) {
        c->isfloating = 1;
    }
}

void updatewmhints(Client *c) {
    XWMHints *wmh;

    if ((wmh = XGetWMHints(dpy, c->win))) {
        if (c == selmon->sel && wmh->flags & XUrgencyHint) {
            wmh->flags &= ~XUrgencyHint;
            XSetWMHints(dpy, c->win, wmh);
        } else {
            c->isurgent = (wmh->flags & XUrgencyHint) ? 1 : 0;
        }
        if (wmh->flags & InputHint) {
            c->neverfocus = !wmh->input;
        } else {
            c->neverfocus = 0;
        }
        XFree(wmh);
    }
}

void view(const Arg *arg) {
    if ((arg->ui & TAGMASK) == selmon->tagset[selmon->seltags]) {
        return;
    }
    selmon->seltags ^= 1; /* toggle sel tagset */
    if (arg->ui & TAGMASK) {
        selmon->tagset[selmon->seltags] = arg->ui & TAGMASK;
    }
    focus(NULL);
    selmon->arrange();
}

Client *wintoclient(Window w) {
    Client *c;
    Monitor *m;

    for (m = mons; m; m = m->next) {
        for (c = m->clients; c; c = c->next) {
            if (c->win == w) {
                return c;
            }
        }
    }
    return NULL;
}

Monitor *wintomon(Window w) {
    int x, y;
    Client *c;
    Monitor *m;

    if (w == root && getrootptr(&x, &y)) {
        return recttomon(x, y, 1, 1);
    }
    for (m = mons; m; m = m->next) {
        if (w == m->barwin) {
            return m;
        }
    }
    if ((c = wintoclient(w))) {
        return c->mon;
    }
    return selmon;
}

/* There's no way to check accesses to destroyed windows, thus those cases are
 * ignored (especially on UnmapNotify's). Other types of errors call Xlibs
 * default error handler, which may call exit. */
int xerror(Display *dpy, XErrorEvent *ee) {
    if (ee->error_code == BadWindow ||
        (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch) ||
        (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable) ||
        (ee->request_code == X_PolyFillRectangle &&
         ee->error_code == BadDrawable) ||
        (ee->request_code == X_PolySegment && ee->error_code == BadDrawable) ||
        (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch) ||
        (ee->request_code == X_GrabButton && ee->error_code == BadAccess) ||
        (ee->request_code == X_GrabKey && ee->error_code == BadAccess) ||
        (ee->request_code == X_CopyArea && ee->error_code == BadDrawable)) {
        return 0;
    }
    fprintf(stderr, "dwm: fatal error: request code=%d, error code=%d\n",
            ee->request_code, ee->error_code);
    return xerrorxlib(dpy, ee); /* may call exit */
}

int xerrordummy(Display *dpy, XErrorEvent *ee) { return 0; }

/* Startup Error handler to check if another window manager
 * is already running. */
int xerrorstart(Display *dpy, XErrorEvent *ee) {
    die("dwm: another window manager is already running");
    return -1;
}

void zoom(const Arg *arg) {
    Client *c = selmon->sel;

    if (!selmon->lt[selmon->sellt]->arrange || !c || c->isfloating) {
        return;
    }
    if (c == selmon->clients->nexttiled() && !(c = c->next->nexttiled())) {
        return;
    }
    c->pop();
}

int main(int argc, char *argv[]) {
    if (argc == 2 && !strcmp("-v", argv[1])) {
        die("dwm-" VERSION);
    } else if (argc != 1) {
        die("usage: dwm [-v]");
    }
    if (!setlocale(LC_CTYPE, "") || !XSupportsLocale()) {
        fputs("warning: no locale support\n", stderr);
    }
    if (!(dpy = XOpenDisplay(NULL))) {
        die("dwm: cannot open display");
    }
    checkotherwm();
    setup();
#ifdef __OpenBSD__
    if (pledge("stdio rpath proc exec", NULL) == -1) {
        die("pledge");
    }
#endif /* __OpenBSD__ */
    scan();
    run();
    cleanup();
    XCloseDisplay(dpy);
    return EXIT_SUCCESS;
}
