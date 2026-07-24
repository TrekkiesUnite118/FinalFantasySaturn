
#include <sega_spr.h>
#include <sega_scl.h>
#include "battle.h"
#include "static.h"
#include "print.h"
#include "menu.h"
#include "vblank.h"

#define ENEMY_NAME_LOOKUP_TABLE (0x0022D4F0)
#define ENEMY_NAME_PTR_ADD (0x00224010)
#define ENEMY_DATA_TABLE (0x00230530)
#define BATTLE_DATA_TABLE (0x0022C410)
#define BATTLE_RNG_TABLE (0x0023FD01)
#define BATTLE_PALETTE_TABLE (0x00230F30)
#define BATTLE_MSG_TABLE (0x0022CC50)
#define BATTLE_MSG_PTR_TABLE (0x0022CF60)

#define UPPER_CASE_LETTER_VALUE (0x8A)
#define UPPER_CASE_ASCII_DIFF (0x49)
#define NUMERICAL_SYMBOL_VALUE (0x80)
#define NUMERICAL_ASCII_DIFF (0x50)
#define LOWER_CASE_LETTER_VALUE (0xA4)
#define LOWER_CASE_ASCII_DIFF (0x43)

#define SMALL_ENEMY_START_Y MTH_FIXED(40)
#define SMALL_ENEMY_COL_1 MTH_FIXED(40)
#define SMALL_ENEMY_COL_2 MTH_FIXED(72)
#define SMALL_ENEMY_COL_3 MTH_FIXED(104)

#define LARGE_ENEMY_START_Y MTH_FIXED(72)
#define LARGE_ENEMY_COL_1 MTH_FIXED(40)
#define LARGE_ENEMY_COL_2 MTH_FIXED(88)

#define BATTLE_BANK_OFFSET (0x00230000)

const int ENEMY_SPRITE_PATTERN_DATA_OFFSET = 0x0021C010;

Uint8 ATTACKER_MESSAGE_INDEX = 0;
Uint8 ATTACK_MESSAGE_INDEX = 1;
Uint8 DEFENDER_MESSAGE_INDEX = 2;
Uint8 DAMAGE_MESSAGE_INDEX = 3;
Uint8 BOTTOM_MESSAGE_INDEX = 4;

Uint32 pal_0[] = {
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
};
Uint32 pal_1[] = {
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
};

Uint8 numOfSmallEnemySlots = 0;
Uint8 numOfLargeEnemySlots = 0;
Uint8 numOfFiendSlots = 0;
Uint8 numOfChaosSlots = 0;

Uint8 numOfSmallEnemySlotsAvailable = 9;
Uint8 numOfLargeEnemySlotsAvailable = 4;
Uint8 numOfFiendSlotsAvailable = 1;
Uint8 numOfChaosSlotsAvailable = 1;

Uint8 totalEnemies = 0;
Uint8 totalLargeEnemies = 0;
Uint8 totalSmallEnemies = 0;
Uint8 enemiesRemaining = 0;

Uint8 enemy0_num = 0;
Uint8 enemy1_num = 0;
Uint8 enemy2_num = 0;
Uint8 enemy3_num = 0;

Uint8 enemy_name_0[8];
Uint8 enemy_name_1[8];
Uint8 enemy_name_2[8];
Uint8 enemy_name_3[8];

Uint8 picassignment[4];
Uint8 palassignment[4];

Uint8 enemyNums[9];
Uint8 battle_rng_seed = 0;
Uint32 rtc_offset = 0;
Uint32 rng_init = 0;

Uint8 battle_formation_0_order[9] = {1, 0, 2, 4, 3, 5, 7, 6, 8};
Uint8 battle_formation_1_order[4] = {0, 1, 2, 3};
Uint8 battle_formation_2_order[8] = {0, 1, 3, 2, 4, 6, 5, 7};

static Uint32	PadWorkArea[7];
Uint8 attackerMessageCounter = 0;
Uint8 lastDeletedEnemyRow = 0;


BATTLE_INFO_BOX *info_box_luts = (BATTLE_INFO_BOX*) BATTLE_INFO_BOX_LUT;

