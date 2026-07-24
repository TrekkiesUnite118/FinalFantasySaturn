#include <sega_def.h>
#include <sega_mth.h>
#include <sega_scl.h>
#define        _SPR2_
#include <SEGA_SPR.H>
#include <sega_sys.h>
#include <string.h>
#include "cd.h"
#include "player.h"
#include "print.h"
#include "overworld.h"
#include "scroll.h"
#include "sprite.h"
#include "vblank.h"
#include "battle.h"
#include "menu.h"


#define DOWN (MTH_FIXED(0))
#define UP (MTH_FIXED(1))
#define LEFT (MTH_FIXED(2))
#define RIGHT (MTH_FIXED(3))

#define WARRIOR_START_STAT_OFFSET (0x00203050)
#define THIEF_START_STAT_OFFSET (0x00203060)
#define MONK_START_STAT_OFFSET (0x00203070)
#define REDMAGE_START_STAT_OFFSET (0x00203080)
#define WMAGE_START_STAT_OFFSET (0x00203090)
#define BMAGE_START_STAT_OFFSET (0x002030A0)

#define WARRIOR_VALUE (0)
#define THIEF_VALUE (1)
#define MONK_VALUE (2)
#define REDMAGE_VALUE (3)
#define WMAGE_VALUE (4)
#define BMAGE_VALUE (5)
#define KNIGHT_VALUE (6)
#define NINJA_VALUE (7)
#define MASTER_VALUE (8)
#define REDWIZ_VALUE (9)
#define BLWIZ_VALUE (10)
#define WHWIZ_VALUE (11)

#define WARRIOR_OFFSET (0)
#define THIEF_OFFSET (8)
#define MONK_OFFSET (16)
#define REDMAGE_OFFSET (24)
#define WMAGE_OFFSET (32)
#define BMAGE_OFFSET (40)
#define KNIGHT_OFFSET (48)
#define NINJA_OFFSET (56)
#define MASTER_OFFSET (64)
#define REDWIZ_OFFSET (72)
#define BLWIZ_OFFSET (80)
#define WHWIZ_OFFSET (88)

#define FOOT (MTH_FIXED(1))
#define CANOE (MTH_FIXED(2))
#define SHIP (MTH_FIXED(4))
#define AIRSHIP (MTH_FIXED(8))
#define IN_FOREST (MTH_FIXED(16))

#define PLAYER_MAXXPOS (MTH_FIXED(152))
#define PLAYER_MAXYPOS (MTH_FIXED(92))
#define PLAYER_MAXYPOSB (MTH_FIXED(100))

#define FRAME_STAND (169)
#define FRAME_WALK1 (FRAME_STAND + 1)
#define FRAME_STAND2 (FRAME_STAND + 2)
#define FRAME_WALK2 (FRAME_STAND + 3)

#define FRAME_STAND3 (FRAME_STAND + 4)
#define FRAME_STAND4 (FRAME_STAND + 5)

#define FRAME_WALK3 (FRAME_STAND + 6)
#define FRAME_WALK4 (FRAME_STAND + 7)

#define FRAME_BATTLE_START (96)

#define NEW_GAME_POS (MTH_FIXED(88))
#define CONTINUE_POS (MTH_FIXED(128))

#define NAME_LETTER_MIN_X (MTH_FIXED(64))
#define NAME_LETTER_MIN_Y (MTH_FIXED(80))

#define NAME_LETTER_MAX_X (MTH_FIXED(208))
#define NAME_LETTER_MAX_Y (MTH_FIXED(176))

#define BAT_MENU_MIN_X (MTH_FIXED(120))
#define BAT_MENU_MAX_X (MTH_FIXED(168))

#define BAT_MENU_MIN_Y (MTH_FIXED(158))
#define BAT_MENU_MAX_Y (MTH_FIXED(206))

const Uint16 START_POS_X = 153;
const Uint16 START_POS_Y = 165;

Uint8 current_x_pos;
Uint8 current_y_pos;

Uint8 prev_current_x_pos;
Uint8 prev_current_y_pos;

int prev_bat_menu_y_pos = BAT_MENU_MIN_Y;
int prev_bat_menu_x_pos = BAT_MENU_MIN_X;

int vehicle = FOOT;

Uint8 currentTurn = 0;
Uint8 currentTurnBufferIndex = 0;
Uint8 isValidTurn = FALSE;
Uint8 targetIndex = 0;
Uint8 ineffective = FALSE;

Uint8 explosionCounter = 0;

SPRITE_INFO player;
SPRITE_INFO playerb;
SPRITE_INFO cursor;


SPRITE_INFO playerBat1;
SPRITE_INFO playerBat2;
SPRITE_INFO playerBat3;
SPRITE_INFO playerBat4;
SPRITE_INFO weaponBat;
SPRITE_INFO explosionBat;

Uint8 isMoving = FALSE;
Uint8 isStanding = TRUE;
Uint8 totalMovement = 0;
Uint8 changeToBat = 0;
Uint8 responseRate = 1;
Uint8 runFailed = FALSE;

int direction = DOWN;
int currentChar = WARRIOR_OFFSET;
int currentBattleState = 0;

Uint16 MIRROR_ANIM_1 = MIRROR_HORIZ;
Uint16 MIRROR_ANIM_2 = 0;

char selectName1[7] = "FIGHTER ";
char selectName2[7] = "THIEF   ";
char selectName3[7] = "Bl.BELT ";
char selectName4[7] = "RedMAGE ";
char selectName5[7] = "Wh.MAGE ";
char selectName6[7] = "Bl.MAGE ";

char *classNames[6] = {&selectName1, &selectName2, &selectName3, &selectName4, &selectName5, &selectName6};

Uint8 cur_char_create = 0;
Uint8 cur_letter = 0;
Uint8 cur_letter_index = 0;
Uint8 nameBuffer[7] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};
Uint8 selected_class = 0;
Uint8 cur_char_battle_menu = 0;
Uint8 cur_battle_menu_selection = FIGHT;
Uint8 prev_battle_menu_selection = FIGHT;
Uint8 cur_battle_enemy_target = 0;
Uint8 cur_battle_party_target = 0;
Uint8 weapon_char = 0;

Sint8 currentSlot = 0;

Uint8 ignoreA = FALSE;
Uint8 ignoreB = FALSE;
Uint8 ignoreC = FALSE;
Uint8 ignoreX = FALSE;
Uint8 ignoreY = FALSE;
Uint8 ignoreZ = FALSE;
Uint8 ignoreS = FALSE;

Uint8 *response_rate_lut = 0x0022DA32;

Uint8 drawBattleCursor = FALSE;
Uint8 drawTargetNames = FALSE;

Uint8 rotated = FALSE;

Uint8 char_name_screen_lookup_table[70] =  { 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 0x92, 0x93,
                                            0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D,
                                            0x9E, 0x9F, 0xA0, 0xA1, 0xA2, 0xA3, 0xBE, 0xBF, 0xC0, 0xFF,
                                            0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
                                            0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD,
                                            0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
                                            0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xC2, 0xC3, 0xC4, 0xC5};

Uint32 pal_3[] = {
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
};
Uint32 pal_4[] = {
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
};
Uint32 pal_5[] = {
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
};
Uint32 pal_6[] = {
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
};

Uint8 getResponseRate() {
    return responseRate;
}

void player_cursor_init(Fixed32 startX, Fixed32 startY) {

    if(state == TO_CHAR_SELECT_STATE) {

        if(cur_char_create > 3) {
            startX = MTH_FIXED(360);
            startY = MTH_FIXED(256);
        } else {
            startX = MTH_FIXED(getPartyCreateCursorPosX(cur_char_create));
            startY = MTH_FIXED(getPartyCreateCursorPosY(cur_char_create));
        }
    }

    sprite_make(cursorSpriteIndex,  startX,  startY, (Sint32) CURSOR_PRIORITY, &cursor);
    cursor.xSize = MTH_FIXED(16);
    cursor.ySize = MTH_FIXED(16);
    
}

void player_cursor_draw() {
    SPRITE_INFO temp;
    memcpy(&temp, &cursor, sizeof(temp));;

    sprite_draw(&temp);
}

void draw_response_rate() {
    Uint8 responseBuff[2] = {responseRate + 0x80, 0x00};
    drawTextLine(22, 22, responseBuff, 0, 1);
}

void draw_char_name() {
     drawTextLine(13, 4, nameBuffer, 1, 1);
}

void update_player_sprites() {

    Uint16 owsprite_size = 64;
    Uint16 owsprite_width = 16;
    Uint16 owsprite_height = 8;

    Uint8 curr_class = determine_curr_char_class();

    Uint8 *palPtr_a = (Uint8*) MAPMAN_SPRITE_PALETTE_OFFSET;
    Uint8 *palPtr_b = (Uint8*) MAPMAN_SPRITE_PALETTE_OFFSET_2 + (2 * curr_class);


    initMapManPalette(&pal_3, &pal_4, palPtr_a, palPtr_b);

    SCL_SetColRam(SCL_SPR, mapSpriteCramStart, 16, &pal_3);
    SCL_SetColRam(SCL_SPR, mapSpriteCramStart + 16, 16, &pal_4);

    for (int i = 0; i < 8; i++) {
        SPR_2SetChar(i + FRAME_STAND + currentChar, COLOR_0, mapSpriteCramStart, owsprite_width, owsprite_height, (Uint8 *)(LWRAM_MAP_SPRITES) + ((i * owsprite_size) + (currentChar * owsprite_size)));
        i++;
        SPR_2SetChar(i + FRAME_STAND + currentChar, COLOR_0, mapSpriteCramStart + 16, owsprite_width, owsprite_height, (Uint8 *)(LWRAM_MAP_SPRITES) + ((i * owsprite_size) + (currentChar * owsprite_size)));
    }
    player.char_num = FRAME_STAND + currentChar;
    playerb.char_num = FRAME_WALK1 + currentChar;
}

