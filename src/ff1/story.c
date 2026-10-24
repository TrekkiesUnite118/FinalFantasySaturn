#include <string.h>
#include "types.h"
#include "video.h"
#include "graphicrefs.h"
#include "nesutils.h"
#include "player.h"
#include "menu.h"
#include "sprdata.h"
#include "vblank.h"
#include "cd.h"
#include "memmap.h"
#include "static.h"
#include "music.h"
#include "story.h"
#include "romaddr.h"

#define CREDITS_PAGES (4)

//NES story box, and its interior in pixels.
#define STORY_BOX_X (4)
#define STORY_BOX_Y (4)
#define STORY_BOX_W (0x12)
#define STORY_BOX_H (0x0C)
#define STORY_BOX_PAL (3)
#define STORY_INNER_LEFT (((STORY_BOX_X + 1) + CENTER_ADJUSTMENT) * 8)
#define STORY_INNER_RIGHT (((STORY_BOX_X + STORY_BOX_W - 1) + CENTER_ADJUSTMENT) * 8 - 1)
//The 5 visible shutters, 16 lines each.
#define SHUTTER_TOP (40)
#define SHUTTER_COUNT (5)
#define SHUTTER_LINES (16)

//"The End" is drawn into tiles $80-$E3, 10x10 tiles at NT column 8, row 5.
#define THE_END_TILE (0x80)
#define THE_END_X (8)
#define THE_END_Y (5)
#define THE_END_SIZE (80)
#define THE_END_CLEAR_TILES (0x70)

enum {
    STORY_STEP_START,
    STORY_STEP_FIRST_PAGE,
    STORY_STEP_PAGE_DRAW,
    STORY_STEP_OPEN,
    STORY_STEP_WAIT,
    STORY_STEP_CLOSE,
    STORY_STEP_STALL,
    STORY_STEP_PAL_CYCLE,
    STORY_STEP_END_BOX,
    STORY_STEP_THE_END
};

enum {
    THE_END_CLEAR,
    THE_END_FILL_NT,
    THE_END_ATTR,
    THE_END_PAL,
    THE_END_CLEAR_BUF,
    THE_END_OUTLINE,
    THE_END_FILL,
    THE_END_DONE
};

static Uint8 storyEnding;
static Uint8 storyStep;
static Uint8 storyPage; //NES story_page.
static Uint8 storyCredits; //NES story_credits.
static Uint8 storyPal[16]; //NES cur_pal BG colors.
static Uint8 storyAttr[64]; //Attribute table of the NES $2000 nametable.
static Uint8 storyClosed; //Closed lines at the top of each shutter this frame.
static Uint8 storyShutter; //NES shutter_a or shutter_b, whichever is the closed count.
static Uint8 storyFrame; //NES framecounter, which these loops count themselves.
static Uint8 storyTimer; //NES story_timer.
static Uint16 storyWait;
static PadBits storyPadPrev;
static Uint8 storyPalCycCtr;

static Uint8 theEndStep;
static Uint8 *theEndSrc; //NES theend_src.
static Sint16 theEndX; //NES theend_x.
static Sint16 theEndY;
static Uint8 theEndFilling;
static Sint16 theEndFillX;
static Sint16 theEndFillY;
static Uint8 theEndBuf[THE_END_SIZE][THE_END_SIZE / 8]; //NES theend_drawbuf.
static Uint32 theEndTiles[100][8];
static volatile Sint16 theEndDirtyTile = -1;
static volatile Uint8 theEndClearPending;

//Picks the scene TO_STORY sets up.
void storySelect(Uint8 ending) {
    storyEnding = ending;
}

static void storyUploadPalettes() {
    Uint32 *pals[4] = {menu_pal_0, menu_pal_1, menu_pal_2, menu_pal_3};

    for(Uint8 p = 0; p < 4; p++) {
        for(Uint8 i = 0; i < 4; i++) {
            pals[p][i] = nes_pal[storyPal[(p * 4) + i]];
        }
        vidSetPalette(VID_NBG1, p * 16, 16, pals[p]);
    }
}

