#include <string.h>
#include "menu.h"
#include "graphicrefs.h"
#include "nesutils.h"
#include "sprdata.h"
#include "player.h"
#include "battle.h"
#include "static.h"
#include "cd.h"
#include "memmap.h"
#include "magic.h"
#include "dialog.h"

Uint16 menuPlane[64][64] = {{0x00FF}};

Uint8 boarderPalette_black[4] = {0x0F, 0x00, 0x0F, 0x30};
Uint8 boarderPalette_blue[4] = {0x0F, 0x00, 0x01, 0x30};
Uint8 boarderPalette_red[4] = {0x0F, 0x00, 0x16, 0x30};

static Uint8 prelude_pallete_1[4] = {0x0F, 0x00, 0x01, 0x01};
static Uint8 prelude_pallete_2[4] = {0x0F, 0x00, 0x01, 0x00};
static Uint8 prelude_pallete_3[4] = {0x0F, 0x00, 0x01, 0x10};
static Uint8 prelude_pallete_4[4] = {0x0F, 0x00, 0x01, 0x20};
static Uint8 prelude_pallete_5[4] = {0x0F, 0x00, 0x01, 0x30};

static PARTY_GEN *partyInitBuff;

//Box and text drawing target, redirected to msgPlane while a message block is built.
Uint16 (*drawPlane)[64] = menuPlane;

//NES battle message area: 12 rows of 25 tiles, drawn to the screen a row at a time.
#define MSG_ROW_START (18)
#define MSG_COL_START (3)
#define MSG_ROWS (12)
#define MSG_COLS (25)
#define MSG_MAX_BLOCKS (8)
#define MSG_QUEUE_SIZE (16)

static Uint16 msgPlane[64][64];
static Uint16 msgBuffer[MSG_ROWS][MSG_COLS];
static Uint16 msgBlocks[MSG_MAX_BLOCKS][MSG_ROWS][MSG_COLS]; //Area contents before each block was drawn.
static Uint8 msgBlockCount = 0;
static Uint16 (*msgSavedPlane)[64];

static Uint16 msgQueue[MSG_QUEUE_SIZE][MSG_ROWS][MSG_COLS];
static Uint8 msgQueueUndraw[MSG_QUEUE_SIZE];
static volatile Uint8 msgQueueHead = 0;
static volatile Uint8 msgQueueTail = 0;
static Uint8 msgAnimStep = 0;

static void msgCopyArea(Uint16 dst[MSG_ROWS][MSG_COLS], Uint16 src[MSG_ROWS][MSG_COLS]) {
    for(Uint8 y = 0; y < MSG_ROWS; y++) {
        for(Uint8 x = 0; x < MSG_COLS; x++) {
            dst[y][x] = src[y][x];
        }
    }
}

static void msgCopyRow(Uint16 (*plane)[64], Uint16 area[MSG_ROWS][MSG_COLS], Uint8 row, Uint8 toPlane) {
    for(Uint8 x = 0; x < MSG_COLS; x++) {
        if(toPlane) {
            plane[MSG_ROW_START + row][MSG_COL_START + x] = area[row][x];
        } else {
            area[row][x] = plane[MSG_ROW_START + row][MSG_COL_START + x];
        }
    }
}

static void msgEnqueue(Uint8 undraw) {
    Uint8 next = (msgQueueTail + 1) % MSG_QUEUE_SIZE;
    Uint8 slot = msgQueueTail;

    //When full, the newest entry is replaced so the final picture is still right.
    if(next == msgQueueHead) {
        slot = (msgQueueTail + MSG_QUEUE_SIZE - 1) % MSG_QUEUE_SIZE;
        next = msgQueueTail;
    }
    msgCopyArea(msgQueue[slot], msgBuffer);
    msgQueueUndraw[slot] = undraw;
    msgQueueTail = next;
}

//NES ClearBattleMessageBuffer: empty area with the bottom edges of the enemy and party frames.
void msgReset() {
    msgQueueHead = msgQueueTail;
    msgAnimStep = 0;
    msgBlockCount = 0;

    Uint16 (*saved)[64] = drawPlane;
    drawPlane = msgPlane;
    for(Uint8 y = 0; y < MSG_ROWS; y++) {
        for(Uint8 x = 0; x < MSG_COLS; x++) {
            msgPlane[MSG_ROW_START + y][MSG_COL_START + x] = 0x00;
        }
    }
    mainBattleBoxBottoms();
    drawPlane = saved;

    for(Uint8 y = 0; y < MSG_ROWS; y++) {
        msgCopyRow(msgPlane, msgBuffer, y, FALSE);
    }
}

//Draws between msgBeginBlock and msgEndBlock become one NES block.
void msgBeginBlock() {
    if(msgBlockCount < MSG_MAX_BLOCKS) {
        msgCopyArea(msgBlocks[msgBlockCount], msgBuffer);
    }
    for(Uint8 y = 0; y < MSG_ROWS; y++) {
        msgCopyRow(msgPlane, msgBuffer, y, TRUE);
    }
    msgSavedPlane = drawPlane;
    drawPlane = msgPlane;
}

void msgEndBlock() {
    drawPlane = msgSavedPlane;
    for(Uint8 y = 0; y < MSG_ROWS; y++) {
        msgCopyRow(msgPlane, msgBuffer, y, FALSE);
    }
    if(msgBlockCount < MSG_MAX_BLOCKS) {
        msgBlockCount++;
    }
    msgEnqueue(FALSE);
}

//NES UndrawNBattleBlocks: removes the newest blocks one at a time.
void msgUndraw(Uint8 count) {
    while(count > 0 && msgBlockCount > 0) {
        msgBlockCount--;
        msgCopyArea(msgBuffer, msgBlocks[msgBlockCount]);
        msgEnqueue(TRUE);
        count--;
    }
}

Uint8 msgGetBlockCount() {
    return msgBlockCount;
}

Uint8 msgBusy() {
    return msgQueueHead != msgQueueTail;
}

//One frame of the queued draws, called in vblank before menuPlane is uploaded.
//Drawing writes 1 row per frame top down, undrawing 2 rows per frame bottom up.
void msgAnimate() {
    if(msgQueueHead == msgQueueTail) {
        return;
    }

    Uint8 head = msgQueueHead;
    if(msgQueueUndraw[head]) {
        msgCopyRow(menuPlane, msgQueue[head], MSG_ROWS - 1 - msgAnimStep * 2, TRUE);
        msgCopyRow(menuPlane, msgQueue[head], MSG_ROWS - 2 - msgAnimStep * 2, TRUE);
        msgAnimStep++;
        if(msgAnimStep == MSG_ROWS / 2) {
            msgAnimStep = 0;
            msgQueueHead = (head + 1) % MSG_QUEUE_SIZE;
        }
    } else {
        msgCopyRow(menuPlane, msgQueue[head], msgAnimStep, TRUE);
        msgAnimStep++;
        if(msgAnimStep == MSG_ROWS) {
            msgAnimStep = 0;
            msgQueueHead = (head + 1) % MSG_QUEUE_SIZE;
        }
    }
}

