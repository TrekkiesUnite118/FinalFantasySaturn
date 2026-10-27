#include <string.h>
#include "types.h"

#include "cd.h"
#include "memmap.h"
#include "nesutils.h"
#include "player.h"
#include "sprdata.h"
#include "vblank.h"
#include "stdmap.h"
#include "mapobj.h"
#include "msxgfx.h"

//Screen position of NES sprite 0,0, matching the player sprite.
#define SM_OBJ_SCREEN_X (40)
#define SM_OBJ_SCREEN_Y (0)

//NES lut_2x2MapObj_Right/Left/Up/Down as 16x8 halves (tile pairs 0-1, 2-3, ...), per walk frame.
typedef struct {
    Uint8 top;
    Uint8 bottom;
    Uint8 topMirror;
    Uint8 bottomMirror;
} SM_OBJ_FRAME;

static const SM_OBJ_FRAME smObjFrames[4][2] = {
    {{4, 5, 1, 1}, {6, 7, 1, 1}},
    {{4, 5, 0, 0}, {6, 7, 0, 0}},
    {{2, 3, 0, 0}, {2, 3, 0, 1}},
    {{0, 1, 0, 0}, {0, 1, 0, 1}}
};

enum {
    OBJ_TSA_RIGHT,
    OBJ_TSA_LEFT,
    OBJ_TSA_UP,
    OBJ_TSA_DOWN
};

typedef struct {
    Uint8 visible;
    Sint16 x;
    Sint16 y;
    Uint8 tsa;
    Uint8 anim;
    Uint8 slot;
} SM_OBJ_DRAW;

static SM_OBJ_DRAW smObjDraw[SM_OBJ_COUNT];
static Uint8 smObjDrawCount = 0;
static SM_OBJ_DRAW smObjRecorded[SM_OBJ_COUNT];
static Uint8 smObjRecordedCount = 0;
static Uint8 cur_mapobj = 0;
static Uint8 npcdir_seed = 0x1B; //The NES ORs $1B into uninitialized RAM at reset.

//NES LoadMapObjects/LoadSingleMapObject.
void smLoadMapObjects() {
    Uint8 *src = (Uint8 *)(LUT_MAP_OBJECTS + cur_map * 0x30);

    for(Uint8 i = 0; i < SM_OBJ_COUNT; i++, src += 3) {
        SM_MAPOBJ *obj = &mapobj[i];
        memset(obj, 0, sizeof(SM_MAPOBJ));
        obj->rawid = src[0];
        obj->id = (gameFlags[src[0]] & 0x01) ? src[0] : 0;
        obj->flgs = src[1] & 0xC0;
        obj->physX = obj->gfxX = src[1] & 0x3F;
        obj->physY = obj->gfxY = src[2] & 0x3F;
        obj->tsa = OBJ_TSA_DOWN;
    }
}

//NES LoadMapObjCHR. Each slot gets its object's 16 tiles, hidden objects included.
void smLoadMapObjectChr() {
    Uint8 *objTbl = (Uint8 *)(LUT_MAP_OBJECTS + cur_map * 0x30);
    Uint8 *gfx = (Uint8 *)LUT_MAP_OBJ_GFX;
    Uint8 *halves = (Uint8 *)LWRAM_BUFFER + 0x1000;

    for(Uint8 i = 0; i < SM_OBJ_COUNT; i++) {
        Uint8 *chr = (Uint8 *)(LUT_MAP_OBJ_CHR + gfx[objTbl[i * 3]] * 0x100);
        convertNesTilesToSatTiles(chr, (Uint8 *)LWRAM_BUFFER, 0x100);
        buildMapSprites((Uint8 *)LWRAM_BUFFER, halves, 0x200);
        if(MSX_ON) {
            msxRecolorObject(gfx[objTbl[i * 3]], halves);
        }
        for(Uint8 h = 0; h < 8; h++) {
            sprSetChar(SM_OBJ_CHAR_START + i * 8 + h, SPRITE_4BPP, (h & 1) ? SM_OBJ_BOTTOM_CRAM : SM_OBJ_TOP_CRAM,
                16, 8, halves + h * 64);
        }
    }
}

