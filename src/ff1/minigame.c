#include "types.h"
#include "video.h"
#include "graphicrefs.h"
#include "nesutils.h"
#include "player.h"
#include "menu.h"
#include "sprdata.h"
#include "vblank.h"
#include "cd.h"
#include "music.h"
#include "dialog.h"
#include "story.h"
#include "minigame.h"
#include "romaddr.h"

#define MG_BOX_X (8)
#define MG_BOX_Y (5)
#define MG_BOX_SIZE (0x0A)
#define MG_BOX_PAL (3)
#define MG_PIECE_X (0x48)
#define MG_PIECE_Y (0x2F)
//Each piece is a 16x8 top and bottom half, same size as the map objects sharing these chars.
#define MG_CHAR_START (384)
#define MG_SPRITE_CRAM (160)
#define MG_NO_SLIDE (-1)
#define MG_SLIDE_FRAMES (16)
#define MG_REWARD_GP (100)

//NES joy bits.
#define JOY_A (0x80)
#define JOY_B (0x40)

enum {
    MG_STEP_LOOP,
    MG_STEP_SLIDE,
    MG_STEP_SOLVED
};

typedef struct {
    Sint16 x;
    Sint16 y;
    Uint8 piece;
    Uint8 shown;
} MG_PIECE_SPR;

static Uint8 mgPuzzle[16]; //NES puzzle.
static Uint8 mgCursor;
static Uint8 mgSlideDir; //NES mg_slidedir: 0 right, 1 left, 2 down, 3 up.
static Sint8 mgSlideSpr[3]; //NES mg_slidespr.
static Uint8 mgStep;
static Uint8 mgSlideFrame;
static Uint8 mgJoyA; //NES joy_a/joy_b press counters.
static Uint8 mgJoyB;
static Uint8 mgJoyPrev; //Buttons held at the last NES UpdateJoy, standing in for joy_ignore.
static Uint8 mgJoyPrevDir;
static Uint8 mgSolvedColor;
static MG_PIECE_SPR mgOam[16]; //NES oam, 4 sprites per slot.
static Uint32 mgSprPal[16];
static Uint8 mgChr[15 * 2][64];

static const Sint8 mgDirY[4] = {0, 0, 1, -1};
static const Sint8 mgDirX[4] = {1, -1, 0, 0};

//NES INC framecounter, which leaves the high byte alone.
static Uint8 mgIncFrame() {
    Uint8 lo = (Uint8)(nesFrameCounter + 1);
    nesFrameCounter = (nesFrameCounter & 0xFF00) | lo;
    return lo;
}

//NES MiniGame_ShufflePuzzle: $7F random slides into the empty slot, skipping illegal ones.
static void mgShuffle() {
    static const Sint8 dirs[4] = {1, -1, 4, -4};

    for(Uint8 i = 0; i < 0x7F; i++) {
        Sint8 dir = dirs[LUT_RNG[mgIncFrame()] & 0x03];
        Uint8 empty = 0x0F;
        Sint8 from;

        while(mgPuzzle[empty]) {
            empty--;
        }
        from = empty + dir;
        if(from < 0 || from >= 0x10) {
            continue;
        }
        if((dir == 1 && (from & 0x03) == 0) || (dir == -1 && (from & 0x03) == 0x03)) {
            continue;
        }
        mgPuzzle[empty] = mgPuzzle[from];
        mgPuzzle[from] = 0;
    }
}

//NES lut_MinigameCHR expanded to 2bpp: the unnumbered piece is every piece's high bitplane.
static void mgLoadChr() {
    for(Uint8 p = 1; p < 16; p++) {
        for(Uint8 half = 0; half < 2; half++) {
            Uint8 *dst = mgChr[((p - 1) * 2) + half];
            for(Uint8 row = 0; row < 8; row++) {
                for(Uint8 x = 0; x < 16; x += 2) {
                    Uint8 t = (half * 2) + (x >> 3);
                    Uint8 lo = LUT_MINIGAME_CHR[(((p * 4) + t) * 8) + row];
                    Uint8 hi = LUT_MINIGAME_CHR[(t * 8) + row];
                    Uint8 bit = 7 - (x & 0x07);
                    Uint8 a = ((lo >> bit) & 1) | (((hi >> bit) & 1) << 1);
                    Uint8 b = ((lo >> (bit - 1)) & 1) | (((hi >> (bit - 1)) & 1) << 1);
                    *dst++ = (a << 4) | b;
                }
            }
            sprSetChar(MG_CHAR_START + ((p - 1) * 2) + half, SPRITE_4BPP, MG_SPRITE_CRAM, 16, 8, mgChr[((p - 1) * 2) + half]);
        }
    }
}