//Sets each tile's palette in rows first to last from storyAttr.
static void storyApplyAttr(Uint8 first, Uint8 last) {
    for(Uint8 r = first; r <= last; r++) {
        for(Uint8 c = 0; c < 32; c++) {
            Uint8 shift = ((r & 0x02) << 1) | (c & 0x02);
            Uint8 pal = (storyAttr[((r >> 2) * 8) + (c >> 2)] >> shift) & 0x03;
            Uint16 *cell = &menuPlane[r][c + CENTER_ADJUSTMENT];
            *cell = (*cell & 0x0FFF) | (pal << 12);
        }
    }
}

//NES LoadBridgeSceneGFX/LoadEpilogueSceneGFX, then Bridge_StartPPU/Ending_StartPPU.
void setupStoryScreen() {
    Uint8 *nt = storyEnding ? EPILOGUE_NT : BRIDGE_NT;
    Uint8 *pal = storyEnding ? LUT_ENDING_PAL : LUT_BRIDGE_PAL;

    musicApuWrite(0x4015, 0);
    if(storyEnding) {
        music_track = MUSIC_ENDING;
    }

    convertNesTilesToSatTiles(storyEnding ? EPILOGUE_CHR : BRIDGE_CHR, (Uint8 *)LWRAM_BUFFER, 0x800);
    memcpy((Uint8 *)LWRAM_BUFFER + 0x1000, (Uint8 *)LWRAM_MENU_TILES + 0x1000, 0x1000);

    memcpy(storyAttr, nt + 0x3C0, sizeof(storyAttr));
    //Outside the NES picture: tile $FF is solid color 2, which is $0F in palette 3 of both scenes.
    for(Uint8 r = 0; r < 64; r++) {
        for(Uint8 c = 0; c < 64; c++) {
            menuPlane[r][c] = (3 << 12) | 0xFF;
        }
    }
    for(Uint8 r = 0; r < 30; r++) {
        for(Uint8 c = 0; c < 32; c++) {
            menuPlane[r][c + CENTER_ADJUSTMENT] = nt[(r * 32) + c];
        }
    }
    storyApplyAttr(0, 29);

    memcpy(storyPal, pal, sizeof(storyPal));
    for(Uint8 i = 0; i < 4; i++) {
        menu_pal_0[i] = nes_pal[storyPal[i]];
        menu_pal_1[i] = nes_pal[storyPal[4 + i]];
        menu_pal_2[i] = nes_pal[storyPal[8 + i]];
        menu_pal_3[i] = nes_pal[storyPal[12 + i]];
    }
    static_init_tiles((Uint8 *)LWRAM_BUFFER);

    if(!storyEnding) {
        music_track = MUSIC_BRIDGE;
    }
    storyPage = storyEnding ? *LUT_BRIDGE_LAST_PAGE : 0;
    storyCredits = FALSE;
    storyClosed = 0;
    storyPadPrev = PadData1;
    theEndDirtyTile = -1;
    theEndClearPending = FALSE;
    storyStep = STORY_STEP_START;
}

//NES Story_DrawText for credits: strings of raw tiles, each after a 2 byte PPU address. Returns the string count.
static Uint8 storyDrawCredits() {
    Uint8 *ptr = LUT_CREDITS_TEXT + (storyPage * 2);
    Uint8 *s = ROM_PTR(0x0D, ptr[0] | (ptr[1] << 8));
    Uint8 strings = 0;

    while(TRUE) {
        Uint16 pos = (s[0] | (s[1] << 8)) & 0x3FF;
        Uint8 c;

        s += 2;
        strings++;
        while((c = *s) > 0x01) {
            Uint8 row = pos >> 5;
            if(row < 30) {
                menuPlane[row][(pos & 0x1F) + CENTER_ADJUSTMENT] = (menuPlane[row][(pos & 0x1F) + CENTER_ADJUSTMENT] & 0xF000) | c;
            }
            pos++;
            s++;
        }
        if(c == 0x00) {
            return strings;
        }
        s++;
    }
}

