#include "monitor.h"

void Monitor::arrange() {
    showhide(this->stack);
    this->arrangemon();
    restack(this);
}

void Monitor::arrangemon() {
    strncpy(ltsymbol, lt[sellt]->symbol, sizeof ltsymbol);
    if (lt[sellt]->arrange) {
        lt[sellt]->arrange(this);
    }
}