void player_ow_init(Fixed32 startX, Fixed32 startY) {

    sprite_make(FRAME_STAND + currentChar,  startX,  startY, (Sint32) CHARACTER_PRIORITY, &player);
    sprite_make(FRAME_WALK1 + currentChar,  startX,  startY + 8, (Sint32) CHARACTER_PRIORITY, &playerb);
    player.xSize = MTH_FIXED(16);
    player.ySize = MTH_FIXED(8);
    playerb.xSize = MTH_FIXED(16);
    playerb.ySize = MTH_FIXED(8);
    MIRROR_ANIM_2 &= ~MIRROR_HORIZ;

    current_x_pos = START_POS_X;
    current_y_pos = START_POS_Y;
    
    prev_current_x_pos = current_x_pos;
    prev_current_y_pos = current_y_pos;

}

void player_bat_init() {
    
    isPreemptive = FALSE;
    isSurprised = FALSE;
    currentTurnBufferIndex = 0;

    Uint8 *startXPos = BATTLE_CHAR_DEFAULT_XPOS;
    Uint8 *startYPos = BATTLE_CHAR_DEFAULT_YPOS_1;

    isStanding = TRUE;
    sprite_make(FRAME_BATTLE_START + (party[0].char_class * 6), MTH_FIXED(startXPos[0] + 24), MTH_FIXED(startYPos[0]), (Sint32) CHARACTER_PRIORITY, &playerBat1);
    sprite_make(FRAME_BATTLE_START + (party[1].char_class * 6), MTH_FIXED(startXPos[0] + 24), MTH_FIXED(startYPos[5]), (Sint32) CHARACTER_PRIORITY, &playerBat2);
    sprite_make(FRAME_BATTLE_START + (party[2].char_class * 6), MTH_FIXED(startXPos[0] + 24), MTH_FIXED(startYPos[10]), (Sint32) CHARACTER_PRIORITY, &playerBat3);
    sprite_make(FRAME_BATTLE_START + (party[3].char_class * 6), MTH_FIXED(startXPos[0] + 24), MTH_FIXED(startYPos[15]), (Sint32) CHARACTER_PRIORITY, &playerBat4);

    playerBat1.xSize = MTH_FIXED(16);
    playerBat1.ySize = MTH_FIXED(24);
    playerBat2.xSize = MTH_FIXED(16);
    playerBat2.ySize = MTH_FIXED(24);
    playerBat3.xSize = MTH_FIXED(16);
    playerBat3.ySize = MTH_FIXED(24);
    playerBat4.xSize = MTH_FIXED(16);
    playerBat4.ySize = MTH_FIXED(24);

    
    sprite_make(weaponSpriteIndex, MTH_FIXED(startXPos[0] + 16), MTH_FIXED(startYPos[0]) - 8, (Sint32) WEAPON_PRIORITY, &weaponBat);
    
    sprite_make(explosionSpriteIndex, MTH_FIXED(startXPos[0] + 16), MTH_FIXED(startYPos[0]) - 8, (Sint32) WEAPON_PRIORITY, &explosionBat);
}

void player_bat_stand() {

    for(Uint8 i = 0; i < 4; i++) {
        if(battle_char_stats[i].hp > 0) {
            player_stand(i);
        }
    }
}

void party_flip() {

    for(Uint8 i = 0; i < 4; i++) {
        if(battle_char_stats[i].hp > 0) {
            player_flip(i);
        }
    }
}

void char_init(int classValue, int partyNum, char newName[7]) {

    int startingStatOffset;
    PLAYER_CHARACTER character;
    character.char_class = classValue;

    switch(classValue) {
        case WARRIOR_VALUE:
            startingStatOffset = WARRIOR_START_STAT_OFFSET;
            character.weapons[0] = 3 + 0x80;
            character.armors[0] = 3 + 0x80;
            break;
        case THIEF_VALUE:
            startingStatOffset = THIEF_START_STAT_OFFSET;
            character.weapons[0] = 1 + 0x80;
            character.armors[0] = 2 + 0x80;
            break;
        case MONK_VALUE:
            startingStatOffset = MONK_START_STAT_OFFSET;
            character.weapons[0] = 0 + 0x80;
            character.armors[0] = 2 + 0x80;
            break;
        case REDMAGE_VALUE:
            startingStatOffset = REDMAGE_START_STAT_OFFSET;
            character.weapons[0] = 3 + 0x80;
            character.armors[0] = 3 + 0x80;
            break;
        case WMAGE_VALUE:
            startingStatOffset = WMAGE_START_STAT_OFFSET;
            character.weapons[0] = 4 + 0x80;
            character.armors[0] = 1 + 0x80;
            break;
        case BMAGE_VALUE:
            startingStatOffset = BMAGE_START_STAT_OFFSET;
            character.weapons[0] = 2 + 0x80;
            character.armors[0] = 1 + 0x80;
            break;
        default:
            startingStatOffset = WARRIOR_START_STAT_OFFSET;
            character.weapons[0] = 0;
            character.armors[0] = 0;
            break;
    }

    Uint8 *statPtr = (Uint8*) startingStatOffset;
    Uint8 startHP = statPtr[1];
    Uint8 startStr = statPtr[2];
    Uint8 startAgi = statPtr[3];
    Uint8 startInt = statPtr[4];
    Uint8 startVit = statPtr[5];
    Uint8 startLck = statPtr[6];
    Uint8 startDmg = statPtr[7];
    Uint8 startHit = statPtr[8];
    Uint8 startEva = statPtr[9];
    Uint8 startMDef = statPtr[10];

    character.current_hp = startHP;
    character.max_hp = startHP;
    character.str = startStr;
    character.agi = startAgi;
    character.intel = startInt;
    character.vit = startVit;
    character.luck = startLck;
    character.damage = startDmg;
    character.hit_percent = startHit;
    character.evade_percent = startEva;
    character.ailment = 0;
    character.absorb = 0;
    character.mag_def = startMDef;
    character.name[0] = newName[0];
    character.name[1] = newName[1];
    character.name[2] = newName[2];
    character.name[3] = newName[3];
    character.name[4] = newName[4];
    character.name[5] = newName[5];
    character.name[6] = newName[6];
    character.level = 1;
    character.exp = 0;
    character.exp_to_next_lvl = 40;

    for(int mp_iter = 0; mp_iter < 8; mp_iter++) {
        character.current_mp[mp_iter] = 0;
        character.max_mp[mp_iter] = 0;
    }

    if(classValue != WARRIOR_VALUE && classValue != THIEF_VALUE && classValue != MONK_VALUE){
        character.current_mp[0] = 2;
        character.max_mp[0] = 2;
    }

    for(int i = 1; i < 4; i++) {
        character.weapons[i] = 0;
        character.armors[i] = 0;
    }

    for(int maglevel = 0; maglevel < 8; maglevel++) {
        for(int magslot = 0; magslot < 3; magslot++) {
            character.magic[maglevel][magslot] = 0;
        }
    }

    party[partyNum] = character;
    applyEquippedWeapon(partyNum);
    applyEquippedArmor(partyNum);
}

