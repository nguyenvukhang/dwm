#pragma once

#include <X11/Xlib.h>

struct Monitor;

struct Client {
    char name[256];
    float mina, maxa;
    int x, y, w, h;
    int oldx, oldy, oldw, oldh;
    int basew, baseh, incw, inch, maxw, maxh, minw, minh, hintsvalid;
    int bw, oldbw;
    unsigned int tags;
    int isfixed, isfloating, isurgent, neverfocus, oldstate, isfullscreen;
    Client *next;
    Client *snext;
    Monitor *mon;
    Window win;

    void applyrules();
    int applysizehints(int *x, int *y, int *w, int *h, int interact);
    void attach();
    void attachstack();
    void configure() const;
    void detach() const;
    void detachstack() const;
    Atom getatomprop(Atom prop) const;
    void grabbuttons(int focused) const;
    Client *nexttiled();
    void pop();
    void resize(int x, int y, int w, int h, int interact);
    void resizeclient(int x, int y, int w, int h);
    int sendevent(Atom proto) const;
    void sendmon(Monitor *);
    void setclientstate(long state) const;
    void setfocus() const;
};
