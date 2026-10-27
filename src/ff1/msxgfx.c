#include <string.h>
#include "types.h"

#include "cd.h"
#include "memmap.h"
#include "nespal.h"
#include "msxgfx.h"

#define MSX_VERSION (1)
#define MSX_MAPS (61)
#define MSX_COLOR_SETS (21)
#define MSX_ENEMIES (124)
#define MSX_FIRST_FIEND (119)
#define MSX_BATTLE_FRAMES (MSX_ID_BATTLE_PAL - MSX_ID_BATTLE_FRAMES)

enum {
    MSX_KIND_TILES,
    MSX_KIND_BITMAP,
    MSX_KIND_PALETTE,
    MSX_KIND_RAW
};

typedef struct {
    Uint16 id;
    Uint8 kind;
    Uint8 pad;
    Uint16 width;
    Uint16 height;
    Uint32 offset;
    Uint32 size;
} MSX_ENTRY;

typedef struct {
    Uint8 magic[4];
    Uint16 version;
    Uint16 count;
    MSX_ENTRY entries[MSX_ID_COUNT];
} MSX_FILE;

//Entries the port uses, with the size it expects.
typedef struct {
    Uint16 id;
    Uint16 count;
    Uint8 kind;
    Uint16 width;
    Uint16 height;
} MSX_SHAPE;

static const MSX_SHAPE msxShapes[] = {
    {MSX_ID_MAP_TILES, 1, MSX_KIND_TILES, 256, 64},
    {MSX_ID_MAP_TILES + 1, 8, MSX_KIND_TILES, 256, 32},
    {MSX_ID_MAP_PALS, 9, MSX_KIND_PALETTE, 16, 1},
    {MSX_ID_MAP_COLOR_TABLE, 1, MSX_KIND_RAW, MSX_MAPS, 1},
    {MSX_ID_MAP_COLOR_SETS, 1, MSX_KIND_PALETTE, MSX_COLOR_SETS * 8, 1},
    {MSX_ID_ENEMY_TILES + MSX_FIRST_FIEND, 4, MSX_KIND_TILES, 64, 64},
    {MSX_ID_ENEMY_TILES + MSX_ENEMIES - 1, 1, MSX_KIND_TILES, 112, 96},
    {MSX_ID_ENEMY_PALS, MSX_ENEMIES, MSX_KIND_PALETTE, 16, 1},
    {MSX_ID_BATTLE_PAL, 1, MSX_KIND_PALETTE, 16, 1},
    {MSX_ID_MAP_SPRITE_PAL, 1, MSX_KIND_PALETTE, 16, 1},
};

//The 3-bit MSX color levels, as tools/msxconv converts them.
static const Uint8 msxLevels[8] = {0, 39, 82, 115, 155, 186, 224, 255};

//The NES monochrome greys, $00, $10 and $20.
static const Uint32 msxGrays[3] = {0x727272, 0xBCBCBC, 0xFEFEFE};

#define MSX_FILE_DATA ((const MSX_FILE *)LWRAM_MSX_GFX)

Uint8 msxAvailable = FALSE;
Uint8 gfxMsx = FALSE;