void battle_init(Uint8 id) {
    enemy0_num = 0;
    enemy1_num = 0;
    enemy2_num = 0;
    enemy3_num = 0;

    for(Uint8 i = 0; i < 9; i++) {
        enemy_init(&enemies[i]);
        slotOccupied[i] = FALSE;
        slotToEnemyNum[i] = 0;
        slotXCoords[i] = 0;
        slotYCoords[i] = 0;
    }

    numOfSmallEnemySlots = 0;
    numOfLargeEnemySlots = 0;
    numOfFiendSlots = 0;
    numOfChaosSlots = 0;
    numOfSmallEnemySlotsAvailable = 9;
    numOfLargeEnemySlotsAvailable = 4;
    numOfFiendSlotsAvailable = 1;
    numOfChaosSlotsAvailable = 1;
    if(!rng_init) {
        //Set the rng seed to whatever the seconds value is in the RTC.
        Uint8 *timePtr = (Uint8*) rtc_offset;
        battle_rng_seed = timePtr[0];
        rng_init = 1;
    }

    parseBattleData(&battle, id); 

   // initPalette(&pal_0, battle.palette_id_0);
    //initPalette(&pal_1, battle.palette_id_1);

    //Determine what enemy frames to use.
    int pic = 0;
	for(int co = 1, co2 = 128; co <= 128; co <<= 1, co2 >>= 1) {
		if(battle.pic_assignment & co2) {
            pic |= co;
        } 
    }
       for(int i = 3; i >= 0; i--, pic >>= 2) {
		picassignment[i] = pic & 3;
    }

    //Determine which palettes to use.
    int pal = battle.palette_assignment;
    for(int j = 3; j >= 0; j--, pal >>= 1) {
		palassignment[j] = (pal & 1) == 1;
    }

    if(battle.isFormation2) {
        enemy0_num = rand_min_max(battle.min_max_en0_b >> 4, battle.min_max_en0_b & 0x0F);
        enemy1_num = rand_min_max(battle.min_max_en1_b >> 4, battle.min_max_en1_b & 0x0F);
        parseBattleGroup(enemy0_num, picassignment[0], palassignment[0], battle.enemy_id_0, 0);
        parseBattleGroup(enemy1_num, picassignment[1], palassignment[1], battle.enemy_id_1, enemy0_num);
        
        parseEnemyName(&enemy_name_0, battle.enemy_id_0);
        parseEnemyName(&enemy_name_1, battle.enemy_id_1);

    } else {
        enemy0_num = rand_min_max(battle.min_max_en0_a >> 4, battle.min_max_en0_a & 0x0F);
        enemy1_num = rand_min_max(battle.min_max_en1_a >> 4, battle.min_max_en1_a & 0x0F);
        enemy2_num = rand_min_max(battle.min_max_en2_a >> 4, battle.min_max_en2_a & 0x0F);
        enemy3_num = rand_min_max(battle.min_max_en3_a >> 4, battle.min_max_en3_a & 0x0F);
        parseBattleGroup(enemy0_num, picassignment[0], palassignment[0], battle.enemy_id_0, 0);
        parseBattleGroup(enemy1_num, picassignment[1], palassignment[1], battle.enemy_id_1, enemy0_num);
        parseBattleGroup(enemy2_num, picassignment[2], palassignment[2], battle.enemy_id_2, enemy0_num + enemy1_num);
        parseBattleGroup(enemy3_num, picassignment[3], palassignment[3], battle.enemy_id_3, enemy0_num + enemy1_num + enemy2_num);

        parseEnemyName(&enemy_name_0, battle.enemy_id_0);
        parseEnemyName(&enemy_name_1, battle.enemy_id_1);
        parseEnemyName(&enemy_name_2, battle.enemy_id_2);
        parseEnemyName(&enemy_name_3, battle.enemy_id_3);
    } 

    determineTotalEnemies();
    enemiesRemaining = totalEnemies;

    Uint8 *patternData = ENEMY_SPRITE_PATTERN_DATA_OFFSET + ((battle.pattern_selection << 11) + 0x120);
    initEnemyPatternData(patternData, battle.palette_id_0, battle.palette_id_1);
}

void parseBattleGroup(int numOfEnemies, int picNum, int palNum, int enemyId, int seed) {

    int isLarge = FALSE;
    if(picNum > 1) {
        isLarge = TRUE;
    }

    for(int i = seed; i < numOfEnemies + seed; i++) {
        if(isLarge) {
            if(numOfLargeEnemySlotsAvailable > 0) {
                parseEnemyData(&enemies[i], enemyId, picNum, palNum);
                numOfLargeEnemySlotsAvailable--;
            }
        } else {
            if(numOfSmallEnemySlotsAvailable > 0) {
                parseEnemyData(&enemies[i], enemyId, picNum, palNum);
                numOfSmallEnemySlotsAvailable--;
            }
        }
    }

}

void enemy_init(ENEMY_DATA *enemy) {
    enemy->experience = 0;
    enemy->money_reward = 0;
    enemy->hp = 0;
    enemy->morale = 0;
    enemy->ai_id = 0;
    enemy->evade = 0;
    enemy->absorb = 0;
    enemy->hit_num = 0;
    enemy->hit_rate = 0;
    enemy->strength = 0;
    enemy->crit_rate = 0;
    enemy->unknown = 0;
    enemy->atk_ailment = 0;
    enemy->category = 0;
    enemy->mag_def = 0;
    enemy->elem_weak = 0;
    enemy->elem_resist = 0;
    enemy->pal_num = 0;
    enemy->pic_num = 0;
    enemy->ailment = 0;
    enemy->name[0] = 0x00;
    enemy->name[1] = 0x00;
    enemy->name[2] = 0x00;
    enemy->name[3] = 0x00;
    enemy->name[4] = 0x00;
    enemy->name[5] = 0x00;
    enemy->name[6] = 0x00;
    enemy->name[7] = 0x00;
}

void parseEnemyData(ENEMY_DATA *enemy, int id, int picNum, int palNum) {

    int enemy_data_index = id * 0x14;
    int enemy_data_offset = ENEMY_DATA_TABLE + enemy_data_index;

    Uint8 *statPtr = (Uint8*) enemy_data_offset;
    BYTE expB1 = statPtr[0];
    BYTE expB2 = statPtr[1];
    Uint8 exp = expB1 + (expB2 <<8);
    BYTE moneyB1 = statPtr[2];
    BYTE moneyB2 = statPtr[3];
    Uint8 money = moneyB1 + (moneyB2 <<8);
    BYTE hpB1 = statPtr[4];
    BYTE hpB2 = statPtr[5];
    Uint8 hp = hpB1 + (hpB2 <<8);

    Uint8 morale = statPtr[6];
    Uint8 ai = statPtr[7];
    Uint8 evade = statPtr[8];
    Uint8 absorb = statPtr[9];
    Uint8 numHits = statPtr[10];
    Uint8 hitRate = statPtr[11];
    Uint8 strength = statPtr[12];
    Uint8 critRate = statPtr[13];
    Uint8 unknown = statPtr[14];
    Uint8 atkAilment = statPtr[15];
    Uint8 category = statPtr[16];
    Uint8 magDef = statPtr[17];
    Uint8 elWeak = statPtr[18];
    Uint8 elResist = statPtr[19];
    
    enemy->experience = exp;
    enemy->money_reward = money;
    enemy->hp = hp;
    enemy->morale = morale;
    enemy->ai_id = ai;
    enemy->evade = evade;
    enemy->absorb = absorb;
    enemy->hit_num = numHits;
    enemy->hit_rate = hitRate;
    enemy->strength = strength;
    enemy->crit_rate = critRate;
    enemy->unknown = unknown;
    enemy->atk_ailment = atkAilment;
    enemy->category = category;
    enemy->mag_def = magDef;
    enemy->elem_weak = elWeak;
    enemy->elem_resist = elResist;
    enemy->pal_num = palNum;
    enemy->pic_num = picNum;
    enemy->ailment = 0;

    char enemyName[8];

    parseEnemyName(&enemyName, id);

    enemy->name[0] = enemyName[0];
    enemy->name[1] = enemyName[1];
    enemy->name[2] = enemyName[2];
    enemy->name[3] = enemyName[3];
    enemy->name[4] = enemyName[4];
    enemy->name[5] = enemyName[5];
    enemy->name[6] = enemyName[6];
    enemy->name[7] = enemyName[7];
}

