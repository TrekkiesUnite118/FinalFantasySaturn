#include <sega_def.h>
#include <sega_mth.h>
#include <sega_scl.h>
#define        _SPR2_
#include <SEGA_SPR.H>
#include <sega_sys.h>
#include <string.h>
#include "player.h"
#include "print.h"
#include "overworld.h"
#include "scroll.h"
#include "sprite.h"
#include "vblank.h"
#include "battle.h"

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

#define WARRIOR_OFFSET (0)
#define THIEF_OFFSET (8)
#define MONK_OFFSET (16)
#define REDMAGE_OFFSET (24)
#define WMAGE_OFFSET (32)
#define BMAGE_OFFSET (40)

#define FOOT (MTH_FIXED(1))
#define CANOE (MTH_FIXED(2))
#define SHIP (MTH_FIXED(4))
#define AIRSHIP (MTH_FIXED(8))
#define IN_FOREST (MTH_FIXED(16))

#define PLAYER_MAXXPOS (MTH_FIXED(152))
#define PLAYER_MAXYPOS (MTH_FIXED(92))

#define PLAYER_MAXYPOSB (MTH_FIXED(100))

#define FRAME_STAND (96)
#define FRAME_WALK1 (FRAME_STAND + 1)
#define FRAME_STAND2 (FRAME_STAND + 2)
#define FRAME_WALK2 (FRAME_STAND + 3)

#define FRAME_STAND3 (FRAME_STAND + 4)
#define FRAME_STAND4 (FRAME_STAND + 5)

#define FRAME_WALK3 (FRAME_STAND + 6)
#define FRAME_WALK4 (FRAME_STAND + 7)

#define NEW_GAME_POS (MTH_FIXED(81))
#define CONTINUE_POS (MTH_FIXED(121))

#define SELECT_1_POS_X (MTH_FIXED(80))
#define SELECT_1_POS_Y (MTH_FIXED(55))
#define SELECT_2_POS_X (MTH_FIXED(192))
#define SELECT_2_POS_Y (MTH_FIXED(55))
#define SELECT_3_POS_X (MTH_FIXED(80))
#define SELECT_3_POS_Y (MTH_FIXED(143))
#define SELECT_4_POS_X (MTH_FIXED(192))
#define SELECT_4_POS_Y (MTH_FIXED(143))

#define NAME_LETTER_MIN_X (MTH_FIXED(64))
#define NAME_LETTER_MIN_Y (MTH_FIXED(71))

#define NAME_LETTER_MAX_X (MTH_FIXED(208))
#define NAME_LETTER_MAX_Y (MTH_FIXED(183))

const int START_POS_X = 153;
const int START_POS_Y = 165;

int current_x_pos;
int current_y_pos;

int vehicle = FOOT;

int forest = IN_FOREST;

SPRITE_INFO player;
SPRITE_INFO playerb;
SPRITE_INFO cursor;


SPRITE_INFO playerBat1;
SPRITE_INFO playerBat2;
SPRITE_INFO playerBat3;
SPRITE_INFO playerBat4;

SPRITE_INFO playerSel1;
SPRITE_INFO playerSel2;
SPRITE_INFO playerSel3;
SPRITE_INFO playerSel4;

Uint8 isMoving = 0;
Uint8 totalMovement = 0;
Uint8 changeToBat = 0;
Uint8 responseRate = 1;

int direction = DOWN;
int currentChar = WARRIOR_OFFSET;
int currentBattleState = 0;

Uint16 MIRROR_ANIM_1 = MIRROR_HORIZ;
Uint16 MIRROR_ANIM_2 = MIRROR_HORIZ;

char selectName1[7] = "FIGHTER ";
char selectName2[7] = "THIEF   ";
char selectName3[7] = "Bl.BELT ";
char selectName4[7] = "RedMAGE ";
char selectName5[7] = "Wh.MAGE ";
char selectName6[7] = "Bl.MAGE ";

char *classNames[6] = {&selectName1, &selectName2, &selectName3, &selectName4, &selectName5, &selectName6};

int cur_char_create = 0;
int cur_letter = 0;
int cur_letter_index = 0;
char nameBuffer[6] = "      ";
int selected_class = 0;

int ignoreA = 0;
int ignoreB = 0;
int ignoreC = 0;
int ignoreX = 0;
int ignoreY = 0;
int ignoreZ = 0;

char char_name_screen_lookup_table[70] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J',
                      'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T',
                      'U', 'V', 'W', 'X', 'Y', 'Z', '\'', ',', '.', ' ',
                      '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                      'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j',
                      'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't',
                      'u', 'v', 'w', 'x', 'y', 'z', '-', '"', '!', '?'};



