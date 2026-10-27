#include <string.h>
#include <stddef.h>
#include "types.h"
#include "backup.h"

#include "player.h"
#include "sprdata.h"
#include "menu.h"
#include "vblank.h"
#include "stdmap.h"
#include "battle.h"
#include "save.h"
#include "music.h"
#include "audio.h"
#include "sound.h"
#include "bugfix.h"
#include "msxgfx.h"

#define SV_SLOTS (3)
#define SV_DEVICES (4) //Units as listed on screen, the floppy once per side.
#define SV_FLOPPY_UNIT (2)
#define SV_NO_UNIT (0xFF)
#define SV_VERSION (1)
#define SV_PAL (0)
#define SV_FRAME_BATTLE_START (96)

//Box layout in NES tile coords. The 27 rows used (28 Japanese) sit 1 row from the top.
//Japanese text needs a free row above it for dakuten marks, like the Japanese ROM's taller boxes.
#ifdef FF1_JP
#define SV_TXT(us, jp) (jp)
#define SV_TOP_H (4)
#define SV_NAME_Y (1)
#define SV_RIGHT_COL (15)
#define SV_TEXT_COL (2)
#define SV_GOLD_Y (4)
#define SV_BOOT_LINE_H (2)
#else
#define SV_TXT(us, jp) (us)
#define SV_TOP_H (3)
#define SV_NAME_Y (0)
#define SV_RIGHT_COL (16)
#define SV_TEXT_COL (3)
#define SV_GOLD_Y (SV_SLOT_H - 2)
#define SV_BOOT_LINE_H (1)
#endif
#define SV_LABEL_X (0)
#define SV_LABEL_W (7)
#define SV_MSG_X (7)
#define SV_MSG_W (25)
#define SV_TOP_Y (1)
#define SV_TEXT_Y (SV_TOP_Y + SV_TOP_H - 2)
#define SV_MSG_TEXT_X (SV_MSG_X + 1)
#define SV_YES_X (SV_MSG_TEXT_X + 13)
#define SV_NO_X (SV_MSG_TEXT_X + 19)
#define SV_SLOT_Y (SV_TOP_Y + SV_TOP_H)
#define SV_SLOT_H (8)
#define SV_SLOT_W (32)

//Boot message box.
#define SV_BOOT_BOX_Y (3)
#define SV_BOOT_BOX_H (24)
#define SV_BOOT_TEXT_X (2)
#define SV_BOOT_TEXT_Y (SV_BOOT_BOX_Y + 2)
#define SV_BOOT_MSG_MAX (3)

#ifdef FF1_JP
static const char *svDeviceNames[SV_DEVICES] = {"ほんたいメモリ", "カートリッジメモリ", "フロッピードライブ1", "フロッピードライブ2"};
#else
static const char *svDeviceNames[SV_DEVICES] = {"System Memory", "Cartridge Memory", "Floppy Drive 1", "Floppy Drive 2"};
#endif
static const Uint8 svDeviceUnit[SV_DEVICES] = {0, 1, SV_FLOPPY_UNIT, SV_FLOPPY_UNIT};
static const Uint8 svDevicePart[SV_DEVICES] = {0, 0, 0, 1};

typedef struct {
    Uint8 magic[4];
    Uint8 version;
    Uint8 vehicle;
    Uint8 owX;
    Uint8 owY;
    Uint32 gold;
    Uint8 items[sizeof(items)];
    Uint8 gameFlags[sizeof(gameFlags)];
    Uint8 shipVisible, shipX, shipY;
    Uint8 airshipVisible, airshipX, airshipY;
    Uint8 bridgeVisible, bridgeX, bridgeY;
    Uint8 canalVisible, canalX, canalY;
    Uint8 hasCanoe;
    Uint8 bridgeScene;
    PLAYER_CHARACTER party[4];
    Uint32 checksum;
    //After the checksum, so older saves without them are a prefix and load with them off.
    Uint8 bugFixes[BUGFIX_BYTES];
    Uint8 gfxMsx;
    Uint8 sndMsx;
} SAVE_DATA;