static Uint8 msxCheck(Sint32 fileSize) {
    const MSX_FILE *f = MSX_FILE_DATA;

    if(fileSize < (Sint32)sizeof(MSX_FILE) || memcmp(f->magic, "MSXG", 4) != 0 || f->version != MSX_VERSION
       || f->count != MSX_ID_COUNT) {
        return FALSE;
    }
    for(Uint16 i = 0; i < MSX_ID_COUNT; i++) {
        const MSX_ENTRY *e = &f->entries[i];
        if(e->id != i || (e->offset & 3) || e->offset > (Uint32)fileSize || e->size > (Uint32)fileSize - e->offset) {
            return FALSE;
        }
    }
    for(Uint8 s = 0; s < sizeof(msxShapes) / sizeof(msxShapes[0]); s++) {
        const MSX_SHAPE *shape = &msxShapes[s];
        for(Uint16 i = 0; i < shape->count; i++) {
            const MSX_ENTRY *e = &f->entries[shape->id + i];
            Uint32 size = shape->kind == MSX_KIND_PALETTE ? shape->width * 4
                        : shape->kind == MSX_KIND_RAW ? shape->width
                        : (Uint32)shape->width * shape->height / 2;
            if(e->kind != shape->kind || e->width != shape->width || e->height != shape->height || e->size != size) {
                return FALSE;
            }
        }
    }
    //Regular enemies are small (32x32) or large (48x48).
    for(Uint8 i = 0; i < MSX_FIRST_FIEND; i++) {
        const MSX_ENTRY *e = &f->entries[MSX_ID_ENEMY_TILES + i];
        if(e->kind != MSX_KIND_TILES || e->width != e->height || (e->width != 32 && e->width != 48)
           || e->size != (Uint32)e->width * e->height / 2) {
            return FALSE;
        }
    }
    //Party battle frames are 16x24, but for the 24x16 dead pose.
    for(Uint16 i = 0; i < MSX_BATTLE_FRAMES; i++) {
        const MSX_ENTRY *e = &f->entries[MSX_ID_BATTLE_FRAMES + i];
        Uint8 dead = i % MSX_BATTLE_CLASS_FRAMES == MSX_BATTLE_FRAME_DEAD;
        if(e->kind != MSX_KIND_BITMAP || e->width != (dead ? 24 : 16) || e->height != (dead ? 16 : 24)
           || e->size != 16 * 24 / 2) {
            return FALSE;
        }
    }
    //Every color set index in the map table must exist.
    const Uint8 *table = msxData(MSX_ID_MAP_COLOR_TABLE);
    for(Uint8 i = 0; i < MSX_MAPS; i++) {
        if(table[i] >= MSX_COLOR_SETS) {
            return FALSE;
        }
    }
    return TRUE;
}

//VDP1 reads 4bpp pixel 15 as an end code, so the battle frames' color 15 moves to unused color 2.
static void msxFixBattleFrames() {
    Uint32 *pal = (Uint32 *)msxPalette(MSX_ID_BATTLE_PAL);

    pal[2] = pal[15];
    for(Uint16 id = MSX_ID_BATTLE_FRAMES; id < MSX_ID_BATTLE_PAL; id++) {
        Uint8 *px = (Uint8 *)msxData(id);
        for(Uint32 i = 0; i < MSX_FILE_DATA->entries[id].size; i++) {
            if((px[i] >> 4) == 15) {
                px[i] = (2 << 4) | (px[i] & 0x0F);
            }
            if((px[i] & 0x0F) == 15) {
                px[i] = (px[i] & 0xF0) | 2;
            }
        }
    }
}

void msxInit() {
    Sint32 size = cd_load_max("MSXGFX.BIN", (void *)LWRAM_MSX_GFX, LWRAM_MSX_GFX_END - LWRAM_MSX_GFX);

    msxAvailable = size > 0 && msxCheck(size);
    if(msxAvailable) {
        msxFixBattleFrames();
    } else {
        gfxMsx = FALSE;
    }
}

const Uint8 *msxData(Uint16 id) {
    return (const Uint8 *)LWRAM_MSX_GFX + MSX_FILE_DATA->entries[id].offset;
}

const Uint32 *msxPalette(Uint16 id) {
    return (const Uint32 *)msxData(id);
}

void msxMapPalette(Uint8 tileset, Uint8 map, Uint32 *dst) {
    const Uint32 *sets = msxPalette(MSX_ID_MAP_COLOR_SETS);
    Uint8 set = msxData(MSX_ID_MAP_COLOR_TABLE)[map];

    memcpy(dst, msxPalette(MSX_ID_MAP_PALS + 1 + tileset), 8 * sizeof(Uint32));
    memcpy(dst + 8, sets + set * 8, 8 * sizeof(Uint32));
}