void parseEnemyName(char *enemy, int id) {
    
    int lut_index = id * 2;
    int enemy_lut_offset = ENEMY_NAME_LOOKUP_TABLE;

    Uint8 *enemy_lut_ptr = (Uint8*) enemy_lut_offset;

    Uint8 b1 = enemy_lut_ptr[lut_index];
    lut_index++;
    Uint8 b2 = enemy_lut_ptr[lut_index];

    int name_offset_val_1 = b1;
    int name_offset_val_2 = b2 << 8;

    int name_offset = name_offset_val_1 + name_offset_val_2 + ENEMY_NAME_PTR_ADD;
    Uint8 *enemy_name_ptr = (Uint8*) name_offset;

    int i = 0;
    Uint8 current_char = enemy_name_ptr[i];
    
    while(current_char != 0x00) {
        enemy[i] = current_char;
        i++;
        current_char = enemy_name_ptr[i];
    }

    for(;i < 8; i++) {
        enemy[i] = 0x00;
    }

}

void parseBattleData(BATTLE_DATA *battle, int id) {
    if(id >= 0x80) {
        battle->isFormation2 = TRUE;
        id = id - 0x80;
    } else {
        battle->isFormation2 = FALSE;

    }

    int lut_index = id * 0x10;
    int lut_offset = BATTLE_DATA_TABLE + lut_index;

    Uint8 *battle_ptr = (Uint8*) lut_offset;
    Uint8 b1 = battle_ptr[0];

    Uint8 battle_type = b1 >> 4;
    Uint8 pattern_selection = b1 & 0x0F;

    battle->battle_type = battle_type;
    battle->pattern_selection = pattern_selection;
    //This is used for determining what picture in the NES Pattern table to use for the enemy.
    //So you could technically have an enemy type using the wrong sprite.
    battle->pic_assignment = battle_ptr[1];

    battle->enemy_id_0 = battle_ptr[2];
    battle->enemy_id_1 = battle_ptr[3];
    battle->enemy_id_2 = battle_ptr[4];
    battle->enemy_id_3 = battle_ptr[5];
    battle->min_max_en0_a = battle_ptr[6];
    battle->min_max_en1_a = battle_ptr[7];
    battle->min_max_en2_a = battle_ptr[8];
    battle->min_max_en3_a = battle_ptr[9];
    
    battle->palette_id_0 = battle_ptr[10];
    battle->palette_id_1 = battle_ptr[11];

    battle->surprise_rate = battle_ptr[12];
    Uint8 b13 = battle_ptr[13];
    Uint8 palette_assignment = b13 >> 4;
    Uint8 runFlag = b13 & 0x01;

    battle->palette_assignment = palette_assignment;
    battle->no_run = runFlag;
    battle->min_max_en0_b = battle_ptr[14];
    battle->min_max_en1_b = battle_ptr[15];

}