//NES Story_DoPage up to Story_OpenShutters. Returns the frames the NES spends drawing with menustall:
//one per box row, then one per credits string, or one plus one per line break of story text.
static Uint8 storyDrawPage() {
    Uint8 frames = STORY_BOX_H;

    drawBox(STORY_BOX_X, STORY_BOX_Y, STORY_BOX_W, STORY_BOX_H, STORY_BOX_PAL);
    if(storyCredits) {
        return frames + storyDrawCredits();
    }

    Uint8 *ptr = LUT_STORY_TEXT + (storyPage * 2);
    Uint8 *text = ROM_PTR(0x0D, ptr[0] | (ptr[1] << 8));
    frames++;
    for(Uint8 *s = text; *s; s++) {
        if(*s == 0x01 || *s == 0x05) {
            frames++;
        }
    }
    drawComplexString(STORY_BOX_X + 1, STORY_BOX_Y + 2, text, STORY_BOX_PAL);
    return frames;
}

static void storyStartPage() {
    storyWait = storyDrawPage();
    storyClosed = SHUTTER_LINES;
    storyStep = STORY_STEP_PAGE_DRAW;
}

//The next page, or what follows the last one.
static void storyNextPage() {
    if(storyEnding) {
        if(storyPage < *LUT_ENDING_LAST_PAGE) {
            storyStartPage();
            return;
        }
        //NES EnterEndingScene @StoryExit: the box is redrawn empty and opened for "The End".
        drawBox(STORY_BOX_X, STORY_BOX_Y, STORY_BOX_W, STORY_BOX_H, STORY_BOX_PAL);
        storyWait = STORY_BOX_H;
        storyStep = STORY_STEP_END_BOX;
        return;
    }
    if(!storyCredits && storyPage >= *LUT_BRIDGE_LAST_PAGE) {
        storyPage = 0;
        storyCredits = TRUE;
    }
    if(!storyCredits || storyPage < CREDITS_PAGES) {
        storyStartPage();
        return;
    }
    bridgeScene = 0x80;
    storyWait = 0x20;
    storyStep = STORY_STEP_STALL;
}

static void storyStartOpen() {
    storyShutter = SHUTTER_LINES;
    storyStep = STORY_STEP_OPEN;
}

//NES Story_OpenShutters and Story_CloseShutters frames. The closed lines change every other frame.
static Uint8 storyShutterFrame(Sint8 dir) {
    storyClosed = storyShutter;
    storyFrame++;
    if(storyFrame & 0x01) {
        return FALSE;
    }
    storyShutter += dir;
    return dir < 0 ? storyShutter == 0 : storyShutter > SHUTTER_LINES;
}

//NES TheEnd_SetPixel. Only one pixel is set per frame, so one tile is copied in VBlank.
static void theEndSetPixel(Sint16 x, Sint16 y) {
    Uint8 tile = ((y >> 3) * 10) + (x >> 3);
    Uint8 *row = (Uint8 *)&theEndTiles[tile][y & 0x07];

    theEndBuf[y][x >> 3] |= 0x80 >> (x & 0x07);
    row[(x & 0x07) >> 1] |= (x & 0x01) ? 0x01 : 0x10;
    theEndDirtyTile = tile;
}

//NES TheEnd_MovePos. Left and up win over right and down.
static void theEndMove(Uint8 dir) {
    if(dir & 0x05) {
        theEndX += (dir & 0x04) ? -1 : 1;
    }
    if(dir & 0x0A) {
        theEndY += (dir & 0x08) ? -1 : 1;
    }
}

//NES DrawFancyTheEndGraphic @FillLoop_Trace. Moves are free, each filled pixel takes a frame.
static void theEndFillFrame() {
    while(TRUE) {
        if(theEndFilling) {
            theEndFillX--;
            if(theEndFillX >= 0 && !(theEndBuf[theEndFillY][theEndFillX >> 3] & (0x80 >> (theEndFillX & 0x07)))) {
                theEndSetPixel(theEndFillX, theEndFillY);
                return;
            }
            theEndFilling = FALSE;
            theEndSrc++;
        }
        Uint8 dir = *theEndSrc;
        if(dir == 0) {
            theEndStep = THE_END_DONE;
            return;
        }
        if(dir == 0x02 || dir == 0x03 || dir == 0x07) {
            theEndFillX = theEndX;
            theEndFillY = theEndY;
            theEndMove(dir);
            if(LUT_FILL_GRADIENT[theEndFillY]) {
                theEndFilling = TRUE;
                continue;
            }
        } else {
            theEndMove(dir);
        }
        theEndSrc++;
    }
}

