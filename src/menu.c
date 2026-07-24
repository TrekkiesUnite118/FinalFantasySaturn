#include "menu.h"
#include "graphicrefs.h"
#include "sprite.h"
#include "player.h"
#include "battle.h"
#include "static.h"
#include "cd.h"

#define BATTLE_PALETTE_TABLE (0x00230F30)

Uint16 menuPlane[64][64] = {0x00FF};

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
                        
Uint8 lut_BackdropOrder[] = {1,2,3,4,3,4,1,2,1,2,3,4,3,4};

SPRITE_INFO playerSel1;
SPRITE_INFO playerSel2;
SPRITE_INFO playerSel3;
SPRITE_INFO playerSel4;

Uint8 canAnimatePrelude = TRUE;

Uint8 currentPreludeLine = 1;
Uint8 currentFrame = 0;
Uint8 currentCycleIter = 0;

Uint16 *btl_msg_ptr = BATTLE_MSG_TABLE;

Uint16 *battleMsg_lut = BATTLE_MSG_PTR_TABLE;

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
                    menuPlane[yIter][xIter] = tileVal;
                } else if(yIter == height + yPos - 1) {
                    tileVal = palNum << 12;
                    tileVal += BOX_BOTTOM_LEFT_CORNER_CHAR;
                    menuPlane[yIter][xIter] = tileVal;
                } else {
                    tileVal = palNum << 12;
                    tileVal += BOX_LEFT_EDGE_CHAR;
                    menuPlane[yIter][xIter] = tileVal;
                }
            } else if(xIter > xPosStart && xIter < width + xPosStart - 1) {

                if(yIter == yPos) {
                    tileVal = palNum << 12;
                    tileVal += BOX_TOP_EDGE_CHAR;
                    menuPlane[yIter][xIter] = tileVal;
                } else if(yIter == height + yPos -1) {
                    tileVal = palNum << 12;
                    tileVal += BOX_BOTTOM_EDGE_CHAR;
                    menuPlane[yIter][xIter] = tileVal;
                } else {
                    tileVal = palNum << 12;
                    tileVal += BOX_BLANK_CHAR;
                    menuPlane[yIter][xIter] = tileVal;
                }
            } else if(xIter == width + xPosStart - 1) {
                if(yIter == yPos) {
                    tileVal = palNum << 12;
                    tileVal += BOX_TOP_RIGHT_CORNER_CHAR;
                    menuPlane[yIter][xIter] = tileVal;
                } else if(yIter == height + yPos -1) {
                    tileVal = palNum << 12;
                    tileVal += BOX_BOTTOM_RIGHT_CORNER_CHAR;
                    menuPlane[yIter][xIter] = tileVal;
                } else {
                    tileVal = palNum << 12;
                    tileVal += BOX_RIGHT_EDGE_CHAR;
                    menuPlane[yIter][xIter] = tileVal;
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
            menuPlane[yPos][xIter] = tileVal;

        } else if(xIter == width + xPosStart - 1) {
            tileVal = palNum << 12;
            tileVal += BOX_BOTTOM_RIGHT_CORNER_CHAR;
            menuPlane[yPos][xIter] = tileVal;
        } else {
             tileVal = palNum << 12;
            tileVal += BOX_BOTTOM_EDGE_CHAR;
            menuPlane[yPos][xIter] = tileVal;
        }

        tileVal  = 0;
    }


}


void drawTextLine(Uint8 xPos, Uint8 yPos, Uint8 *stringPtr, Uint8 palNum, Uint8 lineSpacing) {
    Uint8 xPosStart = xPos + CENTER_ADJUSTMENT;
    Uint16 tileVal  = 0;
    Uint8 iter = xPosStart;
    Uint8 stringIter = 0;
    while(stringPtr[stringIter] != 0){
        if(stringPtr[stringIter] == 1) {
            yPos += lineSpacing;
            iter = xPosStart;
            stringIter++;
        } else {
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
        }
    }
}

