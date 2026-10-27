#include <string.h>
#include "types.h"

#include "cd.h"
#include "graphicrefs.h"
#include "nesutils.h"
#include "player.h"
#include "sprdata.h"
#include "menu.h"
#include "battle.h"
#include "magic.h"
#include "vblank.h"
#include "stdmap.h"
#include "mapobj.h"
#include "dialog.h"
#include "talk.h"
#include "overworld.h"
#include "rbgscroll.h"
#include "wipe.h"
#include "music.h"
#include "mainmenu.h"
#include "video.h"
#include "nbgscroll.h"
#include "msxgfx.h"

//MAIN.COM only hides room interiors from map 8 on (its map number 9), past the towns.
#define SM_MSX_FIRST_ROOM_MAP (8)

//Screen position of the player tile, matching the overworld.
#define SM_VIEW_X (152)
#define SM_VIEW_Y (111)

static SM_STACK_ENTRY smStack[SM_STACK_MAX];
static Uint8 smStackDepth = 0;
static Uint8 smFacing; //NES facing: all the direction bits held, lowest bit takes priority.
static Uint8 smMoving;
static Uint8 smCtrX; //NES move_ctr_x/y, pixels into the step.
static Uint8 smCtrY;
static Uint8 smTileprop[2]; //Properties of the tile last moved to or tested.
static Uint8 smGray; //NES $2001 monochrome bit.
static Uint8 smEmphasis = 0; //NES $2001 emphasis bits, for the battle transition.
static Uint8 smEnteringShop;
static Uint8 smKeepMap = FALSE; //Reenter the map without reloading it.
static Uint8 smPalCycColors[16]; //NES tmp_pal.
static Uint8 smPalCycling; //smUploadPalettes shows smPalCycColors.
static Uint8 smPalCycReverse;
static Uint8 smPalCycFrame;
static Uint8 smPalCycNext;
static Uint8 smFadeStep; //The MSX fade that stands in for the palette cycle.
static Uint8 smBattleFade; //The MSX fade that stands in for the battle transition emphasis.

#define MAPTILE_CLOSEDDOOR (0x36)
#define MAPTILE_OPENDOOR (0x37)
#define MAPTILE_LOCKEDDOOR (0x3B)

//NES nametable model, for door graphics. The NES only writes them to the nametable,
//so they last until map streaming redraws that nametable cell.
#define SM_DOOR_CELLS (4)

typedef struct {
    Uint8 used;
    Uint8 ntX; //Nametable column 0-31, row 0-14.
    Uint8 ntY;
    Uint8 mapX;
    Uint8 mapY;
} SM_DOOR_CELL;

typedef struct {
    Uint8 pending;
    Uint8 column;
    Uint8 ntX; //First column of a row, or the column.
    Uint8 ntY;
} SM_DRAW_JOB;

static SM_DOOR_CELL smDoorCells[SM_DOOR_CELLS];
static SM_DRAW_JOB smDrawJob;
static Uint8 smNtScrollY; //NES scroll_y, the nametable row at the top of the screen.
static Uint8 smDoorNtX = 0; //NES doorppuaddr. Never reset, so a close can land on another cell.
static Uint8 smDoorNtY = 0;

//NES ProcessSMInput/ShowDialogueBox steps.
enum {
    SM_DLG_NONE,
    SM_DLG_WAIT, //Drawing the box and text offscreen.
    SM_DLG_OPEN,
    SM_DLG_WAIT_LIFT, //Waiting for directions to be released.
    SM_DLG_WAIT_PRESS, //Waiting for A or a direction.
    SM_DLG_WAIT_SFX, //Waiting for the jingle to end.
    SM_DLG_CLOSE
};

static Uint8 smDlgState = SM_DLG_NONE;
static Uint8 smDlgTimer;
static Uint8 smDlgLines; //NES tmp+11.
static Uint8 smDlgHide = FALSE;
static PadBits smJoyPrev; //Buttons at the last NES UpdateJoy.
static Uint8 smJoyA; //NES joy_a, an A press waiting to be handled.
static Uint8 smJoyStart;
static Uint8 smJoySelect;
static Uint8 smAltarLines; //NES DoAltarEffect tmp+1, 0 when not running.
static Uint8 smAltarDelay; //NES tmp.
static Uint8 smAltarRetract; //NES tmp+2.
static Uint8 smAltarShake; //Added to the map's X scroll.

static void smUpdateDisplay();
static void smStartPalCycle(Uint8 reverse, Uint8 nextState);
static void smWipeOpened();
static void smGetTargetCoords(Uint8 facing, Uint8 *x, Uint8 *y);

//NES EntrTele setup from DoOWTransitions.
void smTeleportIn(Uint8 teleId) {
    Uint8 *teleX = (Uint8 *)LUT_ENTR_TELE_X;
    Uint8 *teleY = (Uint8 *)LUT_ENTR_TELE_Y;
    Uint8 *teleMap = (Uint8 *)LUT_ENTR_TELE_MAP;
    Uint8 *tilesets = (Uint8 *)LUT_TILESETS;

    sm_scroll_x = (teleX[teleId] - 7) & 0x3F;
    sm_scroll_y = (teleY[teleId] - 7) & 0x3F;
    cur_map = teleMap[teleId];
    cur_tileset = tilesets[cur_map];
    inroom = 0;
    smStackDepth = 0;
}

