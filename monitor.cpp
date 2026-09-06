#include "monitor.h"
#include "config.h"

void Monitor::arrange() {
    if (this->stack) {
        this->stack->showhide();
    }
    this->arrangemon();
    this->restack();
}

void Monitor::arrangemon() {
    strncpy(ltsymbol, lt[sellt]->symbol, sizeof ltsymbol);
    if (lt[sellt]->arrange) {
        lt[sellt]->arrange(this);
    }
}

void Monitor::drawbar() const {
    int x, w, tw = 0;
    int boxs = drw->fonts->h / 9;
    int boxw = drw->fonts->h / 6 + 2;
    unsigned int i, occ = 0, urg = 0;
    Client *c;

    if (!this->showbar) {
        return;
    }

    /* draw status first so it can be overdrawn by tags later */
    if (this == selmon) { /* status is only drawn on selected monitor */
        drw_setscheme(drw, scheme[SchemeNorm]);
        tw = TEXTW(stext) - lrpad + 2; /* 2px right padding */
        drw_text(drw, this->w.w - tw, 0, tw, bh, 0, stext, 0);
    }

    for (c = this->clients; c; c = c->next) {
        occ |= c->tags;
        if (c->isurgent) {
            urg |= c->tags;
        }
    }
    x = 0;
    for (i = 0; i < LENGTH(TAGS); i++) {
        w = TEXTW(TAGS[i]);
        drw_setscheme(
            drw, scheme[this->tagset[this->seltags] & 1 << i ? SchemeSel
                                                             : SchemeNorm]);
        drw_text(drw, x, 0, w, bh, lrpad / 2, TAGS[i], urg & 1 << i);
        if (occ & 1 << i) {
            drw_rect(drw, x + boxs, boxs, boxw, boxw,
                     this == selmon && selmon->sel &&
                         selmon->sel->tags & 1 << i,
                     urg & 1 << i);
        }
        x += w;
    }
    w = TEXTW(this->ltsymbol);
    drw_setscheme(drw, scheme[SchemeNorm]);
    x = drw_text(drw, x, 0, w, bh, lrpad / 2, this->ltsymbol, 0);

    if ((w = this->w.w - tw - x) > bh) {
        if (this->sel) {
            drw_setscheme(drw, scheme[this == selmon ? SchemeBar : SchemeNorm]);
            drw_text(drw, x, 0, w, bh, lrpad / 2, this->sel->name, 0);
            if (this->sel->isfloating) {
                drw_rect(drw, x + boxs, boxs, boxw, boxw, this->sel->isfixed,
                         0);
            }
        } else {
            drw_setscheme(drw, scheme[SchemeNorm]);
            drw_rect(drw, x, 0, w, bh, 1, 1);
        }
    }
    drw_map(drw, this->barwin, 0, 0, this->w.w, bh);
}

void Monitor::restack() const {
    Client *c;
    XEvent ev;
    XWindowChanges wc;

    this->drawbar();
    if (!this->sel) {
        return;
    }
    if (this->sel->isfloating || !this->lt[this->sellt]->arrange) {
        XRaiseWindow(dpy, this->sel->win);
    }
    if (this->lt[this->sellt]->arrange) {
        wc.stack_mode = Below;
        wc.sibling = this->barwin;
        for (c = this->stack; c; c = c->snext) {
            if (!c->isfloating && ISVISIBLE(c)) {
                XConfigureWindow(dpy, c->win, CWSibling | CWStackMode, &wc);
                wc.sibling = c->win;
            }
        }
    }
    XSync(dpy, False);
    while (XCheckMaskEvent(dpy, EnterWindowMask, &ev));
}

void Monitor::updatebarpos() {
    this->w.y = this->m.y;
    this->w.h = this->m.h;
    if (this->showbar) {
        this->w.h -= bh;
        this->by = this->topbar ? this->w.y : this->w.y + this->w.h;
        this->w.y = this->topbar ? this->w.y + bh : this->w.y;
    } else {
        this->by = -bh;
    }
}