Uint32 menu_pal_0[] = {
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
};
Uint32 menu_pal_1[] = {
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
};
Uint32 menu_pal_2[] = {
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
};
Uint32 menu_pal_3[] = {
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
};
Uint32 menu_pal_4[] = {
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
};

Uint8 defaultName[7]= {0};

Uint8 BATTLE_MENU_PALETTE = 0;

Uint8 lut_DTE1[80] = { 0xA8,0xFF,0xB7,0xAB,  0xB6,0xAC,0xFF,0xB7,  0xA4,0xB5,0xFF,0xA8,  0xB2,0xA7,0xB7,0xB1,
                        0xB1,0xA8,0xA8,0xFF,  0xB2,0xA4,0xAC,0xFF,  0xB9,0xFF,0xB0,0xB2,  0xFF,0xB6,0xFF,0xA4,
                        0xA8,0xB1,0xB2,0xAB,  0xB6,0xA4,0xA8,0xAB,  0xFF,0xFF,0xB5,0xAF,  0xB2,0xAA,0xA6,0xB2,
                        0x90,0xBC,0xB2,0xB5,  0xAF,0xFF,0xFF,0xA6,  0x96,0xB7,0xA9,0xB8,  0xBC,0xB7,0xAF,0xFF,
                        0xB1,0xAC,0xB5,0xBA,  0xA4,0xA4,0xBA,0xAC,  0xA5,0xB5,0xB8,0xFF,  0xAA,0xFF,0xAF,0xC3 };

Uint8 lut_DTE2[80] = { 0xFF,0xB7,0xAB,0xA8,  0xFF,0xB1,0xA4,0xFF,  0xB1,0xA8,0xB6,0xB5,  0xB8,0xFF,0xB2,0xFF,
                        0xAA,0xA4,0xB6,0xAC,  0xFF,0xB5,0xB6,0xA5,  0xA8,0xBA,0xA8,0xB5,  0xB2,0xB7,0xA6,0xB7,
                        0xB1,0xA7,0xB1,0xAC,  0xA8,0xB6,0xA7,0xA4,  0xB0,0xA9,0xFF,0xA8,  0xBA,0xFF,0xA8,0xB0,
                        0x92,0xFF,0xA9,0xB2,  0xAF,0xB3,0xBC,0xA4,  0x8A,0xA8,0xFF,0xB5,  0xB2,0xAC,0xFF,0xAB,
                        0xA8,0xB7,0xAC,0xA4,  0xA6,0xAF,0xA8,0xAF,  0xA8,0xB6,0xFF,0xAF,  0xA8,0xA7,0xAC,0xC3 };

//NES EnterIntroStory's dest_y. The Japanese text is lower, and skips no rows.
//Fade lines run from the first to before the end, like IntroStory_MainLoop's intro_ataddr
//($23C0-$23F8 US, $23D0-$23F0 Japanese).
#ifdef FF1_JP
#define PRELUDE_TEXT_Y (9)
#define PRELUDE_FIRST_LINE (9)
#define PRELUDE_END_LINE (25)
#else
#define PRELUDE_TEXT_Y (3)
#define PRELUDE_FIRST_LINE (1)
#define PRELUDE_END_LINE (27)
#endif

#ifdef FF1_JP
Uint8 jpKanaMark(Uint8 ch) {
    return (ch >= 0x48 && ch < 0x7A) ? LUT_JP_KANA_MARK[ch - 0x48] : 0xFF;
}

Uint8 jpKanaBase(Uint8 ch) {
    return (ch >= 0x48 && ch < 0x7A) ? LUT_JP_KANA_BASE[ch - 0x48] : ch;
}

//U+3000-U+30FF to the Japanese ROM's text codes, 0 if it has no tile.
static const Uint8 jpKanaCodes[0x100] = {
    0x00, 0x00, 0xB9, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x8A, 0x00, 0x8B, 0x00, 0x8C, 0x00, 0x8D, 0x00, 0x8E, 0x8F, 0x48, 0x90, 0x49, 0x91,
    0x4A, 0x92, 0x4B, 0x93, 0x4C, 0x94, 0x4D, 0x95, 0x4E, 0x96, 0x4F, 0x97, 0x50, 0x98, 0x51, 0x99,
    0x52, 0x9A, 0x53, 0x7C, 0x9B, 0x54, 0x9C, 0x55, 0x9D, 0x56, 0x9E, 0x9F, 0xA0, 0xA1, 0xA2, 0xA3,
    0x57, 0x70, 0xA4, 0x58, 0x71, 0xA5, 0x59, 0x72, 0xA6, 0x5A, 0x73, 0xA7, 0x5B, 0x74, 0xA8, 0xA9,
    0xAA, 0xAB, 0xAC, 0x7D, 0xAD, 0x7E, 0xAE, 0x7F, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0x00, 0xB5,
    0x00, 0x00, 0x7B, 0xB6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0xB7, 0xCA, 0xB8, 0xCB, 0x00, 0xCC, 0xBA, 0xCD, 0xBB, 0xCE, 0xCF, 0x5C, 0xD0, 0x5D, 0xD1,
    0x5E, 0xD2, 0x5F, 0xD3, 0x60, 0xD4, 0x61, 0xD5, 0x62, 0xD6, 0x63, 0xD7, 0x64, 0xD8, 0x65, 0xD9,
    0x66, 0xDA, 0x67, 0xBC, 0xDB, 0x68, 0xDC, 0x69, 0xDD, 0x6A, 0xDE, 0xDF, 0xE0, 0xE1, 0xE2, 0xE3,
    0x6B, 0x75, 0xE4, 0x6C, 0x76, 0xE5, 0x6D, 0x77, 0xE6, 0x6E, 0x78, 0xE7, 0x6F, 0x79, 0xE8, 0xE9,
    0xEA, 0xEB, 0xEC, 0xBD, 0xED, 0xBE, 0xEE, 0xBF, 0xEF, 0xF0, 0xF1, 0xF2, 0xF3, 0xF4, 0x00, 0xF5,
    0x00, 0x00, 0x00, 0xF6, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC2, 0x00, 0x00, 0x00,
};

Uint8 jpTextCode(const char **src) {
    static const char latin[] = "LEHP";
    const Uint8 *s = (const Uint8 *)*src;
    Uint8 ch = *s++;
    const char *l = ch ? strchr(latin, ch) : 0;
    Uint8 code = 0xFF;

    if((ch & 0xF0) == 0xE0 && s[0] && s[1]) {
        Uint16 cp = ((ch & 0x0F) << 12) | ((s[0] & 0x3F) << 6) | (s[1] & 0x3F);
        s += 2;
        if(cp >= 0x3000 && cp <= 0x30FF && jpKanaCodes[cp - 0x3000]) {
            code = jpKanaCodes[cp - 0x3000];
        } else if(cp == 0x2025) {
            //"‥"
            code = 0xC3;
        }
    } else if(ch >= '0' && ch <= '9') {
        code = 0x80 + (ch - '0');
    } else if(l) {
        code = 0xC6 + (l - latin);
    } else if(ch == '!') {
        code = 0xC4;
    } else if(ch == '?') {
        code = 0xC5;
    } else if(ch == '/') {
        code = 0x7A;
    }
    *src = (const char *)s;
    return code;
}
#endif
                        