void player_input() {

    PadData1E = 0;
    //soft reset
    if (PadData1 == (PAD_A | PAD_B | PAD_C | PAD_S)) {
        //go back to the cd player screen if a+b+c+start is pressed
        SYS_Exit(0);
    }

    if(!(PadData1 & PAD_A)) {
        ignoreA = 0;
    }
    if(!(PadData1 & PAD_B)) {
        ignoreB = 0;
    }
    if(!(PadData1 & PAD_C)) {
        ignoreC = 0;
    }
    
    if(!(PadData1 & PAD_X)) {
        ignoreX = 0;
    }

    if(!(PadData1 & PAD_S)) {
        ignoreS = 0;
    }

    if(PadData1 & PAD_S && !ignoreS) {
        if(state == PRELUDE) {
            state = TO_TITLE_STATE;
        }
        ignoreS = 1;
    }

    if (PadData1 & PAD_A && !ignoreA) {    

        performActionButtonInput();
        ignoreA = 1;
    }
    if (PadData1 & PAD_B && !ignoreB) {    
        performCancelButtonInput();
        ignoreB = 1;
    }
    if (PadData1 & PAD_C && !ignoreC) {    
        performActionButtonInput();
        ignoreC = 1;
    }

    if (PadData1 & PAD_X && !ignoreX) {    
        if(state == OW_STATE) {
            if(rotated) {
                scroll_rotate_x(0);
                rotated = FALSE;
            }else {
                scroll_rotate_x(-45);
                rotated = TRUE;
            }
        }

        ignoreX = 1;
    }

    if (PadData1 & PAD_L) {    
    
        if(state == OW_STATE) {
            if(!isMoving && canMove(LEFT)){
                if(player.char_num == FRAME_STAND + currentChar || player.char_num == FRAME_STAND2 + currentChar){    
                    player.char_num = FRAME_STAND3 + currentChar;
                    playerb.char_num = FRAME_STAND4 + currentChar;
                }
                direction = LEFT;
                current_x_pos--;
                isMoving = TRUE;
                player.mirror &= ~MIRROR_HORIZ;
                playerb.mirror &= ~MIRROR_HORIZ;
            }
        } else if(state == TITLE_STATE) {
            if(!canUpdateValue){
                if(responseRate > 1) {
                frame_count_a = 0;
                    responseRate--;
                    canUpdateValue = 1;
                }
            }
        } else if(state == CHAR_SELECT_STATE) {
            if(!canUpdateValue){
                decrementPartyInitBuff(cur_char_create);
                if(party[cur_char_create].char_class == 0) {
                    party[cur_char_create].char_class = 5;
                    selected_class = 5;
                } else {
                    party[cur_char_create].char_class--;
                    selected_class--;
                }
                
                frame_count_a = 0;
                canUpdateValue = 1;
                
            }
        } else if(state == CHAR_NAME_STATE) {
            if(!canMoveCursor){
                if(cursor.xPos > NAME_LETTER_MIN_X) {
                    cur_letter--;
                    cursor.xPos -= MTH_FIXED(16);
                }
                frame_count_b = 0;
                canMoveCursor = 1;
            }
        } else if (state == BAT_MENU_STATE && bat_menu_state == SELECT) {
            if(!canMoveCursor) {
                if(cursor.xPos == BAT_MENU_MIN_X) {
                    //Do Nothing, can't move.
                } else {
                    cursor.xPos = BAT_MENU_MIN_X;
                    cursor.yPos = prev_bat_menu_y_pos;
                    cur_battle_menu_selection = prev_battle_menu_selection;
                }
                frame_count_b = 0;
                canMoveCursor = 1;
            }
        }

    }
    else if (PadData1 & PAD_R) {

        if(state == OW_STATE) {
            if(!isMoving && canMove(RIGHT)){
                if(player.char_num == FRAME_STAND  + currentChar|| player.char_num == FRAME_STAND2 + currentChar){    
                    player.char_num = FRAME_STAND3 + currentChar;
                    playerb.char_num = FRAME_STAND4 + currentChar;
                }
                direction = RIGHT;
                current_x_pos++;
                
                isMoving = TRUE;
                player.mirror = MIRROR_HORIZ;
                playerb.mirror = MIRROR_HORIZ;
                
            }
        } else if(state == TITLE_STATE) {
            if(!canUpdateValue){
                if(responseRate < 8) {
                    frame_count_a = 0;
                    responseRate++;
                    canUpdateValue = 1;
                }
            }
        } else if(state == CHAR_SELECT_STATE) {
            if(!canUpdateValue){
                incrementPartyInitBuff(cur_char_create);
                if(party[cur_char_create].char_class == 5) {
                    party[cur_char_create].char_class = 0;
                    selected_class = 0;
                } else {
                    party[cur_char_create].char_class++;
                    selected_class++;
                }
                frame_count_a = 0;
                canUpdateValue = 1;
                
            }
        } else if(state == CHAR_NAME_STATE) {
            if(!canMoveCursor){
                if(cursor.xPos < NAME_LETTER_MAX_X) {
                    cur_letter++;
                    cursor.xPos += MTH_FIXED(16);
                }
                frame_count_b = 0;
                canMoveCursor = 1;
            }
        } else if (state == BAT_MENU_STATE && bat_menu_state == SELECT) {
            if(!canMoveCursor) {
                if(cursor.xPos == BAT_MENU_MIN_X) {
                   prev_battle_menu_selection = cur_battle_menu_selection;  
                   cur_battle_menu_selection = RUN;
                   prev_bat_menu_y_pos = cursor.yPos;
                   cursor.xPos = BAT_MENU_MAX_X;
                   cursor.yPos = BAT_MENU_MIN_Y;
                } else {
                   //Do nothing, can't move. 
                }
                frame_count_b = 0;
                canMoveCursor = 1;
            }
        }

    }
    else if (PadData1 & PAD_D) {
    
        if(state == OW_STATE) {
            if(!isMoving && canMove(DOWN)){
                player.char_num = FRAME_STAND + currentChar;
                playerb.char_num = FRAME_WALK1 + currentChar;
                current_y_pos++;
                
                direction = DOWN;
                isMoving = TRUE;            
                player.mirror &= ~MIRROR_HORIZ;
                playerb.mirror &= ~MIRROR_HORIZ;
            }
        } else if(state == TITLE_STATE) {
            if(!canMoveCursor) {
                frame_count_b = 0;
                cursor.yPos = CONTINUE_POS;
                canMoveCursor = 1;
            }
        } else if(state == CHAR_NAME_STATE) {
            if(!canMoveCursor){
                if(cursor.yPos < NAME_LETTER_MAX_Y) {
                    cur_letter+= 10;
                    cursor.yPos += MTH_FIXED(16);
                }
                frame_count_b = 0;
                canMoveCursor = 1;
            }
        } else if (state == BAT_MENU_STATE && bat_menu_state == SELECT) {
            if(!canMoveCursor) {
                if(cursor.yPos >= BAT_MENU_MAX_Y || cursor.xPos == BAT_MENU_MAX_X) {
                    //Do Nothing, can't move.
                } else {
                    cursor.yPos += MTH_FIXED(16);
                    cur_battle_menu_selection++;
                }
                frame_count_b = 0;
                canMoveCursor = 1;
            }
        } else if (state == BAT_MENU_STATE && bat_menu_state == SELECT_ENEMY_TARGET) {
            if(!canMoveCursor) {
                currentSlot++;
                currentSlot = getNextOccupiedSlot(currentSlot);
                cursor.xPos = MTH_FIXED(getXSlotCoord(currentSlot));
                cursor.yPos = MTH_FIXED(getYSlotCoord(currentSlot));
                frame_count_b = 0;
                canMoveCursor = 1;
            }
        }
        
    }
    else if (PadData1 & PAD_U) {
        
        if(state == OW_STATE) {
            if(!isMoving && canMove(UP)){
                player.char_num = FRAME_STAND2 + currentChar;
                playerb.char_num = FRAME_WALK2 + currentChar;
                current_y_pos--;
                
                direction = UP;
                isMoving = TRUE;    
                player.mirror &= ~MIRROR_HORIZ;
                playerb.mirror &= ~MIRROR_HORIZ;
                
            }
        } else if(state == TITLE_STATE) {
            if(!canMoveCursor) {
                frame_count_b = 0;
                canMoveCursor = 1; 
                cursor.yPos = NEW_GAME_POS;
            }
        } else if(state == CHAR_NAME_STATE) {
            if(!canMoveCursor){
                if(cursor.yPos > NAME_LETTER_MIN_Y) {
                    cur_letter -= 10;
                    cursor.yPos -= MTH_FIXED(16);
                }
                    frame_count_b = 0;
                    canMoveCursor = 1;
            }
        }  else if (state == BAT_MENU_STATE && bat_menu_state == SELECT) {
            if(!canMoveCursor) {
                if(cursor.yPos - MTH_FIXED(16) < BAT_MENU_MIN_Y) {
                    //Do Nothing, can't move.
                } else {
                    cursor.yPos -=  MTH_FIXED(16);
                    cur_battle_menu_selection--;
                }
                frame_count_b = 0;
                canMoveCursor = 1;
            }
        } else if (state == BAT_MENU_STATE && bat_menu_state == SELECT_ENEMY_TARGET) {
            if(!canMoveCursor) {
                currentSlot--;
                currentSlot = getPrevOccupiedSlot(currentSlot);
                cursor.xPos = MTH_FIXED(getXSlotCoord(currentSlot));
                cursor.yPos = MTH_FIXED(getYSlotCoord(currentSlot));
                frame_count_b = 0;
                canMoveCursor = 1;
            }
        }
    }

    if(state == OW_STATE) {
        if(isMoving) {
            switch(direction) {
                case LEFT:
                    player.dy =  MTH_FIXED(0);
                    playerb.dy =  MTH_FIXED(0);
                    movePlayerLeft();
                    break;
                case RIGHT:
                    player.dy =  MTH_FIXED(0);
                    playerb.dy =  MTH_FIXED(0);
                    movePlayerRight();
                    break;
                case UP:
                    movePlayerUp();
                    player.dx =  MTH_FIXED(0);
                    playerb.dx =  MTH_FIXED(0);
                    break;
                case DOWN:
                    movePlayerDown();
                    player.dx =  MTH_FIXED(0);
                    playerb.dx =  MTH_FIXED(0);
                    break;
                default:
                    break;
            }            
        } else {
            player.dy =  MTH_FIXED(0);
            playerb.dy =  MTH_FIXED(0);
            player.dx =  MTH_FIXED(0);
            playerb.dx =  MTH_FIXED(0);
        }

        player.xPos += player.dx;
        playerb.xPos += playerb.dx;
        player.yPos += player.dy;
        playerb.yPos += playerb.dy;
    }
}

void player_animate() {

    if (isMoving && (direction == UP || direction == DOWN)) {
        if(playerb.mirror == MIRROR_ANIM_1) {
            playerb.mirror = MIRROR_ANIM_2;
        } else {
            playerb.mirror = MIRROR_ANIM_1;
        }
    }
    if (isMoving && (direction == LEFT || direction == RIGHT)) {
            
        if(player.char_num == FRAME_STAND3 + currentChar) {
            player.char_num = FRAME_WALK3 + currentChar;
            playerb.char_num = FRAME_WALK4 + currentChar;
        } else {
            player.char_num = FRAME_STAND3 + currentChar;
            playerb.char_num = FRAME_STAND4 + currentChar;
        }
    }

}