//NES LoadStandardMap/DecompressMap.
static void smDecompress() {
    Uint8 *ptrTbl = (Uint8 *)LUT_SM_PTR_TBL;
    Uint16 ptr = ptrTbl[cur_map * 2] | (ptrTbl[cur_map * 2 + 1] << 8);
    Uint8 *src = (Uint8 *)(LUT_SM_PTR_TBL + ptr);
    Uint8 *dest = &smMap[0][0];
    Uint16 pos = 0;

    while(pos < 4096 && *src != 0xFF) {
        Uint8 tile = *src++;
        if(tile < 0x80) {
            dest[pos++] = tile;
        } else {
            Uint16 run = *src++;
            if(run == 0) {
                run = 256;
            }
            for(; run > 0 && pos < 4096; run--) {
                dest[pos++] = tile & 0x7F;
            }
        }
    }
}

//NES monochrome keeps only the brightness bits.
static Uint8 smColor(Uint8 nesColor) {
    return smGray ? (nesColor & 0x30) : nesColor;
}

//NES cur_pal, or inroom_pal in a room.
static Uint8 *smBgNesPal() {
    Uint8 *mapPal = (Uint8 *)(LUT_SM_PALETTES + cur_map * 0x30);
    return inroom ? mapPal + 0x20 : mapPal;
}

static Uint8 smMsxFadeStep() {
    return smPalCycling ? smFadeStep : smBattleFade;
}

static Uint32 smMsxSpriteColor(Uint32 color) {
    return msxFadeColor(msxShadeColor(color, smEmphasis, smGray), smMsxFadeStep(), MSX_FADE_STEPS);
}

void smUploadPalettes() {
    Uint8 *mapPal = (Uint8 *)(LUT_SM_PALETTES + cur_map * 0x30);
    Uint8 *bgPal = smPalCycling && !MSX_ON ? smPalCycColors : smBgNesPal();
    Uint32 palette[16];
    Uint8 sprPal[4];
    Uint8 classColors[2];

    //The MSX map uses one 16 color palette. Outside rooms, castles and dungeons hide room interiors in colors 12-15.
    if(MSX_ON) {
        msxMapPalette(cur_tileset, cur_map, palette);
        if(!inroom && cur_map >= SM_MSX_FIRST_ROOM_MAP) {
            msxHideRooms(palette);
        }
        for(Uint8 i = 0; i < 16; i++) {
            palette[i] = msxFadeColor(msxShadeColor(palette[i], smEmphasis, smGray), smMsxFadeStep(), MSX_FADE_STEPS);
        }
        vidSetPalette(VID_NBG0, 0, 16, palette);
    } else {
        for(Uint8 p = 0; p < 4; p++) {
            memset(palette, 0, sizeof(palette));
            for(Uint8 i = 0; i < 4; i++) {
                palette[i] = emphasizeColor(smColor(bgPal[p * 4 + i]), smEmphasis);
            }
            vidSetPalette(VID_NBG0, p * 16, 16, palette);
        }
    }
    setBattleBackColor(smColor(bgPal[0]));

    //The dialogue box uses BG palette 3.
    memset(palette, 0, sizeof(palette));
    if(MSX_ON) {
        msxBoxPalette(MSX_BOX_MAP, palette);
        for(Uint8 i = 0; i < 4; i++) {
            palette[i] = msxFadeColor(msxShadeColor(palette[i], 0, smGray), smMsxFadeStep(), MSX_FADE_STEPS);
        }
    } else {
        for(Uint8 i = 0; i < 4; i++) {
            palette[i] = nes_pal[smColor(bgPal[12 + i])];
        }
    }
    vidSetPalette(VID_NBG2, DLG_BOX_PAL * 16, 16, palette);

    //Player uses sprite palettes 0 and 1 with the class colors at $12/$16, like LoadMapPalettes.
    Uint32 palA[16] = {0};
    Uint32 palB[16] = {0};
    Uint8 *classPal = (Uint8 *)MAPMAN_SPRITE_PALETTE_OFFSET_2 + (2 * determine_curr_char_class());
    for(Uint8 i = 0; i < 4; i++) {
        sprPal[i] = smColor(mapPal[0x10 + i]);
    }
    classColors[0] = smColor(classPal[0]);
    classColors[1] = smColor(classPal[1]);
    if(MSX_ON) {
        msxMapManPalette(determine_curr_char_class(), palA, palB);
        for(Uint8 i = 1; i < 4; i++) {
            palA[i] = smMsxSpriteColor(palA[i]);
            palB[i] = smMsxSpriteColor(palB[i]);
        }
    } else {
        initMapManPalette(palA, palB, sprPal, classColors);
    }
    vidSetPalette(VID_SPR, mapSpriteCramStart, 16, palA);
    vidSetPalette(VID_SPR, mapSpriteCramStart + 16, 16, palB);

    //Map objects use sprite palettes 2 and 3. The MSX objects share one palette.
    if(MSX_ON) {
        msxSpritePalette(palette);
        for(Uint8 i = 1; i < 16; i++) {
            palette[i] = smMsxSpriteColor(palette[i]);
        }
        vidSetPalette(VID_SPR, SM_OBJ_TOP_CRAM, 16, palette);
        vidSetPalette(VID_SPR, SM_OBJ_BOTTOM_CRAM, 16, palette);
        return;
    }
    for(Uint8 p = 0; p < 2; p++) {
        memset(palette, 0, sizeof(palette));
        for(Uint8 i = 0; i < 4; i++) {
            palette[i] = nes_pal[smColor(mapPal[0x18 + p * 4 + i])];
        }
        vidSetPalette(VID_SPR, p ? SM_OBJ_BOTTOM_CRAM : SM_OBJ_TOP_CRAM, 16, palette);
    }
}