//Sizes of saves from before the bug fixes, the graphics option and the sound option.
#define SV_OLD_SIZE (offsetof(SAVE_DATA, bugFixes))
#define SV_BUGFIX_SIZE (offsetof(SAVE_DATA, gfxMsx))
#define SV_GFX_SIZE (offsetof(SAVE_DATA, sndMsx))

//The loaded save's sound option.
static Uint8 svSndMsx;

enum {
    SLOT_EMPTY,
    SLOT_DATA,
    SLOT_BROKEN
};

enum {
    DEV_OK,
    DEV_NONE,
    DEV_UNFORMAT
};

enum {
    SV_SELECT,
    SV_CONFIRM,
    SV_BUSY,
    SV_RESULT
};

enum {
    SV_ACT_SAVE,
    SV_ACT_LOAD
};

enum {
    SV_REQ_NONE,
    SV_REQ_SCAN,
    SV_REQ_WRITE,
    SV_REQ_LOAD
};

enum {
    SV_BOOT_CORRUPT_SYSTEM,
    SV_BOOT_CORRUPT_CART,
    SV_BOOT_NO_SPACE
};

static SAVE_DATA svSlotData[SV_SLOTS];
static SAVE_DATA svWriteData;
static Uint8 svSlotState[SV_SLOTS];
static Uint8 svDevState;
static Uint8 svHasSpace;

static Uint8 svMode;
static Uint8 svDevice = 0;
static Uint8 svCursor;
static Uint8 svStep;
static Uint8 svAction;
static Uint8 svYes;
static volatile Uint8 svRequest;
static Uint8 svLastOk;
static PadBits svPadPrev;
static SPRITE_INFO svSprites[SV_SLOTS * 4];

static Uint8 svBootMsgs[SV_BOOT_MSG_MAX];
static Uint8 svBootMsgCount;
static Uint8 svBootMsgIndex;
static BkStat svBootStat[SV_DEVICES];

static void svFileName(Uint8 *name, Uint8 slot) {
    strcpy((char *)name, "FINALFAN_00");
    name[9] = '0' + (slot + 1) / 10;
    name[10] = '0' + (slot + 1) % 10;
}

static Uint32 svChecksum(SAVE_DATA *d) {
    Uint8 *p = (Uint8 *)d;
    Uint32 sum = 0;

    for(Uint32 i = 0; i < offsetof(SAVE_DATA, checksum); i++) {
        sum += p[i];
    }
    for(Uint32 i = 0; i < BUGFIX_BYTES; i++) {
        sum += d->bugFixes[i];
    }
    sum += d->gfxMsx;
    sum += d->sndMsx;
    return sum;
}

static Uint8 svValid(SAVE_DATA *d) {
    return memcmp(d->magic, "FF1S", 4) == 0 && d->version == SV_VERSION && d->checksum == svChecksum(d);
}

static void svCapture(SAVE_DATA *d) {
    memset(d, 0, sizeof(SAVE_DATA));
    memcpy(d->magic, "FF1S", 4);
    d->version = SV_VERSION;
    d->vehicle = getOWVehicle();
    d->owX = current_x_pos;
    d->owY = current_y_pos;
    d->gold = partyGold;
    memcpy(d->items, items, sizeof(items));
    memcpy(d->gameFlags, gameFlags, sizeof(gameFlags));
    d->shipVisible = shipVisible;
    d->shipX = shipX;
    d->shipY = shipY;
    d->airshipVisible = airshipVisible;
    d->airshipX = airshipX;
    d->airshipY = airshipY;
    d->bridgeVisible = bridgeVisible;
    d->bridgeX = bridgeX;
    d->bridgeY = bridgeY;
    d->canalVisible = canalVisible;
    d->canalX = canalX;
    d->canalY = canalY;
    d->hasCanoe = hasCanoe;
    d->bridgeScene = bridgeScene;
    memcpy(d->party, party, sizeof(party));
    memcpy(d->bugFixes, bugFixes, sizeof(bugFixes));
    d->gfxMsx = gfxMsx;
    d->sndMsx = sndMsx;
    d->checksum = svChecksum(d);
}

