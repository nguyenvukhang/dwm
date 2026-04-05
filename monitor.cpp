#include "monitor.h"

void Monitor::arrange() {
    showhide(this->stack);
    arrangemon(this);
    restack(this);
}
