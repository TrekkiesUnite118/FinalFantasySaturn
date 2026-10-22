#include <string.h>
#include <sega_def.h>
#include <sega_per.h>
#include <sega_bup.h>

#include "sys.h"
#include "backup.h"

static Uint32 bupLib[BUP_LIB_SIZE4];
static Uint32 bupWork[BUP_WORK_SIZE4];
static BupConfig bupConf[BK_UNITS];

void bkResetDisable() {
    PER_SMPC_RES_DIS();
}

void bkResetEnable() {
    PER_SMPC_RES_ENA();
}

void bkInit() {
    PER_SMPC_RES_DIS();
    BUP_Init(bupLib, bupWork, bupConf);
    PER_SMPC_RES_ENA();
}

Uint8 bkUnitPresent(Uint8 unit) {
    return bupConf[unit].unit_id != 0;
}

Uint8 bkPartitions(Uint8 unit) {
    return bupConf[unit].partition;
}

void bkSelectPartition(Uint8 unit, Uint8 part) {
    BUP_SelPart(unit, part);
}

Uint8 bkStat(Uint8 unit, Uint32 size, BkStat *stat) {
    BupStat s;
    Sint32 ret = BUP_Stat(unit, size, &s);

    if(ret == BUP_UNFORMAT) {
        return BK_UNFORMAT;
    }
    if(ret != 0) {
        return BK_ERROR;
    }
    stat->blocksize = s.blocksize;
    stat->freeblock = s.freeblock;
    stat->datanum = s.datanum;
    return BK_OK;
}

Sint32 bkFind(Uint8 unit, const char *name) {
    BupDir dir;

    if(BUP_Dir(unit, (Uint8 *)name, 1, &dir) <= 0 || strcmp((char *)dir.filename, name) != 0) {
        return -1;
    }
    return dir.datasize;
}

Uint8 bkRead(Uint8 unit, const char *name, void *data) {
    return BUP_Read(unit, (Uint8 *)name, (Uint8 *)data) == 0;
}

//The SMPC clock is BCD.
static Uint32 bkTimeStamp() {
    const Uint8 *time = rtcTime();
    BupDate date;

    date.year = (Uint8)((time[6] >> 4) * 1000 + (time[6] & 0x0F) * 100 + (time[5] >> 4) * 10 + (time[5] & 0x0F) - 1980);
    date.month = time[4] & 0x0F;
    date.day = (time[3] >> 4) * 10 + (time[3] & 0x0F);
    date.time = (time[2] >> 4) * 10 + (time[2] & 0x0F);
    date.min = (time[1] >> 4) * 10 + (time[1] & 0x0F);
    date.week = time[4] >> 4;
    return BUP_SetDate(&date);
}

Uint8 bkWrite(Uint8 unit, const char *name, const char *comment, void *data, Uint32 size) {
    BupDir dir;

    memset(&dir, 0, sizeof(dir));
    strcpy((char *)dir.filename, name);
    strcpy((char *)dir.comment, comment);
    dir.language = BUP_ENGLISH;
    dir.date = bkTimeStamp();
    dir.datasize = size;
    if(BUP_Write(unit, &dir, (Uint8 *)data, OFF) != 0) {
        return FALSE;
    }
    return BUP_Verify(unit, dir.filename, (Uint8 *)data) == 0;
}