void drawPreludeTextLine(Uint8 xPos, Uint8 yPos, Uint8 *stringPtr, Uint8 palNum, Uint8 lineSpacing) {
    Uint8 xPosStart = xPos + CENTER_ADJUSTMENT;
    Uint16 tileVal  = 0;
    Uint8 iter = xPosStart;
    Uint8 stringIter = 0;
    while(stringPtr[stringIter] != 0){
        if(stringPtr[stringIter] == 1) {
            yPos += lineSpacing;
            if(yPos == 15 || yPos == 20 ) {
                yPos++;
            }
            iter = xPosStart;
            stringIter++;
        } else {
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
        Uint8 *className_ptr = CLASS_NAME_OFFSET + partyInitBuff[i].class * 8;
        drawBox(partyInitBuff[i].box_xPos, partyInitBuff[i].box_yPos, 10, 10, 0);
        drawTextLine(partyInitBuff[i].class_xPos, partyInitBuff[i].class_yPos, className_ptr, 0, 1);
    }

    sprite_make(FRAME_BATTLE_START + (partyInitBuff[0].class * 6), MTH_FIXED(partyInitBuff[0].sprite_xPos + 32), MTH_FIXED(partyInitBuff[0].sprite_yPos), (Sint32) CHARACTER_PRIORITY, &playerSel1);
    sprite_make(FRAME_BATTLE_START + (partyInitBuff[1].class * 6), MTH_FIXED(partyInitBuff[1].sprite_xPos + 32), MTH_FIXED(partyInitBuff[1].sprite_yPos), (Sint32) CHARACTER_PRIORITY, &playerSel2);
    sprite_make(FRAME_BATTLE_START + (partyInitBuff[2].class * 6), MTH_FIXED(partyInitBuff[2].sprite_xPos + 32), MTH_FIXED(partyInitBuff[2].sprite_yPos), (Sint32) CHARACTER_PRIORITY, &playerSel3);
    sprite_make(FRAME_BATTLE_START + (partyInitBuff[3].class * 6), MTH_FIXED(partyInitBuff[3].sprite_xPos + 32), MTH_FIXED(partyInitBuff[3].sprite_yPos), (Sint32) CHARACTER_PRIORITY, &playerSel4);

    playerSel1.xSize = MTH_FIXED(16);
    playerSel1.ySize = MTH_FIXED(24);
    playerSel2.xSize = MTH_FIXED(16);
    playerSel2.ySize = MTH_FIXED(24);
    playerSel3.xSize = MTH_FIXED(16);
    playerSel3.ySize = MTH_FIXED(24);
    playerSel4.xSize = MTH_FIXED(16);
    playerSel4.ySize = MTH_FIXED(24);
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



void incrementPartyInitBuff(Uint8 cur_char_create){
    if(partyInitBuff[cur_char_create].class == 5){
        partyInitBuff[cur_char_create].class = 0;
    } else {
       partyInitBuff[cur_char_create].class++;
    }
    
    Uint8 *className2_ptr = CLASS_NAME_OFFSET + partyInitBuff[cur_char_create].class * 8;
    drawTextLine(partyInitBuff[cur_char_create].class_xPos, partyInitBuff[cur_char_create].class_yPos, className2_ptr, 0, 1);
}

void decrementPartyInitBuff(Uint8 cur_char_create){
    if(partyInitBuff[cur_char_create].class == 0){
        partyInitBuff[cur_char_create].class = 5;
    } else {
       partyInitBuff[cur_char_create].class--;
    }
    Uint8 *className2_ptr = CLASS_NAME_OFFSET + partyInitBuff[cur_char_create].class * 8;
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
    //Respond Rate Box
    drawBox(8, 20, 16, 4, 0);

    Uint8 *textPtr1 = CONTINUE_OFFSET;
    drawTextLine(12, 12, textPtr1, 0, 1);
    
    Uint8 *textPtr2 = NEW_GAME_OFFSET;
    drawTextLine(12, 17, textPtr2, 0, 1);

    Uint8 *textPtr3 = RESPOND_RATE_OFFSET;
    drawTextLine(9, 22, textPtr3, 0, 1);
    
    //Copyright Text
    Uint8 *textPtr4 = COPYRIGHT_1_OFFSET;
    drawTextLine(8, 25, textPtr4, 0, 1);
    
    Uint8 *textPtr5 = COPYRIGHT_2_OFFSET;
    drawTextLine(8, 26, textPtr5, 0, 1);

   
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

    Uint8 *textPtr = NAME_INPUT_LUT_OFFSET;
    drawTextLine(6, 10, textPtr, 0, 2);

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

    
    Uint16 tileVal  = 0x00FF;

    for(Uint8 y = 0; y < 64; y++ ){
        for (Uint8 x = 0; x < 64; x++) {
            menuPlane[y][x] = tileVal;
        }
    }
    
    Uint8 *textPtr = PRELUDE_TEXT_OFFEST;
    drawPreludeTextLine(1, 3, textPtr, 0, 2);
}

void preludeAnimate() {
    Uint16 newTile = 0;
    Uint16 tileVal = 0;
    Uint8 charVal = 0;
    Uint8 palVal = 0;
    
    if(currentPreludeLine < 27) {
        if(currentFrame == 15) {
            currentFrame = 0;
            currentCycleIter++;
            if(currentCycleIter == 4) {
                if(currentPreludeLine == 15 || currentPreludeLine == 20) {
                    currentPreludeLine++;
                } else {
                   currentPreludeLine+=2;
                }
                currentCycleIter = 0;
            }
        } else {
            for(Uint8 iter = 0; iter < 40; iter++) {
                tileVal = menuPlane[currentPreludeLine][iter];
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
                menuPlane[currentPreludeLine][iter] = newTile;
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

    Uint8 *pal_ptr = BATTLE_BACKDROP_PALETTE_OFFSET + (4 * battleValue);

    menu_pal_2[0] = nes_pal[pal_ptr[0]];
    menu_pal_2[1] = nes_pal[pal_ptr[1]];
    menu_pal_2[2] = nes_pal[pal_ptr[2]];
    menu_pal_2[3] = nes_pal[pal_ptr[3]];

    drawMainBattleBoxes();

    Uint8 *statWidth = BATTLE_STAT_WIDTH;
    Uint8 *statXPos = BATTLE_STAT_XPOS;
    Uint8 *statYPos = BATTLE_STAT_YPOS;

    //Stat Boxes
    for(Uint8 i = 0; i < 4; i++) {
        drawBox(*statXPos - 1, *statYPos, *statWidth + 2, *statWidth + 1, BATTLE_MENU_PALETTE);
        drawTextLine(*statXPos, *statYPos + 2, party[3 - i].name, BATTLE_MENU_PALETTE, 1);
        statYPos += 5;
    }
    
    updateAilmentandHP();
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
    
    Uint8 *battleBox_xy = BATTLE_MAIN_BOX_XY;
    Uint8 *battleBox_width = BATTLE_MAIN_BOX_WIDTH;
    Uint8 *battleBox_height = BATTLE_MAIN_BOX_HEIGHT;
    drawBottomBoxLine(*battleBox_xy - 1, *battleBox_xy + *battleBox_height - 1, *battleBox_width, BATTLE_MENU_PALETTE);

    Uint8 *partyBox_x = BATTLE_CHAR_BOX_X;
    Uint8 *partyBox_y = BATTLE_CHAR_BOX_Y;
    Uint8 *partyBox_width = BATTLE_CHAR_BOX_WIDTH;
    Uint8 *partyBox_height = BATTLE_CHAR_BOX_HEIGHT;
    drawBottomBoxLine(*partyBox_x - 1, *partyBox_y + *partyBox_height - 1, *partyBox_width, BATTLE_MENU_PALETTE);
}

void drawMainBattleBoxes() {
    Uint8 *battleBox_xy = BATTLE_MAIN_BOX_XY;
    Uint8 *battleBox_width = BATTLE_MAIN_BOX_WIDTH;
    Uint8 *battleBox_height = BATTLE_MAIN_BOX_HEIGHT;

    //Enemy Box
    drawBox(*battleBox_xy - 1, *battleBox_xy, *battleBox_width, *battleBox_height, BATTLE_MENU_PALETTE);

    Uint8 *partyBox_x = BATTLE_CHAR_BOX_X;
    Uint8 *partyBox_y = BATTLE_CHAR_BOX_Y;
    Uint8 *partyBox_width = BATTLE_CHAR_BOX_WIDTH;
    Uint8 *partyBox_height = BATTLE_CHAR_BOX_HEIGHT;

    //Party Box
    drawBox(*partyBox_x - 1, *partyBox_y, *partyBox_width, *partyBox_height, BATTLE_MENU_PALETTE);

    for(Uint8 i = 0; i < 4; i++) {
        drawBattleBackdropRow(i);
    }
}

void clearArea(Uint8 xPos, Uint8 yPos, Uint8 width, Uint8 height) {
    for(Uint8 y = yPos; y < yPos + height; y++) {
        for(Uint8 x = xPos + CENTER_ADJUSTMENT; x < width + xPos + CENTER_ADJUSTMENT; x++) {
             menuPlane[y][x] = 0x00;
        }
    }
}

void drawBattleBackdropRow(Uint8 row) {
    
    Uint8 *bg_width = BATTLE_MAIN_BACKGROUND_WIDTH_1;
    
    Uint8 *battleBox_xy = BATTLE_MAIN_BOX_XY;
  
    Uint8 xPos = *battleBox_xy + CENTER_ADJUSTMENT;
    Uint8 yPos = *battleBox_xy + 1;
    Uint16 tileVal = 0;
    for(Uint8 i = 0; i < *bg_width; i++) {
        tileVal = 2 << 12;
        tileVal += lut_BackdropOrder[i];
        menuPlane[row + yPos][xPos + i] = tileVal + (row * 4);
    }

    xPos += *bg_width + 2;
    bg_width = BATTLE_MAIN_BACKGROUND_WIDTH_2;
    
    for(Uint8 i = 0; i < *bg_width; i++) {
        tileVal = 2 << 12;
        tileVal += lut_BackdropOrder[i];
        menuPlane[row + yPos][xPos + i] = tileVal + (row * 4);
    }

}

void updateAilmentandHP() {
    Uint8 yAilmentPos = 7;

    for(Uint8 i = 0; i < 4; i++) {
        Uint8 ailmentByte = determineAilmentBit(i);
        ailmentByte += 0x41;
    
        btl_msg_ptr = BATTLE_MSG_TABLE;
        Uint16 tmpOffset = battleMsg_lut[ailmentByte - 2];
        btl_msg_offset = (tmpOffset >> 8) | (tmpOffset << 8) - 0x8C40;

        btl_msg_ptr += btl_msg_offset;
        drawTextLine(27, yAilmentPos, (Uint8*) BATTLE_MSG_TABLE + btl_msg_offset, BATTLE_MENU_PALETTE, 1);
        drawHpNum(battle_char_stats[i].hp, 27, yAilmentPos +1);
        yAilmentPos+=6;
    }
}

Uint8 determineAilmentBit(Uint8 currChar) {
    Uint8 ailmentByte = battle_char_stats[currChar].ailment;

    Uint8 ailmentIter = 0;

    // counting the position of first set bit
    for (Uint8 i = 0; i < 8; i++) {
        if (!(ailmentByte & (1 << i)))
            ailmentIter++;
        else
            break;
    }
    if(ailmentIter == 0) {
        ailmentIter = 7;
    }

    return ailmentIter;

}

void drawHpNum(Uint32 num, Uint8 xPos, Uint8 yPos) {

    Uint8 numBuffer[4] = {0xFF, 0xFF, 0xFF, 0XFF};

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

    drawTextLine(xPos, yPos, &numBuffer, BATTLE_MENU_PALETTE, 1);

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
    Uint8 stringBuffer[21] = {0xFF};
    

    Uint16 tmpOffset = battleMsg_lut[BTLMSG_MONSTERS_ - 1];
    btl_msg_offset = (tmpOffset >> 8) | (tmpOffset << 8);
    btl_msg_offset -= 0x8C40;
    Uint8 *monsters_msg_ptr = (Uint8*) BATTLE_MSG_TABLE;
    monsters_msg_ptr += btl_msg_offset;

    tmpOffset = battleMsg_lut[BTLMSG_PERISHED - 1];
    btl_msg_offset = (tmpOffset >> 8) | (tmpOffset << 8);
    btl_msg_offset -= 0x8C40;
    Uint8 *perished_msg_ptr = (Uint8*) BATTLE_MSG_TABLE;
    perished_msg_ptr += btl_msg_offset;

    for(Uint8 i = 0; i < 9; i++) {
        stringBuffer[i] = monsters_msg_ptr[i];
    }

    
    for(Uint8 i = 0; i < 8; i++) {
        stringBuffer[i + 9] = perished_msg_ptr[i];
    }
    
    drawTextLine(xPos, yPos++, stringBuffer, BATTLE_MENU_PALETTE, 1);

}

void drawPartyPerished(Uint8 xPos, Uint8 yPos, Uint8 *name_ptr, Uint8 pal);

void clearMenuPlane() {
    memset(&menuPlane, 0x00 , 0x2000);
}