//MAIN.COM $4F03: each 3-bit level of color 10 goes up one, short of 7.
void msxHideRooms(Uint32 *palette) {
    Uint32 color = 0;

    for(Uint8 ch = 0; ch < 3; ch++) {
        Uint8 value = (palette[10] >> (ch * 8)) & 0xFF;
        Uint8 level = 0;
        while(level < 7 && msxLevels[level] != value) {
            level++;
        }
        if(level < 7) {
            level++;
        }
        color |= (Uint32)msxLevels[level] << (ch * 8);
    }
    for(Uint8 i = 12; i < 16; i++) {
        palette[i] = color;
    }
}

//Monochrome picks the NES grey nearest the color's brightness.
Uint32 msxShadeColor(Uint32 color, Uint8 emphasis, Uint8 gray) {
    if(gray) {
        Uint32 luma = ((color & 0xFF) * 77 + ((color >> 8) & 0xFF) * 150 + ((color >> 16) & 0xFF) * 29) >> 8;
        color = msxGrays[luma < 0x60 ? 0 : luma < 0xC0 ? 1 : 2];
    }
    return emphasizeRgb(color, emphasis);
}

Uint32 msxFadeColor(Uint32 color, Uint8 step, Uint8 steps) {
    Uint32 result = 0;

    for(Uint8 ch = 0; ch < 3; ch++) {
        Uint32 value = (color >> (ch * 8)) & 0xFF;
        result |= (value * (steps - step) / steps) << (ch * 8);
    }
    return result;
}

//JOBS colors per class for the top and bottom of the map sprite's NES color 2.
static const Uint8 msxClassColors[12][2] = {
    {5, 5}, {2, 7}, {6, 2}, {5, 5}, {3, 3}, {6, 2},
    {5, 5}, {7, 7}, {6, 2}, {5, 5}, {5, 3}, {7, 2}
};

//JOBS colors for NES colors 1-3 of each map object graphic's top and bottom halves. 0 is unused.
static const Uint8 msxObjectColors[MSX_OBJECT_GFX][2][3] = {
    {{1, 6, 4}, {1, 5, 4}}, {{1, 5, 4}, {1, 6, 4}}, {{1, 6, 4}, {1, 5, 4}}, {{1, 5, 4}, {1, 2, 4}},
    {{2, 7, 0}, {2, 7, 3}}, {{1, 7, 4}, {1, 5, 4}}, {{1, 6, 4}, {1, 7, 4}}, {{1, 7, 4}, {1, 2, 4}},
    {{1, 5, 4}, {1, 6, 4}}, {{1, 7, 4}, {1, 2, 4}}, {{1, 5, 4}, {1, 6, 4}}, {{1, 7, 4}, {1, 5, 4}},
    {{1, 6, 4}, {1, 2, 4}}, {{1, 6, 4}, {1, 7, 4}}, {{1, 7, 4}, {1, 3, 4}}, {{1, 6, 4}, {1, 5, 4}},
    {{1, 7, 3}, {1, 6, 3}}, {{2, 7, 4}, {2, 0, 0}}, {{2, 3, 4}, {2, 7, 4}}, {{1, 2, 4}, {1, 2, 4}},
    {{1, 6, 4}, {1, 6, 4}}, {{1, 7, 6}, {1, 7, 6}}, {{1, 6, 4}, {1, 6, 4}}, {{0, 6, 4}, {0, 6, 4}},
    {{1, 6, 4}, {1, 7, 4}}, {{1, 6, 4}, {1, 7, 4}}, {{1, 6, 4}, {1, 7, 4}}, {{0, 2, 7}, {2, 0, 7}},
    {{2, 3, 4}, {2, 3, 4}}, {{5, 6, 4}, {5, 6, 4}}
};

void msxSpritePalette(Uint32 *dst) {
    memcpy(dst, msxPalette(MSX_ID_MAP_SPRITE_PAL), 16 * sizeof(Uint32));
    dst[0] = 0;
}

void msxMapManPalette(Uint8 charClass, Uint32 *top, Uint32 *bottom) {
    const Uint32 *jobs = msxPalette(MSX_ID_MAP_SPRITE_PAL);

    memset(top, 0, 16 * sizeof(Uint32));
    memset(bottom, 0, 16 * sizeof(Uint32));
    top[1] = bottom[1] = jobs[1];
    top[3] = bottom[3] = jobs[4];
    top[2] = jobs[msxClassColors[charClass % 12][0]];
    bottom[2] = jobs[msxClassColors[charClass % 12][1]];
}