//NES DrawAllPuzzlePieces. The cursor's piece goes behind the solid box interior every other 2 frames.
static void mgDrawAllPieces() {
    for(Uint8 s = 0; s < 16; s++) {
        mgOam[s].x = MG_PIECE_X + ((s & 0x03) * 16);
        mgOam[s].y = MG_PIECE_Y + ((s >> 2) * 16);
        mgOam[s].piece = mgPuzzle[s];
        mgOam[s].shown = mgPuzzle[s] != 0 && !(mgCursor == s && (nesFrameCounter & 0x02));
    }
}

static Uint8 mgReadJoy() {
    Uint8 joy = 0;

    if(PadData1 & (PAD_A | PAD_C)) joy |= JOY_A;
    if(PadData1 & PAD_B) joy |= JOY_B;
    if(PadData1 & PAD_R) joy |= 0x01;
    if(PadData1 & PAD_L) joy |= 0x02;
    if(PadData1 & PAD_D) joy |= 0x04;
    if(PadData1 & PAD_U) joy |= 0x08;
    return joy;
}

//NES MiniGame_ProcessInput.
static void mgProcessInput() {
    Uint8 joy = mgReadJoy();
    Uint8 pressed = joy & ~mgJoyPrev;
    Uint8 dir = joy & 0x0F;
    Sint8 change;

    mgJoyPrev = joy;
    if(pressed & JOY_A) {
        mgJoyA++;
    }
    if(pressed & JOY_B) {
        mgJoyB++;
    }
    if(dir == mgJoyPrevDir) {
        return;
    }
    mgJoyPrevDir = dir;
    if(dir == 0) {
        return;
    }
    if(dir >= 0x04) {
        change = (dir == 0x04) ? 4 : -4;
    } else {
        change = (dir == 0x01) ? 1 : -1;
    }
    do {
        mgCursor = (mgCursor + change) & 0x0F;
    } while(mgPuzzle[mgCursor] == 0);
}

//NES MiniGame_VertSlide/HorzSlide: shifts the pieces between the cursor and the empty slot.
static void mgSlide(Uint8 empty, Sint8 step) {
    Sint8 dist = (Sint8)(mgCursor - empty) / step;
    Uint8 x = empty;

    while(x != mgCursor) {
        mgPuzzle[x] = mgPuzzle[x + step];
        x += step;
    }
    mgPuzzle[x] = 0;
    if(step == 4) {
        mgSlideDir = 3;
    } else if(step == -4) {
        mgSlideDir = 2;
    } else {
        mgSlideDir = (step == 1) ? 1 : 0;
    }
    for(Sint8 i = 0; i < dist; i++) {
        mgSlideSpr[i] = mgCursor - (i * step);
    }
    //NES MiniGame_AnimateSlide.
    mgCursor = empty;
    mgSlideFrame = 0;
    mgStep = MG_STEP_SLIDE;
}

//NES MiniGameLoop @A_Pressed.
static void mgPressA() {
    Uint8 empty = 0;

    mgJoyA = 0;
    nesFrameCounter &= 0xFF00;
    mgSlideSpr[0] = MG_NO_SLIDE;
    mgSlideSpr[1] = MG_NO_SLIDE;
    mgSlideSpr[2] = MG_NO_SLIDE;
    while(mgPuzzle[empty]) {
        empty++;
    }
    if((mgCursor & 0x03) == (empty & 0x03)) {
        mgDrawAllPieces();
        mgSlide(empty, (mgCursor > empty) ? 4 : -4);
    } else if((mgCursor & 0x0C) == (empty & 0x0C)) {
        mgDrawAllPieces();
        mgSlide(empty, (mgCursor > empty) ? 1 : -1);
    } else {
        sfxMinigameError();
    }
}

