#include <sega_def.h>
#include <sega_spr.h>

#include "slave.h"

void slaveRun(void (*routine)(void *), void *parm) {
    SPR_RunSlaveSH(routine, parm);
}

void slaveWait() {
    SPR_WaitEndSlaveSH();
}