//Draws tile at map cell x,y with the palette of the map's own tile there, like an NES nametable-only write.
static void smDrawCell(Uint8 x, Uint8 y, Uint8 tile) {
    Uint8 *attr = (Uint8 *)(LUT_SM_TILESET_ATTR + cur_tileset * 0x80);
    Uint16 *pn = (Uint16 *)SM_NBG0_PN;
    Uint16 page = ((y >> 5) << 1) | (x >> 5);

    pn[page * 1024 + (y & 31) * 32 + (x & 31)] = (MSX_ON ? 0 : (attr[smMap[y][x]] & 3) << 12) | tile;
}

//Writes the 64x64 map as 2x2 pages of 32x32 cells.
static void smBuildPlane() {
    for(Uint8 y = 0; y < 64; y++) {
        for(Uint8 x = 0; x < 64; x++) {
            smDrawCell(x, y, smMap[y][x]);
        }
    }
}

//NES DrawFullMap leaves scroll_y at 0 and redraws every cell.
void smFullRedraw() {
    smNtScrollY = 0;
    memset(smDoorCells, 0, sizeof(smDoorCells));
    smBuildPlane();
}

static void smRestoreDoorCell(SM_DOOR_CELL *cell) {
    smDrawCell(cell->mapX, cell->mapY, smMap[cell->mapY][cell->mapX]);
    cell->used = FALSE;
}

//NES DoMapDrawJob. Door graphics in the redrawn row or column go back to the map's tiles.
static void smDoDrawJob() {
    for(Uint8 i = 0; i < SM_DOOR_CELLS; i++) {
        SM_DOOR_CELL *cell = &smDoorCells[i];
        if(!cell->used) {
            continue;
        }
        if(smDrawJob.column) {
            if(cell->ntX == smDrawJob.ntX) {
                smRestoreDoorCell(cell);
            }
        } else if(cell->ntY == smDrawJob.ntY && ((cell->ntX - smDrawJob.ntX) & 0x1F) < 16) {
            smRestoreDoorCell(cell);
        }
    }
    smDrawJob.pending = FALSE;
}

//NES StartMapMove: sets up the row or column the step streams in.
static void smStartMapMove() {
    smGetTargetCoords(smFacing, &sm_player_x, &sm_player_y);
    smDrawJob.pending = TRUE;
    if(smFacing & 0x01) {
        smDrawJob.column = TRUE;
        smDrawJob.ntX = (sm_scroll_x + 16) & 0x1F;
    } else if(smFacing & 0x02) {
        smDrawJob.column = TRUE;
        smDrawJob.ntX = (sm_scroll_x - 1) & 0x1F;
    } else if(smFacing & 0x04) {
        smDrawJob.column = FALSE;
        smDrawJob.ntX = sm_scroll_x & 0x1F;
        smDrawJob.ntY = smNtScrollY;
    } else {
        smDrawJob.column = FALSE;
        smDrawJob.ntX = sm_scroll_x & 0x1F;
        smDrawJob.ntY = smNtScrollY ? smNtScrollY - 1 : 14;
    }
}

//NES RedrawDoor's nametable write to doorppuaddr. Off-screen slots are redrawn before they scroll in.
static void smRedrawDoorTile(Uint8 tile) {
    Uint8 col = (smDoorNtX - sm_scroll_x) & 0x1F;
    SM_DOOR_CELL *cell = 0;

    for(Uint8 i = 0; i < SM_DOOR_CELLS; i++) {
        if(smDoorCells[i].used && smDoorCells[i].ntX == smDoorNtX && smDoorCells[i].ntY == smDoorNtY) {
            smRestoreDoorCell(&smDoorCells[i]);
        }
    }
    if(col >= 16) {
        return;
    }
    for(Uint8 i = 0; i < SM_DOOR_CELLS && !cell; i++) {
        if(!smDoorCells[i].used) {
            cell = &smDoorCells[i];
        }
    }
    if(!cell) {
        cell = &smDoorCells[0];
        smRestoreDoorCell(cell);
    }
    cell->used = TRUE;
    cell->ntX = smDoorNtX;
    cell->ntY = smDoorNtY;
    cell->mapX = (sm_scroll_x + col) & 0x3F;
    cell->mapY = (sm_scroll_y + (smDoorNtY + 15 - smNtScrollY) % 15) & 0x3F;
    smDrawCell(cell->mapX, cell->mapY, tile);
}

//NES RedrawDoor. Finishes an inroom transition started by a door or close-room tile.
static void smRedrawDoor() {
    Uint8 tile;

    if(inroom == 0 || (inroom & 0x80)) {
        return;
    }
    switch(inroom & 0x07) {
        case 0x01:
            inroom = 0x81;
            tile = MAPTILE_OPENDOOR;
            break;
        case 0x02:
            inroom = 0x82; //In the room, but outside sprites stay visible.
            tile = MAPTILE_OPENDOOR;
            break;
        case 0x05:
            inroom = 0x00;
            tile = MAPTILE_CLOSEDDOOR;
            break;
        default:
            inroom = 0x00;
            tile = MAPTILE_LOCKEDDOOR;
            break;
    }
    smRedrawDoorTile(tile);
    smUploadPalettes();
}

