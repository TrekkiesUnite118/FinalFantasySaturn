#include "types.h"
#include "video.h"

#include "player.h"
#include "menu.h"
#include "magic.h"
#include "stdmap.h"
#include "dialog.h"
#include "bugfix.h"

extern Uint8 lut_DTE1[];
extern Uint8 lut_DTE2[];

//NES coords of the box (8x8 tiles) and of the start of its text.
#define DLG_BOX_X (2)
#define DLG_BOX_Y (2)
#define DLG_BOX_WIDTH (28)
#define DLG_BOX_HEIGHT (11)
#define DLG_TEXT_X (4)
#define DLG_TEXT_Y (4)

//Screen x of the box's columns, NES x 16-239.
#define DLG_WIN_X_START (56)
#define DLG_WIN_X_END (279)

static Uint8 dlgItemId; //NES dlg_itemid, the item named by code $02.
static Uint8 dlgX;
static Uint8 dlgY;
static Uint8 dlgStall; //NES tmp+7, DTE codes until the next frame stall.
//DTE codes, or Japanese kana with marks, between frame stalls.
#ifdef FF1_JP
#define DLG_STALL (5)
#else
#define DLG_STALL (10)
#endif
static Uint8 dlgFrames;

//NES AddGPToParty. The bytes are compared separately, so a high byte over the max
//with a middle byte under it isn't capped.
void dlgAddGold(Uint16 gp) {
    Uint32 gold = (partyGold + gp) & 0xFFFFFF;
    Uint8 hi = gold >> 16;
    Uint8 mid = (gold >> 8) & 0xFF;
    Uint8 lo = gold & 0xFF;

    if(hi >= 0x0F && (mid > 0x42 || (mid == 0x42 && lo >= 0x40))) {
        gold = 999999;
    } else if(BUGFIX_ON(BUGFIX_GOLD_CAP) && gold > 999999) {
        gold = 999999;
    }
    partyGold = gold;
}

//NES FindEmptyWeaponSlot/FindEmptyArmorSlot.
Uint8 *dlgFindEmptySlot(Uint8 armor) {
    for(Uint8 c = 0; c < 4; c++) {
        Uint8 *slots = armor ? party[c].armors : party[c].weapons;
        for(Uint8 i = 0; i < 4; i++) {
            if(slots[i] == 0) {
                return &slots[i];
            }
        }
    }
    return 0;
}

//NES OpenTreasureChest. Returns the dialogue ID.
Uint8 dlgOpenTreasureChest(Uint8 chest) {
    Uint8 id = ((Uint8 *)LUT_TREASURE)[chest];

    dlgItemId = id;
    if(id < TCITYPE_WEPSTART) {
        if(items[id] < 99) {
            items[id]++;
        } else if(BUGFIX_ON(BUGFIX_FULL_CHEST)) {
            return DLGID_CANTCARRY;
        }
        //Marked open even when the item wasn't given.
        gameFlags[chest] |= GMFLG_TCOPEN;
        //Key items get the fanfare.
        dlgSfx += (id >= ITEM_TENT) ? 2 : 1;
        return DLGID_TCGET;
    }
    if(id < TCITYPE_GPSTART) {
        Uint8 armor = (id >= TCITYPE_ARMSTART);
        Uint8 *slot = dlgFindEmptySlot(armor);
        if(!slot) {
            return DLGID_CANTCARRY;
        }
        *slot = id - (armor ? TCITYPE_ARMSTART : TCITYPE_WEPSTART) + 1;
    } else {
        Uint8 *price = (Uint8 *)LUT_ITEM_PRICES + id * 2;
        dlgAddGold(price[0] | (price[1] << 8));
    }
    gameFlags[chest] |= GMFLG_TCOPEN;
    dlgSfx += 2;
    return DLGID_TCGET;
}

static void dlgPutTile(Uint8 tile) {
    drawPlane[dlgY][dlgX + CENTER_ADJUSTMENT] = (DLG_BOX_PAL << 12) | tile;
    dlgX++;
}

//NES DrawDialogueString. Counts the frames the NES stalls for.
static void dlgDrawText(Uint8 *str) {
    Uint8 ch;

    while((ch = *str++) != 0x00) {
#ifdef FF1_JP
        if(ch >= 0x48) {
            //Kana with a mark are drawn on two rows, and stall like DTE codes.
            if(jpKanaMark(ch) != 0xFF) {
                drawPlane[dlgY - 1][dlgX + CENTER_ADJUSTMENT] = (DLG_BOX_PAL << 12) | jpKanaMark(ch);
                if(--dlgStall == 0) {
                    dlgStall = DLG_STALL;
                    dlgFrames++;
                }
            }
            dlgPutTile(jpKanaBase(ch));
        } else if(ch == 0x01) {
            dlgStall = DLG_STALL;
            dlgFrames++;
            dlgX = DLG_TEXT_X;
            dlgY += 2;
#else
        if(ch >= 0x7A) {
            dlgPutTile(ch);
        } else if(ch >= 0x1A) {
            dlgPutTile(lut_DTE1[ch - 0x1A]);
            dlgPutTile(lut_DTE2[ch - 0x1A]);
            if(--dlgStall == 0) {
                dlgStall = DLG_STALL;
                dlgFrames++;
            }
#endif
        } else if(ch == 0x02) {
            dlgDrawText(getItemName(dlgItemId));
        } else if(ch == 0x03) {
            //The NES points the text at the lead's name, which the disassembly notes ends the string.
            for(Uint8 i = 0; i < 6 && party[0].name[i] != 0x00; i++) {
                dlgPutTile(party[0].name[i]);
            }
            return;
        } else {
            //The other codes are a line break, or nothing on the Japanese ROM.
#ifndef FF1_JP
            dlgStall = 8;
            dlgFrames++;
            dlgX = DLG_TEXT_X;
            dlgY++;
#endif
        }
    }
}

//NES DrawDialogueBox/DrawDialogueString into the NBG2 plane. Returns the frames the NES spends drawing.
Uint8 dlgDraw(Uint8 dlgId) {
    Uint8 *ptr = (Uint8 *)LUT_DIALOGUE_PTR + dlgId * 2;
    Uint16 (*saved)[64] = drawPlane;

    drawPlane = (Uint16 (*)[64])SM_NBG2_PN;
    drawBox(DLG_BOX_X, DLG_BOX_Y, DLG_BOX_WIDTH, DLG_BOX_HEIGHT, DLG_BOX_PAL);
    dlgX = DLG_TEXT_X;
    dlgY = DLG_TEXT_Y;
    dlgStall = DLG_STALL;
    dlgFrames = 1;
    dlgDrawText((Uint8 *)DIALOGUE_BANK_ADD + (ptr[0] | (ptr[1] << 8)));
    drawPlane = saved;

    //Each of the 7 rows of 16x16 tiles takes 2 frames.
    return 14 + dlgFrames;
}

//NES DialogueBox_Frame. The box shows from NES scanline 16 to 8 + lines, and the map is hidden
//behind it so the backdrop shows through the box's color 0 like on the NES. Below 10 lines nothing shows.
void dlgSetVisible(Uint8 lines) {
    if(lines < 10) {
        //A start line that's never reached leaves the window empty.
        vidSetWindow(VID_W0, 0, VID_NBG2, VID_NBG2, 0, 0x1FF, 0, 0x1FF);
    } else {
        vidSetWindow(VID_W0, 0, VID_NBG2, VID_NBG2, DLG_WIN_X_START, 15, DLG_WIN_X_END, lines + 6);
    }
}