void player_ow_draw() {
    SPRITE_INFO temp;
    SPRITE_INFO tempb;
    memcpy(&temp, &player, sizeof(temp));
    memcpy(&tempb, &playerb, sizeof(tempb));

    if (temp.xPos > PLAYER_MAXXPOS) {
        Fixed32 newXPos = temp.xPos;
        Fixed32 newYPos = temp.yPos;
        scroll_set(SCROLL_BACKGROUND1, newXPos, newYPos);
        temp.xPos = PLAYER_MAXXPOS;
        tempb.xPos = PLAYER_MAXXPOS;
        temp.yPos = PLAYER_MAXYPOS;
        tempb.yPos = PLAYER_MAXYPOSB;
    } else if (temp.xPos < PLAYER_MAXXPOS) {
        Fixed32 newXPos = temp.xPos;
        Fixed32 newYPos = temp.yPos;
        scroll_set(SCROLL_BACKGROUND1, newXPos, newYPos);
        temp.xPos = PLAYER_MAXXPOS;
        tempb.xPos = PLAYER_MAXXPOS;
        temp.yPos = PLAYER_MAXYPOS;
        tempb.yPos = PLAYER_MAXYPOSB;
    } 
    
    sprite_draw(&temp);
    if(!inForest()){
        sprite_draw(&tempb);
    }
}

void player_bat_draw() {
    sprite_draw(&playerBat1);
    sprite_draw(&playerBat2);
    sprite_draw(&playerBat3);
    sprite_draw(&playerBat4);

    if(turn_exec_state == ANIMATE2 && currentTurn > 0x7F){
        sprite_draw(&weaponBat);
    }
    if(turn_exec_state == EXPLOSION) {
        sprite_draw(&explosionBat);
    }
}

void movePlayerLeft(){
    
    player.dx = MTH_FIXED(-1);
    playerb.dx = MTH_FIXED(-1);

    if(totalMovement == 8){
        player_animate();
    }

    totalMovement += 1;

    if(totalMovement == 17){
        
        player.dx = MTH_FIXED(0);
        playerb.dx = MTH_FIXED(0);
        player_animate();
        isMoving = FALSE;
        totalMovement = 0;


        currentBattleState = doBattle();

        switch(currentBattleState){
            case NORMAL_BATTLE:
                state = TO_BAT_STATE;
                break;
            case RIVER_BATTLE_1:
                state = TO_BAT_STATE;
                break;
            case RIVER_BATTLE_2:
                state = TO_BAT_STATE;
                break;
            case OCEAN_BATTLE:
                state = TO_BAT_STATE;
                break;
            default:
            break;
        }
    }
}

void movePlayerRight(){
    player.dx = MTH_FIXED(1);
    playerb.dx = MTH_FIXED(1);

    if(totalMovement == 8){
        player_animate();
    }

    totalMovement += 1;

    if(totalMovement == 17){
        
        player.dx = MTH_FIXED(0);
        playerb.dx = MTH_FIXED(0);
        player_animate();
        isMoving = FALSE;
        

        totalMovement = 0;
        currentBattleState = doBattle();
        switch(currentBattleState){
            case NORMAL_BATTLE:
                state = TO_BAT_STATE;
                break;
            case RIVER_BATTLE_1:
                state = TO_BAT_STATE;
                break;
            case RIVER_BATTLE_2:
                state = TO_BAT_STATE;
                break;
            case OCEAN_BATTLE:
                state = TO_BAT_STATE;
                break;
            default:
            break;
        }
    }
    
}

void movePlayerUp(){
    player.dy =  MTH_FIXED(-1);
    playerb.dy =  MTH_FIXED(-1);

    if(totalMovement == 8){
        player_animate();
    }

    totalMovement += 1;

    if(totalMovement == 17){
        player_animate();
        isMoving = FALSE;
        
        player.dy = MTH_FIXED(0);
        playerb.dy = MTH_FIXED(0);
        totalMovement = 0;
        currentBattleState = doBattle();
        switch(currentBattleState){
            case NORMAL_BATTLE:
                state = TO_BAT_STATE;
                break;
            case RIVER_BATTLE_1:
                state = TO_BAT_STATE;
                break;
            case RIVER_BATTLE_2:
                state = TO_BAT_STATE;
                break;
            case OCEAN_BATTLE:
                state = TO_BAT_STATE;
                break;
            default:
            break;
        }
    }
}

void movePlayerDown(){
    player.dy =  MTH_FIXED(1);
    playerb.dy =  MTH_FIXED(1);
    if(totalMovement == 8){
        player_animate();
    }

    totalMovement += 1;

    if(totalMovement == 17){
        
        player.dy = MTH_FIXED(0);
        playerb.dy = MTH_FIXED(0);
        player_animate();
        isMoving = FALSE;
        
        totalMovement = 0;
        currentBattleState = doBattle();
        switch(currentBattleState){
            case NORMAL_BATTLE:
                state = TO_BAT_STATE;
                break;
            case RIVER_BATTLE_1:
                state = TO_BAT_STATE;
                break;
            case RIVER_BATTLE_2:
                state = TO_BAT_STATE;
                break;
            case OCEAN_BATTLE:
                state = TO_BAT_STATE;
                break;
            default:
            break;
        }
    }
}

void drawSmallFormationExplosion(Uint8 slotNum) {
    Uint8 *lut_ptr = (Uint8 *) LUT_SMALL_FORMATION_EXPLOSION_LUT;
    Uint8 xIter = slotNum * 2;
    Uint8 minX = lut_ptr[xIter];
    Uint8 minY = lut_ptr[xIter + 1];
    Uint8 maxX = minX + 0x18;
    Uint8 maxY = minY + 0x18;

    Uint8 numExplosions = 0x06;

    drawExplosions(minX, maxX, minY, maxY);

}

void drawSmallMixFormationExplosion(Uint8 slotNum) {
    Uint8 *lut_ptr = (Uint8 *) LUT_MIX_SMALL_FORMATION_EXPLOSION_LUT;
    Uint8 xIter = slotNum * 2;
    Uint8 minX = lut_ptr[xIter];
    Uint8 minY = lut_ptr[xIter + 1];
    Uint8 maxX = minX + 0x18;
    Uint8 maxY = minY + 0x18;

    
    Uint8 numExplosions = 0x0F; //Odd but original game did this.

    drawExplosions(minX, maxX, minY, maxY);

}

void drawLargeFormationExplosion(Uint8 slotNum) {
    Uint8 *lut_ptr = (Uint8 *) LUT_LARGE_FORMATION_EXPLOSION_LUT;
    Uint8 xIter = slotNum * 2;
    Uint8 minX = lut_ptr[xIter];
    Uint8 minY = lut_ptr[xIter + 1];
    Uint8 maxX = minX + 0x28;
    Uint8 maxY = minY + 0x28;

    Uint8 numExplosions = 0x08; 
    
    drawExplosions(minX, maxX, minY, maxY);

}

void drawExplosions(Uint8 minX, Uint8 maxX, Uint8 minY, Uint8 maxY) {

    Uint8 xPos = rand_min_max(minX, maxX);
    Uint8 yPos = rand_min_max(minY, maxY);
    Uint8 frame = rand_min_max(0, 2);
    explosionBat.xPos = MTH_FIXED(xPos + 24);
    explosionBat.yPos = MTH_FIXED(yPos);
    explosionBat.char_num = frame + explosionSpriteIndex;
    turn_exec_state = EXPLOSION;
    frameCount++;

}


void swingWeapon(int partyNum) {
    if(frameCount % 2 == 0) {

        switch(partyNum) {
                case 0:
                    if(isStanding == TRUE) {
                        playerBat1.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6) + 2;
                        weaponBat.xPos = playerBat1.xPos - MTH_FIXED(16);
                        weaponBat.yPos = playerBat1.yPos;
                        weaponBat.mirror = 0;
                        isStanding = FALSE;
                    } else {
                        playerBat1.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6) + 1;
                        weaponBat.xPos =  playerBat1.xPos + MTH_FIXED(8);
                        weaponBat.yPos = playerBat1.yPos - MTH_FIXED(8);
                        weaponBat.mirror = MIRROR_HORIZ;
                        isStanding = TRUE;
                    }
                    break;
                case 1:
                    if(isStanding == TRUE) {
                        playerBat2.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6) + 2;
                        weaponBat.xPos = playerBat2.xPos - MTH_FIXED(16);
                        weaponBat.yPos = playerBat2.yPos;
                        weaponBat.mirror = 0;
                        isStanding = FALSE;
                    } else {
                        playerBat2.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6) + 1;
                        weaponBat.xPos = playerBat2.xPos + MTH_FIXED(8);
                        weaponBat.yPos = playerBat2.yPos - MTH_FIXED(8);
                        weaponBat.mirror = MIRROR_HORIZ;
                        isStanding = TRUE;
                    }
                    break;
                case 2:
                    if(isStanding == TRUE) {
                        playerBat3.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6) + 2;
                        weaponBat.xPos = playerBat3.xPos - MTH_FIXED(16);
                        weaponBat.yPos = playerBat3.yPos;
                        weaponBat.mirror = 0;
                        isStanding = FALSE;
                    } else {
                        playerBat3.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6) + 1;
                        weaponBat.xPos = playerBat3.xPos + MTH_FIXED(8);
                        weaponBat.yPos = playerBat3.yPos - MTH_FIXED(8);
                        weaponBat.mirror = MIRROR_HORIZ;
                        isStanding = TRUE;
                    }
                    break;
                case 3:
                    if(isStanding == TRUE) {
                        playerBat4.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6) + 2;
                        weaponBat.xPos = playerBat4.xPos - MTH_FIXED(16);
                        weaponBat.yPos = playerBat4.yPos;
                        weaponBat.mirror = 0;
                        isStanding = FALSE;
                    } else {
                        playerBat4.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6) + 1;
                        weaponBat.xPos = playerBat4.xPos + MTH_FIXED(8);
                        weaponBat.yPos = playerBat4.yPos - MTH_FIXED(8);
                        weaponBat.mirror = MIRROR_HORIZ;
                        isStanding = TRUE;
                    }
                    break;
                default:
                    break;
            }
    }

    if(frameCount == 8) {
        player_stand(partyNum);
        turn_exec_state = PREP_EXPLOSION;  
        sprite_delete(&weaponBat);
        explosionCounter = 0; 
        frameCount = 0;
    }  

}