void sm_init() {
    Uint8 reenter = smKeepMap;

    if(!reenter) {
        smDecompress();
        smLoadMapObjects();
    }
    smKeepMap = FALSE;
    //NES ReenterStandardMap cycles in. EnterStandardMap wipes the screen open instead.
    if(reenter) {
        smStartPalCycle(TRUE, SM_STATE);
    }

    //NES PrepStandardMap.
    dlgMusicBackup = musicTilesetTrack(cur_tileset);
    music_track = dlgMusicBackup;
    smEnteringShop = FALSE;
    smDrawJob.pending = FALSE;
    smFacing = 0x04;
    smMoving = FALSE;
    smCtrX = 0;
    smCtrY = 0;
    smTileprop[0] = 0;
    smTileprop[1] = 0;
    smGray = FALSE;
    smEmphasis = 0;
    smBattleFade = 0;
    altareffect = 0;
    smAltarLines = 0;
    smAltarShake = 0;
    smAltarReset();
    sm_player_x = (sm_scroll_x + 7) & 0x3F;
    sm_player_y = (sm_scroll_y + 7) & 0x3F;
    smDlgState = SM_DLG_NONE;
    smDlgHide = FALSE;
    smJoyA = 0;
    smJoyStart = 0;
    smJoySelect = 0;
    smJoyPrev = PadData1;
    smObjCoverY = 0;
    smLoadMapObjectChr();
    smUpdateDisplay();

    smScreenInit(reenter, smWipeOpened);
    if(reenter) {
        state = SM_PAL_CYCLE;
    }
}

void sm_draw() {
    SPRITE_INFO temp = player;
    SPRITE_INFO tempb = playerb;

    temp.xPos = FIXED(SM_VIEW_X);
    temp.yPos = FIXED(SM_VIEW_Y - 3);
    tempb.xPos = FIXED(SM_VIEW_X);
    tempb.yPos = FIXED(SM_VIEW_Y + 5);
    smDrawMapObjects();
    sprite_draw(&temp);
    sprite_draw(&tempb);
}


static void smPushMap() {
    if(smStackDepth < SM_STACK_MAX) {
        SM_STACK_ENTRY *e = &smStack[smStackDepth++];
        e->scrollX = sm_scroll_x;
        e->scrollY = sm_scroll_y;
        e->inroom = inroom;
        e->map = cur_map;
        e->tileset = cur_tileset;
    }
}

//NES GetSMTargetCoords.
static void smGetTargetCoords(Uint8 facing, Uint8 *x, Uint8 *y) {
    Sint8 dx = 0;
    Sint8 dy = 0;

    if(facing & 0x01) {
        dx = 1;
    } else if(facing & 0x02) {
        dx = -1;
    } else if(facing & 0x04) {
        dy = 1;
    } else {
        dy = -1;
    }
    *x = (sm_scroll_x + 7 + dx) & 0x3F;
    *y = (sm_scroll_y + 7 + dy) & 0x3F;
}

static void smGetTileProperties(Uint8 x, Uint8 y) {
    Uint8 *prop = (Uint8 *)(LUT_SM_TILESET_PROP + cur_tileset * 0x100) + smMap[y][x] * 2;
    smTileprop[0] = prop[0];
    smTileprop[1] = prop[1];
}

//NES SMMove_HaveSpecialItem/NoSpecialItem.
static Uint8 smSpecialItemMove(Uint8 haveItem) {
    if(!haveItem) {
        smTileprop[0] = 0;
        return FALSE;
    }
    music_track = MUSIC_FANFARE;
    return TRUE;
}

//NES SMMove_xOrb. Lights the altar's orb the first time it's stepped on.
static Uint8 smOrbAltarMove(Uint8 orb) {
    if(!items[ITEM_ORB_START + orb]) {
        items[ITEM_ORB_START + orb] = 1;
        altareffect++;
    }
    return TRUE;
}

//NES SMMove_Door. Starts the inroom transition and sets doorppuaddr for the tile moved to.
static Uint8 smDoorMove(Uint8 prop, Uint8 targetX) {
    Uint8 door = (prop >> 1) & 0x03;
    Uint8 row = smNtScrollY + 7;

    if(door == (TP_SPEC_LOCKED >> 1)) {
        smTileprop[1] = 0;
        if(!items[ITEM_MYSTIC_KEY]) {
            return FALSE;
        }
    }
    if(!(inroom & 0x80)) {
        sfxDoor();
    }
    inroom = door;

    //The row comes from the raw direction bits, so a diagonal that includes down picks the row above.
    if(smFacing == 0x04) {
        row++;
    } else if(smFacing > 0x04) {
        row--;
    }
    smDoorNtY = (row >= 15) ? row - 15 : row;
    smDoorNtX = targetX & 0x1F;

    if(smTileprop[1]) {
        shop_id = smTileprop[1];
        smEnteringShop = TRUE;
    }
    return TRUE;
}