//NES MiniGame_CheckVictory. Either end can hold the empty slot, so each piece only has to follow the one before.
static Uint8 mgCheckVictory() {
    Uint8 need = mgPuzzle[0];

    for(Uint8 x = 1; x < 0x0F; x++) {
        need++;
        if(mgPuzzle[x] != need) {
            return FALSE;
        }
    }
    return TRUE;
}

static void mgUploadPal() {
    vidSetPalette(VID_SPR, MG_SPRITE_CRAM, 16, mgSprPal);
}

static void mgExit(Uint8 solved) {
    if(solved) {
        //NES MinigameReward.
        dlgAddGold(MG_REWARD_GP);
    }
    storyStartPalCycle();
    state = STORY;
}

//NES EnterMiniGame after LoadBridgeSceneGFX, which setupStoryScreen has done.
void setupMinigame() {
    Uint16 (*savedPlane)[64] = drawPlane;

    musicApuWrite(0x4015, 0);
    drawPlane = menuPlane;
    drawBox(MG_BOX_X, MG_BOX_Y, MG_BOX_SIZE, MG_BOX_SIZE, MG_BOX_PAL);
    drawPlane = savedPlane;

    mgLoadChr();
    for(Uint8 i = 0; i < 16; i++) {
        mgPuzzle[i] = LUT_PUZZLE_START[i];
        mgSprPal[i] = 0;
    }
    mgShuffle();
    music_track = MUSIC_BRIDGE;
    mgSprPal[2] = nes_pal[0x15];
    mgSprPal[3] = nes_pal[0x30];
    mgUploadPal();

    for(Uint8 s = 0; s < 16; s++) {
        mgOam[s].shown = FALSE;
    }
    mgJoyA = 0;
    mgJoyB = 0;
    mgJoyPrevDir = 0;
    //joy_ignore isn't cleared, and A and B were held to start the minigame.
    mgJoyPrev = JOY_A | JOY_B;
    mgCursor = (mgPuzzle[0] == 0) ? 1 : 0;
    mgStep = MG_STEP_LOOP;
}

//NES MiniGameLoop, MiniGame_AnimateSlide and the MiniGame_CheckVictory wait, one frame per call.
void doMinigame() {
    switch(mgStep) {
        case MG_STEP_LOOP:
            mgIncFrame();
            mgDrawAllPieces();
            mgProcessInput();
            if(mgJoyB) {
                mgExit(FALSE);
            } else if(mgJoyA) {
                mgPressA();
            } else if(mgCheckVictory()) {
                music_track = MUSIC_FANFARE;
                mgCursor = 0x12;
                mgDrawAllPieces();
                mgJoyA = 0;
                mgJoyB = 0;
                mgSolvedColor = 0x30;
                mgStep = MG_STEP_SOLVED;
            }
            break;
        case MG_STEP_SLIDE:
            for(Uint8 i = 0; i < 3; i++) {
                if(mgSlideSpr[i] != MG_NO_SLIDE) {
                    mgOam[(Uint8)mgSlideSpr[i]].x += mgDirX[mgSlideDir];
                    mgOam[(Uint8)mgSlideSpr[i]].y += mgDirY[mgSlideDir];
                }
            }
            if(++mgSlideFrame == MG_SLIDE_FRAMES) {
                mgStep = MG_STEP_LOOP;
            }
            break;
        case MG_STEP_SOLVED:
            //The color set last frame is drawn now. Only the backs cycle, once every 8 frames.
            mgSprPal[3] = nes_pal[mgSolvedColor];
            mgUploadPal();
            mgSolvedColor = (mgIncFrame() << 1) & 0x30;
            mgProcessInput();
            if(mgJoyA || mgJoyB) {
                mgExit(TRUE);
            }
            break;
    }
}

void minigame_draw() {
    SPRITE_INFO spr;

    for(Uint8 s = 0; s < 16; s++) {
        if(!mgOam[s].shown) {
            continue;
        }
        sprite_make(MG_CHAR_START + ((mgOam[s].piece - 1) * 2), FIXED(mgOam[s].x + (CENTER_ADJUSTMENT * 8)),
            FIXED(mgOam[s].y + 1), (Sint32) CHARACTER_PRIORITY, &spr);
        sprite_draw(&spr);
        spr.char_num++;
        spr.yPos += FIXED(8);
        sprite_draw(&spr);
    }
}