static void svApply(SAVE_DATA *d) {
    partyGold = d->gold;
    memcpy(items, d->items, sizeof(items));
    memcpy(gameFlags, d->gameFlags, sizeof(gameFlags));
    shipVisible = d->shipVisible;
    shipX = d->shipX;
    shipY = d->shipY;
    airshipVisible = d->airshipVisible;
    airshipX = d->airshipX;
    airshipY = d->airshipY;
    bridgeVisible = d->bridgeVisible;
    bridgeX = d->bridgeX;
    bridgeY = d->bridgeY;
    canalVisible = d->canalVisible;
    canalX = d->canalX;
    canalY = d->canalY;
    hasCanoe = d->hasCanoe;
    bridgeScene = d->bridgeScene;
    memcpy(party, d->party, sizeof(party));
    memcpy(bugFixes, d->bugFixes, sizeof(bugFixes));
    gfxMsx = d->gfxMsx && msxAvailable;
    //Switched by svService once the slave SH-2 gives back the audio frames.
    svSndMsx = d->sndMsx && musicMsxAvailable();
    setOWPosition(d->owX, d->owY);
    setOWVehicle(d->vehicle);
}

#ifdef FF1_JP
//UTF-8 to the Japanese ROM's text codes.
static void svText(Uint8 *dst, const char *src) {
    while(*src) {
        *dst++ = jpTextCode(&src);
    }
    *dst = 0x00;
}
#else
//ASCII to the NES menu font.
static void svText(Uint8 *dst, const char *src) {
    for(; *src; src++, dst++) {
        char ch = *src;
        if(ch >= 'A' && ch <= 'Z') {
            *dst = 0x8A + (ch - 'A');
        } else if(ch >= 'a' && ch <= 'z') {
            *dst = 0xA4 + (ch - 'a');
        } else if(ch >= '0' && ch <= '9') {
            *dst = 0x80 + (ch - '0');
        } else if(ch == '.') {
            *dst = 0xC0;
        } else if(ch == '!') {
            *dst = 0xC4;
        } else if(ch == '?') {
            *dst = 0xC5;
        } else if(ch == '/') {
            *dst = 0x7A;
        } else if(ch == ',') {
            *dst = 0xBF;
        } else {
            *dst = 0xFF;
        }
    }
    *dst = 0x00;
}
#endif

static void svDrawText(Uint8 x, Uint8 y, const char *str) {
    Uint8 buffer[32];

    svText(buffer, str);
    drawComplexString(x, y, buffer, SV_PAL);
}

//Yes and No share the line, so a question can only be 10 characters.
static void svMessage(const char *line, Uint8 yesNo) {
    drawBox(SV_MSG_X, SV_TOP_Y, SV_MSG_W, SV_TOP_H, SV_PAL);
    svDrawText(SV_MSG_TEXT_X, SV_TEXT_Y, line);
    if(yesNo) {
        svDrawText(SV_YES_X, SV_TEXT_Y, SV_TXT("Yes", "はい"));
        svDrawText(SV_NO_X, SV_TEXT_Y, SV_TXT("No", "いいえ"));
    }
}

static void svDefaultMessage() {
    svMessage(svDeviceNames[svDevice], FALSE);
}

static Uint8 svSlotTop(Uint8 slot) {
    return SV_SLOT_Y + slot * SV_SLOT_H;
}

static void svDrawCharacter(PLAYER_CHARACTER *c, Uint8 x, Uint8 y) {
    Uint8 buffer[12];

    memcpy(buffer, c->name, 6);
    buffer[6] = 0x00;
    drawComplexString(x, y, buffer, SV_PAL);
    svDrawText(x + 7, y, SV_TXT("LV", "L"));
    formatNumber(buffer, c->level, 2);
    drawComplexString(x + SV_TXT(9, 8), y, buffer, SV_PAL);

    svDrawText(x, y + 1, "HP");
    formatNumber(buffer, c->current_hp, 3);
    buffer[3] = 0x7A;
    formatNumber(buffer + 4, c->max_hp, 3);
    drawComplexString(x + 3, y + 1, buffer, SV_PAL);
}

