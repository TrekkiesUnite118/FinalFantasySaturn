#include <string.h>
#include "types.h"
#include "video.h"

#include "graphicrefs.h"
#include "nesutils.h"
#include "overworld.h"
#include "player.h"
#include "menu.h"
#include "static.h"
#include "sprdata.h"
#include "battle.h"
#include "vblank.h"
#include "minimap.h"
#include "music.h"

#define MINIMAP_DECOR_TILES (0x30)
#define MINIMAP_TITLE_TILES (0x28)
#define MINIMAP_MAX_MARKERS (63)

//Unused sprite character numbers and CRAM bank.
//After the overworld player's 96 characters at mapSpriteStart.
#define MINIMAP_HERE_CHAR (mapSpriteStart + 96)
#define MINIMAP_MARKER_CHAR (mapSpriteStart + 97)
#define MINIMAP_SPRITE_CRAM (144)

extern Uint8 current_x_pos;
extern Uint8 current_y_pos;

enum {
    MM_MAP_PREP,
    MM_MAP_ROWS,
    MM_DECOR,
    MM_TITLE,
    MM_DONE
};

//The NES BG pattern table, which the minimap uses as its canvas.
static Uint8 mmCanvas[0x1000];
static Uint32 mmSatTiles[0x2000 / 4];
static Uint8 mmDrawBuf[0x100]; //NES mm_drawbuf: 8 pixel rows of 16 tiles.

static Uint8 mmDirtyTiles[256];
static Uint8 mmTileQueued[256];
static volatile Uint16 mmDirtyCount;

static Uint32 mmSpriteChr[16];
static Uint32 mmSpritePal[16];
static SPRITE_INFO mmSprite;

static Uint8 mmMarkerX[MINIMAP_MAX_MARKERS];
static Uint8 mmMarkerY[MINIMAP_MAX_MARKERS];
static Uint8 mmMarkerVisible[MINIMAP_MAX_MARKERS];
static Uint8 mmMarkerCount;
static Uint8 mmHereX;
static Uint8 mmHereY;

static Uint8 mmPhase;
static Uint8 mmTileRow;
static Uint8 mmStep;
static Uint8 mmFrameCounter;
static Uint8 mmColorHere;
static Uint8 mmColorMarker;
static Uint8 mmSpritesOn;
static PadBits mmPadPrev;

static void updateTile(Uint8 tile) {
    convertNesTilesToSatTiles(&mmCanvas[tile * 16], (Uint8 *)mmSatTiles + (tile * 32), 16);
    if(!mmTileQueued[tile]) {
        mmTileQueued[tile] = TRUE;
        mmDirtyTiles[mmDirtyCount++] = tile;
    }
}

//VRAM A1 has no CPU access cycles during display, so tiles are copied in VBlank.
void minimap_vblank_in() {
    for(Uint16 i = 0; i < mmDirtyCount; i++) {
        Uint8 tile = mmDirtyTiles[i];
        volatile Uint32 *dst = (volatile Uint32 *)(VID_VRAM_A1 + (tile * 32));
        Uint32 *src = &mmSatTiles[tile * 8];
        for(Uint8 j = 0; j < 8; j++) {
            dst[j] = src[j];
        }
        mmTileQueued[tile] = FALSE;
    }
    mmDirtyCount = 0;
}

//NES Minimap_FillNTPal. The NES scrolls to Y $E8, so NT row 29 is shown at the top and the rest 8 px lower.
static void drawMinimapNT() {
    Uint8 *nt = (Uint8 *)MINIMAP_NT;
    Uint8 *attr = nt + 0x3C0;

    for(Uint8 r = 0; r < 30; r++) {
        for(Uint8 c = 0; c < 32; c++) {
            Uint8 shift = ((r & 0x02) << 1) | (c & 0x02);
            Uint8 pal = (attr[((r >> 2) * 8) + (c >> 2)] >> shift) & 0x03;
            menuPlane[(r + 1) % 30][c + CENTER_ADJUSTMENT] = (pal << 12) | nt[(r * 32) + c];
        }
    }
}