void moveBattlePlayerLeft(int partyNum) {

    if(frameCount % 2== 0) {
        switch(partyNum) {
            case 0:
                playerBat1.xPos += MTH_FIXED(-2);
                break;
            case 1:
                playerBat2.xPos += MTH_FIXED(-2);
                break;
            case 2:
                playerBat3.xPos += MTH_FIXED(-2);
                break;
            case 3:
                playerBat4.xPos += MTH_FIXED(-2);
                break;
            default:
                break;
        }
    }
        
        totalMovement++;
        

        if(frameCount % 4 == 0){
            player_animate_battle_walk(partyNum);
        }


        if(totalMovement == 16){
            isMoving = FALSE;
            player_stand(partyNum);
            totalMovement = 0;
            if(state == BAT_MENU_STATE) {
                bat_menu_state = SELECT;
            } else if (state == BAT_EXECUTE_STATE && turn_exec_state == ANIMATE1) {
                turn_exec_state = CALCULATE;
            }
        }
    

}
void moveBattlePlayerRight(int partyNum) {
    
    if(frameCount % 2 == 0) {
        switch(partyNum) {
            case 0:
                playerBat1.xPos += MTH_FIXED(2);
                break;
            case 1:
                playerBat2.xPos += MTH_FIXED(2);
                break;
            case 2:
                playerBat3.xPos += MTH_FIXED(2);
                break;
            case 3:
                playerBat4.xPos += MTH_FIXED(2);
                break;
            default:
                break;
        }
    }
        
        totalMovement++;

        if(frameCount % 4 == 0){
            player_animate_battle_walk(partyNum);
        }


        if(totalMovement == 16){
            isMoving = FALSE;
            isStanding = TRUE;
            totalMovement = 0;
            player_stand(partyNum);
            Uint8 *startXPos = BATTLE_CHAR_DEFAULT_XPOS;

            switch(partyNum) {
                case 0:
                    playerBat1.xPos  = MTH_FIXED(startXPos[0] + 24);
                    break;
                case 1:
                    playerBat2.xPos = MTH_FIXED(startXPos[0] + 24);
                    break;
                case 2:
                    playerBat3.xPos = MTH_FIXED(startXPos[0] + 24);
                    break;
                case 3:
                    playerBat4.xPos = MTH_FIXED(startXPos[0] + 24);
                    break;
                default:
                    break;
            }

            if(isPlayerLowHealth(partyNum, party[partyNum].max_hp)){
                player_crouch(partyNum);
            }

            if(state == BAT_MENU_STATE) {
                if(bat_menu_state == SELECT_CANCEL) {
                    if(cur_char_battle_menu > 0) {
                        cur_char_battle_menu -= 1;
                    }
                } else {
                    cur_char_battle_menu += 1;
                    while(isPlayerDead(cur_char_battle_menu) && cur_char_battle_menu < 4) {
                        cur_char_battle_menu++;
                    }
                }

                if(cur_char_battle_menu == 4) {
                    cur_char_battle_menu = 0;
                    bat_menu_state = END;
                } else {
                    state = TO_BAT_MENU_STATE;
                }

            } else if (state == BAT_EXECUTE_STATE && turn_exec_state == FINISH_TURN) {
                currentTurnBufferIndex++;
                turn_exec_state = BETWEEN_TURN;
            }
        }
    
}

void player_animate_battle_walk(int partyNum) {
    
    if (isMoving) {

        switch(partyNum) {
            case 0:
                if(isStanding == TRUE) {
                    playerBat1.char_num++;
                    isStanding = FALSE;
                } else {
                    playerBat1.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6);
                    isStanding = TRUE;
                }
                break;
            case 1:
                if(isStanding == TRUE) {
                    playerBat2.char_num++;
                    isStanding = FALSE;
                } else {
                    playerBat2.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6);
                    isStanding = TRUE;
                }
                break;
            case 2:
                if(isStanding == TRUE) {
                    playerBat3.char_num++;
                    isStanding = FALSE;
                } else {
                    playerBat3.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6);
                    isStanding = TRUE;
                }
                break;
            case 3:
                if(isStanding == TRUE) {
                    playerBat4.char_num++;
                    isStanding = FALSE;
                } else {
                    playerBat4.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6);
                    isStanding = TRUE;
                }
                break;
            default:
                break;
        }
      
    }
}

void player_crouch(int partyNum) {
    
    switch(partyNum) {
        case 0:
            playerBat1.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6) + 4;
            playerBat1.angle = 0;
            playerBat1.xSize = MTH_FIXED(16);
            playerBat1.ySize = MTH_FIXED(24);
            break;
        case 1:
            playerBat2.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6) + 4;
            playerBat2.angle = 0;
            playerBat2.xSize = MTH_FIXED(16);
            playerBat2.ySize = MTH_FIXED(24);
            break;
        case 2:
            playerBat3.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6) + 4;
            playerBat3.angle = 0;
            playerBat3.xSize = MTH_FIXED(16);
            playerBat3.ySize = MTH_FIXED(24);
            break;
        case 3:
            playerBat4.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6) + 4;
            playerBat4.angle = 0;
            playerBat4.xSize = MTH_FIXED(16);
            playerBat4.ySize = MTH_FIXED(24);
            break;
        default:
            break;
    }
    
}

void player_stand(int partyNum) {
    
    if(!isMoving) {
        switch(partyNum) {
            case 0:
                playerBat1.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6);
                playerBat1.angle = 0;
                playerBat1.xSize = MTH_FIXED(16);
                playerBat1.ySize = MTH_FIXED(24);
                break;
            case 1:
                playerBat2.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6);
                playerBat2.angle = 0;
                playerBat2.xSize = MTH_FIXED(16);
                playerBat2.ySize = MTH_FIXED(24);
                break;
            case 2:
                playerBat3.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6);
                playerBat3.angle = 0;
                playerBat3.xSize = MTH_FIXED(16);
                playerBat3.ySize = MTH_FIXED(24);
                break;
            case 3:
                playerBat4.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6);
                playerBat4.angle = 0;
                playerBat4.xSize = MTH_FIXED(16);
                playerBat4.ySize = MTH_FIXED(24);
                break;
            default:
                break;
        }
    }
    
}

void player_flip(int partyNum) {
    switch(partyNum) {
            case 0:
                playerBat1.mirror = MIRROR_HORIZ;
                break;
            case 1:
                playerBat2.mirror = MIRROR_HORIZ;
                break;
            case 2:
                playerBat3.mirror = MIRROR_HORIZ;
                break;
            case 3:
                playerBat4.mirror = MIRROR_HORIZ;
                break;
            default:
                break;
        }
}

void player_dead(int partyNum) {
    
    switch(partyNum) {
        case 0:
            playerBat1.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6) + 5;
            playerBat1.angle = 0;
            playerBat1.xSize = MTH_FIXED(24);
            playerBat1.ySize = MTH_FIXED(16);
            playerBat1.xPos = MTH_FIXED(192);
            break;
        case 1:
            playerBat2.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6) + 5;
            playerBat2.angle = 0;
            playerBat2.xSize = MTH_FIXED(24);
            playerBat2.ySize = MTH_FIXED(16);
            playerBat2.xPos = MTH_FIXED(192);
            break;
        case 2:
            playerBat3.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6) + 5;
            playerBat3.angle = 0;
            playerBat3.xSize = MTH_FIXED(24);
            playerBat3.ySize = MTH_FIXED(16);
            playerBat3.xPos = MTH_FIXED(192);
            break;
        case 3:
            playerBat4.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6) + 5;
            playerBat4.angle = 0;
            playerBat4.xSize = MTH_FIXED(24);
            playerBat4.ySize = MTH_FIXED(16);
            playerBat4.xPos = MTH_FIXED(192);
            break;
        default:
            break;
    }
    
    
}

void party_crouch() {
    for(Uint8 i = 0; i < 4; i++) {
        if(isPlayerLowHealth(i, party[i].max_hp)){
            player_crouch(i);
        }
    }
}

void party_dead() {
    for(Uint8 i = 0; i < 4; i++) {
        if(battle_char_stats[i].hp == 0) {
            player_dead(i);
        }
    }
}


void party_animate_battle_cheer() {

     if(totalMovement == 16){
        if(isStanding) {
            if(battle_char_stats[0].hp > 0) {
                playerBat1.char_num += 3;
            }
            
            if(battle_char_stats[1].hp > 0) {
                playerBat2.char_num += 3;
            }
            
            if(battle_char_stats[2].hp > 0) {
                playerBat3.char_num += 3;
            }
            
            if(battle_char_stats[3].hp > 0) {
                playerBat4.char_num += 3;
            }
            
            isStanding = FALSE;
        } else {
        if(battle_char_stats[0].hp > 0) {
                playerBat1.char_num -= 3;
            }
            
            if(battle_char_stats[1].hp > 0) {
                playerBat2.char_num -= 3;
            }
            
            if(battle_char_stats[2].hp > 0) {
                playerBat3.char_num -= 3;
            }
            
            if(battle_char_stats[3].hp > 0) {
                playerBat4.char_num -= 3;
            }
            
            isStanding = TRUE;
        }
        totalMovement = 0;
    }


    totalMovement += 1;
}