Uint8 lut_BackdropOrder[] = {1,2,3,4,3,4,1,2,1,2,3,4,3,4};

SPRITE_INFO playerSel1;
SPRITE_INFO playerSel2;
SPRITE_INFO playerSel3;
SPRITE_INFO playerSel4;

Uint8 canAnimatePrelude = TRUE;

Uint8 currentPreludeLine = PRELUDE_FIRST_LINE;
Uint8 currentFrame = 0;
Uint8 currentCycleIter = 0;

static Uint16 *battleMsg_lut = (Uint16 *)BATTLE_MSG_PTR_TABLE;

Uint16 btl_msg_offset = 0;

#define FRAME_BATTLE_START (96)

void drawBox(Uint8 xPos, Uint8 yPos, Uint8 width, Uint8 height, Uint8 palNum) {

    Uint8 xPosStart = xPos + CENTER_ADJUSTMENT;
    Uint16 tileVal  = 0;
    for(Uint8 yIter = yPos; yIter < height + yPos; yIter++) {

        for(Uint8 xIter = xPosStart; xIter < width + xPosStart; xIter++) {

            tileVal  = 0;
            if(xIter == xPosStart) {
                if(yIter == yPos) {
                    tileVal = palNum << 12;
                    tileVal += BOX_TOP_LEFT_CORNER_CHAR;
                    drawPlane[yIter][xIter] = tileVal;
                } else if(yIter == height + yPos - 1) {
                    tileVal = palNum << 12;
                    tileVal += BOX_BOTTOM_LEFT_CORNER_CHAR;
                    drawPlane[yIter][xIter] = tileVal;
                } else {
                    tileVal = palNum << 12;
                    tileVal += BOX_LEFT_EDGE_CHAR;
                    drawPlane[yIter][xIter] = tileVal;
                }
            } else if(xIter > xPosStart && xIter < width + xPosStart - 1) {

                if(yIter == yPos) {
                    tileVal = palNum << 12;
                    tileVal += BOX_TOP_EDGE_CHAR;
                    drawPlane[yIter][xIter] = tileVal;
                } else if(yIter == height + yPos -1) {
                    tileVal = palNum << 12;
                    tileVal += BOX_BOTTOM_EDGE_CHAR;
                    drawPlane[yIter][xIter] = tileVal;
                } else {
                    tileVal = palNum << 12;
                    tileVal += BOX_BLANK_CHAR;
                    drawPlane[yIter][xIter] = tileVal;
                }
            } else if(xIter == width + xPosStart - 1) {
                if(yIter == yPos) {
                    tileVal = palNum << 12;
                    tileVal += BOX_TOP_RIGHT_CORNER_CHAR;
                    drawPlane[yIter][xIter] = tileVal;
                } else if(yIter == height + yPos -1) {
                    tileVal = palNum << 12;
                    tileVal += BOX_BOTTOM_RIGHT_CORNER_CHAR;
                    drawPlane[yIter][xIter] = tileVal;
                } else {
                    tileVal = palNum << 12;
                    tileVal += BOX_RIGHT_EDGE_CHAR;
                    drawPlane[yIter][xIter] = tileVal;
                }
            }
            tileVal  = 0;
        }
        tileVal  = 0;
    }
}

void drawBottomBoxLine(Uint8 xPos, Uint8 yPos, Uint8 width, Uint8 palNum) {

    Uint8 xPosStart = xPos + CENTER_ADJUSTMENT;
    Uint16 tileVal  = 0;
    for(Uint8 xIter = xPosStart; xIter < width + xPosStart; xIter++) {
        if(xIter == xPosStart) {
            tileVal = palNum << 12;
            tileVal += BOX_BOTTOM_LEFT_CORNER_CHAR;
            drawPlane[yPos][xIter] = tileVal;

        } else if(xIter == width + xPosStart - 1) {
            tileVal = palNum << 12;
            tileVal += BOX_BOTTOM_RIGHT_CORNER_CHAR;
            drawPlane[yPos][xIter] = tileVal;
        } else {
             tileVal = palNum << 12;
            tileVal += BOX_BOTTOM_EDGE_CHAR;
            drawPlane[yPos][xIter] = tileVal;
        }

        tileVal  = 0;
    }


}


void drawTextLine(Uint8 xPos, Uint8 yPos, Uint8 *stringPtr, Uint8 palNum, Uint8 lineSpacing) {
    Uint8 xPosStart = xPos + CENTER_ADJUSTMENT;
#ifndef FF1_JP
    Uint16 tileVal  = 0;
#endif
    Uint8 iter = xPosStart;
    Uint8 stringIter = 0;
    while(stringPtr[stringIter] != 0){
        if(stringPtr[stringIter] == 1) {
            yPos += lineSpacing;
            iter = xPosStart;
            stringIter++;
        } else {
#ifdef FF1_JP
            //Codes $02-$47 draw nothing.
            Uint8 ch = stringPtr[stringIter++];
            if(ch >= 0x48) {
                if(jpKanaMark(ch) != 0xFF) {
                    drawPlane[yPos - 1][iter] = (palNum << 12) | jpKanaMark(ch);
                }
                drawPlane[yPos][iter++] = (palNum << 12) | jpKanaBase(ch);
            }
#else
            tileVal = palNum << 12;
            if(stringPtr[stringIter] < 0x7A){
                Uint8 dteVal = stringPtr[stringIter] - 0x1A;
                tileVal += lut_DTE1[dteVal];
                drawPlane[yPos][iter] = tileVal;
                tileVal = 0;
                iter++;

                tileVal = palNum << 12;
                tileVal += lut_DTE2[dteVal];
                drawPlane[yPos][iter] = tileVal;
                tileVal = 0;
                iter++;
                stringIter++;
            } else {
                tileVal += stringPtr[stringIter];
                drawPlane[yPos][iter] = tileVal;
                tileVal = 0;
                iter++;
                stringIter++;
            }
#endif
        }
    }
}