//NES DrawFancyTheEndGraphic, one frame per call.
static void theEndFrame() {
    switch(theEndStep) {
        case THE_END_CLEAR:
            //Tiles $80-$EF are cleared $80 bytes a frame.
            if(storyWait == THE_END_CLEAR_TILES / 8) {
                theEndClearPending = TRUE;
            }
            if(--storyWait == 0) {
                for(Uint8 r = 0; r < 10; r++) {
                    for(Uint8 c = 0; c < 10; c++) {
                        menuPlane[THE_END_Y + r][THE_END_X + c + CENTER_ADJUSTMENT] =
                            (menuPlane[THE_END_Y + r][THE_END_X + c + CENTER_ADJUSTMENT] & 0xF000) | (THE_END_TILE + (r * 10) + c);
                    }
                }
                storyWait = 10;
                theEndStep = THE_END_FILL_NT;
            }
            break;
        case THE_END_FILL_NT:
            //One row of the 10x10 tiles a frame, all cleared tiles.
            if(--storyWait == 0) {
                theEndStep = THE_END_ATTR;
            }
            break;
        case THE_END_ATTR:
            memcpy(&storyAttr[0x0A], LUT_THE_END_ATTR, 0x13);
            storyApplyAttr(4, 15);
            theEndStep = THE_END_PAL;
            break;
        case THE_END_PAL:
            storyPal[1] = 0x30;
            storyPal[2] = 0x16;
            storyUploadPalettes();
            theEndStep = THE_END_CLEAR_BUF;
            break;
        case THE_END_CLEAR_BUF:
            memset(theEndBuf, 0, sizeof(theEndBuf));
            memset(theEndTiles, 0, sizeof(theEndTiles));
            theEndSrc = LUT_THE_END_DRAW;
            theEndX = 0x0C;
            theEndY = 0;
            theEndStep = THE_END_OUTLINE;
            break;
        case THE_END_OUTLINE:
            if(*theEndSrc != 0) {
                theEndMove(*theEndSrc++);
                theEndSetPixel(theEndX, theEndY);
                break;
            }
            //NES @StartFill traces the outline again from the start.
            theEndSrc = LUT_THE_END_DRAW;
            theEndX = 0x0C;
            theEndY = 0;
            theEndFilling = FALSE;
            theEndStep = THE_END_FILL;
            theEndFillFrame();
            break;
        case THE_END_FILL:
            theEndFillFrame();
            break;
        default:
            //NES EnterEndingScene @FinalLoop: only the music continues.
            break;
    }
}

//NES CyclePalettes with mode 4, cycling the scene out before EnterOW_PalCyc.
static void storyPalCycleFrame() {
    if(storyPalCycCtr != 0 && (storyPalCycCtr & 0x03) == 0) {
        Uint8 changed = 0;
        for(Uint8 i = 0; i < 16; i++) {
            Uint8 color = storyPal[i];
            if(color != 0x0F) {
                Uint8 chroma = (color & 0x0F) + 1;
                storyPal[i] = (chroma >= 0x0D) ? 0x0F : ((color & 0x30) | chroma);
                changed++;
            }
        }
        if(changed == 0) {
            storyClosed = 0;
            exitOwMenu();
            return;
        }
    }
    storyUploadPalettes();
    storyPalCycCtr++;
}

//The minigame leaves through the same cycle out as the bridge scene.
void storyStartPalCycle() {
    storyClosed = 0;
    storyPalCycCtr = 0;
    storyStep = STORY_STEP_PAL_CYCLE;
}