void doBattleMenu() {

    if(bat_menu_state == SELECT || bat_menu_state == SELECT_ENEMY_TARGET) {
        drawBattleCursor = TRUE;
    } else {
        drawBattleCursor = FALSE;
    }

    if(bat_menu_state == SELECT_MAGIC || bat_menu_state == TO_SELECT_MAGIC) {
        drawTargetNames = FALSE;
    } else {
        drawTargetNames = TRUE;
    }

    switch(bat_menu_state) {
        case INIT:
            cur_battle_menu_selection = FIGHT;
            prev_battle_menu_selection = FIGHT;
            if (isPlayerDead(cur_char_battle_menu)) {
                cur_char_battle_menu++;
            } else {
                if(isPlayerLowHealth(cur_char_battle_menu, party[cur_char_battle_menu].max_hp)) {
                    player_stand(cur_char_battle_menu);
                }
                isMoving = TRUE;
                moveBattlePlayerLeft(cur_char_battle_menu);
            }
            break;
        case SELECT:
            break;
        case SELECT_CANCEL:
            if(isPlayerLowHealth(cur_char_battle_menu, party[cur_char_battle_menu].max_hp)) {
                player_stand(cur_char_battle_menu);
            }
            isMoving = TRUE;
            moveBattlePlayerRight(cur_char_battle_menu);
            break;
        case TO_SELECT_ENEMY_TARGET:
            currentSlot = getNextOccupiedSlot(0);

            if(!canMoveCursor) {
                cursor.xPos = MTH_FIXED(getXSlotCoord(currentSlot));
                cursor.yPos = MTH_FIXED(getYSlotCoord(currentSlot));
                frame_count_b = 0;
                canMoveCursor = 1;
                bat_menu_state = SELECT_ENEMY_TARGET;
            }
            break;
         case TO_SELECT_MAGIC:
            drawMagicMenu(cur_char_battle_menu, 0);
            break;
        case SELECT_ENEMY_TARGET:
            break;
        case END_SELECT:
            if(isPlayerLowHealth(cur_char_battle_menu, party[cur_char_battle_menu].max_hp)) {
                player_stand(cur_char_battle_menu);
            }
            isMoving = TRUE;
            moveBattlePlayerRight(cur_char_battle_menu);           
            break;
        case END:
            currentTurnBufferIndex = 0;
            state = TO_BAT_EXECUTE_STATE;
        default:
            break;
    }
}

void doBattleExec() {
    
     Uint8 respondRate = response_rate_lut[getResponseRate()];
    switch(turn_exec_state) {
        case BETWEEN_TURN:
        
        if(respondRateCounter < respondRate) {
            break;
        } else {
            clearMessageArea();
            ineffective = FALSE;

            if(allEnemiesDead()) {
                state = TO_BAT_VICTORY_STATE;
                turn_exec_state = EXEC_END;
            }
            if(isPartyDead()) {
                state = TO_GAME_OVER;
                turn_exec_state = EXEC_END;
            }

            currentTurn = battleOrderBuffer[currentTurnBufferIndex];
            if(currentTurn > 0x7F) {
                if(isPlayerDead(currentTurn - 0x80)) {
                    currentTurnBufferIndex++;
                } else if(isSurprised) {
                    currentTurnBufferIndex++;
                } else {
                    turn_exec_state = EXEC_INIT;
                }
            } else if(isPreemptive) {
                currentTurnBufferIndex++;
            } else if(slotOccupied[currentTurn]) {
                turn_exec_state = EXEC_INIT;
            } else {
                currentTurnBufferIndex++;
            }
            
            if(currentTurnBufferIndex > 12) {
                turn_exec_state = EXEC_END;
            }
            break;
        }
        case EXEC_INIT:
            if(respondRateCounter < respondRate) {
                break;
            } else {
                respondRateCounter = 0;
                if(currentTurn > 0x7F) {
                    switch(playerActions[currentTurn - 0x80].action) {
                        case FIGHT:
                            targetIndex = playerActions[currentTurn - 0x80].targetIndex;
                            drawAttackerDefenderMessage(battle_char_stats[currentTurn - 0x80].name, enemies[targetIndex].name);
                            if(enemies[targetIndex].hp == 0) {
                                ineffective = TRUE;
                            }
                            break;
                        case RUN:
                            drawAttemptRunMessage(battle_char_stats[currentTurn - 0x80].name);
                            if(canRun(currentTurn - 0x80)) {
                                runFailed = FALSE;
                                turn_exec_state = EXEC_END;
                                state = TO_BAT_RUN_AWAY;
                            } else {
                                runFailed = TRUE;
                                turn_exec_state = RESULT;
                            }
                            break;
                        default:
                            break;
                    }
                    
                    turn_exec_state = ANIMATE1;
                } else {
                    curr_enemy_target = determinePlayerTarget();
                    
                    drawAttackerDefenderMessage(enemies[slotToEnemyNum[currentTurn]].name, battle_char_stats[curr_enemy_target].name);
                    turn_exec_state = CALCULATE;
                }
            }
            break;
        case ANIMATE1:
            if(isPlayerLowHealth(currentTurn - 0x80, party[currentTurn - 0x80].max_hp)) {
                player_stand(currentTurn - 0x80);
            }
            isMoving = TRUE;
            moveBattlePlayerLeft(currentTurn - 0x80);
            break;
        case CALCULATE:
            if(currentTurn > 0x7F) {
                if(!ineffective) {
                    takePlayersTurn(currentTurn - 0x80);
                }

              weapon_char = (battle_char_stats[currentTurn - 0x80].wep_gfx -0x80) / 4;
                weaponBat.char_num = weaponSpriteIndex + weapon_char;
                switch(currentTurn  - 0x80) {
                    case 0:
                        weaponBat.yPos = playerBat1.yPos;
                        break;
                    case 1:
                        weaponBat.yPos = playerBat2.yPos;
                        break;
                    case 2:
                        weaponBat.yPos = playerBat3.yPos;
                        break;
                    case 3:
                        weaponBat.yPos = playerBat4.yPos;
                        break;
                }
                updateWeaponPalettes(battle_char_stats[currentTurn - 0x80].wep_pal, TRUE);
                frameCount = 0;
                turn_exec_state = ANIMATE2;
                break;
            } else {
                takeEnemyTurn(currentTurn);
                frameCount = 0;
                turn_exec_state = SCREEN_SHAKE;
                break;
            }
        case SCREEN_SHAKE:
            shakeScreen();
            break;
        case ANIMATE2:  
            swingWeapon(currentTurn - 0x80);
            break;
        case PREP_EXPLOSION:
        case EXPLOSION:
            if(battle.battle_type == 0) {
                if(explosionCounter < 6) {
                    if(frameCount < 3){
                        drawSmallFormationExplosion(targetIndex);
                        frameCount++;
                    } else {
                       explosionCounter++; 
                       frameCount = 0;
                    }
                } else {
                    sprite_delete(&explosionBat);
                    turn_exec_state = RESULT;
                }
            } else if(battle.battle_type == 1) {
                if(explosionCounter < 8) {
                    if(frameCount < 3){
                        drawLargeFormationExplosion(targetIndex);
                        frameCount++;
                    } else {
                       explosionCounter++; 
                       frameCount = 0;
                    }
                } else {
                    sprite_delete(&explosionBat);
                    turn_exec_state = RESULT;
                }
            } else if(battle.battle_type == 2) {
                if(explosionCounter < 16) {
                    if(frameCount < 3){
                        if(getEnemySlotNum(targetIndex) < 2) {
                            drawLargeFormationExplosion(targetIndex);
                            frameCount++; 
                        } else {
                            drawSmallMixFormationExplosion(targetIndex);
                            frameCount++; 

                        }
                    } else {
                       explosionCounter++; 
                       frameCount = 0;
                    }
                } else {
                    sprite_delete(&explosionBat);
                    turn_exec_state = RESULT;
                }
            }
            break;
        case DELETING_ENEMY:
            targetIndex = playerActions[currentTurn - 0x80].targetIndex;
            Uint8 xy = 4;
            if(enemies[targetIndex].pic_num >= 2) {
                xy = 6;
            }
            Uint8 slotNum = getEnemySlotNum(targetIndex);
            slotOccupied[slotNum] = FALSE;
            deleteEnemy(slotXCoords[slotNum], slotYCoords[slotNum], xy);
            break;
        case RESULT:  
            if(respondRateCounter < respondRate) {
                break;
            } else {
                if(curr_attack_miss) {
                    drawLutResultMessage(BTLMSG_MISSED);
                } else if(ineffective) {
                    drawLutBottomdMessage(BTLMSG_INEFFECTIVE);
                }  else if(runFailed) {
                    drawLutBottomdMessage(BTLMSG_CANTRUN);
                } else {
                    drawDmgString(curr_attack_dmg);
                    
                }
                turn_exec_state = RESULT_2;
            }
        case RESULT_2: 
            if(respondRateCounter < respondRate) {
                break;
            } else {

                if(!ineffective && !runFailed) {
                    if(currentTurn > 0x7F) {
                        targetIndex = playerActions[currentTurn - 0x80].targetIndex;
                        if(!curr_attack_miss && curr_attack_dmg >= enemies[targetIndex].hp) {
                            drawLutBottomdMessage(BTLMSG_TERMINATED);
                            setLastDeletedEnemyRow(0);
                            turn_exec_state = DELETING_ENEMY;
                        }
                    } else {
                        if(!curr_attack_miss && curr_attack_dmg >= battle_char_stats[curr_enemy_target].hp) {
                            drawLutBottomdMessage(BTLMSG_SLAIN);
                        }
                    }
                }

                if(turn_exec_state != DELETING_ENEMY) {
                    turn_exec_state = FINISH_TURN;
                }
                respondRateCounter = 0;
                break;
            }

        case WAIT:
            break;
        case FINISH_TURN:
            respondRateCounter = 0;
            if(currentTurn > 0x7F) {
                if(!ineffective && !runFailed) {
                    targetIndex = playerActions[currentTurn - 0x80].targetIndex;
                    if(!curr_attack_miss) {
                        if(curr_attack_dmg >= enemies[targetIndex].hp) {
                            enemies[targetIndex].hp = 0;
                            enemies[targetIndex].ailment ^ AIL_DEAD;
                        } else {
                            enemies[targetIndex].hp -= curr_attack_dmg;
                        }
                    }
                }
                if(!runFailed) {
                    if(isPlayerLowHealth(currentTurn - 0x80, party[currentTurn - 0x80].max_hp)) {
                        player_stand(currentTurn - 0x80);
                    }
                    isMoving = TRUE;
                    curr_attack_dmg = 0;
                    curr_attack_miss = FALSE;
                    moveBattlePlayerRight(currentTurn - 0x80);
                } else {
                    curr_attack_dmg = 0;
                    curr_attack_miss = FALSE;
                    runFailed = FALSE;
                }
            } else {
                if(!curr_attack_miss) {
                    if(curr_attack_dmg >= battle_char_stats[curr_enemy_target].hp) {
                        battle_char_stats[curr_enemy_target].hp = 0;
                        battle_char_stats[curr_enemy_target].ailment ^ AIL_DEAD;
                        player_dead(curr_enemy_target);
                    } else {
                        battle_char_stats[curr_enemy_target].hp -= curr_attack_dmg;
                        if(isPlayerLowHealth(curr_enemy_target, party[curr_enemy_target].max_hp)) {
                            player_crouch(curr_enemy_target);
                        }
                    }
                }
                curr_attack_dmg = 0;
                curr_attack_miss = FALSE;
                currentTurnBufferIndex++;
                turn_exec_state = BETWEEN_TURN;
            }
        
        break;
        case EXEC_END:
            isSurprised = FALSE;
            isPreemptive = FALSE;
            if(state != TO_BAT_VICTORY_STATE || state != TO_GAME_OVER || state != TO_BAT_RUN_AWAY) {
                state = TO_BAT_MENU_STATE;
            }
            break;

    }
}