//NES CanPlayerMoveSM and lut_SMMoveJmpTbl.
static Uint8 smCanPlayerMove() {
    Uint8 x, y;

    smGetTargetCoords(smFacing, &x, &y);
    if(smIsObjectInPath(x, y)) {
        smTileprop[0] = 0;
        smTileprop[1] = 0;
        return FALSE;
    }
    smGetTileProperties(x, y);

    Uint8 prop = smTileprop[0] & (TP_SPEC_MASK | TP_NOMOVE);
    if(prop == TP_NOMOVE) {
        smTileprop[0] = 0;
        smTileprop[1] = 0;
        return FALSE;
    }

    switch(prop & TP_SPEC_MASK) {
        case TP_SPEC_DOOR:
        case TP_SPEC_LOCKED:
            return smDoorMove(prop, x);
        case TP_SPEC_CLOSEROOM:
            //NES SMMove_CloseRoom: $81/$82 become $05/$06 for the door redraw.
            if(inroom & 0x80) {
                inroom ^= 0x84;
                sfxDoor();
            }
            return TRUE;
        case TP_SPEC_BATTLE:
            //NES SMMove_Battle. A clear bit 7 is a forced battle with that formation.
            if(!(smTileprop[1] & 0x80)) {
                btlformation = smTileprop[1];
                smTileprop[0] = TP_BATTLEMARKER;
            } else if(battleStepRNG() < ((Uint8 *)LUT_BATTLE_RATES)[cur_map + 1]) {
                btlformation = getBattleNum(SM_DOMAIN_START + cur_map);
                smTileprop[0] = TP_BATTLEMARKER;
            }
            return TRUE;
        case TP_SPEC_TREASURE:
            return FALSE;
        case TP_SPEC_CROWN:
            return smSpecialItemMove(items[ITEM_CROWN]);
        case TP_SPEC_CUBE:
            return smSpecialItemMove(items[ITEM_CUBE]);
        case TP_SPEC_4ORBS:
            return smSpecialItemMove(items[ITEM_ORB_START] & items[ITEM_ORB_START + 1]
                & items[ITEM_ORB_START + 2] & items[ITEM_ORB_START + 3]);
        case TP_SPEC_FIREORB:
            return smOrbAltarMove(0);
        case TP_SPEC_WATERORB:
            return smOrbAltarMove(1);
        case TP_SPEC_AIRORB:
            return smOrbAltarMove(2);
        case TP_SPEC_EARTHORB:
            return smOrbAltarMove(3);
        default:
            return TRUE;
    }
}

//NES SMMove_Right/Left/Down/Up. Right and down change tiles at the end of the step, left and up at the start.
static void smMovePlayer() {
    //Columns are drawn on the first frame of a step, rows halfway through.
    if(smDrawJob.pending && (smDrawJob.column || smCtrY == 8)) {
        smDoDrawJob();
    }

    if(smFacing & 0x01) {
        smCtrX = (smCtrX + 1) & 0x0F;
        if(smCtrX == 0) {
            smMoving = FALSE;
            sm_scroll_x = (sm_scroll_x + 1) & 0x3F;
        }
    } else if(smFacing & 0x02) {
        if(smCtrX == 0) {
            sm_scroll_x = (sm_scroll_x - 1) & 0x3F;
        }
        smCtrX = (smCtrX - 1) & 0x0F;
        if(smCtrX == 0) {
            smMoving = FALSE;
        }
    } else if(smFacing & 0x04) {
        smCtrY = (smCtrY + 1) & 0x0F;
        if(smCtrY == 0) {
            smMoving = FALSE;
            sm_scroll_y = (sm_scroll_y + 1) & 0x3F;
            smNtScrollY = (smNtScrollY == 14) ? 0 : smNtScrollY + 1;
        }
    } else {
        if(smCtrY == 0) {
            sm_scroll_y = (sm_scroll_y - 1) & 0x3F;
            smNtScrollY = smNtScrollY ? smNtScrollY - 1 : 14;
        }
        smCtrY = (smCtrY - 1) & 0x0F;
        if(smCtrY == 0) {
            smMoving = FALSE;
        }
    }
}

//NES MapPoisonDamage sound, played every movement frame while anyone is poisoned.
void smPoisonSfx() {
    for(Uint8 i = 0; i < 4; i++) {
        if(party[i].ailment == AIL_POISON) {
            sfxPoison();
            return;
        }
    }
}

//NES MapPoisonDamage and AssignMapTileDamage, once per finished step. HP doesn't drop below 1.
void smStepDamage(Uint8 poisonOnly) {
    for(Uint8 i = 0; i < 4; i++) {
        if(poisonOnly && party[i].ailment != AIL_POISON) {
            continue;
        }
        if(party[i].current_hp >= 2) {
            party[i].current_hp--;
        }
    }
}

static void smWipeClosedToSM() {
    state = TO_SM_STATE;
}

static void smWipeClosedToOW() {
    sm_disable();
    owWipeIn = TRUE;
    state = TO_OW_STATE;
}

static void smWipeOpened() {
    state = SM_STATE;
}

static void smExitToOverworld() {
    smStackDepth = 0;
    wipeStart(FALSE, VID_NBG0, smWipeClosedToOW);
}

static void smPopMap() {
    SM_STACK_ENTRY *e = &smStack[--smStackDepth];
    sm_scroll_x = e->scrollX;
    sm_scroll_y = e->scrollY;
    inroom = e->inroom;
    cur_map = e->map;
    cur_tileset = e->tileset;
}