void enemy_bat_draw() {

    int i = 0;

    Uint8 startPosY = 10;
    Uint8 startPosX = 2;
    Uint8 sizeToSkip = 0;
    Uint8 slotNum = 0;

    if(battle.battle_type == 0) {
    
        /**
         * Battle Arrangement is like this:
         * 
         * SMALL2    SMALL5    SMALL8
         * SMALL1    SMALL4    SMALL7
         * SMALL3    SMALL6    SMALL9
         * 
         **/
        
        for(i = 0; i < totalEnemies; i++) {
            
            drawEnemy(picassignment[enemyNums[i]], palassignment[enemyNums[i]], startPosX, startPosY);
            
            slotNum = battle_formation_0_order[i];
            slotToEnemyNum[slotNum] = i;
            if(enemies[i].hp > 0) {
                slotOccupied[slotNum] = TRUE;
                slotXCoords[slotNum] = startPosX;
                slotYCoords[slotNum] = startPosY;
            }  else {
                slotOccupied[slotNum] = FALSE;
                Uint8 enemy_num_to_check = enemyNums[slotNum];
                if(enemy_num_to_check == 0) {
                    enemy0_num--;
                } else if(enemy_num_to_check == 1) {
                    enemy1_num--;
                 }else if(enemy_num_to_check == 2) {
                    enemy2_num--;
                } else if(enemy_num_to_check == 3) {
                    enemy3_num--;
                }
            }

            if(i == 1 || i == 4) {
                startPosY += 8;
            } else if( i == 2 ||  i == 5) {
                startPosX += 4;
                startPosY -= 4;
            } else {
                startPosY -= 4;
            }

        }
        
    }

    if(battle.battle_type == 1)  {

         /**
         * Battle Arrangement is like this:
         * 
         * LARGE1   LARGE3
         *     
         * LARGE2   LARGE4
         * 
         **/
         startPosY = 10;
         for(i = 0; i < totalEnemies; i++) {
            
            drawEnemy(picassignment[enemyNums[i]], palassignment[enemyNums[i]], startPosX, startPosY);


            slotNum = battle_formation_1_order[i];
            slotToEnemyNum[slotNum] = i;
            if(enemies[i].hp > 0) {
                slotOccupied[slotNum] = TRUE;
                slotXCoords[slotNum] = startPosX;
                slotYCoords[slotNum] = startPosY;
            } else {
                slotOccupied[slotNum] = FALSE;
                Uint8 enemy_num_to_check = enemyNums[slotNum];
                if(enemy_num_to_check == 0) {
                    enemy0_num--;
                } else if(enemy_num_to_check == 1) {
                    enemy1_num--;
                 }else if(enemy_num_to_check == 2) {
                    enemy2_num--;
                } else if(enemy_num_to_check == 3) {
                    enemy3_num--;
                }
            }
            
            if(i == 2) {
                startPosX += 6;
                startPosY -= 12;
            } else {
                startPosY += 6;
            }

        }
    }

    if(battle.battle_type == 2)  {
        
        /**
         * Battle Arrangement is like this:
         * 
         * LARGE1    SMALL2    SMALL5
         *           SMALL1    SMALL4
         * LARGE2    SMALL3    SMALL6
         * 
         **/


        Uint8 smallEnemyNums[9];
        Uint8 largeEnemyNums[9];
        int largeIndex = 0;
        int smallIndex = 0;
        Uint8 current_slot = 0;
        for(int j = 0; j < 9; j++) {
                if(picassignment[enemyNums[j]] > 1) {
                    largeEnemyNums[largeIndex] = enemyNums[j];
                    largeIndex++;
                } else {
                    smallEnemyNums[smallIndex] = enemyNums[j];
                    smallIndex++;
                }
        }

        startPosY = 10;
        for(i = 0; i < totalLargeEnemies; i++) {

            
            drawEnemy(picassignment[largeEnemyNums[i]], palassignment[largeEnemyNums[i]], startPosX, startPosY);

            slotNum = battle_formation_2_order[current_slot];
            
            slotToEnemyNum[slotNum] = current_slot;
            if(enemies[i].hp > 0) {
                slotOccupied[slotNum] = TRUE;
                slotXCoords[slotNum] = startPosX;
                slotYCoords[slotNum] = startPosY;
            } else {
                slotOccupied[slotNum] = FALSE;
                Uint8 enemy_num_to_check = enemyNums[slotNum];
                if(enemy_num_to_check == 0) {
                    enemy0_num--;
                } else if(enemy_num_to_check == 1) {
                    enemy1_num--;
                 }else if(enemy_num_to_check == 2) {
                    enemy2_num--;
                } else if(enemy_num_to_check == 3) {
                    enemy3_num--;
                }
            }
            
            if(i == 2) {
                startPosX += 6;
                startPosY -= 12;
            } else {
                startPosY += 6;
            }

            current_slot++;
        }
        
        startPosY = 10;
        startPosX = 8;


        for(i = 0; i < totalSmallEnemies; i++) {
            drawEnemy(picassignment[smallEnemyNums[i]], palassignment[smallEnemyNums[i]], startPosX, startPosY);

          
            slotNum = battle_formation_2_order[current_slot];
            slotToEnemyNum[slotNum] = current_slot;
            if(enemies[i].hp > 0) {
                slotOccupied[slotNum] = TRUE;
                slotXCoords[slotNum] = startPosX;
                slotYCoords[slotNum] = startPosY;
            } else {
                slotOccupied[slotNum] = FALSE;
                Uint8 enemy_num_to_check = enemyNums[slotNum];
                if(enemy_num_to_check == 0) {
                    enemy0_num--;
                } else if(enemy_num_to_check == 1) {
                    enemy1_num--;
                 }else if(enemy_num_to_check == 2) {
                    enemy2_num--;
                } else if(enemy_num_to_check == 3) {
                    enemy3_num--;
                }
            }

            current_slot++;
            
           
            if(i == 1 || i == 4) {
                startPosY += 8;
            } else if( i == 2 ||  i == 5) {
                startPosX += 4;
                startPosY -= 4;
            } else {
                startPosY -= 4;
            }

        }
    }

    if(battle.battle_type == 3) {
        numOfFiendSlots = 1;
    }
    if(battle.battle_type == 4) {
        numOfChaosSlots = 1;
    }

    
}

Uint8 rand_min_max(int min, int max) {

    Uint8 range = (max - min) + 1;
    Uint8 random = battleRNG();
    Uint16 randproduct = random * range;

    Uint8 tmp = randproduct / 256;

    return tmp + min;
}

Uint8 battleRNG() {
    battle_rng_seed++;
    Uint8 *batRngPtr = (Uint8*)BATTLE_RNG_TABLE;
    Uint8 random = batRngPtr[battle_rng_seed];

    return random;
}

void setRngSeed(void) {
    //The original game relied on the randomness of NES RAM on 
    //Power On. The Saturn clears it's RAM on start up, so to
    //try and mimick the NES behavior I'm getting the 
    //seconds value from the RTC.
    PER_LInit(PER_KD_PERTIM,2,PER_SIZE_DGT,PadWorkArea,0);
    
    rtc_offset =  PER_GET_TIM();
}

void drawEnemyNames() {
    Uint8 startYText = 20;

    if(enemy0_num > 0) {
        drawTextLine(2, startYText, enemy_name_0, BATTLE_MENU_PALETTE, 1);
        startYText +=2;
    }
    if(enemy1_num > 0) {
        drawTextLine(2, startYText, enemy_name_1, BATTLE_MENU_PALETTE, 1);
        startYText +=2;
    } else {
        startYText++;
    }

    if(enemy2_num > 0) {
        drawTextLine(2, startYText, enemy_name_2, BATTLE_MENU_PALETTE, 1);
        startYText +=2;
    } else {
        startYText++;
    }
    if(enemy3_num > 0) {
        drawTextLine(2, startYText, enemy_name_3, BATTLE_MENU_PALETTE, 1);
        startYText +=2;
    } else {
        startYText++;
    }
}

void drawInitMessage() {
    
    Uint8 *battleBox_height = BATTLE_MAIN_BOX_HEIGHT;

    if (isSurprised) {
        drawBox(info_box_luts[BOTTOM_MESSAGE_INDEX].xPos - 1, info_box_luts[BOTTOM_MESSAGE_INDEX].yPos + *battleBox_height, info_box_luts[BOTTOM_MESSAGE_INDEX].width, info_box_luts[BOTTOM_MESSAGE_INDEX].height, BATTLE_MENU_PALETTE);
        drawBattleMsg(info_box_luts[BOTTOM_MESSAGE_INDEX].text_xPos - 1, info_box_luts[BOTTOM_MESSAGE_INDEX].text_yPos + *battleBox_height, BTLMSG_SURPRISED, BATTLE_MENU_PALETTE);
    } else if (isPreemptive) {
        drawBox(info_box_luts[BOTTOM_MESSAGE_INDEX].xPos - 1, info_box_luts[BOTTOM_MESSAGE_INDEX].yPos  + *battleBox_height, info_box_luts[BOTTOM_MESSAGE_INDEX].width, info_box_luts[BOTTOM_MESSAGE_INDEX].height, BATTLE_MENU_PALETTE);
        drawBattleMsg(info_box_luts[BOTTOM_MESSAGE_INDEX].text_xPos - 1, info_box_luts[BOTTOM_MESSAGE_INDEX].text_yPos + *battleBox_height, BTLMSG_STRIKEFIRST, BATTLE_MENU_PALETTE);
    }
}