Uint8 canMove(int dir){
    Uint16 prop = 0;
    
    if(state != OW_STATE) {
        return 0;
    }

    switch(dir) {
        case LEFT:
            prop = getTileProperty1(current_y_pos, current_x_pos - 1);
            break;
        case RIGHT:
            prop = getTileProperty1(current_y_pos, current_x_pos + 1);
            break;
        case UP:
            prop = getTileProperty1(current_y_pos - 1, current_x_pos);
            break;
        case DOWN:
            prop = getTileProperty1(current_y_pos + 1, current_x_pos);
            break;
    }

    int property = MTH_FIXED(prop);

    int ableTOMove = property & vehicle;

    if(ableTOMove == 0){
        return TRUE;
    } else {
        return FALSE;
    }
}

Uint8 inForest() {
    Uint16 prop = getTileProperty1(current_y_pos, current_x_pos);
    int property = MTH_FIXED(prop);

    int inForest = property & IN_FOREST;
    if(inForest == 0){
        return FALSE;
    } else {
        return TRUE;
    }
}

int doBattle() {
    Uint16 prop = getTileProperty2(current_y_pos, current_x_pos);
    int property = MTH_FIXED(prop);
    int domain; 
    Uint8 battle;

    prev_current_x_pos = current_x_pos;
    prev_current_y_pos = current_y_pos;

    switch(property) {
        case NORMAL_BATTLE:
            if(encounterBattle()) {
                currentBackDrop = getTileBattleBackdrop(current_y_pos, current_x_pos);
                domain = determineBattleDomain(current_x_pos, current_y_pos, property);
                battle = getBattleNum(domain);
                battle_init(battle);
                return NORMAL_BATTLE;
            } else {
                return 0;
            }
        case RIVER_BATTLE_1:
            if(encounterBattle()) {
                currentBackDrop = getTileBattleBackdrop(current_y_pos, current_x_pos);
                domain = determineBattleDomain(current_x_pos, current_y_pos, property);
                battle = getBattleNum(domain);
                battle_init(battle);
                return RIVER_BATTLE_1;
            } else {
                return 0;
            }
        case OCEAN_BATTLE:
            if(encounterBattle()) {
                currentBackDrop = getTileBattleBackdrop(current_y_pos, current_x_pos);
                domain = determineBattleDomain(current_x_pos, current_y_pos, property);
                battle = getBattleNum(domain);
                battle_init(battle);
                return OCEAN_BATTLE;
            } else {
                return 0;
            }
        case RIVER_BATTLE_2:
            if(encounterBattle()) {
                currentBackDrop = getTileBattleBackdrop(current_y_pos, current_x_pos);
                domain = determineBattleDomain(current_x_pos, current_y_pos, property);
                battle = getBattleNum(domain);
                battle_init(battle);
                return RIVER_BATTLE_2;
            } else {
                return 0;
            }
        default:
            return 0;
    }
}

void determineSurprise() {

    Uint16 leader_agi = party[0].agi;
    Uint16 leader_luck = party[0].luck;

    Uint8 seed = (leader_agi + leader_luck) / 8;
    Uint8 value = (rand_min_max((seed + seed), (seed + 100)) - battle.surprise_rate);

    if(value < 11) {
        isSurprised = TRUE;
    } else if(value > 90) {
        isPreemptive = TRUE;
    } else {
        isSurprised = FALSE;
        isPreemptive = FALSE;
    }
}

copyPlayerStatsToBattleStats() {
    for(Uint8 i = 0; i < 4; i++) {
        battle_char_stats[i].slot_index = i;
        battle_char_stats[i].char_class = party[i].char_class;
        battle_char_stats[i].level = party[i].level;
        battle_char_stats[i].luck = party[i].luck;
        battle_char_stats[i].ailment = party[i].ailment;
        battle_char_stats[i].hp = party[i].current_hp;
        battle_char_stats[i].hit_percent = party[i].hit_percent;
        battle_char_stats[i].mag_def = party[i].mag_def;
        battle_char_stats[i].evade_percent = party[i].evade_percent;
        battle_char_stats[i].absorb = party[i].absorb;
        battle_char_stats[i].damage = party[i].damage;
        battle_char_stats[i].elem_resist = party[i].resist;
        battle_char_stats[i].numhitsmult = 1;
        Uint8 numHits = party[i].hit_percent;
        battle_char_stats[i].numhits = (numHits >> 5) + 1;

        //These should come from the equipped weapon but the original game goofed here. 
        //TODO: add flag to use intended behavior.
        battle_char_stats[i].category = 0;
        battle_char_stats[i].elem_weak = 0;
        //Get Equiped Weapon data
        Uint8 wepId = 0;

        for(Uint8 weaponIter = 0; i < 4; weaponIter++) {
            wepId = party[i].weapons[weaponIter];

            if(wepId > 0x7F) {
                break;
            }
        }

        if(wepId > 0) {
            wepId = wepId - 0x80;
            int lut_index = wepId * 0x8;
            int lut_offset = WEAPON_LUT_LOCATION + lut_index;

            Uint8 *wep_ptr = (Uint8*) lut_offset;

            
            battle_char_stats[i].crit_rate = wepId; //Set to Weapon Index - Wrong but what the original game does. TODO: add flag for correct behavior.
            battle_char_stats[i].wep_gfx = wep_ptr[6];
            battle_char_stats[i].wep_pal = wep_ptr[7];
        } else {
            battle_char_stats[i].crit_rate = 0; //Set to Weapon Index - Wrong but what the original game does.
            battle_char_stats[i].wep_gfx = 0;
            battle_char_stats[i].wep_pal = 0;
        }

        //Master and Blackbelt get special perks if they're unarmed.
        if(battle_char_stats[i].char_class == 2 || battle_char_stats[i].char_class == 8) {

            if(battle_char_stats[i].wep_gfx == 0 ) {
                battle_char_stats[i].wep_gfx = 0xAC; //Unarmed blackbelt fist
                battle_char_stats[i].wep_pal = 0x28; //Unarmed blackbelt palette
                battle_char_stats[i].crit_rate = 2 * battle_char_stats[i].level;
                battle_char_stats[i].numhits = 2 * battle_char_stats[i].numhits;
            }
        }

        for(Uint8 magicLvl = 0; magicLvl < 8; magicLvl++) {
            battle_char_stats[i].current_mp[magicLvl] = party[i].current_mp[magicLvl];
            battle_char_stats[i].max_mp[magicLvl] = party[i].max_mp[magicLvl];
            for(Uint8 spellNum = 0; spellNum < 3; spellNum++){
                battle_char_stats[i].magic[magicLvl][spellNum] = party[i].magic[magicLvl][spellNum];
            }
        }


        battle_char_stats[i].name[0] = party[i].name[0];
        battle_char_stats[i].name[1] = party[i].name[1];
        battle_char_stats[i].name[2] = party[i].name[2];
        battle_char_stats[i].name[3] = party[i].name[3];
        battle_char_stats[i].name[4] = party[i].name[4];
        battle_char_stats[i].name[5] = party[i].name[5];
        battle_char_stats[i].name[6] = 0;
        battle_char_stats[i].name[7] = 0;
    }
}