void msxRecolorObject(Uint8 gfx, Uint8 *halves) {
    if(gfx >= MSX_OBJECT_GFX) {
        return;
    }
    for(Uint8 h = 0; h < 8; h++) {
        const Uint8 *colors = msxObjectColors[gfx][h & 1];
        Uint8 *px = halves + h * 64;
        for(Uint8 b = 0; b < 64; b++) {
            Uint8 hi = px[b] >> 4;
            Uint8 lo = px[b] & 0x0F;
            hi = (hi >= 1 && hi <= 3) ? colors[hi - 1] : hi;
            lo = (lo >= 1 && lo <= 3) ? colors[lo - 1] : lo;
            px[b] = (hi << 4) | lo;
        }
    }
}

//FONT.PIC box colors: 0 outside, 2 inside, 3 text, 6 border.
#define MSX_BOX_OUTSIDE (0)
#define MSX_BOX_INSIDE (2)
#define MSX_BOX_TEXT (3)
#define MSX_BOX_BORDER (6)

void msxBoxPalette(Uint8 style, Uint32 *dst) {
    const Uint32 *pal;

    switch(style) {
        case MSX_BOX_MAP:
            pal = msxPalette(MSX_ID_MAP_PALS);
            break;
        case MSX_BOX_BRIDGE:
            pal = msxPalette(MSX_ID_BRIDGE_PAL);
            break;
        case MSX_BOX_ENDING:
            pal = msxPalette(MSX_ID_ENDING_PAL);
            break;
        default:
            pal = msxPalette(MSX_ID_MENU_PAL);
            break;
    }

    dst[0] = pal[MSX_BOX_OUTSIDE];
    dst[1] = pal[MSX_BOX_BORDER];
    dst[2] = style == MSX_BOX_BATTLE ? pal[MSX_BOX_OUTSIDE] : pal[MSX_BOX_INSIDE];
    dst[3] = pal[MSX_BOX_TEXT];
}

//MENU colors for the cursor's NES $00, $10 and $30.
#define MSX_CURSOR_DARK (15)
#define MSX_CURSOR_MID (4)
#define MSX_CURSOR_LIGHT (3)

void msxCursorPalette(const Uint8 *nesPal, Uint32 *dst) {
    const Uint32 *pal = msxPalette(MSX_ID_MENU_PAL);

    for(Uint8 i = 0; i < 4; i++) {
        switch(nesPal[i]) {
            case 0x00:
                dst[i] = pal[MSX_CURSOR_DARK];
                break;
            case 0x10:
                dst[i] = pal[MSX_CURSOR_MID];
                break;
            case 0x30:
                dst[i] = pal[MSX_CURSOR_LIGHT];
                break;
            default:
                dst[i] = nes_pal[nesPal[i]];
                break;
        }
    }
}

#define MSX_ORB_TILES_WIDE (10)

void msxOrbTiles(Uint8 orb, Uint8 *dst) {
    const Uint8 *src = msxData(MSX_ID_ORBS);

    for(Uint8 t = 0; t < 4; t++) {
        const Uint8 *tile = src + ((t / 2) * MSX_ORB_TILES_WIDE + orb * 2 + (t % 2)) * 32;
        for(Uint8 i = 0; i < 32; i++) {
            //Index 0 is transparent on the menu layer, the MSX draws it as the box background.
            Uint8 hi = tile[i] >> 4;
            Uint8 lo = tile[i] & 0x0F;
            *dst++ = ((hi ? hi : MSX_BOX_INSIDE) << 4) | (lo ? lo : MSX_BOX_INSIDE);
        }
    }
}

//NES enemies 119-126 are Lich, Kary, Kraken and Tiamat twice each, then Chaos.
Uint8 msxEnemy(Uint8 nesEnemy) {
    return nesEnemy < MSX_FIRST_FIEND ? nesEnemy : MSX_FIRST_FIEND + (nesEnemy - MSX_FIRST_FIEND) / 2;
}