void drawAttackerDefenderMessage(char * attacker, char * defender) {
    
    Uint8 *battleBox_height = BATTLE_MAIN_BOX_HEIGHT;
    drawBox(info_box_luts[ATTACKER_MESSAGE_INDEX].xPos - 1, info_box_luts[ATTACKER_MESSAGE_INDEX].yPos + *battleBox_height, info_box_luts[ATTACKER_MESSAGE_INDEX].width, info_box_luts[ATTACKER_MESSAGE_INDEX].height, BATTLE_MENU_PALETTE);
    drawTextLine(info_box_luts[ATTACKER_MESSAGE_INDEX].text_xPos - 1, info_box_luts[ATTACKER_MESSAGE_INDEX].text_yPos + *battleBox_height, attacker, BATTLE_MENU_PALETTE, 1);

    drawBox(info_box_luts[DEFENDER_MESSAGE_INDEX].xPos - 1, info_box_luts[DEFENDER_MESSAGE_INDEX].yPos + *battleBox_height, info_box_luts[DEFENDER_MESSAGE_INDEX].width, info_box_luts[DEFENDER_MESSAGE_INDEX].height, BATTLE_MENU_PALETTE);
    drawTextLine(info_box_luts[DEFENDER_MESSAGE_INDEX].text_xPos - 1, info_box_luts[DEFENDER_MESSAGE_INDEX].text_yPos + *battleBox_height, defender, BATTLE_MENU_PALETTE, 1);

}

void drawMonstersPerishedMessage() {
    Uint8 *battleBox_height = BATTLE_MAIN_BOX_HEIGHT;
    drawBox(info_box_luts[BOTTOM_MESSAGE_INDEX].xPos - 1, info_box_luts[BOTTOM_MESSAGE_INDEX].yPos  + *battleBox_height, info_box_luts[BOTTOM_MESSAGE_INDEX].width, info_box_luts[BOTTOM_MESSAGE_INDEX].height, BATTLE_MENU_PALETTE);
    drawMonstersPerished(info_box_luts[BOTTOM_MESSAGE_INDEX].text_xPos - 1, info_box_luts[BOTTOM_MESSAGE_INDEX].text_yPos + *battleBox_height, BATTLE_MENU_PALETTE);
}

void drawAttemptRunMessage(char * attacker) {

    Uint8 *battleBox_height = BATTLE_MAIN_BOX_HEIGHT;
    drawBox(info_box_luts[ATTACKER_MESSAGE_INDEX].xPos - 1, info_box_luts[ATTACKER_MESSAGE_INDEX].yPos + *battleBox_height, info_box_luts[ATTACKER_MESSAGE_INDEX].width, info_box_luts[ATTACKER_MESSAGE_INDEX].height, BATTLE_MENU_PALETTE);
    drawTextLine(info_box_luts[ATTACKER_MESSAGE_INDEX].text_xPos - 1, info_box_luts[ATTACKER_MESSAGE_INDEX].text_yPos + *battleBox_height, attacker, BATTLE_MENU_PALETTE, 1);

}

void drawRunSuccessMessage() {
    Uint8 *battleBox_height = BATTLE_MAIN_BOX_HEIGHT;
    drawBox(info_box_luts[BOTTOM_MESSAGE_INDEX].xPos - 1, info_box_luts[BOTTOM_MESSAGE_INDEX].yPos + *battleBox_height, info_box_luts[BOTTOM_MESSAGE_INDEX].width, info_box_luts[BOTTOM_MESSAGE_INDEX].height, BATTLE_MENU_PALETTE);
    drawBattleMsg(info_box_luts[BOTTOM_MESSAGE_INDEX].text_xPos - 1, info_box_luts[BOTTOM_MESSAGE_INDEX].text_yPos + *battleBox_height, BTLMSG_CLOSECALL, BATTLE_MENU_PALETTE);

}

void drawLutBottomdMessage(Uint8 msg_indx) {
    Uint8 *battleBox_height = BATTLE_MAIN_BOX_HEIGHT;
    drawBox(info_box_luts[BOTTOM_MESSAGE_INDEX].xPos - 1, info_box_luts[BOTTOM_MESSAGE_INDEX].yPos + *battleBox_height, info_box_luts[BOTTOM_MESSAGE_INDEX].width, info_box_luts[BOTTOM_MESSAGE_INDEX].height, BATTLE_MENU_PALETTE);
    drawBattleMsg(info_box_luts[BOTTOM_MESSAGE_INDEX].text_xPos - 1, info_box_luts[BOTTOM_MESSAGE_INDEX].text_yPos + *battleBox_height, msg_indx, BATTLE_MENU_PALETTE);

}

void drawLutAttackMessage(Uint8 msg_index){
    Uint8 *battleBox_height = BATTLE_MAIN_BOX_HEIGHT;
    drawBox(info_box_luts[ATTACK_MESSAGE_INDEX].xPos - 1, info_box_luts[ATTACK_MESSAGE_INDEX].yPos + *battleBox_height, info_box_luts[ATTACK_MESSAGE_INDEX].width, info_box_luts[ATTACK_MESSAGE_INDEX].height, BATTLE_MENU_PALETTE);
    drawBattleMsg(info_box_luts[ATTACK_MESSAGE_INDEX].text_xPos - 1, info_box_luts[ATTACK_MESSAGE_INDEX].text_yPos + *battleBox_height, msg_index, BATTLE_MENU_PALETTE);

}