//NES StandardMapLoop teleport handling. Each closes the screen with a wipe first.
static void smTeleport(Uint8 type) {
    Uint8 id = smTileprop[1];

    if(type == TP_TELE_WARP) {
        if(smStackDepth == 0) {
            smExitToOverworld();
            return;
        }
        smPopMap();
        wipeStart(FALSE, VID_NBG0, smWipeClosedToSM);
    } else if(type == TP_TELE_NORM) {
        Uint8 *teleX = (Uint8 *)LUT_NORM_TELE_X;
        Uint8 *teleY = (Uint8 *)LUT_NORM_TELE_Y;
        Uint8 *teleMap = (Uint8 *)LUT_NORM_TELE_MAP;
        Uint8 *tilesets = (Uint8 *)LUT_TILESETS;

        smPushMap();
        sm_scroll_x = (teleX[id] - 7) & 0x3F;
        sm_scroll_y = (teleY[id] - 7) & 0x3F;
        cur_map = teleMap[id];
        cur_tileset = tilesets[cur_map];
        wipeStart(FALSE, VID_NBG0, smWipeClosedToSM);
    } else {
        Uint8 *teleX = (Uint8 *)LUT_EXIT_TELE_X;
        Uint8 *teleY = (Uint8 *)LUT_EXIT_TELE_Y;

        setOWPosition(teleX[id], teleY[id]);
        smExitToOverworld();
    }
}

//NES BattleTransition emphasis, called from doBattleTransition.
void smSetEmphasis(Uint8 emphasis) {
    smEmphasis = emphasis;
    smUploadPalettes();
}

void smSetMsxFade(Uint8 step) {
    smBattleFade = step;
    smUploadPalettes();
}

//NES CyclePalettes for standard maps.
static void smStartPalCycle(Uint8 reverse, Uint8 nextState) {
    Uint8 *target = smBgNesPal();

    for(Uint8 i = 0; i < 16; i++) {
        smPalCycColors[i] = target[i];
        if(reverse && smPalCycColors[i] != 0x0F) {
            smPalCycColors[i] = (smPalCycColors[i] & 0x30) | 0x0C;
        }
    }
    smFadeStep = reverse ? MSX_FADE_STEPS : 0;
    smPalCycling = TRUE;
    smPalCycReverse = reverse;
    smPalCycNext = nextState;
    //Same frame timing as the overworld cycle.
    smPalCycFrame = reverse ? 0 : 0xFF;
}

//NES PalCyc_Step. Returns the number of colors changed, 0 once the cycle is done.
static Uint8 smPalCycStep() {
    Uint8 *target = smBgNesPal();
    Uint8 changed = 0;

    //The MSX fades to black and back instead.
    if(MSX_ON) {
        if(smPalCycReverse ? smFadeStep == 0 : smFadeStep == MSX_FADE_STEPS) {
            return 0;
        }
        smFadeStep += smPalCycReverse ? -1 : 1;
        return 1;
    }
    for(Uint8 i = 0; i < 16; i++) {
        Uint8 color = smPalCycColors[i];
        if(smPalCycReverse) {
            if(color != target[i]) {
                smPalCycColors[i] = color - 1;
                changed++;
            }
        } else if(color != 0x0F) {
            Uint8 chroma = (color & 0x0F) + 1;
            smPalCycColors[i] = (chroma >= 0x0D) ? 0x0F : ((color & 0x30) | chroma);
            changed++;
        }
    }
    return changed;
}

//A step every 4th frame until no colors change. Sprites aren't drawn in the SM_PAL_CYCLE state.
void doSmPalCycle() {
    if(smPalCycFrame != 0 && (smPalCycFrame & 0x03) == 0) {
        if(smPalCycStep() == 0) {
            smPalCycling = FALSE;
            if(smPalCycReverse) {
                state = SM_STATE;
            } else {
                sm_disable();
                state = smPalCycNext;
            }
            return;
        }
        smUploadPalettes();
    }
    smPalCycFrame++;
}

//NES ReenterStandardMap: reloads the screen without reloading the map or its objects, then cycles in.
void smReenterMap() {
    smKeepMap = TRUE;
    state = TO_SM_STATE;
}

//NES GetSMTilePropNow.
static void smGetTilePropNow() {
    Uint8 x = (sm_scroll_x + 7) & 0x3F;
    Uint8 y = (sm_scroll_y + 7) & 0x3F;
    Uint8 *prop = (Uint8 *)(LUT_SM_TILESET_PROP + cur_tileset * 0x100) + smMap[y][x] * 2;
    smTilepropNow = prop[0] & TP_SPEC_MASK;
}

//Leaves the map for a menu or shop through the NES cycle out, and returns through smReenterMap.
static void smOpenMenu(Uint8 menuState) {
    mmFromMap = menuState == TO_MAIN_MENU;
    smGetTilePropNow();
    menuReturnSM = TRUE;
    smStartPalCycle(FALSE, menuState);
    state = SM_PAL_CYCLE;
}

//NES UseItem_Rod/Lute: TRUE if used, hiding the plate with NES HideMapObject.
Uint8 smUsePlateItem(Uint8 objId, Uint8 spec) {
    if(!menuReturnSM || smTilepropNow != spec || !(gameFlags[objId] & GMFLG_OBJVISIBLE)) {
        return FALSE;
    }
    gameFlags[objId] &= ~GMFLG_OBJVISIBLE;
    for(Uint8 i = 0; i < SM_OBJ_COUNT; i++) {
        if(mapobj[i].id == objId) {
            mapobj[i].id = 0;
            break;
        }
    }
    return TRUE;
}

//NES UseMagic_WARP/EXIT from a standard map menu. The map layers are already off.
void smMenuExit(Uint8 warp) {
    menuReturnSM = FALSE;
    if(warp && smStackDepth > 0) {
        smPopMap();
        state = TO_SM_STATE;
        return;
    }
    smStackDepth = 0;
    owWipeIn = TRUE;
    state = TO_OW_STATE;
}