void drawPreludeTextLine(Uint8 xPos, Uint8 yPos, Uint8 *stringPtr, Uint8 palNum, Uint8 lineSpacing) {
    Uint8 xPosStart = xPos + CENTER_ADJUSTMENT;
#ifndef FF1_JP
    Uint16 tileVal  = 0;
#endif
    Uint8 iter = xPosStart;
    Uint8 stringIter = 0;
    while(stringPtr[stringIter] != 0){
        if(stringPtr[stringIter] == 1) {
            yPos += lineSpacing;
#ifndef FF1_JP
            if(yPos == 15 || yPos == 20 ) {
                yPos++;
            }
#endif
            iter = xPosStart;
            stringIter++;
        } else {
#ifdef FF1_JP
            Uint8 ch = stringPtr[stringIter++];
            if(ch >= 0x48) {
                if(jpKanaMark(ch) != 0xFF) {
                    menuPlane[yPos - 1][iter] = (palNum << 12) | jpKanaMark(ch);
                }
                menuPlane[yPos][iter++] = (palNum << 12) | jpKanaBase(ch);
            }
#else
            tileVal = palNum << 12;
            if(stringPtr[stringIter] < 0x7A){
                Uint8 dteVal = stringPtr[stringIter] - 0x1A;
                tileVal += lut_DTE1[dteVal];
                menuPlane[yPos][iter] = tileVal;
                tileVal = 0;
                iter++;

                tileVal = palNum << 12;
                tileVal += lut_DTE2[dteVal];
                menuPlane[yPos][iter] = tileVal;
                tileVal = 0;
                iter++;
                stringIter++;
            } else {
                tileVal += stringPtr[stringIter];
                menuPlane[yPos][iter] = tileVal;
                tileVal = 0;
                iter++;
                stringIter++;
            }
#endif
        }
    }
}
    
void setupPartyCreateScreen() {
    //Set up Menu Palettes
    menu_pal_0[0] = nes_pal[boarderPalette_blue[0]];
    menu_pal_0[1] = nes_pal[boarderPalette_blue[1]];
    menu_pal_0[2] = nes_pal[boarderPalette_blue[2]];
    menu_pal_0[3] = nes_pal[boarderPalette_blue[3]];

    menu_pal_1[0] = nes_pal[boarderPalette_red[0]];
    menu_pal_1[1] = nes_pal[boarderPalette_red[1]];
    menu_pal_1[2] = nes_pal[boarderPalette_red[2]];
    menu_pal_1[3] = nes_pal[boarderPalette_red[3]];

    for(Uint8 i = 0; i < 4; i++) {
        Uint8 *className_ptr = (Uint8 *)CLASS_NAME_OFFSET +partyInitBuff[i].class * 8;
        drawBox(partyInitBuff[i].box_xPos, partyInitBuff[i].box_yPos, 10, 10, 0);
        drawTextLine(partyInitBuff[i].class_xPos, partyInitBuff[i].class_yPos, className_ptr, 0, 1);
    }

    sprite_make(FRAME_BATTLE_START + (partyInitBuff[0].class * 6), FIXED(partyInitBuff[0].sprite_xPos + 32), FIXED(partyInitBuff[0].sprite_yPos), (Sint32) CHARACTER_PRIORITY, &playerSel1);
    sprite_make(FRAME_BATTLE_START + (partyInitBuff[1].class * 6), FIXED(partyInitBuff[1].sprite_xPos + 32), FIXED(partyInitBuff[1].sprite_yPos), (Sint32) CHARACTER_PRIORITY, &playerSel2);
    sprite_make(FRAME_BATTLE_START + (partyInitBuff[2].class * 6), FIXED(partyInitBuff[2].sprite_xPos + 32), FIXED(partyInitBuff[2].sprite_yPos), (Sint32) CHARACTER_PRIORITY, &playerSel3);
    sprite_make(FRAME_BATTLE_START + (partyInitBuff[3].class * 6), FIXED(partyInitBuff[3].sprite_xPos + 32), FIXED(partyInitBuff[3].sprite_yPos), (Sint32) CHARACTER_PRIORITY, &playerSel4);

    playerSel1.xSize = FIXED(16);
    playerSel1.ySize = FIXED(24);
    playerSel2.xSize = FIXED(16);
    playerSel2.ySize = FIXED(24);
    playerSel3.xSize = FIXED(16);
    playerSel3.ySize = FIXED(24);
    playerSel4.xSize = FIXED(16);
    playerSel4.ySize = FIXED(24);
}

//NES EnterLineupMenu. ClearNT sets every attribute to the blue border palette.
//The NES box is at $0A, $0E wide. It's 2 tiles wider here for 6 character names, keeping the same center.
void setupLineupScreen() {
    for(Uint8 i = 0; i < 4; i++) {
        menu_pal_0[i] = nes_pal[boarderPalette_blue[i]];
    }
    drawBox(0x09, 0x05, 0x10, 0x13, 0);
}

void player_select_draw() {
    playerSel1.char_num = FRAME_BATTLE_START + (partyInitBuff[0].class * 6);
    playerSel2.char_num = FRAME_BATTLE_START + (partyInitBuff[1].class * 6);
    playerSel3.char_num = FRAME_BATTLE_START + (partyInitBuff[2].class * 6);
    playerSel4.char_num = FRAME_BATTLE_START + (partyInitBuff[3].class * 6);
    sprite_draw(&playerSel1);
    sprite_draw(&playerSel2);
    sprite_draw(&playerSel3);
    sprite_draw(&playerSel4);

    drawTextLine(partyInitBuff[0].name_xPos - 2, partyInitBuff[0].name_yPos, party[0].name, 0, 1);
    drawTextLine(partyInitBuff[1].name_xPos - 2, partyInitBuff[1].name_yPos, party[1].name, 0, 1);
    drawTextLine(partyInitBuff[2].name_xPos - 2, partyInitBuff[2].name_yPos, party[2].name, 0, 1);
    drawTextLine(partyInitBuff[3].name_xPos - 2, partyInitBuff[3].name_yPos, party[3].name, 0, 1);

}



//NES lut_PtyGenBuf's classes. The buffer is the ROM's copy, which class changes write over.
void resetPartyInitBuff() {
    PARTY_GEN *buff = (PARTY_GEN*) PARTY_GEN_OFFSET;

    for(Uint8 i = 0; i < 4; i++) {
        buff[i].class = i;
    }
}

void incrementPartyInitBuff(Uint8 cur_char_create){
    if(partyInitBuff[cur_char_create].class == 5){
        partyInitBuff[cur_char_create].class = 0;
    } else {
       partyInitBuff[cur_char_create].class++;
    }
    
    Uint8 *className2_ptr = (Uint8 *)CLASS_NAME_OFFSET +partyInitBuff[cur_char_create].class * 8;
    drawTextLine(partyInitBuff[cur_char_create].class_xPos, partyInitBuff[cur_char_create].class_yPos, className2_ptr, 0, 1);
}

void decrementPartyInitBuff(Uint8 cur_char_create){
    if(partyInitBuff[cur_char_create].class == 0){
        partyInitBuff[cur_char_create].class = 5;
    } else {
       partyInitBuff[cur_char_create].class--;
    }
    Uint8 *className2_ptr = (Uint8 *)CLASS_NAME_OFFSET +partyInitBuff[cur_char_create].class * 8;
    drawTextLine(partyInitBuff[cur_char_create].class_xPos, partyInitBuff[cur_char_create].class_yPos, className2_ptr, 0, 1);
}