void drawLutResultMessage(Uint8 msg_index){
    Uint8 *battleBox_height = BATTLE_MAIN_BOX_HEIGHT;
    drawBox(info_box_luts[DAMAGE_MESSAGE_INDEX].xPos - 1, info_box_luts[DAMAGE_MESSAGE_INDEX].yPos + *battleBox_height, info_box_luts[DAMAGE_MESSAGE_INDEX].width, info_box_luts[DAMAGE_MESSAGE_INDEX].height, BATTLE_MENU_PALETTE);
    drawBattleMsg(info_box_luts[DAMAGE_MESSAGE_INDEX].text_xPos - 1, info_box_luts[DAMAGE_MESSAGE_INDEX].text_yPos + *battleBox_height, msg_index, BATTLE_MENU_PALETTE);

}

void drawAttackMessage(Uint8 *msg){
    Uint8 *battleBox_height = BATTLE_MAIN_BOX_HEIGHT;
    drawBox(info_box_luts[ATTACK_MESSAGE_INDEX].xPos - 1, info_box_luts[ATTACK_MESSAGE_INDEX].yPos + *battleBox_height, info_box_luts[ATTACK_MESSAGE_INDEX].width, info_box_luts[ATTACK_MESSAGE_INDEX].height, BATTLE_MENU_PALETTE);
    drawTextLine(info_box_luts[ATTACK_MESSAGE_INDEX].text_xPos - 1, info_box_luts[ATTACK_MESSAGE_INDEX].text_yPos + *battleBox_height, msg, BATTLE_MENU_PALETTE, 1);

}

void drawResultMessage(Uint8 *msg) {
    Uint8 *battleBox_height = BATTLE_MAIN_BOX_HEIGHT;
    drawBox(info_box_luts[DAMAGE_MESSAGE_INDEX].xPos - 1, info_box_luts[DAMAGE_MESSAGE_INDEX].yPos + *battleBox_height, info_box_luts[DAMAGE_MESSAGE_INDEX].width, info_box_luts[DAMAGE_MESSAGE_INDEX].height, BATTLE_MENU_PALETTE);
    drawTextLine(info_box_luts[DAMAGE_MESSAGE_INDEX].text_xPos - 1, info_box_luts[DAMAGE_MESSAGE_INDEX].text_yPos + *battleBox_height, msg, BATTLE_MENU_PALETTE, 1);

}


void clearMessageArea(){
    Uint8 *battleBox_xy = BATTLE_MAIN_BOX_XY;
    Uint8 *battleBox_width = BATTLE_MAIN_BOX_WIDTH;
    Uint8 *battleBox_height = BATTLE_MAIN_BOX_HEIGHT;
    Uint8 *partyBox_width = BATTLE_CHAR_BOX_WIDTH;

    clearArea(*battleBox_xy - 1, *battleBox_xy + *battleBox_height, *battleBox_width + *partyBox_width, 20);

    mainBattleBoxBottoms();
}

void drawMenu() {
    
    Uint8 *battleBox_xy = BATTLE_MAIN_BOX_XY;
    Uint8 *battleBox_height = BATTLE_MAIN_BOX_HEIGHT;

    BATTLE_BOX *enemyRosterBox = ENEMY_ROSTER_BOX_LUT;
    COMMAND_BOX *commandBox = COMMAND_BOX_LUT;

    drawBox(enemyRosterBox[0].xPos - 1, enemyRosterBox[0].yPos + *battleBox_height, enemyRosterBox[0].width, enemyRosterBox[0].height, BATTLE_MENU_PALETTE);

    drawBox(commandBox[0].xPos - 1, commandBox[0].yPos + *battleBox_height, commandBox[0].width, commandBox[0].height, BATTLE_MENU_PALETTE);

    for(Uint8 i = 0; i < 5; i++) {

        Uint16 offset = commandBox[0].text[i].ptr_high << 8;
        offset += commandBox[0].text[i].ptr_low + 0x10;
        Uint32 text_address = BATTLE_BANK_OFFSET + offset;

        Uint8 *text_ptr = text_address;
        drawTextLine(commandBox[0].text[i].xPos - 1, commandBox[0].text[i].yPos + *battleBox_height + 1, text_ptr, BATTLE_MENU_PALETTE, 1);
    }

}

void drawMagicMenu(Uint8 playerIndex, Uint8 pageNum) {
    
    BATTLE_BOX *magicMenuBox = ITEM_AND_MAGIC_BOX_LUT;
    Uint8 *battleBox_height = BATTLE_MAIN_BOX_HEIGHT;

    drawBox(magicMenuBox[0].xPos - 1, magicMenuBox[0].yPos + *battleBox_height, magicMenuBox[0].width, magicMenuBox[0].height, BATTLE_MENU_PALETTE);

    Uint8 magicLevelStr[3] = {0x95, 0x81, 0x00} ;
    Uint8 mpLevelStr[2] = {0x80, 0x00} ;
    if(pageNum == 1) {
        magicLevelStr[1] = 0x85;
    }

    for(Uint8 i = 0; i < 4; i++) {
        Uint8 *text_ptr = magicLevelStr;
        drawTextLine(magicMenuBox[0].xPos, magicMenuBox[0].yPos + *battleBox_height + ((i + 1) * 2), text_ptr, BATTLE_MENU_PALETTE, 1);
        Uint8 Xpos = magicMenuBox[0].xPos + magicMenuBox[0].width - 4;
        Uint8 magicLvl = i;
        if(pageNum == 1) {
            magicLvl = 4;
        }
        mpLevelStr[0] = battle_char_stats[playerIndex].current_mp[magicLvl] + 0x80;
        Uint8 *mp_text_ptr = mpLevelStr;
        drawTextLine(Xpos, magicMenuBox[0].yPos + *battleBox_height + ((i + 1) * 2), mp_text_ptr, BATTLE_MENU_PALETTE, 1);
        magicLevelStr[1]++;
    }

}