//NES EnterBridgeScene and EnterEndingScene, one frame per call.
void doStory() {
    PadBits pressed = PadData1 & ~storyPadPrev;
    storyPadPrev = PadData1;

    switch(storyStep) {
        case STORY_STEP_START:
            //Story_StartPPU shows the $2000 nametable for a frame before the first page closes it.
            storyClosed = 0;
            storyWait = 1;
            storyStep = STORY_STEP_FIRST_PAGE;
            break;
        case STORY_STEP_FIRST_PAGE:
            //VDP1 shows the shutters a frame after VDP2 shows plane writes, so the page waits until they're up.
            storyClosed = SHUTTER_LINES;
            if(storyWait-- == 0) {
                storyStartPage();
                //The frames already spent closed count toward the NES drawing time.
                storyWait -= 2;
            }
            break;
        case STORY_STEP_PAGE_DRAW:
        case STORY_STEP_END_BOX:
            storyClosed = SHUTTER_LINES;
            if(--storyWait == 0) {
                storyStartOpen();
            }
            break;
        case STORY_STEP_OPEN:
            if(storyShutterFrame(-1)) {
                if(storyEnding && storyPage >= *LUT_ENDING_LAST_PAGE) {
                    storyWait = THE_END_CLEAR_TILES / 8;
                    theEndStep = THE_END_CLEAR;
                    storyStep = STORY_STEP_THE_END;
                } else {
                    //NES Story_Wait: 512 frames, or until A or B unless input is dropped.
                    storyFrame = 0;
                    storyTimer = 2;
                    storyStep = STORY_STEP_WAIT;
                }
            }
            break;
        case STORY_STEP_WAIT:
            storyClosed = 0;
            if(!storyEnding && (pressed & (PAD_A | PAD_B | PAD_C))) {
                storyPage++;
                storyShutter = 1;
                storyStep = STORY_STEP_CLOSE;
                break;
            }
            if(++storyFrame == 0 && --storyTimer == 0) {
                storyPage++;
                storyShutter = 1;
                storyStep = STORY_STEP_CLOSE;
            }
            break;
        case STORY_STEP_CLOSE:
            if(storyShutterFrame(1)) {
                storyClosed = SHUTTER_LINES;
                storyNextPage();
            }
            break;
        case STORY_STEP_STALL:
            storyClosed = SHUTTER_LINES;
            if(--storyWait == 0) {
                //Empties the box under the shutters so the last page can't show during the cycle out.
                drawBox(STORY_BOX_X, STORY_BOX_Y, STORY_BOX_W, STORY_BOX_H, STORY_BOX_PAL);
                storyPalCycCtr = 0;
                storyStep = STORY_STEP_PAL_CYCLE;
            }
            break;
        case STORY_STEP_PAL_CYCLE:
            storyPalCycleFrame();
            break;
        case STORY_STEP_THE_END:
            storyClosed = 0;
            theEndFrame();
            break;
    }
}

//Closed shutters are blue boxes over the box interior, in palette 0's color 2 like the NES $2400 nametable.
void story_draw() {
    SprPoint xy[4];
    Uint32 color;
    Uint16 rgb;

    if(storyClosed == 0) {
        return;
    }
    color = nes_pal[storyPal[2]];
    rgb = RGB16_COLOR((color & 0xFF) >> 3, ((color >> 8) & 0xFF) >> 3, ((color >> 16) & 0xFF) >> 3);
    for(Uint8 i = 0; i < SHUTTER_COUNT; i++) {
        Sint16 top = SHUTTER_TOP + (i * SHUTTER_LINES);
        xy[0].x = STORY_INNER_LEFT;
        xy[0].y = top;
        xy[1].x = STORY_INNER_RIGHT;
        xy[1].y = top;
        xy[2].x = STORY_INNER_RIGHT;
        xy[2].y = top + storyClosed - 1;
        xy[3].x = STORY_INNER_LEFT;
        xy[3].y = top + storyClosed - 1;
        sprPolygon(CURSOR_PRIORITY, SPRITE_ECDSPD_OFF | SPRITE_REPLACE, rgb, xy);
    }
}

//VRAM A1 has no CPU access cycles during display, so tiles are written in VBlank.
void story_vblank_in() {
    if(theEndClearPending) {
        memset((Uint8 *)(VID_VRAM_A1 + (THE_END_TILE * 32)), 0, THE_END_CLEAR_TILES * 32);
        theEndClearPending = FALSE;
    }
    if(theEndDirtyTile >= 0) {
        volatile Uint32 *dst = (volatile Uint32 *)(VID_VRAM_A1 + ((THE_END_TILE + theEndDirtyTile) * 32));
        for(Uint8 i = 0; i < 8; i++) {
            dst[i] = theEndTiles[theEndDirtyTile][i];
        }
        theEndDirtyTile = -1;
    }
}