static void svDrawSlot(Uint8 slot) {
    Uint8 top = svSlotTop(slot);
    const char *text = 0;

    drawBox(0, top, SV_SLOT_W, SV_SLOT_H, SV_PAL);
    if(svDevState == DEV_NONE) {
        text = SV_TXT("Not connected", "せつぞく されていません");
    } else if(svDevState == DEV_UNFORMAT) {
        text = SV_TXT("Not formatted", "フォーマット されていません");
    } else if(svSlotState[slot] == SLOT_BROKEN) {
        text = SV_TXT("This file cannot be used", "このデータは つかえません");
    } else if(svSlotState[slot] == SLOT_EMPTY) {
        text = (svMode == SV_MODE_SAVE && !svHasSpace) ? SV_TXT("Not enough space", "ようりょうが たりません") : SV_TXT("Empty", "データなし");
    }
    if(text) {
        svDrawText(3, top + 3, text);
        return;
    }

    SAVE_DATA *d = &svSlotData[slot];
    Uint8 buffer[10];
    for(Uint8 i = 0; i < 4; i++) {
        svDrawCharacter(&d->party[i], ((i & 1) ? SV_RIGHT_COL : 1) + SV_TEXT_COL, top + 1 + SV_NAME_Y + (i >> 1) * 3);
    }
    formatNumber(buffer, d->gold, 6);
#ifdef FF1_JP
    //"ギル"
    buffer[6] = 0x5D;
    buffer[7] = 0xF2;
#else
    buffer[6] = 0xFF;
    buffer[7] = 0x90;
#endif
    buffer[8] = 0x00;
    drawComplexString(SV_SLOT_W - 9, top + SV_GOLD_Y, buffer, SV_PAL);
}

static void svDrawSlots() {
    for(Uint8 i = 0; i < SV_SLOTS; i++) {
        svDrawSlot(i);
    }
}

static Uint8 svSlotSelectable(Uint8 slot) {
    if(svDevState != DEV_OK) {
        return FALSE;
    }
    if(svMode == SV_MODE_LOAD) {
        return svSlotState[slot] == SLOT_DATA;
    }
    return svSlotState[slot] != SLOT_EMPTY || svHasSpace;
}

//Reads the device's status and each slot's file. Runs from the main loop.
//The BUP unit for a device, or SV_NO_UNIT. Each floppy side is a partition, selected before every access
//like Sega's SMPBUP1 sample does.
static Uint8 svUnit(Uint8 dev) {
    Uint8 unit = svDeviceUnit[dev];

    if(!bkUnitPresent(unit)) {
        return SV_NO_UNIT;
    }
    if(unit == SV_FLOPPY_UNIT) {
        if(bkPartitions(unit) <= svDevicePart[dev]) {
            return SV_NO_UNIT;
        }
        bkSelectPartition(unit, svDevicePart[dev]);
    }
    return unit;
}

static void svScan() {
    BkStat stat;
    Uint8 name[12];
    Uint8 unit = svUnit(svDevice);

    for(Uint8 i = 0; i < SV_SLOTS; i++) {
        svSlotState[i] = SLOT_EMPTY;
    }
    svHasSpace = FALSE;
    if(unit == SV_NO_UNIT) {
        svDevState = DEV_NONE;
        return;
    }
    Uint8 ret = bkStat(unit, sizeof(SAVE_DATA), &stat);
    if(ret == BK_UNFORMAT) {
        svDevState = DEV_UNFORMAT;
        return;
    }
    if(ret != BK_OK) {
        svDevState = DEV_NONE;
        return;
    }
    svDevState = DEV_OK;
    svHasSpace = stat.datanum > 0;
    for(Uint8 i = 0; i < SV_SLOTS; i++) {
        svFileName(name, i);
        Sint32 size = bkFind(unit, (char *)name);
        if(size < 0) {
            continue;
        }
        svSlotState[i] = SLOT_BROKEN;
        memset(&svSlotData[i], 0, sizeof(SAVE_DATA));
        if((size == sizeof(SAVE_DATA) || size == SV_GFX_SIZE || size == SV_BUGFIX_SIZE || size == SV_OLD_SIZE)
           && bkRead(unit, (char *)name, &svSlotData[i])
           && svValid(&svSlotData[i])) {
            svSlotState[i] = SLOT_DATA;
        }
    }
}

static Uint8 svWrite() {
    Uint8 name[12];
    Uint8 unit = svUnit(svDevice);

    if(unit == SV_NO_UNIT) {
        return FALSE;
    }
    svFileName(name, svCursor);
    svCapture(&svWriteData);
    return bkWrite(unit, (char *)name, "FINAL FANT", &svWriteData, sizeof(SAVE_DATA));
}