static void smUpdateDisplay() {
    Uint16 x = ((((sm_scroll_x + 7) & 0x3F) * 16) + smCtrX - SM_VIEW_X + smAltarShake) & 0x3FF;
    Uint16 y = ((((sm_scroll_y + 7) & 0x3F) * 16) + smCtrY - SM_VIEW_Y) & 0x3FF;

    smScreenScroll(x, y);
    setMapmanSprite(smFacing, ((smCtrX ? smCtrX : smCtrY) & 0x08) != 0);
}

//NES UpdateJoy. A, Start and Select presses are caught until handled. Returns the held directions as NES bits.
static Uint8 smUpdateJoy() {
    PadBits pressed = PadData1 & ~smJoyPrev;
    Uint8 dir = 0;

    smJoyPrev = PadData1;
    if(pressed & (PAD_A | PAD_C)) {
        smJoyA++;
    }
    if(pressed & PAD_S) {
        smJoyStart++;
    }
    if(pressed & PAD_RB) {
        smJoySelect++;
    }
    if(PadData1 & PAD_R) dir |= 0x01;
    if(PadData1 & PAD_L) dir |= 0x02;
    if(PadData1 & PAD_D) dir |= 0x04;
    if(PadData1 & PAD_U) dir |= 0x08;
    return dir;
}

//NES TalkToSMTile.
static Uint8 smTalkToTile(Uint8 x, Uint8 y) {
    smGetTileProperties(x, y);
    if((smTileprop[0] & TP_SPEC_MASK) == TP_SPEC_TREASURE) {
        if(gameFlags[smTileprop[1]] & GMFLG_TCOPEN) {
            return DLGID_EMPTYTC;
        }
        return dlgOpenTreasureChest(smTileprop[1]);
    }
    if(smTileprop[0] & TP_NOTEXT_MASK) {
        return DLGID_NOTHING;
    }
    return smTileprop[1];
}

//NES ProcessSMInput A button. The box and text are drawn now, behind the window.
static void smStartTalk() {
    Uint8 x, y, dlg;
    Sint8 obj;

    smJoyA = 0;
    dlgSfx = 0;
    dlgflg_reentermap = 0;
    smGetTargetCoords(smFacing, &x, &y);
    obj = smCanTalkToMapObject(x, y);
    //Recorded before talking, so an object the talk hides stays drawn until the box closes.
    smRecordMapObjects(smFacing);
    if(obj >= 0) {
        smTileprop[0] = 0;
        dlg = smTalkToObject(obj, smTileprop);
    } else {
        dlg = smTalkToTile(x, y);
        smTileprop[0] = 0;
    }
    //Plus the NES's wait before drawing the box.
    smDlgTimer = 1 + dlgDraw(dlg);
    smDlgState = SM_DLG_WAIT;
}

static void smShowDialogueFrame() {
    smObjCoverY = smDlgLines;
    dlgSetVisible(smDlgLines);
}

//NES ShowDialogueBox @ExitDialogue: a jingle still playing is waited out, then the map music restarts.
static void smExitDialogue() {
    if(dlgSfx) {
        if(music_track != MUSIC_SILENCED) {
            smDlgState = SM_DLG_WAIT_SFX;
            return;
        }
        music_track = dlgMusicBackup;
        dlgSfx = 0;
    }
    sfxDialogueBox(0x95, 37);
    smDlgState = SM_DLG_CLOSE;
}

//NES DrawDialogueBox through ShowDialogueBox, one frame per call.
static void smDoDialogue() {
    Uint8 dir;

    //The NES frame counter only counts map loop frames.
    nesFrameCounter--;
    switch(smDlgState) {
        case SM_DLG_WAIT:
            if(--smDlgTimer == 0) {
                //NES sprite DMA after the text, with the object facing the player.
                smShowRecordedMapObjects();
                sfxDialogueBox(0x8E, 53);
                smDlgLines = 8;
                smDlgState = SM_DLG_OPEN;
            }
            break;
        case SM_DLG_OPEN:
            smShowDialogueFrame();
            smDlgLines += 2;
            if(smDlgLines >= DLG_LINES_OPEN) {
                if(dlgSfx) {
                    music_track = (dlgSfx == 1) ? MUSIC_FANFARE : MUSIC_TREASURE;
                }
                smDlgState = SM_DLG_WAIT_LIFT;
            }
            break;
        case SM_DLG_WAIT_LIFT:
        case SM_DLG_WAIT_PRESS:
            smShowDialogueFrame();
            dir = smUpdateJoy();
            if(smJoyA) {
                smExitDialogue();
                break;
            }
            //The NES restarts the map music whenever a track has ended, not only after a jingle.
            if(music_track == MUSIC_SILENCED) {
                music_track = dlgMusicBackup;
                dlgSfx = 0;
            }
            if(smDlgState == SM_DLG_WAIT_PRESS && dir) {
                smExitDialogue();
            } else if(!dir) {
                smDlgState = SM_DLG_WAIT_PRESS;
            }
            break;
        case SM_DLG_WAIT_SFX:
            smShowDialogueFrame();
            smExitDialogue();
            break;
        default:
            smShowDialogueFrame();
            smDlgLines -= 3;
            if(smDlgLines < 0x12) {
                smJoyA = 0;
                smJoyStart = 0;
                smJoySelect = 0;
                smObjCoverY = 0;
                smDlgHide = TRUE;
                smDlgState = SM_DLG_NONE;
                //The NES map loop updates the objects before the next frame.
                smUpdateMapObjects(smFacing, smCtrX, smCtrY);
                //NES ReenterStandardMap, so a class change reloads the mapman graphics.
                if(dlgflg_reentermap) {
                    smReenterMap();
                }
            }
            break;
    }
}