Uint8 getPartyCreateCursorPosX(Uint8 cur_char_create) {
    return partyInitBuff[cur_char_create].cursor_xPos + 32;
}

Uint8 getPartyCreateCursorPosY(Uint8 cur_char_create){
    return partyInitBuff[cur_char_create].cursor_yPos;
}

void setupTitleScreen() {
    //Set up Menu Palettes
    menu_pal_0[0] = nes_pal[boarderPalette_blue[0]];
    menu_pal_0[1] = nes_pal[boarderPalette_blue[1]];
    menu_pal_0[2] = nes_pal[boarderPalette_blue[2]];
    menu_pal_0[3] = nes_pal[boarderPalette_blue[3]];

    menu_pal_1[0] = nes_pal[boarderPalette_red[0]];
    menu_pal_1[1] = nes_pal[boarderPalette_red[1]];
    menu_pal_1[2] = nes_pal[boarderPalette_red[2]];
    menu_pal_1[3] = nes_pal[boarderPalette_red[3]];

    //Continue Box
    drawBox(11, 10, 10, 4, 0);
    //New Game Box
    drawBox(11, 15, 10, 4, 0);
    //Respond Rate Box
    drawBox(8, 20, 16, 4, 0);

    drawTextLine(12, 12, (Uint8 *)CONTINUE_OFFSET, 0, 1);
    drawTextLine(12, 17, (Uint8 *)NEW_GAME_OFFSET, 0, 1);
    drawTextLine(9, 22, (Uint8 *)RESPOND_RATE_OFFSET, 0, 1);

#ifndef FF1_JP
    //Copyright Text
    drawTextLine(8, 25, (Uint8 *)COPYRIGHT_1_OFFSET, 0, 1);
    drawTextLine(8, 26, (Uint8 *)COPYRIGHT_2_OFFSET, 0, 1);
#endif

    partyInitBuff = (PARTY_GEN*) PARTY_GEN_OFFSET;

    char_init(partyInitBuff[0].class, 0, defaultName);
    char_init(partyInitBuff[1].class, 1, defaultName);
    char_init(partyInitBuff[2].class, 2, defaultName);
    char_init(partyInitBuff[3].class, 3, defaultName);
}


void setupNameEntryScreen() {
    //Set up Menu Palettes
    menu_pal_0[0] = nes_pal[boarderPalette_blue[0]];
    menu_pal_0[1] = nes_pal[boarderPalette_blue[1]];
    menu_pal_0[2] = nes_pal[boarderPalette_blue[2]];
    menu_pal_0[3] = nes_pal[boarderPalette_blue[3]];

    menu_pal_1[0] = nes_pal[boarderPalette_red[0]];
    menu_pal_1[1] = nes_pal[boarderPalette_red[1]];
    menu_pal_1[2] = nes_pal[boarderPalette_red[2]];
    menu_pal_1[3] = nes_pal[boarderPalette_red[3]];

    //Text Select Box
    drawBox(4, 8, 24, 20, 0);

     //Name Box
    drawBox(12, 2, 8, 4, 1);

    drawTextLine(6, 10, (Uint8 *)NAME_INPUT_LUT_OFFSET, 0, 2);
}

void setupPreludeScreen() {
    //Set up Menu Palettes
    menu_pal_0[0] = nes_pal[prelude_pallete_1[0]];
    menu_pal_0[1] = nes_pal[prelude_pallete_1[1]];
    menu_pal_0[2] = nes_pal[prelude_pallete_1[2]];
    menu_pal_0[3] = nes_pal[prelude_pallete_1[3]];

    menu_pal_1[0] = nes_pal[prelude_pallete_2[0]];
    menu_pal_1[1] = nes_pal[prelude_pallete_2[1]];
    menu_pal_1[2] = nes_pal[prelude_pallete_2[2]];
    menu_pal_1[3] = nes_pal[prelude_pallete_2[3]];

    menu_pal_2[0] = nes_pal[prelude_pallete_3[0]];
    menu_pal_2[1] = nes_pal[prelude_pallete_3[1]];
    menu_pal_2[2] = nes_pal[prelude_pallete_3[2]];
    menu_pal_2[3] = nes_pal[prelude_pallete_3[3]];

    menu_pal_3[0] = nes_pal[prelude_pallete_4[0]];
    menu_pal_3[1] = nes_pal[prelude_pallete_4[1]];
    menu_pal_3[2] = nes_pal[prelude_pallete_4[2]];
    menu_pal_3[3] = nes_pal[prelude_pallete_4[3]];

    menu_pal_4[0] = nes_pal[prelude_pallete_5[0]];
    menu_pal_4[1] = nes_pal[prelude_pallete_5[1]];
    menu_pal_4[2] = nes_pal[prelude_pallete_5[2]];
    menu_pal_4[3] = nes_pal[prelude_pallete_5[3]];

    for(Uint8 y = 0; y < 64; y++ ){
        for (Uint8 x = 0; x < 64; x++) {
            menuPlane[y][x] = 0x00FF;
        }
    }

    drawPreludeTextLine(1, PRELUDE_TEXT_Y, (Uint8 *)PRELUDE_TEXT_OFFEST, 0, 2);

    //A soft reset can come back here after an earlier prelude finished.
    currentPreludeLine = PRELUDE_FIRST_LINE;
    currentFrame = 0;
    currentCycleIter = 0;
}

void preludeAnimate() {
    Uint16 newTile = 0;
    Uint16 tileVal = 0;
    Uint8 charVal = 0;
    Uint8 palVal = 0;
    
    if(currentPreludeLine < PRELUDE_END_LINE) {
        if(currentFrame == 15) {
            currentFrame = 0;
            currentCycleIter++;
            if(currentCycleIter == 4) {
#ifndef FF1_JP
                if(currentPreludeLine == 15 || currentPreludeLine == 20) {
                    currentPreludeLine++;
                } else
#endif
                {
                   currentPreludeLine+=2;
                }
                currentCycleIter = 0;
            }
        } else {
            //Japanese lines also fade their marks' row.
            for(Uint8 row = currentPreludeLine - TEXT_MARK_ROWS; row <= currentPreludeLine; row++)
            for(Uint8 iter = 0; iter < 40; iter++) {
                tileVal = menuPlane[row][iter];
                charVal = tileVal & 0xFF;
                palVal = 4;

                if(currentFrame % 2 == 0) {
                    if(currentCycleIter == 0) {
                        palVal = 1;
                    } else if(currentCycleIter == 1) {
                        palVal = 2;
                    } else if(currentCycleIter == 2) {
                        palVal = 3;
                    } else if(currentCycleIter == 3) {
                        palVal = 4;
                    } 
                } else {
                    if(currentCycleIter == 0) {
                        palVal = 0;
                    } else if(currentCycleIter == 1) {
                        palVal = 1;
                    } else if(currentCycleIter == 2) {
                        palVal = 2;
                    } else if(currentCycleIter == 3) {
                        palVal = 3;
                    }
                }

                newTile = palVal << 12;
                newTile += charVal;
                menuPlane[row][iter] = newTile;
            }
        }
        
        currentFrame++;
    }
    
}