void determineTotalEnemies() {
    switch(battle.battle_type) {
        case 0:
            numOfSmallEnemySlots = 9;
            totalEnemies = numOfSmallEnemySlots - numOfSmallEnemySlotsAvailable;
            break;
        case 1:
            numOfLargeEnemySlots = 4;
            totalEnemies = numOfLargeEnemySlots - numOfLargeEnemySlotsAvailable;
            break; 
        case 2:
            numOfSmallEnemySlots = 6;
            numOfLargeEnemySlots = 2;
            totalLargeEnemies = numOfLargeEnemySlots - (numOfLargeEnemySlotsAvailable - 2);
            totalSmallEnemies = numOfSmallEnemySlots - (numOfSmallEnemySlotsAvailable - 3);
            totalEnemies = totalLargeEnemies + totalSmallEnemies;
        default:
            break;
    }
}

Uint8 getNextOccupiedSlot(Sint8 start) {
    for(Sint8 i = start; i < 9; i++) {
        if(slotOccupied[i] == TRUE) {
            return i;
        }
    }
    if(enemiesRemaining > 0) {
        for(Sint8 i = 0; i < 9; i++) {
            if(slotOccupied[i] == TRUE) {
                return i;
            }
        }
    }
}

Uint8 getPrevOccupiedSlot(Sint8 start) {
    for(Sint8 i = start; i >= 0; i--) {
        if(slotOccupied[i] == TRUE) {
            return i;
        }
    }
    if(enemiesRemaining > 0) {
        for(Sint8 i = 8; i >= 0; i--) {
            if(slotOccupied[i] == TRUE) {
                return i;
            }
        }
    }
}


Uint16 getXSlotCoord(Uint8 index) {
    return (slotXCoords[index] + 1) * 8;
}

Uint16 getYSlotCoord(Uint8 index) {
    return slotYCoords[index] * 8;
}

void updatePlayerBattleAction(Uint8 partyNum, Uint8 targetNum, Uint8 action, Uint8 subAction, Uint8 isPlayerTarget) {

    Uint8 target = 0;
    if(action == RUN) {
        target = 0;
        isPlayerTarget = FALSE;
        subAction = 0;
    } else {
        if(!isPlayerTarget) {
            switch(battle.battle_type) {
                case 0:
                    target = battle_formation_0_order[targetNum];
                    break;
                case 1:
                    target = battle_formation_1_order[targetNum];
                    break;
                case 2:
                    target = battle_formation_2_order[targetNum];
                    break;
                default:
                    break;
            }
        } else {
            target = targetNum;
        }
    }

    playerActions[partyNum].turnOwnerIndex = partyNum;
    playerActions[partyNum].isPlayer = TRUE;
    playerActions[partyNum].targetIndex = target;
    playerActions[partyNum].targetIsPlayer = isPlayerTarget;
    playerActions[partyNum].action = action;
    playerActions[partyNum].secondaryActionId = subAction;

}

 void determineBattleOrder() {

    Uint8 *batOrderPtr = (Uint8*) BATTLE_ORDER_DEFAULT_LOCATION;

    battleOrderBuffer[0] = batOrderPtr[0];
    battleOrderBuffer[1] = batOrderPtr[1];
    battleOrderBuffer[2] = batOrderPtr[2];
    battleOrderBuffer[3] = batOrderPtr[3];
    battleOrderBuffer[4] = batOrderPtr[4];
    battleOrderBuffer[5] = batOrderPtr[5];
    battleOrderBuffer[6] = batOrderPtr[6];
    battleOrderBuffer[7] = batOrderPtr[7];
    battleOrderBuffer[8] = batOrderPtr[8];
    battleOrderBuffer[9] = batOrderPtr[9];
    battleOrderBuffer[10] = batOrderPtr[10];
    battleOrderBuffer[11] = batOrderPtr[11];
    battleOrderBuffer[12] = batOrderPtr[12];


    Uint8 swapIndexA = 0;
    Uint8 swapIndexB = 0;

    Uint8 swapValA = 0;
    Uint8 swapValB = 0;

    for(Uint8 i = 0; i < 16; i++) {
        swapIndexA = rand_min_max(0, 12);
        swapIndexB = rand_min_max(0, 12);

        swapValA = battleOrderBuffer[swapIndexA];
        swapValB = battleOrderBuffer[swapIndexB];

        battleOrderBuffer[swapIndexA] = swapValB;
        battleOrderBuffer[swapIndexB] = swapValA;

    }
}

void determineWeaponMagicPalette(Uint32 *palette, Uint8 pal_id, Uint8 isMagic) {

    palette[0] =  nes_pal[0x0F];
    for (Uint8 i = 1; i < 4; i++) {
         palette[i] =  nes_pal[pal_id];
         pal_id -= 0x10;
    }

    if(isMagic) {
        palette[3] = nes_pal[0x30];
    }

}

Uint16 calculateBasePhysicalDamage(Uint8 attackerIndex) {
    
    Uint8 dmg = 0;

    if(attackerIndex > 8) {
        dmg = battle_char_stats[attackerIndex & 0x7F].damage;
    } else {
        dmg = enemies[attackerIndex].strength;
    }

    Uint16 rndDmg = rand_min_max(dmg, dmg * 2);
   
    return rndDmg;
    
}

void takePlayersTurn(Uint8 playerIndex) {
    curr_attack_dmg = 0;
    Uint8 targetIndex = playerActions[playerIndex].targetIndex;
    Uint16 baseDmg = calculateBasePhysicalDamage(playerIndex + 0x80);
    Uint8 playerHitPercent = battle_char_stats[playerIndex].hit_percent;
    Uint8 targetEvade = enemies[targetIndex].evade;
    Uint16 chanceToHit = (168 + playerHitPercent) - targetEvade;

    Uint8 randZeroTo200 = rand_min_max(0, 200);

    if(randZeroTo200 <= chanceToHit) {
        curr_attack_miss = FALSE;
        //Hit
        Uint8 weaponCritRate =  battle_char_stats[playerIndex].crit_rate;
            if(randZeroTo200 <= weaponCritRate) {
                baseDmg = baseDmg * 2;
            }
        
        Uint8 targetAbsorb = enemies[targetIndex].absorb;
        if(targetAbsorb >= baseDmg) {
            baseDmg = 1;
        } else {
            baseDmg -= targetAbsorb;
        }
        curr_attack_dmg = baseDmg;
    } else{
        curr_attack_miss = TRUE;
    }

}