//NES IsObjectInPath. An object on the tile blocks the player and has its wait cut,
//an object stepping off it is shoved to move faster.
Uint8 smIsObjectInPath(Uint8 x, Uint8 y) {
    for(Uint8 i = 0; i < SM_OBJ_COUNT; i++) {
        SM_MAPOBJ *obj = &mapobj[i];
        if(obj->id && obj->physX == x && obj->physY == y) {
            obj->movectr &= 0x03;
            return TRUE;
        }
    }
    for(Uint8 i = 0; i < SM_OBJ_COUNT; i++) {
        SM_MAPOBJ *obj = &mapobj[i];
        Uint8 fromX = obj->gfxX;
        Uint8 fromY = obj->gfxY;
        if(!obj->id) {
            continue;
        }
        if(obj->spdX < 0) {
            fromX = (fromX + 1) & 0x3F;
        } else if(obj->spdY < 0) {
            fromY = (fromY + 1) & 0x3F;
        }
        if(fromX == x && fromY == y) {
            obj->pl = 1;
            return FALSE;
        }
    }
    return FALSE;
}

//NES CanTalkToMapObject. An object past the halfway point of a step counts as on the next tile.
//Objects hidden by the room filter can still be talked to.
Sint8 smCanTalkToMapObject(Uint8 x, Uint8 y) {
    for(Uint8 i = 0; i < SM_OBJ_COUNT; i++) {
        SM_MAPOBJ *obj = &mapobj[i];
        if(!obj->id) {
            continue;
        }
        if(((obj->gfxX + (obj->ctrX >= 8)) & 0x3F) == x && ((obj->gfxY + (obj->ctrY >= 8)) & 0x3F) == y) {
            obj->pl |= 0x80;
            return i;
        }
    }
    return -1;
}

//NES CanMapObjMove.
static Uint8 smCanMapObjMove(Uint8 index, Uint8 x, Uint8 y) {
    Uint8 *prop = (Uint8 *)(LUT_SM_TILESET_PROP + cur_tileset * 0x100);

    if(prop[smMap[y][x] * 2] & (TP_TELE_MASK | TP_NOMOVE)) {
        return FALSE;
    }
    if(sm_player_x == x && sm_player_y == y) {
        return FALSE;
    }
    for(Uint8 i = 0; i < SM_OBJ_COUNT; i++) {
        if(i != index && mapobj[i].id && mapobj[i].physX == x && mapobj[i].physY == y) {
            return FALSE;
        }
    }
    return TRUE;
}

static Uint8 smIsBat(Uint8 id) {
    return (id >= OBJID_SKYWAR_FIRST && id <= OBJID_SKYWAR_LAST) || id == OBJID_BAT;
}