void setupBattleScreen(Uint8 battleValue) {
    
    //Set up Menu Palettes
    menu_pal_0[0] = nes_pal[boarderPalette_black[0]];
    menu_pal_0[1] = nes_pal[boarderPalette_black[1]];
    menu_pal_0[2] = nes_pal[boarderPalette_black[2]];
    menu_pal_0[3] = nes_pal[boarderPalette_black[3]];
    
    menu_pal_1[0] = nes_pal[boarderPalette_blue[0]];
    menu_pal_1[1] = nes_pal[boarderPalette_blue[1]];
    menu_pal_1[2] = nes_pal[boarderPalette_blue[2]];
    menu_pal_1[3] = nes_pal[boarderPalette_blue[3]];

    Uint8 *pal_ptr = (Uint8 *)BATTLE_BACKDROP_PALETTE_OFFSET + (4 * battleValue);

    menu_pal_2[0] = nes_pal[pal_ptr[0]];
    menu_pal_2[1] = nes_pal[pal_ptr[1]];
    menu_pal_2[2] = nes_pal[pal_ptr[2]];
    menu_pal_2[3] = nes_pal[pal_ptr[3]];

    drawMainBattleBoxes();

    Uint8 *statWidth = (Uint8 *)BATTLE_STAT_WIDTH;
    Uint8 *statXPos = (Uint8 *)BATTLE_STAT_XPOS;
    Uint8 *statYPos = (Uint8 *)BATTLE_STAT_YPOS;

    //Stat Boxes. Their contents are drawn after the fade in.
    for(Uint8 i = 0; i < 4; i++) {
        drawBox(*statXPos - 1, *statYPos, *statWidth + 2, *statWidth + 1, BATTLE_MENU_PALETTE);
        statYPos += 5;
    }
}

//The stat box LUT runs bottom to top, so character 3's box is first.
void drawCharacterStatus(Uint8 index) {
    Uint8 *statXPos = (Uint8 *)BATTLE_STAT_XPOS;
    Uint8 *statYPos = (Uint8*) BATTLE_STAT_YPOS + (5 * (3 - index));

    drawTextLine(*statXPos, *statYPos + 2, party[index].name, BATTLE_MENU_PALETTE, 1);
    drawAilmentAndHP(index);
}


void initEnemyPatternData(Uint8 *patternData, Uint8 pal_id_0, Uint8 pal_id_1) {

    
    Uint16 pal_index_0 = pal_id_0 * 4;

    Uint32 pal_offset_0 = BATTLE_PALETTE_TABLE + pal_index_0;

    Uint8 *pal_ptr_0 = (Uint8*) pal_offset_0;

    menu_pal_3[0] = nes_pal[pal_ptr_0[0]];
    menu_pal_3[1] = nes_pal[pal_ptr_0[1]];
    menu_pal_3[2] = nes_pal[pal_ptr_0[2]];
    menu_pal_3[3] = nes_pal[pal_ptr_0[3]];


    Uint16 pal_index_1 = pal_id_1 * 4;

    Uint32 pal_offset_1 = BATTLE_PALETTE_TABLE + pal_index_1;

    Uint8 *pal_ptr_1 = (Uint8*) pal_offset_1;

    menu_pal_4[0] = nes_pal[pal_ptr_1[0]];
    menu_pal_4[1] = nes_pal[pal_ptr_1[1]];
    menu_pal_4[2] = nes_pal[pal_ptr_1[2]];
    menu_pal_4[3] = nes_pal[pal_ptr_1[3]];

    convertNesTilesToSatTiles(patternData, (Uint8 *)(LWRAM_MENU_TILES + 0x240), 0x6E0);
}

void drawEnemy(Uint8 enemy_num, Uint8 palette_assignment, Uint8 xPos, Uint8 yPos) {
   
    Uint16 tileVal  = 0;
    palette_assignment +=3;
    Uint8 xPosStart = xPos + CENTER_ADJUSTMENT - 1;
    Uint8 char_start = 18;
    
    if(enemy_num > 1) {
        //draw Large Enemy
        char_start += 32;
        if(enemy_num == 3) {
            char_start += 36;
        }

        for(Uint8 yIter = yPos; yIter < (6 + yPos); yIter++) {

            for(Uint8 xIter = xPosStart; xIter < 6 + xPosStart; xIter++) {
                tileVal = 0;
                tileVal = palette_assignment << 12;
                tileVal += char_start;
                menuPlane[yIter][xIter] = tileVal;
                char_start++;
            }
        }
    } else {
        //draw Small Enemy

         if(enemy_num == 1) {
            char_start += 16;
        }

        for(Uint8 yIter = yPos; yIter < 4 + yPos; yIter++) {

            for(Uint8 xIter = xPosStart; xIter < 4 + xPosStart; xIter++) {
                tileVal = 0;
                tileVal = palette_assignment << 12;
                tileVal += char_start;
                menuPlane[yIter][xIter] = tileVal;
                char_start++;
            }
        }
    }
}

void mainBattleBoxBottoms() {
    
    Uint8 *battleBox_xy = (Uint8 *)BATTLE_MAIN_BOX_XY;
    Uint8 *battleBox_width = (Uint8 *)BATTLE_MAIN_BOX_WIDTH;
    Uint8 *battleBox_height = (Uint8 *)BATTLE_MAIN_BOX_HEIGHT;
    drawBottomBoxLine(*battleBox_xy - 1, *battleBox_xy + *battleBox_height - 1, *battleBox_width, BATTLE_MENU_PALETTE);

    Uint8 *partyBox_x = (Uint8 *)BATTLE_CHAR_BOX_X;
    Uint8 *partyBox_y = (Uint8 *)BATTLE_CHAR_BOX_Y;
    Uint8 *partyBox_width = (Uint8 *)BATTLE_CHAR_BOX_WIDTH;
    Uint8 *partyBox_height = (Uint8 *)BATTLE_CHAR_BOX_HEIGHT;
    drawBottomBoxLine(*partyBox_x - 1, *partyBox_y + *partyBox_height - 1, *partyBox_width, BATTLE_MENU_PALETTE);
}

