#pragma once

#include "dtypes.h"

struct Layout;

struct Monitor {
    char ltsymbol[16];
    float mfact;
    int nmaster;
    int num;
    /// Bar geometry.
    int by;
    /// The Rect that every pixel on the monitor lives in.
    Rect m;
    /// The Rect that windows live in. This is simply the monitor's Rect minus
    /// the status bar's Rect.
    Rect w;
    /// Bitmask of selected tags.
    unsigned int seltags;
    /// Index of selected layout.
    unsigned int sellt;
    unsigned int tagset[2];
    /// 0 means no bar.
    int showbar;
    /// 0 means bottom bar.
    int topbar;
    /// Linked list of clients.
    Client *clients;
    /// Selected client.
    Client *sel;
    /// Clients ordered by stack.
    Client *stack;
    Monitor *next;
    Window barwin;
    const Layout *lt[2];

    void arrange();
    void arrangemon();
    void drawbar() const;
    void restack() const;
    void updatebarpos();
};