void player_cursor_init(Fixed32 startX, Fixed32 startY) {

    if(state == TO_CHAR_SELECT_STATE) {
        if(cur_char_create == 0) {
            startX = SELECT_1_POS_X;
            startY = SELECT_1_POS_Y;
        } else if(cur_char_create == 1) {
            startX = SELECT_2_POS_X;
            startY = SELECT_2_POS_Y;
        } else if(cur_char_create == 2) {
            startX = SELECT_3_POS_X;
            startY = SELECT_3_POS_Y;
        } else if(cur_char_create == 3) {
            startX = SELECT_4_POS_X;
            startY = SELECT_4_POS_Y;
        }
    }

    sprite_make(cursorSpriteIndex,  startX,  startY, &cursor);
    player.xSize = MTH_FIXED(16);
    player.ySize = MTH_FIXED(16);
    
}


void player_select_init(){
    sprite_make(144 + (party[0].char_class * 6),  MTH_FIXED(96),  MTH_FIXED(56), &playerSel1);
    sprite_make(144 + (party[1].char_class * 6),  MTH_FIXED(208),  MTH_FIXED(56), &playerSel2);
    sprite_make(144 + (party[2].char_class * 6),  MTH_FIXED(96),  MTH_FIXED(152), &playerSel3);
    sprite_make(144 + (party[3].char_class * 6),  MTH_FIXED(208),  MTH_FIXED(152), &playerSel4);

    playerSel1.xSize = MTH_FIXED(16);
    playerSel1.ySize = MTH_FIXED(24);
    playerSel2.xSize = MTH_FIXED(16);
    playerSel2.ySize = MTH_FIXED(24);
    playerSel3.xSize = MTH_FIXED(16);
    playerSel3.ySize = MTH_FIXED(24);
    playerSel4.xSize = MTH_FIXED(16);
    playerSel4.ySize = MTH_FIXED(24);

}

void player_cursor_draw() {
    SPRITE_INFO temp;
    memcpy(&temp, &cursor, sizeof(temp));;

    sprite_draw(&temp);
}

void draw_response_rate() {
     print_hp_num(responseRate, 21, 24);
}

void draw_char_name() {
     print_string(nameBuffer, 3, 17, 6);
}

void player_select_draw() {
    playerSel1.char_num = 144 + (party[0].char_class * 6);
    playerSel2.char_num = 144 + (party[1].char_class * 6);
    playerSel3.char_num = 144 + (party[2].char_class * 6);
    playerSel4.char_num = 144 + (party[3].char_class * 6);
    sprite_draw(&playerSel1);
    sprite_draw(&playerSel2);
    sprite_draw(&playerSel3);
    sprite_draw(&playerSel4);

    print_string(classNames[party[0].char_class], 5, 9, 7);
    print_string(classNames[party[1].char_class], 5, 23, 7);
    print_string(classNames[party[2].char_class], 17, 9, 7);
    print_string(classNames[party[3].char_class], 17, 23, 7);
}

void player_ow_init(Fixed32 startX, Fixed32 startY) {
    sprite_make(FRAME_STAND + currentChar,  startX,  startY, &player);
    sprite_make(FRAME_WALK1 + currentChar,  startX,  startY + 8, &playerb);
    player.xSize = MTH_FIXED(16);
    player.ySize = MTH_FIXED(8);
    playerb.xSize = MTH_FIXED(16);
    playerb.ySize = MTH_FIXED(8);
    MIRROR_ANIM_2 &= ~MIRROR_HORIZ;

    current_x_pos = START_POS_X;
    current_y_pos = START_POS_Y;
    
}

void player_bat_init(Fixed32 startX, Fixed32 startY) {
    sprite_make(144 + (party[0].char_class * 6),  startX,  MTH_FIXED(40), &playerBat1);
    startY += 24;
    sprite_make(144 + (party[1].char_class * 6),  startX,  MTH_FIXED(64), &playerBat2);
    startY += 24;
    sprite_make(144 + (party[2].char_class * 6),  startX,  MTH_FIXED(88), &playerBat3);
    startY += 24;
    sprite_make(144 + (party[3].char_class * 6),  startX,  MTH_FIXED(112), &playerBat4);

    playerBat1.xSize = MTH_FIXED(16);
    playerBat1.ySize = MTH_FIXED(24);
    playerBat2.xSize = MTH_FIXED(16);
    playerBat2.ySize = MTH_FIXED(24);
    playerBat3.xSize = MTH_FIXED(16);
    playerBat3.ySize = MTH_FIXED(24);
    playerBat4.xSize = MTH_FIXED(16);
    playerBat4.ySize = MTH_FIXED(24);
    
}