void drawMainBattleBoxes() {
    Uint8 *battleBox_xy = (Uint8 *)BATTLE_MAIN_BOX_XY;
    Uint8 *battleBox_width = (Uint8 *)BATTLE_MAIN_BOX_WIDTH;
    Uint8 *battleBox_height = (Uint8 *)BATTLE_MAIN_BOX_HEIGHT;

    //Enemy Box
    drawBox(*battleBox_xy - 1, *battleBox_xy, *battleBox_width, *battleBox_height, BATTLE_MENU_PALETTE);

    Uint8 *partyBox_x = (Uint8 *)BATTLE_CHAR_BOX_X;
    Uint8 *partyBox_y = (Uint8 *)BATTLE_CHAR_BOX_Y;
    Uint8 *partyBox_width = (Uint8 *)BATTLE_CHAR_BOX_WIDTH;
    Uint8 *partyBox_height = (Uint8 *)BATTLE_CHAR_BOX_HEIGHT;

    //Party Box
    drawBox(*partyBox_x - 1, *partyBox_y, *partyBox_width, *partyBox_height, BATTLE_MENU_PALETTE);

    //Transparent interiors so the magic flash shows. The NES clears tile $FF instead, which also flashes the text boxes.
    clearArea(*battleBox_xy, *battleBox_xy + 1, *battleBox_width - 2, *battleBox_height - 2);
    clearArea(*partyBox_x, *partyBox_y + 1, *partyBox_width - 2, *partyBox_height - 2);

    for(Uint8 i = 0; i < 4; i++) {
        drawBattleBackdropRow(i);
    }
}

void clearArea(Uint8 xPos, Uint8 yPos, Uint8 width, Uint8 height) {
    for(Uint8 y = yPos; y < yPos + height; y++) {
        for(Uint8 x = xPos + CENTER_ADJUSTMENT; x < width + xPos + CENTER_ADJUSTMENT; x++) {
             drawPlane[y][x] = 0x00;
        }
    }
}

void drawBattleBackdropRow(Uint8 row) {
    
    Uint8 *bg_width = (Uint8 *)BATTLE_MAIN_BACKGROUND_WIDTH_1;

    Uint8 *battleBox_xy = (Uint8 *)BATTLE_MAIN_BOX_XY;
  
    Uint8 xPos = *battleBox_xy + CENTER_ADJUSTMENT;
    Uint8 yPos = *battleBox_xy + 1;
    Uint16 tileVal = 0;
    for(Uint8 i = 0; i < *bg_width; i++) {
        tileVal = 2 << 12;
        tileVal += lut_BackdropOrder[i];
        menuPlane[row + yPos][xPos + i] = tileVal + (row * 4);
    }

    xPos += *bg_width + 2;
    bg_width = (Uint8 *)BATTLE_MAIN_BACKGROUND_WIDTH_2;
    
    for(Uint8 i = 0; i < *bg_width; i++) {
        tileVal = 2 << 12;
        tileVal += lut_BackdropOrder[i];
        menuPlane[row + yPos][xPos + i] = tileVal + (row * 4);
    }

}

void updateAilmentandHP() {
    for(Uint8 i = 0; i < 4; i++) {
        drawAilmentAndHP(i);
    }
}

void drawAilmentAndHP(Uint8 index) {
    Uint8 yAilmentPos = 7 + (6 * index);

    drawBattleMsg(27, yAilmentPos, determineAilmentBit(index) + 0x41, BATTLE_MENU_PALETTE);
    drawHpNum(battle_char_stats[index].hp, 27, yAilmentPos + 1);
}

Uint8 determineAilmentBit(Uint8 currChar) {
    Uint8 ailmentByte = battle_char_stats[currChar].ailment;

    Uint8 ailmentIter = 0;

    //First set bit of 0-6. 7 means none, which shows "HP".
    while(ailmentIter < 7 && !(ailmentByte & (1 << ailmentIter))) {
        ailmentIter++;
    }

    return ailmentIter;

}

void drawHpNum(Uint32 num, Uint8 xPos, Uint8 yPos) {

    Uint8 numBuffer[5] = {0xFF, 0xFF, 0xFF, 0XFF, 0x00};

    Uint8 right_col = 3; //rightmost column
    Uint8 i;
    Uint8 numOfDigits = 3;
    if(num < 1000) {
        numOfDigits = 2;
    }
    if(num < 100) {
        numOfDigits = 1;
    }
    if(num < 10) {
        numOfDigits = 0;
    }
    Uint8 charVal = 0xFF;

    for (i = 0; i <= 3; i++) {
        if(i <= numOfDigits) {
            Uint8 digitVal = (num % 10);
            charVal = digitVal + 0x80;
            numBuffer[right_col--] = charVal;
            num /= 10;
        } else {
            numBuffer[right_col--] = 0xFF;
        }
    }

    drawTextLine(xPos, yPos, numBuffer, BATTLE_MENU_PALETTE, 1);

}

void drawBattleMsg(Uint8 xPos, Uint8 yPos, Uint8 msg_id, Uint8 pal) {
    msg_id--;
    Uint16 tmpOffset = battleMsg_lut[msg_id];
    btl_msg_offset = (tmpOffset >> 8) | (tmpOffset << 8);
    btl_msg_offset -= 0x8C40;

    Uint8 *msg_ptr = (Uint8*) BATTLE_MSG_TABLE;
    msg_ptr += btl_msg_offset;
    drawTextLine(xPos, yPos++, msg_ptr, BATTLE_MENU_PALETTE, 1);
}


void drawMonstersPerished(Uint8 xPos, Uint8 yPos, Uint8 pal) {
    Uint8 stringBuffer[21];
    Uint8 len = 0;

    len = appendBattleMsg(stringBuffer, len, BTLMSG_MONSTERS_);
    len = appendBattleMsg(stringBuffer, len, BTLMSG_PERISHED);
    stringBuffer[len] = 0x00;

    drawTextLine(xPos, yPos++, stringBuffer, BATTLE_MENU_PALETTE, 1);

}

//Appends battle message msg_id up to its terminator and returns the new length.
Uint8 appendBattleMsg(Uint8 *buffer, Uint8 len, Uint8 msg_id) {
    Uint16 tmpOffset = battleMsg_lut[msg_id - 1];
    btl_msg_offset = (tmpOffset >> 8) | (tmpOffset << 8);
    btl_msg_offset -= 0x8C40;
    Uint8 *msg_ptr = (Uint8*) BATTLE_MSG_TABLE + btl_msg_offset;

    while(*msg_ptr != 0x00) {
        buffer[len++] = *msg_ptr++;
    }
    return len;
}

//Appends a character name and returns the new length.
//The NES always prints 4 name characters, here trailing blanks are trimmed.
Uint8 appendTrimmedName(Uint8 *buffer, Uint8 len, Uint8 *name_ptr) {
    Uint8 nameLen = 0;

    while(nameLen < 6 && name_ptr[nameLen] != 0x00) {
        nameLen++;
    }
    while(nameLen > 0 && name_ptr[nameLen - 1] == 0xFF) {
        nameLen--;
    }

    for(Uint8 i = 0; i < nameLen; i++) {
        buffer[len++] = name_ptr[i];
    }
    return len;
}

//Appends a number without leading zeros, like the NES DrawBattle_Number, and returns the new length.
Uint8 appendNumber(Uint8 *buffer, Uint8 len, Uint16 num) {
    Uint16 divisor = 10000;
    Uint8 started = FALSE;

    while(divisor > 1) {
        Uint8 digit = num / divisor;
        if(digit != 0 || started) {
            buffer[len++] = digit + 0x80;
            started = TRUE;
        }
        num %= divisor;
        divisor /= 10;
    }
    buffer[len++] = num + 0x80;
    return len;
}