//NES MapObjectMove. One object per frame waits out its timer, then tries a step in a random direction.
static void smMapObjectMove() {
    Uint8 wrapped;
    SM_MAPOBJ *obj;

    cur_mapobj++;
    wrapped = (cur_mapobj >= SM_OBJ_COUNT);
    if(wrapped) {
        cur_mapobj = 0;
    }
    obj = &mapobj[cur_mapobj];

    if(!obj->id || (obj->flgs & OBJ_FLAG_STILL) || obj->face) {
        return;
    }
    //Objects outside a normal room are frozen while the player is in it.
    if((inroom & 0x01) && !(obj->flgs & OBJ_FLAG_INROOM)) {
        return;
    }
    if(obj->movectr) {
        obj->movectr--;
        return;
    }

    //The carry left over from the slot wrap check is added in, so slot 0 gets +1.
    npcdir_seed = (Uint8)nesFrameCounter + npcdir_seed + wrapped;
    Uint8 x = obj->physX;
    Uint8 y = obj->physY;

    switch(npcdir_seed & 0x03) {
        case 0:
            x = (x + 1) & 0x3F;
            if(!smCanMapObjMove(cur_mapobj, x, y)) {
                return;
            }
            obj->physX = x;
            obj->spdX = 1;
            obj->face = 1;
            obj->tsa = OBJ_TSA_RIGHT;
            break;
        case 1:
            x = (x - 1) & 0x3F;
            if(!smCanMapObjMove(cur_mapobj, x, y)) {
                return;
            }
            obj->gfxX = obj->physX = x;
            obj->spdX = -1;
            obj->ctrX = 0x0F;
            obj->face = 2;
            obj->tsa = OBJ_TSA_LEFT;
            break;
        case 2:
            y = (y - 1) & 0x3F;
            if(!smCanMapObjMove(cur_mapobj, x, y)) {
                return;
            }
            obj->gfxY = obj->physY = y;
            obj->spdY = -1;
            obj->ctrY = 0x0F;
            obj->face = 4;
            obj->tsa = smIsBat(obj->id) ? OBJ_TSA_LEFT : OBJ_TSA_UP;
            break;
        default:
            y = (y + 1) & 0x3F;
            if(!smCanMapObjMove(cur_mapobj, x, y)) {
                return;
            }
            obj->physY = y;
            obj->spdY = 1;
            obj->face = 8;
            obj->tsa = smIsBat(obj->id) ? OBJ_TSA_LEFT : OBJ_TSA_DOWN;
            break;
    }
    obj->movectr = nesFrameCounter & 0x07;
}

//NES AnimateAndDrawMapObject movement: 1 pixel every other frame, every frame while shoved.
static void smAnimateMapObject(SM_MAPOBJ *obj) {
    if(!obj->pl && (nesFrameCounter & 0x01)) {
        return;
    }
    if(obj->spdX) {
        obj->ctrX = (obj->ctrX + obj->spdX) & 0x0F;
        if(obj->ctrX == 0) {
            obj->spdX = 0;
            obj->face = 0;
            obj->pl = 0;
            obj->gfxX = obj->physX;
        }
    } else if(obj->spdY) {
        obj->ctrY = (obj->ctrY + obj->spdY) & 0x0F;
        if(obj->ctrY == 0) {
            obj->spdY = 0;
            obj->face = 0;
            obj->pl = 0;
            obj->gfxY = obj->physY;
        }
    }
}

//NES DrawMapObject, recorded for smDrawMapObjects. Positions are relative to the player, so objects
//in the wider Saturn view are drawn instead of using the NES's 16x15 tile cull.
static void smRecordMapObject(Uint8 slot, Uint8 playerFacing, Uint8 playerCtrX, Uint8 playerCtrY) {
    SM_MAPOBJ *obj = &mapobj[slot];
    SM_OBJ_DRAW *draw = &smObjDraw[smObjDrawCount];
    Uint8 shown = (inroom & 0x01) ? (obj->flgs & OBJ_FLAG_INROOM) : !(obj->flgs & OBJ_FLAG_INROOM);
    Sint16 dx, dy;

    if(!shown) {
        return;
    }
    dx = ((obj->gfxX * 16 + obj->ctrX) - (sm_scroll_x * 16 + playerCtrX)) & 0x3FF;
    dy = ((obj->gfxY * 16 + obj->ctrY) - (sm_scroll_y * 16 + playerCtrY) - 1) & 0x3FF;
    if(dx >= 0x200) {
        dx -= 0x400;
    }
    if(dy >= 0x200) {
        dy -= 0x400;
    }
    draw->x = dx + SM_OBJ_SCREEN_X;
    draw->y = dy - 3 + SM_OBJ_SCREEN_Y;
    if(draw->x <= -16 || draw->x >= 320 || draw->y <= -16 || draw->y >= 240) {
        return;
    }
    draw->slot = slot;

    if(obj->pl & 0x80) {
        //Faces the player while being talked to, for one frame.
        obj->pl &= 0x7F;
        if(playerFacing & 0x01) {
            draw->tsa = OBJ_TSA_LEFT;
        } else if(playerFacing & 0x02) {
            draw->tsa = OBJ_TSA_RIGHT;
        } else if(playerFacing & 0x04) {
            draw->tsa = OBJ_TSA_UP;
        } else {
            draw->tsa = OBJ_TSA_DOWN;
        }
        draw->anim = 0;
    } else {
        draw->tsa = obj->tsa;
        if((obj->flgs & OBJ_FLAG_STILL) || smIsBat(obj->id)) {
            //Walks in place, switching frames every 16 frames.
            draw->anim = (nesFrameCounter >> 4) & 0x01;
        } else {
            //Switches frames every 4 pixels of the step.
            draw->anim = ((obj->ctrX | obj->ctrY) >> 2) & 0x01;
        }
    }
    smObjDrawCount++;
}