static Uint8 svLoad() {
    Uint8 name[12];
    Uint8 unit = svUnit(svDevice);

    svFileName(name, svCursor);
    memset(&svWriteData, 0, sizeof(SAVE_DATA));
    if(unit == SV_NO_UNIT || !bkRead(unit, (char *)name, &svWriteData) || !svValid(&svWriteData)) {
        return FALSE;
    }
    svApply(&svWriteData);
    return TRUE;
}

void saveInit() {
    bkInit();
}

//Saving is offered on the overworld and in towns, the maps using tileset 0.
Uint8 saveAllowed() {
    return !menuReturnSM || cur_tileset == 0;
}

static Uint8 svUnitHasSave(Uint8 unit) {
    Uint8 name[12];

    for(Uint8 i = 0; i < SV_SLOTS; i++) {
        svFileName(name, i);
        if(bkFind(unit, (char *)name) >= 0) {
            return TRUE;
        }
    }
    return FALSE;
}

//Blocks one save file takes. Backup memory's free bytes are 6 less per block and 30 less per file
//than the free blocks, so n blocks hold n * (blocksize - 6) - 30 bytes. Unformatted System Memory uses its 64 byte blocks.
static Uint32 svBlocksNeeded(Uint8 dev) {
    Uint32 blocksize = svBootStat[dev].blocksize ? svBootStat[dev].blocksize : 64;
    Uint32 room = blocksize - 6;

    return (sizeof(SAVE_DATA) + 30 + room - 1) / room;
}

//Finds unformatted memory, and whether a save is possible or one already exists.
void saveBootCheck() {
    Uint8 canSave = FALSE;

    svBootMsgCount = 0;
    svBootMsgIndex = 0;
    bkResetDisable();
    for(Uint8 dev = 0; dev < SV_DEVICES; dev++) {
        Uint8 unit = svUnit(dev);

        svBootStat[dev].blocksize = 0;
        if(unit == SV_NO_UNIT) {
            continue;
        }
        Uint8 ret = bkStat(unit, sizeof(SAVE_DATA), &svBootStat[dev]);
        if(ret == BK_UNFORMAT) {
            svBootStat[dev].blocksize = 0;
            if(dev == 0) {
                svBootMsgs[svBootMsgCount++] = SV_BOOT_CORRUPT_SYSTEM;
            } else if(dev == 1) {
                svBootMsgs[svBootMsgCount++] = SV_BOOT_CORRUPT_CART;
            }
            continue;
        }
        if(ret != BK_OK) {
            svBootStat[dev].blocksize = 0;
            continue;
        }
        if(svBootStat[dev].datanum > 0 || svUnitHasSave(unit)) {
            canSave = TRUE;
        }
    }
    bkResetEnable();
    if(!canSave) {
        svBootMsgs[svBootMsgCount++] = SV_BOOT_NO_SPACE;
    }
}

Uint8 saveBootMessagePending() {
    return svBootMsgIndex < svBootMsgCount;
}

static void svBlocksLine(char *dst, const char *prefix, Uint32 blocks, const char *rest) {
    char digits[10];
    Uint8 n = 0;

    strcpy(dst, prefix);
    dst += strlen(prefix);
    do {
        digits[n++] = '0' + (blocks % 10);
        blocks /= 10;
    } while(blocks);
    while(n) {
        *dst++ = digits[--n];
    }
    strcpy(dst, rest);
}