//NES LoadPrice.
Uint16 getItemPrice(Uint8 itemId) {
    Uint8 *price = (Uint8 *)LUT_ITEM_PRICES + itemId * 2;
    return price[0] | (price[1] << 8);
}

//NES PrintNumber: right aligned in a fixed width, with leading zeros as spaces.
void formatNumber(Uint8 *buffer, Uint32 num, Uint8 digits) {
    for(Sint8 i = digits - 1; i >= 0; i--) {
        buffer[i] = 0x80 + (num % 10);
        num /= 10;
    }
    for(Uint8 i = 0; i < digits - 1 && buffer[i] == 0x80; i++) {
        buffer[i] = 0xFF;
    }
    buffer[digits] = 0x00;
}

//NES out of battle ailment byte: 0 none, 1 dead, 2 stone, 3 poison.
static Uint8 nesOBAilment(Uint8 ailment) {
    return (ailment == AIL_POISON) ? 0x03 : ailment;
}

typedef struct {
    Uint8 startX;
    Uint8 x;
    Uint8 y;
    Uint8 pal;
} COMPLEX_POS;

static void drawComplexText(COMPLEX_POS *pos, Uint8 *str);

static void putComplexTile(COMPLEX_POS *pos, Uint8 tile) {
    drawPlane[pos->y][(Uint8)(pos->x + CENTER_ADJUSTMENT)] = (pos->pal << 12) | tile;
    pos->x++;
}

//NES PrintCharStat and the DrawComplexString stat codes.
static void drawCharStat(COMPLEX_POS *pos, PLAYER_CHARACTER *c, Uint8 code) {
    Uint8 buffer[8];

    if(code == 0x00) {
        for(Uint8 i = 0; i < 6; i++) buffer[i] = c->name[i];
        buffer[6] = 0x00;
    } else if(code == 0x01) {
        drawComplexText(pos, getItemName(0xF0 + c->char_class));
        return;
    } else if(code == 0x02) {
        drawComplexText(pos, getItemName(0xFC + nesOBAilment(c->ailment)));
        return;
    } else if(code == 0x03) {
        formatNumber(buffer, c->level, 2);
    } else if(code == 0x04) {
        formatNumber(buffer, c->exp, 6);
    } else if(code == 0x05) {
        formatNumber(buffer, c->current_hp, 3);
    } else if(code == 0x06) {
        formatNumber(buffer, c->max_hp, 3);
    } else if(code >= 0x07 && code <= 0x0B) {
        Uint8 stats[5] = {c->str, c->agi, c->intel, c->vit, c->luck};
        formatNumber(buffer, stats[code - 0x07], 2);
    } else if(code < 0x14) {
        //Weapon and armor codes $0C-$13 are bugged on the NES and never used.
        return;
    } else if(code < 0x2C) {
        Uint8 level = (code - 0x14) / 3;
        Uint8 spell = c->magic[level][(code - 0x14) % 3];
        if(spell != 0) {
            drawComplexText(pos, getItemName(0xAF + (level * 8) + spell));
        }
        return;
    } else if(code < 0x34) {
        buffer[0] = 0x80 + c->current_mp[code - 0x2C];
        buffer[1] = 0x00;
    } else if(code < 0x3C) {
        buffer[0] = 0x80 + c->max_mp[code - 0x34];
        buffer[1] = 0x00;
    } else if(code < 0x42) {
        Uint8 substats[6] = {c->damage, c->hit_percent, c->absorb, c->evade_percent, c->resist, c->mag_def};
        formatNumber(buffer, substats[code - 0x3C], 3);
    } else {
        formatNumber(buffer, c->exp_to_next_lvl, 5);
    }
    drawComplexText(pos, buffer);
}

//NES DrawComplexString. Substrings continue from the current position, and line breaks return to startX.
static void drawComplexText(COMPLEX_POS *pos, Uint8 *str) {
    Uint8 buffer[8];

    while(*str != 0x00) {
        Uint8 ch = *str++;
#ifdef FF1_JP
        if(ch >= 0x48) {
            if(jpKanaMark(ch) != 0xFF) {
                drawPlane[pos->y - 1][(Uint8)(pos->x + CENTER_ADJUSTMENT)] = (pos->pal << 12) | jpKanaMark(ch);
            }
            putComplexTile(pos, jpKanaBase(ch));
#else
        if(ch >= 0x7A) {
            putComplexTile(pos, ch);
        } else if(ch >= 0x1A) {
            putComplexTile(pos, lut_DTE1[ch - 0x1A]);
            putComplexTile(pos, lut_DTE2[ch - 0x1A]);
#endif
        } else if(ch == 0x01) {
            pos->x = pos->startX;
            pos->y += 2;
        } else if(ch == 0x02) {
            drawComplexText(pos, getItemName(*str++));
        } else if(ch == 0x03) {
            formatNumber(buffer, getItemPrice(*str++), 5);
            drawComplexText(pos, buffer);
        } else if(ch == 0x04) {
            formatNumber(buffer, partyGold, 6);
            drawComplexText(pos, buffer);
        } else if(ch >= 0x10 && ch <= 0x13) {
            drawCharStat(pos, &party[ch & 0x03], *str++);
        } else {
            //The other codes are a single line break, or nothing on the Japanese ROM.
#ifndef FF1_JP
            pos->x = pos->startX;
            pos->y += 1;
#endif
        }
    }
}

void drawComplexString(Uint8 xPos, Uint8 yPos, Uint8 *str, Uint8 palNum) {
    COMPLEX_POS pos = {xPos, xPos, yPos, palNum};
    drawComplexText(&pos, str);
}

//NES lut_MenuText string.
Uint8 *getMenuString(Uint8 id) {
    Uint8 *ptr = (Uint8 *)MENU_TEXT_PTR_TABLE + (id * 2);
    return (Uint8 *)MENU_TEXT_BANK_ADD + (ptr[0] | (ptr[1] << 8));
}

//"<Name> party perished"
void drawPartyPerished(Uint8 xPos, Uint8 yPos, Uint8 *name_ptr, Uint8 pal) {
    Uint8 stringBuffer[24];
    Uint8 len = 0;

    len = appendTrimmedName(stringBuffer, len, name_ptr);
    len = appendBattleMsg(stringBuffer, len, BTLMSG__PARTY);
    len = appendBattleMsg(stringBuffer, len, BTLMSG_PERISHED);
    stringBuffer[len] = 0x00;

    drawTextLine(xPos, yPos, stringBuffer, pal, 1);
}

void clearMenuPlane() {
    memset(&menuPlane, 0x00 , 0x2000);
}

void static_init_tiles(Uint8 *tiles) {
    static Uint32 *pals[5] = {menu_pal_0, menu_pal_1, menu_pal_2, menu_pal_3, menu_pal_4};

    static_setup(tiles, pals, 5, menuPlane);
}

void static_init() {
    static_init_tiles((Uint8 *)LWRAM_MENU_TILES);
}