//NES UpdateAndDrawMapObjects. Odd frames go through the objects backward, which swaps their overlap order.
void smUpdateMapObjects(Uint8 playerFacing, Uint8 playerCtrX, Uint8 playerCtrY) {
    smObjDrawCount = 0;
    if(nesFrameCounter & 0x01) {
        for(Sint8 i = SM_OBJ_COUNT - 1; i >= 0; i--) {
            if(mapobj[i].id) {
                smAnimateMapObject(&mapobj[i]);
                smRecordMapObject(i, playerFacing, playerCtrX, playerCtrY);
            }
        }
    } else {
        for(Uint8 i = 0; i < SM_OBJ_COUNT; i++) {
            if(mapobj[i].id) {
                smAnimateMapObject(&mapobj[i]);
                smRecordMapObject(i, playerFacing, playerCtrX, playerCtrY);
            }
        }
    }
    smMapObjectMove();
}

//NES DrawMapObjectsNoUpdate, kept until smShowRecordedMapObjects like the NES OAM buffer.
//The player isn't moving, so the step counters are 0.
void smRecordMapObjects(Uint8 playerFacing) {
    SM_OBJ_DRAW shown[SM_OBJ_COUNT];
    Uint8 shownCount = smObjDrawCount;

    memcpy(shown, smObjDraw, sizeof(shown));
    smObjDrawCount = 0;
    for(Uint8 i = 0; i < SM_OBJ_COUNT; i++) {
        if(mapobj[i].id) {
            smRecordMapObject(i, playerFacing, 0, 0);
        }
    }
    memcpy(smObjRecorded, smObjDraw, sizeof(smObjRecorded));
    smObjRecordedCount = smObjDrawCount;
    memcpy(smObjDraw, shown, sizeof(shown));
    smObjDrawCount = shownCount;
}

void smShowRecordedMapObjects() {
    memcpy(smObjDraw, smObjRecorded, sizeof(smObjDraw));
    smObjDrawCount = smObjRecordedCount;
}

//NES Dialogue_CoverSprites_VBl. Sprite priority 1 is set below NBG0.
static Uint16 smObjColor(Sint16 y, Uint16 cram) {
    return (smObjCoverY && y - SM_OBJ_SCREEN_Y < smObjCoverY) ? (cram | 0x1000) : 0xFFFF;
}

//Drawn below the player. Later commands draw on top, so the NES order is reversed.
void smDrawMapObjects() {
    SPRITE_INFO spr;

    for(Sint8 i = smObjDrawCount - 1; i >= 0; i--) {
        SM_OBJ_DRAW *draw = &smObjDraw[i];
        const SM_OBJ_FRAME *frame = &smObjFrames[draw->tsa][draw->anim];
        Uint16 base = SM_OBJ_CHAR_START + draw->slot * 8;

        sprite_make(base + frame->top, FIXED(draw->x), FIXED(draw->y), WEAPON_PRIORITY, &spr);
        spr.mirror = frame->topMirror ? MIRROR_HORIZ : 0;
        sprite_draw_pal(&spr, smObjColor(draw->y, SM_OBJ_TOP_CRAM));
        sprite_make(base + frame->bottom, FIXED(draw->x), FIXED(draw->y + 8), WEAPON_PRIORITY, &spr);
        spr.mirror = frame->bottomMirror ? MIRROR_HORIZ : 0;
        sprite_draw_pal(&spr, smObjColor(draw->y + 8, SM_OBJ_BOTTOM_CRAM));
    }
}