//Soft reset: ends the altar effect without restoring the map's colors, and turns the map off.
void sm_reset() {
    if(smAltarLines) {
        smAltarGray(FALSE);
        smAltarLines = 0;
        smAltarShake = 0;
    }
    //The next screen's palettes are written right away, so CRAM has to be in its layout now.
    smScreenCramReset();
    smKeepMap = FALSE;
    if(smScreenShown()) {
        sm_disable();
    }
}

//NES DoAltarEffect setup. The music stops until the effect ends.
static void smStartAltar() {
    smAltarGray(TRUE);
    altareffect = 0;
    smAltarDelay = 138;
    smAltarLines = 2;
    smAltarRetract = FALSE;
    musicHold = TRUE;
    musicApuWrite(0x400C, 0x30);
    musicApuWrite(0x4004, 0x30);
    musicApuWrite(0x4000, 0x30);
    musicApuWrite(0x4008, 0x30);
    musicApuWrite(0x400E, 0x0E);
    musicApuWrite(0x400F, 0x00);
    musicApuWrite(0x4006, 0x00);
    musicApuWrite(0x4007, 0x05);
    //The MSX game keeps its music going under the effect.
    musicMsxSfx(MSX_SFX_ALTAR);
}

//NES DoAltarEffect @MainLoop and AltarFrame. The beam's height sets the sq2 and noise volume. Returns TRUE once done.
static Uint8 smAltarFrame() {
    Uint8 vol = (smAltarLines >> 3) | 0x30;

    musicApuWrite(0x400C, vol);
    musicApuWrite(0x4004, vol);
    //The volume's low bit also shakes the map 1 pixel.
    smAltarShake = vol & 0x01;
    smAltarDraw(smAltarDelay, smAltarLines);
    smUpdateDisplay();
    if(!smAltarRetract) {
        smAltarLines += 2;
        smAltarDelay -= 2;
        if(smAltarDelay < 32) {
            smAltarRetract = TRUE;
        }
        return FALSE;
    }
    smAltarLines -= 2;
    if(smAltarLines >= 8) {
        return FALSE;
    }
    smAltarLines = 0;
    smAltarShake = 0;
    smAltarGray(FALSE);
    //sq2 is left on the effect's pitch, so it's marked busy for MusicPlay to reset it.
    musicApuWrite(0x4007, 0x00);
    musicApuWrite(0x400C, 0x00);
    musicApuWrite(0x4004, 0x00);
    sq2_sfx = 1;
    musicHold = FALSE;
    return TRUE;
}

//NES StandardMapLoop, run once per frame.
void doStandardMap() {
    Uint8 gray = FALSE;

    if(smDlgState != SM_DLG_NONE) {
        smDoDialogue();
        return;
    }
    if(smAltarLines) {
        //The NES frame counter only counts map loop frames.
        nesFrameCounter--;
        if(!smAltarFrame()) {
            return;
        }
    }
    if(smDlgHide) {
        smDlgHide = FALSE;
        dlgSetVisible(0);
    }

    smRedrawDoor();
    if(smMoving) {
        smMovePlayer();
        smPoisonSfx();
        if(!smMoving) {
            smStepDamage(TRUE);
        }
        //Damage tiles flash monochrome every other frame while stepping onto them.
        if((smTileprop[0] & TP_SPEC_MASK) == TP_SPEC_DAMAGE) {
            gray = nesFrameCounter & 1;
            sfxTileDamage();
            if(!smMoving) {
                smStepDamage(FALSE);
            }
        }
    }
    if(gray != smGray) {
        smGray = gray;
        smUploadPalettes();
    }

    if(!smMoving) {
        if(altareffect) {
            smStartAltar();
            return;
        }
        //The NES leaves the room, since shop entrances are also room doors.
        if(smEnteringShop) {
            inroom = 0;
            smOpenMenu(TO_SHOP);
            return;
        }
        Uint8 tele = smTileprop[0] & (TP_TELE_MASK | TP_BATTLEMARKER);
        if(tele == TP_TELE_WARP || tele == TP_TELE_NORM || tele == TP_TELE_EXIT) {
            smTeleport(tele);
            return;
        }
        if(tele == TP_BATTLEMARKER) {
            smTileprop[0] = 0;
            battleReturnSM = TRUE;
            battle_init(btlformation);
            startBattleTransition();
            return;
        }
        //The object update waits until the box closes.
        if(smJoyA) {
            smStartTalk();
            smUpdateDisplay();
            return;
        }
        if(smJoyStart) {
            smJoyStart = 0;
            smOpenMenu(TO_MAIN_MENU);
            return;
        }
        if(smJoySelect) {
            smJoySelect = 0;
            smOpenMenu(TO_LINEUP_MENU);
            return;
        }
        Uint8 dir = smUpdateJoy();
        if(dir) {
            smFacing = dir;
            if(smCanPlayerMove()) {
                smMoving = TRUE;
                smStartMapMove();
            }
        }
    }
    smUpdateMapObjects(smFacing, smCtrX, smCtrY);
    smUpdateDisplay();
}