void setupBootMessage() {
    const char *lines[12];
    char blocks[2][64];
    Uint8 count = 0;
    Uint8 msg = svBootMsgs[svBootMsgIndex];

    setBoxPalette(menu_pal_0, boarderPalette_blue, MSX_BOX_MENU);
    clearMenuPlane();
    drawBox(0, SV_BOOT_BOX_Y, SV_SLOT_W, SV_BOOT_BOX_H, SV_PAL);
#ifdef FF1_JP
    if(msg == SV_BOOT_NO_SPACE) {
        Uint8 cart = svBootStat[1].blocksize != 0;

        lines[count++] = "セーブに ひつような メモリの";
        lines[count++] = "ようりょうが たりません。";
        lines[count++] = "このまま あそべますが";
        lines[count++] = "セーブは できません。";
        lines[count++] = "";
        lines[count++] = "セーブするには メモリマネージャーで";
        if(cart) {
            svBlocksLine(blocks[0], "ほんたいメモリを ", svBlocksNeeded(0), "ブロック または");
            svBlocksLine(blocks[1], "カートリッジメモリを ", svBlocksNeeded(1), "ブロック");
            lines[count++] = blocks[0];
            lines[count++] = blocks[1];
        } else {
            svBlocksLine(blocks[0], "ほんたいメモリを ", svBlocksNeeded(0), "ブロック");
            lines[count++] = blocks[0];
        }
        lines[count++] = "あけてください。";
    } else {
        lines[count++] = (msg == SV_BOOT_CORRUPT_SYSTEM) ? "ほんたいメモリが こわれています。" : "カートリッジメモリが こわれています。";
        lines[count++] = "フォーマットが ひつようです。";
        lines[count++] = "";
        lines[count++] = "サターンを メモリマネージャーで きどうして";
        lines[count++] = "メモリを しょきか してください。";
    }
#else
    if(msg == SV_BOOT_NO_SPACE) {
        Uint8 cart = svBootStat[1].blocksize != 0;

        lines[count++] = "There is not enough memory";
        lines[count++] = "to save the game.";
        lines[count++] = "";
        lines[count++] = "You can still play like this,";
        lines[count++] = "but understand you will not";
        lines[count++] = "be able to save your";
        lines[count++] = "progress.";
        lines[count++] = "";
        lines[count++] = "To be able to save, free up";
        if(cart) {
            svBlocksLine(blocks[0], "", svBlocksNeeded(0), " blocks of System Memory or");
            svBlocksLine(blocks[1], "", svBlocksNeeded(1), " blocks of Cartridge Memory");
            lines[count++] = blocks[0];
            lines[count++] = blocks[1];
            lines[count++] = "in the Memory Manager.";
        } else {
            svBlocksLine(blocks[0], "", svBlocksNeeded(0), " blocks of System Memory in");
            lines[count++] = blocks[0];
            lines[count++] = "the Memory Manager.";
        }
    } else {
        lines[count++] = (msg == SV_BOOT_CORRUPT_SYSTEM) ? "System Memory is corrupt" : "Cartridge Memory is corrupt";
        lines[count++] = "and needs to be formatted.";
        lines[count++] = "";
        lines[count++] = "To format it, boot the";
        lines[count++] = "Saturn into the Memory";
        lines[count++] = "Manager and clear the memory.";
    }
#endif
    for(Uint8 i = 0; i < count; i++) {
        svDrawText(SV_BOOT_TEXT_X, SV_BOOT_TEXT_Y + i * SV_BOOT_LINE_H, lines[i]);
    }
    svDrawText(SV_BOOT_TEXT_X, SV_BOOT_BOX_Y + SV_BOOT_BOX_H - 3, SV_TXT("Press a button.", "ボタンを おしてください。"));
    svPadPrev = PadData1;
}

void doBootMessage() {
    PadBits pressed = PadData1 & ~svPadPrev;
    svPadPrev = PadData1;

    if(pressed & (PAD_A | PAD_B | PAD_C | PAD_S)) {
        sfxMenuSel();
        svBootMsgIndex++;
        state = saveBootMessagePending() ? TO_BOOT_MSG : TO_PRELUDE;
    }
}

void saveOpen(Uint8 mode) {
    svMode = mode;
}