void takeEnemyTurn(Uint8 enemyIndex) {
    curr_attack_dmg = 0;
    Uint16 baseDmg = calculateBasePhysicalDamage(enemyIndex);
    Uint8 enemyHitPercent = enemies[enemyIndex].hit_rate;
    Uint8 targetEvade = battle_char_stats[curr_enemy_target].evade_percent;
    Uint16 chanceToHit = (168 + enemyHitPercent) - targetEvade;

    Uint8 randZeroTo200 = rand_min_max(0, 200);

    if(randZeroTo200 <= chanceToHit) {
        curr_attack_miss = FALSE;
        //Hit
        Uint8 critRate =  enemies[enemyIndex].crit_rate;
            if(randZeroTo200 <= critRate) {
                baseDmg = baseDmg * 2;
            }
        
        Uint8 targetAbsorb = battle_char_stats[curr_enemy_target].absorb;
        if(targetAbsorb >= baseDmg) {
            baseDmg = 1;
        } else {
            baseDmg -= targetAbsorb;
        }
        curr_attack_dmg = baseDmg;
    } else{
        curr_attack_miss = TRUE;
    }

}

Uint8 determinePlayerTarget() {
    Uint8 rand = battleRNG();
    Uint8 playerIndex = 0;
    Uint8 playerDetermined = FALSE;
    while(!playerDetermined) {
        
        Uint8 rand = battleRNG();
        if(rand < 0x20) {
            playerIndex = 3;
        } else if(rand < 0x40 && rand >= 0x20) {
            playerIndex = 2;
        } else if(rand < 0x60 && rand >= 0x40) {
            playerIndex = 1;
        } else if(rand >= 0x80) {
            playerIndex = 0;
        }
        if(!isPlayerDead(playerIndex)) {
            playerDetermined = TRUE;
        }
    }

    return playerIndex;

}

Uint8 isPlayerDead(Uint8 index) {
    Uint16 hp = battle_char_stats[index].hp;

    if(hp == 0) {
        return TRUE;
    }
    return FALSE;
}

Uint8 isPlayerLowHealth(Uint8 index, Uint16 maxHp) {
    Uint16 hp = battle_char_stats[index].hp;

    Uint16 qHp = maxHp / 4;

    if(hp <= qHp) {
        return TRUE;
    }

    return FALSE;
}

Uint8 allEnemiesDead() {
    for(Uint8 i = 0; i < 9; i++) {
        if(enemies[i].hp > 0) {
            return FALSE;
        }
    }

    return TRUE;
}

Uint8 isPartyDead(){
    for(Uint8 i = 0; i < 4; i++) {
        if(battle_char_stats[i].hp > 0) {
            return FALSE;
        }
    }

    return TRUE;
}

Uint8 canRun(Uint8 index) {
    Uint8 level = battle_char_stats[index].level;
    Uint8 luck = battle_char_stats[index].luck;

    Uint8 randLvl15 = rand_min_max(0, level + 15);

    if(luck > randLvl15) {
        return TRUE;
    }

    return FALSE;
}

void waitForRespondRate() {
   // respondRateCounter = 0;
    //Uint8 respondRate = response_rate_lut[getResponseRate()];
   //  
 //   while(respondRateCounter < respondRate) {
   //     SCL_DisplayFrame();
   // }
}

void drawDmgString(Uint16 damage) {
    Uint8 dmgBuffer[7] = {0x00, 0x00, 0x00, 0X00, 0x00, 0x00, 0x00};
    Uint8 i;
    Uint8 numOfDigits = 3;
    if(damage < 1000) {
        numOfDigits = 2;
    }
    if(damage < 100) {
        numOfDigits = 1;
    }
    
    if(damage < 10) {
        numOfDigits = 0;
    }

    Uint8 right_col = numOfDigits;

    for (i = 0; i <= numOfDigits; i++) {
        dmgBuffer[right_col--] = (damage % 10) + 0x80;
        damage /= 10;
    }

    Uint8 dmgColStart = 0;

    if(numOfDigits == 3) {
        dmgColStart = 4;   
    } else if(numOfDigits == 2) {
        dmgColStart = 3;   
    } else if(numOfDigits == 1) {
        dmgColStart = 2;
    } else {
        dmgColStart = 1;
    }


    Uint16 *battleMsg_lut = BATTLE_MSG_PTR_TABLE;
    
    Uint8 msg_id = BTLMSG_DMG;
    msg_id--;
    Uint16 tmpOffset = battleMsg_lut[msg_id];
    Uint16 btl_msg_offset = (tmpOffset >> 8) | (tmpOffset << 8);
    btl_msg_offset -= 0x8C40;
    Uint8 *dmg_msg_ptr = (Uint8*) BATTLE_MSG_TABLE + btl_msg_offset;

    for(Uint8 i = 0; i < 3; i ++){
        dmgBuffer[i + dmgColStart] = dmg_msg_ptr[i];
    }

    Uint8 *txt_ptr = dmgBuffer;

    drawResultMessage(txt_ptr);
}

void deleteEnemy(Uint8 xPos, Uint8 yPos, Uint8 xy) {

    if(lastDeletedEnemyRow < xy) {
        clearArea(xPos - 1, yPos + lastDeletedEnemyRow, xy, 1);
    }
    else {
        
        respondRateCounter = 0;
        turn_exec_state = FINISH_TURN;
    }
    lastDeletedEnemyRow++;

}

void setLastDeletedEnemyRow(Uint8 lastRow) {
    lastDeletedEnemyRow = lastRow;
}

Uint8 getEnemySlotNum(Uint8 enemy) {
    return battle_formation_0_order[enemy];
}

void shakeScreen() {

    if(frameCount % 2 == 0) {
        Uint8 xVal = rand_min_max(0, 3);
        Uint8 yVal = rand_min_max(0, 3);

        static_move_to(MTH_FIXED(xVal), MTH_FIXED(yVal));
    }

    if(frameCount > 12) {
        static_move_to(MTH_FIXED(0), MTH_FIXED(0));
        turn_exec_state = RESULT;
    }

}