void char_init(int classValue, int partyNum, char newName[6]) {

    int startingStatOffset;
    PLAYER_CHARACTER character;
    character.char_class = classValue;

    switch(classValue) {
        case WARRIOR_VALUE:
            startingStatOffset = WARRIOR_START_STAT_OFFSET;
            break;
        case THIEF_VALUE:
            startingStatOffset = THIEF_START_STAT_OFFSET;
            break;
        case MONK_VALUE:
            startingStatOffset = MONK_START_STAT_OFFSET;
            break;
        case REDMAGE_VALUE:
            startingStatOffset = REDMAGE_START_STAT_OFFSET;
            break;
        case WMAGE_VALUE:
            startingStatOffset = WMAGE_START_STAT_OFFSET;
            break;
        case BMAGE_VALUE:
            startingStatOffset = BMAGE_START_STAT_OFFSET;
            break;
        default:
            startingStatOffset = WARRIOR_START_STAT_OFFSET;
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
    character.max_hp = startMDef;
    character.isAlive = 1;
    character.name[0] = newName[0];
    character.name[1] = newName[1];
    character.name[2] = newName[2];
    character.name[3] = newName[3];
    character.name[4] = newName[4];
    character.name[5] = newName[5];
    character.level = 1;
    character.exp = 0;
    character.exp_to_next_lvl = 40;

    for(int mp_iter = 0; mp_iter < 8; mp_iter++) {
        character.current_mp[mp_iter] = 0;
        character.max_mp[mp_iter] = 0;
    }

    if(classValue != WARRIOR_VALUE && classValue != THIEF_VALUE){
        character.current_mp[0] = 2;
        character.max_mp[0] = 2;
    }

    for(int i = 0; i < 4; i++) {
        character.weapons[i] = 0;
        character.armors[i] = 0;
    }

    for(int maglevel = 0; maglevel < 8; maglevel++) {
        for(int magslot = 0; magslot < 3; magslot++) {
            character.magic[maglevel][magslot] = 0;
        }
    }

    party[partyNum] = character;
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
    

    if (PadData1 & PAD_A && !ignoreA) {    

        if(state == OW_STATE) {
            currentChar = WARRIOR_OFFSET;
        } else if(state == BAT_STATE) {
            state = TO_OW_STATE;
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
            if(cur_letter_index > 6) {
                selected_class = party[cur_char_create].char_class;
                char_init(selected_class, cur_char_create, nameBuffer);
                               
                cur_char_create++;
                nameBuffer[0] = ' ';
                nameBuffer[1] = ' ';
                nameBuffer[2] = ' ';
                nameBuffer[3] = ' ';
                nameBuffer[4] = ' ';
                nameBuffer[5] = ' ';
                cur_letter_index = 0;
                cur_letter = 0;
                state = TO_CHAR_SELECT_STATE;
            }
        }
        ignoreA = 1;
        
    }
    if (PadData1 & PAD_B && !ignoreB) {    

        if(state == CHAR_NAME_STATE) {
            if(cur_letter_index == 0) {
                state = TO_CHAR_SELECT_STATE;
            }
            cur_letter_index--;
            nameBuffer[cur_letter_index] = ' ';
            
        } else if(state == CHAR_SELECT_STATE) {
            cur_char_create--;
            selected_class = party[cur_char_create].char_class;
            cur_letter = 0;
            if(cur_char_create == 0) {
                cursor.xPos = SELECT_1_POS_X;
                cursor.yPos = SELECT_1_POS_Y;
            } else if(cur_char_create == 1) {
                cursor.xPos = SELECT_2_POS_X;
                cursor.yPos = SELECT_2_POS_Y;
            } else if(cur_char_create == 2) {
                cursor.xPos = SELECT_3_POS_X;
                cursor.yPos = SELECT_3_POS_Y;
            } else if(cur_char_create == 3) {
                cursor.xPos = SELECT_4_POS_X;
                cursor.yPos = SELECT_4_POS_Y;
            }
    
        }
        ignoreB = 1;
        
    }
    if (PadData1 & PAD_C && !ignoreC) {    

        if(state == TITLE_STATE) {
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
            if(cur_letter_index > 6) {
                selected_class = party[cur_char_create].char_class;
                char_init(selected_class, cur_char_create, nameBuffer);
              
                cur_char_create++;
                nameBuffer[0] = ' ';
                nameBuffer[1] = ' ';
                nameBuffer[2] = ' ';
                nameBuffer[3] = ' ';
                nameBuffer[4] = ' ';
                nameBuffer[5] = ' ';
                cur_letter_index = 0;
                cur_letter = 0;
                state = TO_CHAR_SELECT_STATE;
            }
        }
        ignoreC = 1;
        
    }
    if (PadData1 & PAD_X) {    

        currentChar = REDMAGE_OFFSET;
        
    }
    if (PadData1 & PAD_Y) {    

        currentChar = WMAGE_OFFSET;
        
    }
    if (PadData1 & PAD_Z) {    

        currentChar = BMAGE_OFFSET;
        
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
                if(current_x_pos == -1){
                    current_x_pos = 255;
                }
                isMoving = 1;
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
                
                if(current_x_pos == 256){
                    current_x_pos = 0;
                }
                isMoving = 1;
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
        }

    }
    else if (PadData1 & PAD_D) {
    
        if(state == OW_STATE) {
            if(!isMoving && canMove(DOWN)){
                player.char_num = FRAME_STAND + currentChar;
                playerb.char_num = FRAME_WALK1 + currentChar;
                current_y_pos++;
                
                if(current_y_pos == 256){
                    current_y_pos = 0;
                }
                direction = DOWN;
                isMoving = 1;            
                player.mirror &= ~MIRROR_HORIZ;
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
        }
        
    }
    else if (PadData1 & PAD_U) {
        
        if(state == OW_STATE) {
            if(!isMoving && canMove(UP)){
                player.char_num = FRAME_STAND2 + currentChar;
                playerb.char_num = FRAME_WALK2 + currentChar;
                current_y_pos--;
                
                if(current_y_pos == -1){
                    current_y_pos = 255;
                }
                direction = UP;
                isMoving = 1;    
                player.mirror &= ~MIRROR_HORIZ;
                
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
    SPRITE_INFO temp1;
    SPRITE_INFO temp2;
    SPRITE_INFO temp3;
    SPRITE_INFO temp4;
    
    memcpy(&temp1, &playerBat1, sizeof(temp1));
    memcpy(&temp2, &playerBat2, sizeof(temp2));
    memcpy(&temp3, &playerBat3, sizeof(temp3));
    memcpy(&temp4, &playerBat4, sizeof(temp4));
    sprite_draw(&temp1);
    sprite_draw(&temp2);
    sprite_draw(&temp3);
    sprite_draw(&temp4);

    print_string(party[0].name, 4, 29, 6);
    print_string("HP", 6, 29, 2);
    print_hp_num(party[0].current_hp, 7, 30);
    print_string(party[1].name, 10, 29, 6);
    print_string("HP", 12, 29, 2);
    print_hp_num(party[1].current_hp, 13, 30);
    print_string(party[2].name, 16, 29, 6);
    print_string("HP", 18, 29, 2);
    print_hp_num(party[2].current_hp, 19, 30);
    print_string(party[3].name, 22, 29, 6);
    print_string("HP", 24, 29, 2);
    print_hp_num(party[3].current_hp, 25, 30);
    
}

void movePlayerLeft(){
    
    player.dx = MTH_FIXED(-1);
    playerb.dx = MTH_FIXED(-1);

    if(totalMovement == 8){
        player_animate();
    }

    totalMovement += 1;

    if(totalMovement == 16){
        player_animate();
        isMoving = 0;
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

    if(totalMovement == 16){
        player_animate();
        isMoving = 0;
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

    if(totalMovement == 16){
        player_animate();
        isMoving = 0;
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

    if(totalMovement == 16){
        player_animate();
        isMoving = 0;
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

int canMove(int dir){
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
        return 1;
    } else {
        return 0;
    }
}

int inForest() {
    Uint16 prop = getTileProperty1(current_y_pos, current_x_pos);
    int property = MTH_FIXED(prop);

    int inForest = property & forest;
    if(inForest == 0){
        return 0;
    } else {
        return 1;
    }
}

int doBattle() {
    Uint16 prop = getTileProperty2(current_y_pos, current_x_pos);
    int property = MTH_FIXED(prop);
    int domain; 
    int battle;
    switch(property) {
        case NORMAL_BATTLE:
            if(encounterBattle()) {
                currentBackDrop = MTH_FIXED(getTileBattleBackdrop(current_y_pos, current_x_pos));
                domain = determineBattleDomain(current_x_pos, current_y_pos, property);
                battle = getBattleNum(domain);
                battle_init(battle);
                return NORMAL_BATTLE;
            } else {
                return 0;
            }
        case RIVER_BATTLE_1:
            if(encounterBattle()) {
                currentBackDrop = MTH_FIXED(getTileBattleBackdrop(current_y_pos, current_x_pos));
                domain = determineBattleDomain(current_x_pos, current_y_pos, property);
                battle = getBattleNum(domain);
                battle_init(battle);
                return RIVER_BATTLE_1;
            } else {
                return 0;
            }
        case OCEAN_BATTLE:
            if(encounterBattle()) {
                currentBackDrop = MTH_FIXED(getTileBattleBackdrop(current_y_pos, current_x_pos));
                domain = determineBattleDomain(current_x_pos, current_y_pos, property);
                battle = getBattleNum(domain);
                battle_init(battle);
                return OCEAN_BATTLE;
            } else {
                return 0;
            }
        case RIVER_BATTLE_2:
            if(encounterBattle()) {
                currentBackDrop = MTH_FIXED(getTileBattleBackdrop(current_y_pos, current_x_pos));
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