//NES Minimap_PrepRow for a whole tile row: each pixel ORs the colors of a 2x2 block of map tiles.
static void prepTileRow(Uint8 tileRow) {
    Uint8 *tileset = (Uint8 *)MINIMAP_TILESET;

    for(Uint8 py = 0; py < 8; py++) {
        Uint8 mapY = ((tileRow * 8) + py) * 2;
        for(Uint8 t = 0; t < 16; t++) {
            Uint8 lo = 0;
            Uint8 hi = 0;
            for(Uint8 b = 0; b < 8; b++) {
                Uint8 mapX = ((t * 8) + b) * 2;
                Uint8 v = tileset[decompressedMap[mapY][mapX]] | tileset[decompressedMap[mapY][mapX + 1]]
                        | tileset[decompressedMap[mapY + 1][mapX]] | tileset[decompressedMap[mapY + 1][mapX + 1]];
                lo = (lo << 1) | (v & 0x01);
                hi = (hi << 1) | ((v >> 1) & 0x01);
            }
            mmDrawBuf[(t * 16) + py] = lo;
            mmDrawBuf[(t * 16) + 8 + py] = hi;
        }
    }
}

//NES Minimap_DrawSFX: a steady square 2 tone, its phase reset each drawing frame.
static void drawSfx() {
    musicApuWrite(0x4015, 0x02);
    musicApuWrite(0x4004, 0x36);
    musicApuWrite(0x4005, 0x7F);
    musicApuWrite(0x4006, 0x60);
    musicApuWrite(0x4007, 0x00);
}

//NES Minimap_DrawRows: one pixel row of a tile row per frame.
static void drawPixelRow(Uint8 tileRow, Uint8 py) {
    drawSfx();
    for(Uint8 t = 0; t < 16; t++) {
        Uint8 tile = (tileRow * 16) + t;
        mmCanvas[(tile * 16) + py] = mmDrawBuf[(t * 16) + py];
        mmCanvas[(tile * 16) + 8 + py] = mmDrawBuf[(t * 16) + 8 + py];
        updateTile(tile);
    }
}

//NES Minimap_DrawDecorCHR/DrawTitleCHR: one bitplane row of every tile per frame.
static void drawGraphicStep(Uint8 *chr, Uint8 *destLo, Uint8 *destHi, Uint8 count, Uint8 step) {
    Uint8 *bitplane = (Uint8 *)MINIMAP_BITPLANE_LUT;
    Uint8 offset = bitplane[step];

    drawSfx();
    for(Uint8 i = 0; i < count; i++) {
        Uint16 dest = destLo[i] | (destHi[i] << 8);
        mmCanvas[dest + offset] = chr[(i * 16) + offset];
        updateTile(dest >> 4);
    }
}

//NES DrawDungeonSprite markers, in the order PrepRow makes them. The NES only shows 8 sprites per
//scanline, in OAM order with "you are here" first, so a marker whose one pixel row is past that is hidden.
static void findMarkers() {
    Uint8 *tileset = (Uint8 *)MINIMAP_TILESET;

    mmMarkerCount = 0;
    for(Uint16 y = 0; y < 256; y += 2) {
        for(Uint16 x = 0; x < 256; x += 2) {
            Uint8 v = tileset[decompressedMap[y][x]] | tileset[decompressedMap[y][x + 1]]
                    | tileset[decompressedMap[y + 1][x]] | tileset[decompressedMap[y + 1][x + 1]];
            if((v & 0x04) && mmMarkerCount < MINIMAP_MAX_MARKERS) {
                mmMarkerX[mmMarkerCount] = (x >> 1) + 0x3D;
                mmMarkerY[mmMarkerCount] = (y >> 1) + 0x34;
                mmMarkerCount++;
            }
        }
    }

    mmHereX = (current_x_pos >> 1) + 0x3D;
    mmHereY = (current_y_pos >> 1) + 0x34;

    for(Uint8 i = 0; i < mmMarkerCount; i++) {
        //The NES draws a sprite 1 line below its OAM Y. The marker's pixel is on row 3.
        Uint16 line = mmMarkerY[i] + 1 + 3;
        Uint8 earlier = (line >= mmHereY + 1 && line <= mmHereY + 8);
        for(Uint8 j = 0; j < i; j++) {
            if(line >= mmMarkerY[j] + 1 && line <= mmMarkerY[j] + 8) {
                earlier++;
            }
        }
        mmMarkerVisible[i] = (earlier < 8);
    }
}

static void setSpritePalette() {
    mmSpritePal[1] = nes_pal[mmColorHere];
    mmSpritePal[2] = nes_pal[mmColorMarker];
    vidSetPalette(VID_SPR, MINIMAP_SPRITE_CRAM, 16, mmSpritePal);
}