void setupSaveScreen() {
    setBoxPalette(menu_pal_0, boarderPalette_blue, MSX_BOX_MENU);
    drawBox(SV_LABEL_X, SV_TOP_Y, SV_LABEL_W, SV_TOP_H, SV_PAL);
    svDrawText(SV_LABEL_X + 1, SV_TEXT_Y, (svMode == SV_MODE_SAVE) ? SV_TXT("SAVE", "セーブ") : SV_TXT("LOAD", "ロード"));
    drawBox(SV_MSG_X, SV_TOP_Y, SV_MSG_W, SV_TOP_H, SV_PAL);
    for(Uint8 i = 0; i < SV_SLOTS; i++) {
        drawBox(0, svSlotTop(i), SV_SLOT_W, SV_SLOT_H, SV_PAL);
        sprite_make(SV_FRAME_BATTLE_START, 0, 0, (Sint32) CHARACTER_PRIORITY, &svSprites[i * 4]);
        sprite_make(SV_FRAME_BATTLE_START, 0, 0, (Sint32) CHARACTER_PRIORITY, &svSprites[i * 4 + 1]);
        sprite_make(SV_FRAME_BATTLE_START, 0, 0, (Sint32) CHARACTER_PRIORITY, &svSprites[i * 4 + 2]);
        sprite_make(SV_FRAME_BATTLE_START, 0, 0, (Sint32) CHARACTER_PRIORITY, &svSprites[i * 4 + 3]);
    }
    player_cursor_init(0, 0);
    svCursor = 0;
    svLastOk = FALSE;
    svPadPrev = PadData1;
    svStep = SV_BUSY;
    svRequest = SV_REQ_SCAN;
}

static void svBusy(Uint8 request, const char *line) {
    svMessage(line, FALSE);
    svStep = SV_BUSY;
    svRequest = request;
}

static void svConfirm(Uint8 action, const char *line) {
    svAction = action;
    svYes = TRUE;
    svMessage(line, TRUE);
    svStep = SV_CONFIRM;
}

static void svSelectSlot() {
    if(!svSlotSelectable(svCursor)) {
        sfxError();
        return;
    }
    sfxMenuSel();
    if(svMode == SV_MODE_LOAD) {
        svConfirm(SV_ACT_LOAD, SV_TXT("Load file?", "ロードしますか?"));
    } else if(svSlotState[svCursor] == SLOT_EMPTY) {
        svConfirm(SV_ACT_SAVE, SV_TXT("Save here?", "セーブしますか?"));
    } else {
        svConfirm(SV_ACT_SAVE, SV_TXT("Overwrite?", "うわがきしますか?"));
    }
}

static void svExit() {
    state = (svMode == SV_MODE_SAVE) ? TO_MAIN_MENU : TO_TITLE_STATE;
}

void doSaveMenu() {
    PadBits pressed = PadData1 & ~svPadPrev;
    svPadPrev = PadData1;

    switch(svStep) {
        case SV_SELECT:
            if(pressed & (PAD_A | PAD_C)) {
                svSelectSlot();
            } else if(pressed & PAD_B) {
                sfxMenuSel();
                svExit();
            } else if(pressed & PAD_U) {
                sfxMenuMove();
                svCursor = svCursor ? svCursor - 1 : SV_SLOTS - 1;
            } else if(pressed & PAD_D) {
                sfxMenuMove();
                svCursor = (svCursor + 1 < SV_SLOTS) ? svCursor + 1 : 0;
            } else if(pressed & (PAD_LB | PAD_RB | PAD_L | PAD_R)) {
                sfxMenuMove();
                if(pressed & (PAD_RB | PAD_R)) {
                    svDevice = (svDevice + 1 < SV_DEVICES) ? svDevice + 1 : 0;
                } else {
                    svDevice = svDevice ? svDevice - 1 : SV_DEVICES - 1;
                }
                svBusy(SV_REQ_SCAN, svDeviceNames[svDevice]);
            }
            break;
        case SV_CONFIRM:
            if(pressed & (PAD_L | PAD_R)) {
                sfxMenuMove();
                svYes = !svYes;
            } else if(pressed & PAD_B || ((pressed & (PAD_A | PAD_C)) && !svYes)) {
                sfxMenuSel();
                svDefaultMessage();
                svStep = SV_SELECT;
            } else if(pressed & (PAD_A | PAD_C)) {
                sfxMenuSel();
                if(svAction == SV_ACT_LOAD) {
                    svBusy(SV_REQ_LOAD, SV_TXT("Loading.", "ロードちゅう"));
                } else {
                    svBusy(SV_REQ_WRITE, SV_TXT("Saving. Keep power on.", "セーブちゅう。でんげんを きらないで"));
                }
            }
            break;
        case SV_RESULT:
            if(pressed & (PAD_A | PAD_B | PAD_C)) {
                sfxMenuSel();
                if(svAction == SV_ACT_SAVE && svLastOk) {
                    svExit();
                } else {
                    svDefaultMessage();
                    svStep = SV_SELECT;
                }
            }
            break;
    }
}