copyBattleStatsToPlayerStats() {
    for(Uint8 i = 0; i < 4; i++) {
        party[i].ailment = battle_char_stats[i].ailment;
        party[i].current_hp = battle_char_stats[i].hp;
        party[i].hit_percent = battle_char_stats[i].hit_percent;
        party[i].mag_def = battle_char_stats[i].mag_def;
        party[i].evade_percent = battle_char_stats[i].evade_percent;
        party[i].absorb = battle_char_stats[i].absorb;
        party[i].damage = battle_char_stats[i].damage;
        party[i].resist = battle_char_stats[i].elem_resist;
        party[i].level = battle_char_stats[i].level;
        party[i].luck = battle_char_stats[i].luck;
        
        for(Uint8 magicLvl = 0; magicLvl < 8; magicLvl++) {
            party[i].current_mp[magicLvl] = battle_char_stats[i].current_mp[magicLvl];
            party[i].max_mp[magicLvl] = battle_char_stats[i].max_mp[magicLvl];
            for(Uint8 spellNum = 0; spellNum < 3; spellNum++){
                party[i].magic[magicLvl][spellNum] = battle_char_stats[i].magic[magicLvl][spellNum];
            }
        }
        
    }
}


void applyEquippedWeapon(Uint8 index){

    Uint8 wepId = 0;

    for(Uint8 i = 0; i < 4; i++) {
       wepId = party[index].weapons[i];

       if(wepId > 0x7F) {
           wepId = wepId - 0x80;
           break;
       }
    }

    if(wepId > 0) {
        int lut_index = wepId * 0x8;
        int lut_offset = WEAPON_LUT_LOCATION + lut_index;

        Uint8 *wep_ptr = (Uint8*) lut_offset;

        party[index].hit_percent += wep_ptr[0];
        party[index].damage += wep_ptr[1];

    }

}

void applyEquippedArmor(Uint8 index) {

    Uint8 armId = 0;

    for(Uint8 i = 0; i < 4; i++) {
       armId = party[index].armors[i];

       if(armId > 0x7F) {
           armId = armId - 0x80;
           break;
       }
    }

    if(armId > 0) {
        int lut_index = armId * 0x4;
        int lut_offset = ARMOR_LUT_LOCATION + lut_index;

        Uint8 *arm_ptr = (Uint8*) lut_offset;

        if(arm_ptr[0] > party[index].evade_percent) {
            party[index].evade_percent = 0;
        } else {
            party[index].evade_percent -= arm_ptr[0];
        }

        party[index].absorb += arm_ptr[1];

    }

}

void reset() {
    cur_char_create = 0;
}

Uint8 determine_curr_char_class() {

     for(Uint8 i = 0; i < 4; i++) {
        if(party[i].current_hp > 0) {
            return party[i].char_class;
        }
     }

     return 0;
}


void determine_curr_char() {
    for(Uint8 i = 0; i < 4; i++) {
        if(party[i].current_hp > 0) {

            switch(party[i].char_class ) {
                case WARRIOR_VALUE:
                    currentChar = WARRIOR_OFFSET;
                    break;
                case THIEF_VALUE:
                    currentChar = THIEF_OFFSET;
                    break;
                case MONK_VALUE:
                    currentChar = MONK_OFFSET;
                    break;
                case REDMAGE_VALUE:
                    currentChar = REDMAGE_OFFSET;
                    break;
                case WMAGE_VALUE:
                    currentChar = WMAGE_OFFSET;
                    break;
                case BMAGE_VALUE:
                    currentChar = BMAGE_OFFSET;
                    break;
                default:
                    currentChar = WARRIOR_OFFSET;
                    break;
            }

            i = 4;

        }
    }
    
    current_x_pos = prev_current_x_pos;
    current_y_pos = prev_current_y_pos;
    
    totalMovement = 0;
    
}

void initMapManPalette(Uint32 *palette_a, Uint32 *palette_b, Uint8 *palPtr, Uint8 *palPtr2) {

    palette_a[0] = nes_pal[palPtr[0]];
    palette_a[1] = nes_pal[palPtr[1]];
    palette_a[2] = nes_pal[palPtr2[0]];
    palette_a[3] = nes_pal[palPtr[3]];

    palette_b[0] = nes_pal[palPtr[0]];
    palette_b[1] = nes_pal[palPtr[1]];
    palette_b[2] = nes_pal[palPtr2[1]];
    palette_b[3] = nes_pal[palPtr[3]];
}

void performActionButtonInput() {
    if(state == PRELUDE) {
        state = TO_TITLE_STATE;
    } else if(state == BAT_STATE) {
        if(isSurprised) {
            state = TO_BAT_EXECUTE_STATE;
        } else {
            state = TO_BAT_MENU_STATE;
        }
    } else if(state == BAT_MENU_STATE && bat_menu_state == SELECT) {

        switch(cur_battle_menu_selection) {
            case FIGHT:
                prev_bat_menu_x_pos = cursor.xPos;
                prev_bat_menu_y_pos = cursor.yPos;
                bat_menu_state = TO_SELECT_ENEMY_TARGET;
                break;
            case MAGIC:
                prev_bat_menu_x_pos = cursor.xPos;
                prev_bat_menu_y_pos = cursor.yPos;
                bat_menu_state = TO_SELECT_MAGIC;
                break;
            case DRINK:
                break;
            case ITEM:
                break;
            case RUN: 
                updatePlayerBattleAction(cur_char_battle_menu, 0, cur_battle_menu_selection, 0, FALSE);
                bat_menu_state = END_SELECT;
                break;
        }

    } else if (state == BAT_MENU_STATE && bat_menu_state == SELECT_ENEMY_TARGET) { 
        updatePlayerBattleAction(cur_char_battle_menu, currentSlot, cur_battle_menu_selection, 0, FALSE);
        bat_menu_state = END_SELECT;
    } else if (state == BAT_EXECUTE_STATE && turn_exec_state == WAIT) { 
        clearMessageArea();
        turn_exec_state = FINISH_TURN;
    } else if(state == BAT_VICTORY_STATE) {
        isMoving = FALSE;
        state = TO_OW_STATE;
    } else if(state == BAT_RUN_AWAY) {
        isMoving = FALSE;
        state = TO_OW_STATE;
    } else if(state == GAME_OVER) {
        isMoving = FALSE;
        state = GAME_RESTART;
    } else if(state == TITLE_STATE) {
        state = TO_CHAR_SELECT_STATE;
    } else if(state == CHAR_SELECT_STATE) {
        if(cur_char_create == 4) {
            state = TO_OW_STATE;
        } else {
            state = TO_CHAR_NAME_STATE;
                selected_class = party[cur_char_create].char_class;
        }
    }  else if(state == CHAR_NAME_STATE) {
        nameBuffer[cur_letter_index] = char_name_screen_lookup_table[cur_letter];

        cur_letter_index++;
        if(cur_letter_index == 6) {
            selected_class = party[cur_char_create].char_class;
            char_init(selected_class, cur_char_create, nameBuffer);
                            
            cur_char_create++;
            nameBuffer[0] = 0xFF;
            nameBuffer[1] = 0xFF;
            nameBuffer[2] = 0xFF;
            nameBuffer[3] = 0xFF;
            nameBuffer[4] = 0xFF;
            nameBuffer[5] = 0xFF;
            nameBuffer[6] = 0x00;
            cur_letter_index = 0;
            cur_letter = 0;
            state = TO_CHAR_SELECT_STATE;
        }
    }
}

void performCancelButtonInput() {
    if(state == CHAR_NAME_STATE) {
        if(cur_letter_index == 0) {
            state = TO_CHAR_SELECT_STATE;
            cur_letter_index = 0;
            cur_letter = 0;
        } else {
            cur_letter_index--;
            nameBuffer[cur_letter_index] = 0xFF;
        }
        
    } else if(state == CHAR_SELECT_STATE) {
        
        cur_letter = 0;

        if(cur_char_create > 0) {
            cur_char_create--;

            selected_class = party[cur_char_create].char_class;
            cursor.xPos = MTH_FIXED(getPartyCreateCursorPosX(cur_char_create));
            cursor.yPos = MTH_FIXED(getPartyCreateCursorPosY(cur_char_create));
        } else {
            state = TO_TITLE_STATE;
        }

    } else if(state == BAT_MENU_STATE  && bat_menu_state == SELECT) {
        bat_menu_state = SELECT_CANCEL;
    } else if (state == BAT_MENU_STATE && bat_menu_state == SELECT_ENEMY_TARGET) {
        cursor.xPos = prev_bat_menu_x_pos;
        cursor.yPos = prev_bat_menu_y_pos;
        bat_menu_state = SELECT;
     }else if (state == BAT_MENU_STATE && bat_menu_state == TO_SELECT_MAGIC) {
        cursor.xPos = prev_bat_menu_x_pos;
        cursor.yPos = prev_bat_menu_y_pos;
        clearMessageArea();
        drawMenu();
        bat_menu_state = SELECT;
    }
}

