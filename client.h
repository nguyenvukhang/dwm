#pragma once

#include "rect.h"
#include <X11/Xlib.h>

struct Monitor;

struct SizeHinted {
    Size base;
    /// Incremental size when resizing.
    Size inc;
    Size max;
    Size min;
    /// Maximum aspect ratio (width / height).
    float maxa;
    /// Minimum aspect ratio (height / width).
    /// Note that this is the reciprocal of the conventional notion of the
    /// aspect ratio because of how we'll be using it.
    float mina;
};

struct Client : private SizeHinted, public Rect {
    char name[256];

    int oldx, oldy, oldw, oldh;
    int hintsvalid;
    /// Border width.
    int bw;
    /// Old border width.
    int oldbw;
    /// Bitmask of active tags.
    unsigned int tags;
    int isfixed;
    int isfloating;
    int isurgent;
    int neverfocus;
    /// Old floating state (previous value for `isfloating`).
    int oldstate;
    int isfullscreen;
    /// Next client in the linked list of clients.
    Client *next;
    /// Next client in the display stack.
    Client *snext;
    Monitor *mon;
    Window win;

    void applyrules();
    bool applysizehints(Rect &, int interact);
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
    void resize(Rect, bool interact);
    void resizeclient(const Rect *);
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

    /// The width of this window including the border.
    int effective_width() const;
    /// The height of this window including the border.
    int effective_height() const;
};
