#pragma once

#include <X11/Xlib.h>

struct Monitor;

struct Client {
    char name[256];
    float mina, maxa;
    int x, y, w, h;
    int oldx, oldy, oldw, oldh;
    int basew, baseh, incw, inch, maxw, maxh, minw, minh, hintsvalid;
    /* Border width. */
    int bw;
    /* Old border width. */
    int oldbw;
    /* Bitmask of active tags. */
    unsigned int tags;
    int isfixed;
    int isfloating;
    int isurgent;
    int neverfocus;
    /* Old floating state (previous value for `isfloating`). */
    int oldstate;
    int isfullscreen;
    /* Next client in the linked list of clients. */
    Client *next;
    /* Next client in the display stack. */
    Client *snext;
    Monitor *mon;
    Window win;

    void applyrules();
    int applysizehints(int *x, int *y, int *w, int *h, int interact);
    /// Attach to the head of the (singly) linked list that is
    /// `this->mon->clients`.
    void attach();
    /// Attach to the head of the (singly) linked list that is
    /// `this->mon->stack`.
    void attachstack();
    void configure() const;
    /// Removes this client from the (singly) linked list that is
    /// `this->mon->clients`.
    void detach() const;
    /// Removes this client from the (singly) linked list that is
    /// `this->mon->stack`. And if it's the selected one, chose the next visible
    /// client in the stack to be selected.
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
    void setfullscreen(int fullscreen);
    void seturgent(int urg);
    void showhide();
    void unfocus(int setfocus) const;
    void unmanage(int destroyed);
    void updatesizehints();
    void updatetitle();
    void updatewindowtype();
    void updatewmhints();
};