static void svResult(const char *line) {
    svMessage(line, FALSE);
    svStep = SV_RESULT;
}

//Backup memory is only touched from the main loop, with the reset button and soft reset held off.
static void svService() {
    Uint8 request = svRequest;
    Uint8 ok = TRUE;

    saveBusy = TRUE;
    bkResetDisable();
    audio_slave_begin();
    switch(request) {
        case SV_REQ_WRITE:
            ok = svWrite();
            break;
        case SV_REQ_LOAD:
            ok = svLoad();
            break;
    }
    if(request != SV_REQ_LOAD || !ok) {
        svScan();
    }
    audio_slave_end();
    bkResetEnable();
    saveBusy = FALSE;

    svRequest = SV_REQ_NONE;
    if(request == SV_REQ_LOAD && ok) {
        if(svSndMsx != sndMsx) {
            musicSetMsx(svSndMsx);
        }
        //Overworld loading takes many frames, and the title music would hold its last notes.
        musicApuWrite(0x4015, 0);
        if(sndMsx) {
            music_track = MUSIC_ENDED;
        }
        menuReturnSM = FALSE;
        owWipeIn = TRUE;
        state = TO_OW_STATE;
        return;
    }
    svDrawSlots();
    svLastOk = ok;
    switch(request) {
        case SV_REQ_SCAN:
            svDefaultMessage();
            svStep = SV_SELECT;
            break;
        case SV_REQ_WRITE:
            if(ok) {
                //NES SaveGame jingle, started with square 2 silenced.
                music_track = MUSIC_SAVE;
                musicApuWrite(0x4004, 0x30);
                musicApuWrite(0x4005, 0x7F);
                musicApuWrite(0x4006, 0);
                musicApuWrite(0x4007, 0);
                svResult(SV_TXT("Save complete!", "セーブしました!"));
            } else {
                sfxError();
                svResult(SV_TXT("Save failed. Try again.", "セーブできませんでした。"));
            }
            break;
        case SV_REQ_LOAD:
            sfxError();
            svResult(SV_TXT("Load failed.", "ロードできませんでした。"));
            break;
    }
}

//Dead characters aren't drawn, like the main menu.
static void svDrawSprite(SPRITE_INFO *sprite, PLAYER_CHARACTER *c, Uint16 x, Uint16 y) {
    if(c->ailment == AIL_DEAD) {
        return;
    }
    sprite->char_num = SV_FRAME_BATTLE_START + (c->char_class * 6);
    sprite->xPos = FIXED(x);
    sprite->yPos = FIXED(y);
    if(c->ailment == AIL_STONE) {
        drawStoneSprite(sprite, c->char_class);
        return;
    }
    if(c->ailment == AIL_POISON) {
        sprite->char_num += 4;
    }
    sprite_draw(sprite);
}

void save_draw() {
    if(svRequest != SV_REQ_NONE) {
        svService();
        return;
    }
    if(svDevState == DEV_OK) {
        for(Uint8 s = 0; s < SV_SLOTS; s++) {
            if(svSlotState[s] != SLOT_DATA) {
                continue;
            }
            for(Uint8 i = 0; i < 4; i++) {
                Uint8 col = (i & 1) ? SV_RIGHT_COL : 1;
                Uint8 row = svSlotTop(s) + 1 + (i >> 1) * 3;
                svDrawSprite(&svSprites[s * 4 + i], &svSlotData[s].party[i], (col + CENTER_ADJUSTMENT) * 8, row * 8);
            }
        }
    }
    if(svStep == SV_SELECT) {
        cursor.xPos = FIXED((CENTER_ADJUSTMENT - 2) * 8);
        cursor.yPos = FIXED((svSlotTop(svCursor) + SV_SLOT_H / 2) * 8 - 8);
        player_cursor_draw();
    } else if(svStep == SV_CONFIRM) {
        cursor.xPos = FIXED(((svYes ? SV_YES_X : SV_NO_X) + CENTER_ADJUSTMENT - 2) * 8);
        cursor.yPos = FIXED(SV_TEXT_Y * 8 - 4);
        player_cursor_draw();
    }
}
