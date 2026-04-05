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