void initMinimap() {
    Uint8 *bgPal = (Uint8 *)MINIMAP_BG_PAL;
    Uint32 *pals[4] = {menu_pal_0, menu_pal_1, menu_pal_2, menu_pal_3};

    //NES EnterMinimap. The music isn't played until the drawing is done.
    musicHold = TRUE;
    musicApuWrite(0x4015, 0);
    music_track = MUSIC_PRELUDE;

    for(Uint8 p = 0; p < 4; p++) {
        for(Uint8 i = 0; i < 4; i++) {
            pals[p][i] = nes_pal[bgPal[(p * 4) + i]];
        }
    }

    clearMenuPlane();
    drawMinimapNT();

    memset(mmCanvas, 0x00, sizeof(mmCanvas));
    memset(mmSatTiles, 0x00, sizeof(mmSatTiles));
    memset(mmTileQueued, 0x00, sizeof(mmTileQueued));
    mmDirtyCount = 0;
    static_init_tiles((Uint8 *)mmSatTiles);
    setBattleBackColor(bgPal[0]);

    convertNesTilesToSatTiles((Uint8 *)MINIMAP_SPRITE_CHR, (Uint8 *)mmSpriteChr, 0x20);
    sprSetChar(MINIMAP_HERE_CHAR, SPRITE_4BPP, MINIMAP_SPRITE_CRAM, 8, 8, (Uint8 *)mmSpriteChr);
    sprSetChar(MINIMAP_MARKER_CHAR, SPRITE_4BPP, MINIMAP_SPRITE_CRAM, 8, 8, (Uint8 *)mmSpriteChr + 32);
    sprite_make(MINIMAP_HERE_CHAR, 0, 0, (Sint32) CURSOR_PRIORITY, &mmSprite);

    findMarkers();

    mmPhase = MM_MAP_PREP;
    mmTileRow = 0;
    mmStep = 0;
    mmFrameCounter = 0;
    mmColorHere = 0x0F;
    mmColorMarker = 0x0F;
    mmSpritesOn = FALSE;
    //The NES doesn't read the pad while drawing, so the first press check compares against the overworld's last read.
    mmPadPrev = PadData1;
}

void doMinimap() {
    PadBits pressed;

    switch(mmPhase) {
        case MM_MAP_PREP:
            prepTileRow(mmTileRow);
            mmStep = 0;
            mmPhase = MM_MAP_ROWS;
            //fall through
        case MM_MAP_ROWS:
            drawPixelRow(mmTileRow, mmStep);
            mmStep++;
            if(mmStep == 8) {
                mmTileRow++;
                mmPhase = (mmTileRow == 16) ? MM_DECOR : MM_MAP_PREP;
                mmStep = 0;
            }
            break;
        case MM_DECOR:
            drawGraphicStep((Uint8 *)MINIMAP_DECOR_CHR, (Uint8 *)MINIMAP_DECOR_DEST_LO, (Uint8 *)MINIMAP_DECOR_DEST_HI, MINIMAP_DECOR_TILES, mmStep);
            mmStep++;
            if(mmStep == 16) {
                mmStep = 0;
                mmPhase = MM_TITLE;
            }
            break;
        case MM_TITLE:
            drawGraphicStep((Uint8 *)MINIMAP_TITLE_CHR, (Uint8 *)MINIMAP_TITLE_DEST_LO, (Uint8 *)MINIMAP_TITLE_DEST_HI, MINIMAP_TITLE_TILES, mmStep);
            mmStep++;
            if(mmStep == 16) {
                mmStep = 0;
                mmPhase = MM_DONE;
            }
            break;
        case MM_DONE:
            //NES MinimapFrame: the colors drawn this frame were set by the previous one.
            setSpritePalette();
            mmSpritesOn = TRUE;
            musicHold = FALSE;

            pressed = PadData1 & ~mmPadPrev;
            mmPadPrev = PadData1;

            mmFrameCounter++;
            mmColorHere = (mmFrameCounter & 0x08) ? 0x0F : 0x37;
            mmColorMarker = mmFrameCounter & 0x30;
            if(mmColorMarker == 0x30) {
                mmColorMarker = 0x0F;
            }

            if(pressed & (PAD_A | PAD_B | PAD_C)) {
                exitOwMenu();
            }
            break;
    }
}

void minimap_draw() {
    if(!mmSpritesOn) {
        return;
    }

    //NES sprites draw 1 line below their OAM Y.
    mmSprite.char_num = MINIMAP_HERE_CHAR;
    mmSprite.xPos = FIXED(mmHereX + 32);
    mmSprite.yPos = FIXED(mmHereY + 1);
    sprite_draw(&mmSprite);

    mmSprite.char_num = MINIMAP_MARKER_CHAR;
    for(Uint8 i = 0; i < mmMarkerCount; i++) {
        if(mmMarkerVisible[i]) {
            mmSprite.xPos = FIXED(mmMarkerX[i] + 32);
            mmSprite.yPos = FIXED(mmMarkerY[i] + 1);
            sprite_draw(&mmSprite);
        }
    }
}
