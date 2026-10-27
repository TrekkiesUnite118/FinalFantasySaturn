#include <string.h>
#include "video.h"
#include "sys.h"
#include "battle.h"
#include "static.h"
#include "print.h"
#include "menu.h"
#include "vblank.h"
#include "magic.h"
#include "player.h"
#include "music.h"
#include "bugfix.h"
#include "memmap.h"
#include "msxgfx.h"

static void msxLoadEnemies(void);

#define UPPER_CASE_LETTER_VALUE (0x8A)
#define UPPER_CASE_ASCII_DIFF (0x49)
#define NUMERICAL_SYMBOL_VALUE (0x80)
#define NUMERICAL_ASCII_DIFF (0x50)
#define LOWER_CASE_LETTER_VALUE (0xA4)
#define LOWER_CASE_ASCII_DIFF (0x43)

#define SMALL_ENEMY_START_Y FIXED(40)
#define SMALL_ENEMY_COL_1 FIXED(40)
#define SMALL_ENEMY_COL_2 FIXED(72)
#define SMALL_ENEMY_COL_3 FIXED(104)

#define LARGE_ENEMY_START_Y FIXED(72)
#define LARGE_ENEMY_COL_1 FIXED(40)
#define LARGE_ENEMY_COL_2 FIXED(88)


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
Uint8 placedEnemies = 0;
Uint8 battle_rng_seed = 0;

Uint8 battle_formation_0_order[9] = {1, 0, 2, 4, 3, 5, 7, 6, 8};
Uint8 battle_formation_1_order[4] = {0, 1, 2, 3};
Uint8 battle_formation_2_order[8] = {0, 1, 3, 2, 4, 6, 5, 7};

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
        enemyNums[i] = 0xFF;
    }
    placedEnemies = 0;

    battleFormation = id;
    parseBattleData(&battle, id);

    numOfSmallEnemySlots = 0;
    numOfLargeEnemySlots = 0;
    numOfFiendSlots = 0;
    numOfChaosSlots = 0;
    numOfFiendSlotsAvailable = 1;
    numOfChaosSlotsAvailable = 1;
    switch(battle.battle_type) {
        case 0:
            numOfSmallEnemySlotsAvailable = 9;
            numOfLargeEnemySlotsAvailable = 0;
            break;
        case 1:
            numOfSmallEnemySlotsAvailable = 0;
            numOfLargeEnemySlotsAvailable = 4;
            break;
        case 2:
            numOfSmallEnemySlotsAvailable = 6;
            numOfLargeEnemySlotsAvailable = 2;
            break;
        default:
            numOfSmallEnemySlotsAvailable = 9;
            numOfLargeEnemySlotsAvailable = 4;
            break;
    }

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

    if(battle.battle_type >= 3) {
        //Fiend/Chaos formations have exactly 1 enemy, the first enemy ID, with no quantity roll.
        parseEnemyData(&enemies[0], battle.enemy_id_0, 0, 0);
        enemyNums[0] = 0;
        placedEnemies = 1;
        enemy0_num = 1;
        parseEnemyName(enemy_name_0, battle.enemy_id_0);
    } else if(battle.isFormation2) {
        enemy0_num = parseBattleGroup(battle.min_max_en0_b, picassignment[0], palassignment[0], battle.enemy_id_0, 0);
        enemy1_num = parseBattleGroup(battle.min_max_en1_b, picassignment[1], palassignment[1], battle.enemy_id_1, 1);
        //B formations still roll groups 2-3, with a quantity of 0.
        parseBattleGroup(0x00, picassignment[2], palassignment[2], 0, 2);
        parseBattleGroup(0x00, picassignment[3], palassignment[3], 0, 3);

        parseEnemyName(enemy_name_0, battle.enemy_id_0);
        parseEnemyName(enemy_name_1, battle.enemy_id_1);

    } else {
        enemy0_num = parseBattleGroup(battle.min_max_en0_a, picassignment[0], palassignment[0], battle.enemy_id_0, 0);
        enemy1_num = parseBattleGroup(battle.min_max_en1_a, picassignment[1], palassignment[1], battle.enemy_id_1, 1);
        enemy2_num = parseBattleGroup(battle.min_max_en2_a, picassignment[2], palassignment[2], battle.enemy_id_2, 2);
        enemy3_num = parseBattleGroup(battle.min_max_en3_a, picassignment[3], palassignment[3], battle.enemy_id_3, 3);

        parseEnemyName(enemy_name_0, battle.enemy_id_0);
        parseEnemyName(enemy_name_1, battle.enemy_id_1);
        parseEnemyName(enemy_name_2, battle.enemy_id_2);
        parseEnemyName(enemy_name_3, battle.enemy_id_3);
    } 

    determineTotalEnemies();
    enemiesRemaining = totalEnemies;

    if(MSX_ON) {
        msxLoadEnemies();
    } else {
        Uint8 *patternData = (Uint8 *)BATTLE_TILE_OFFSET +((battle.pattern_selection << 11) + 0x120);
        initEnemyPatternData(patternData, battle.palette_id_0, battle.palette_id_1);
    }
}

Uint8 parseBattleGroup(Uint8 minMax, int picNum, int palNum, int enemyId, int group) {

    int isLarge = FALSE;
    if(picNum > 1) {
        isLarge = TRUE;
    }

    //Mix formations only roll a group's quantity if a slot of its size is free.
    if(battle.battle_type == 2) {
        if((isLarge && numOfLargeEnemySlotsAvailable == 0) || (!isLarge && numOfSmallEnemySlotsAvailable == 0)) {
            return 0;
        }
    }

    Uint8 numOfEnemies = rand_min_max(minMax >> 4, minMax & 0x0F);
    Uint8 placed = 0;

    for(int i = 0; i < numOfEnemies && placedEnemies < 9; i++) {
        Uint8 index = placedEnemies;
        if(isLarge) {
            if(numOfLargeEnemySlotsAvailable == 0) {
                break;
            }
            //Mix formations put large enemies in 0-1 and small enemies from 2.
            if(battle.battle_type == 2) {
                index = 2 - numOfLargeEnemySlotsAvailable;
            }
            numOfLargeEnemySlotsAvailable--;
        } else {
            if(numOfSmallEnemySlotsAvailable == 0) {
                break;
            }
            if(battle.battle_type == 2) {
                index = 8 - numOfSmallEnemySlotsAvailable;
            }
            numOfSmallEnemySlotsAvailable--;
        }
        parseEnemyData(&enemies[index], enemyId, picNum, palNum);
        enemyNums[index] = group;
        placedEnemies++;
        placed++;
    }

    return placed;
}

void enemy_init(ENEMY_DATA *enemy) {
    enemy->experience = 0;
    enemy->money_reward = 0;
    enemy->hp = 0;
    enemy->hp_max = 0;
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
    enemy->ai_mag_pos = 0;
    enemy->ai_atk_pos = 0;
    enemy->num_hits_mult = 0;
    enemy->rom_absorb = 0;
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
    Uint16 exp = expB1 + (expB2 <<8);
    BYTE moneyB1 = statPtr[2];
    BYTE moneyB2 = statPtr[3];
    Uint16 money = moneyB1 + (moneyB2 <<8);
    BYTE hpB1 = statPtr[4];
    BYTE hpB2 = statPtr[5];
    Uint16 hp = hpB1 + (hpB2 <<8);

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
    enemy->hp_max = hp;
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
    enemy->ai_mag_pos = 0;
    enemy->ai_atk_pos = 0;
    enemy->num_hits_mult = 1;
    enemy->rom_absorb = absorb;

    Uint8 enemyName[8];

    parseEnemyName(enemyName, id);

    enemy->name[0] = enemyName[0];
    enemy->name[1] = enemyName[1];
    enemy->name[2] = enemyName[2];
    enemy->name[3] = enemyName[3];
    enemy->name[4] = enemyName[4];
    enemy->name[5] = enemyName[5];
    enemy->name[6] = enemyName[6];
    enemy->name[7] = enemyName[7];
}

void parseEnemyName(Uint8 *enemy, int id) {
    
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

//Draws NES Fiend/Chaos TSA data at NES tile column nesX, row nesY. The attribute bytes
//that follow the tiles cover NES rows 4-19, columns 0-15.
static void drawFiendChaos(Uint8 *tsa, Uint8 width, Uint8 height, Uint8 nesX, Uint8 nesY) {
    //NES BG palettes 0-3 (backdrop, enemy 0, enemy 1, menu) in port palette numbers.
    static const Uint8 nesToPortPal[4] = {2, 3, 4, 0};
    Uint8 *attr = tsa + (width * height);

    for(Uint8 y = 0; y < height; y++) {
        for(Uint8 x = 0; x < width; x++) {
            Uint8 row = nesY + y;
            Uint8 col = nesX + x;
            Uint8 tile = tsa[(y * width) + x];
            Uint8 attrByte = attr[(((row >> 2) - 1) * 4) + (col >> 2)];
            Uint8 shift = ((row & 2) ? 4 : 0) + ((col & 2) ? 2 : 0);
            Uint8 pal = nesToPortPal[(attrByte >> shift) & 3];

            menuPlane[row][col + CENTER_ADJUSTMENT - 1] = (tile == 0) ? 0 : ((pal << 12) + tile);
        }
    }
}

//MSX enemies use the NES enemy characters, and battle palettes 3-6, one per enemy type.
#define MSX_ENEMY_CHAR_START (18)
#define MSX_ENEMY_CHAR_END (128)
#define MSX_ENEMY_FIRST_PAL (3)
#define MSX_ENEMY_TYPES (4)
#define MSX_BOSS_CELLS (14 * 12)
#define BATTLE_FADE_SHADES (4) //NES FadeOutOneShade steps from full color to black.

static Uint32 *msxEnemyPal[MSX_ENEMY_TYPES] = {menu_pal_3, menu_pal_4, menu_pal_5, menu_pal_6};
static Uint8 msxEnemyTypes;
static Uint8 msxTypeEnemy[MSX_ENEMY_TYPES]; //MSX enemy of each type.
static Uint8 msxTypeChar[MSX_ENEMY_TYPES]; //First character of each type.
static Uint8 msxGroupType[4];
static Uint8 msxBossChars[MSX_BOSS_CELLS]; //Fiend/Chaos character of each cell, 0 for blank.

//Fiends and Chaos have blank and repeated cells, which share characters. Chaos wouldn't fit otherwise.
static void msxLoadBoss(Uint8 msx, Uint16 cells) {
    static const Uint8 blank[32] = {0};
    const Uint8 *src = msxData(MSX_ID_ENEMY_TILES + msx);
    Uint8 *chars = (Uint8 *)LWRAM_MENU_TILES;
    Uint8 next = MSX_ENEMY_CHAR_START;

    for(Uint16 c = 0; c < cells; c++) {
        const Uint8 *tile = src + c * 32;
        Uint8 ch = MSX_ENEMY_CHAR_START;

        msxBossChars[c] = 0;
        if(memcmp(tile, blank, 32) == 0) {
            continue;
        }
        while(ch < next && memcmp(chars + ch * 32, tile, 32) != 0) {
            ch++;
        }
        if(ch == next) {
            if(next == MSX_ENEMY_CHAR_END) {
                continue;
            }
            memcpy(chars + next * 32, tile, 32);
            next++;
        }
        msxBossChars[c] = ch;
    }
}

//Loads each enemy type in the formation, in place of the NES pattern page.
static void msxLoadEnemies() {
    Uint8 ids[4] = {battle.enemy_id_0, battle.enemy_id_1, battle.enemy_id_2, battle.enemy_id_3};
    Uint8 next = MSX_ENEMY_CHAR_START;

    msxEnemyTypes = 0;
    for(Uint8 slot = 0; slot < 9; slot++) {
        Uint8 group = enemyNums[slot];
        if(group > 3) {
            continue;
        }
        Uint8 msx = msxEnemy(ids[group]);
        Uint8 type = 0;
        while(type < msxEnemyTypes && msxTypeEnemy[type] != msx) {
            type++;
        }
        if(type == msxEnemyTypes) {
            Uint8 tiles = picassignment[group] > 1 ? 36 : 16;
            if(battle.battle_type == 3) {
                msxLoadBoss(msx, 8 * 8);
            } else if(battle.battle_type == 4) {
                msxLoadBoss(msx, MSX_BOSS_CELLS);
            } else if(next + tiles <= MSX_ENEMY_CHAR_END) {
                memcpy((Uint8 *)LWRAM_MENU_TILES + next * 32, msxData(MSX_ID_ENEMY_TILES + msx), tiles * 32);
            }
            msxTypeEnemy[type] = msx;
            msxTypeChar[type] = next;
            next += tiles;
            msxEnemyTypes++;
        }
        msxGroupType[group] = type;
    }
}

//The enemy palettes, 'shades' steps toward black like the NES battle fade.
static void msxShadeEnemyPalettes(Uint8 shades) {
    for(Uint8 t = 0; t < msxEnemyTypes; t++) {
        const Uint32 *src = msxPalette(MSX_ID_ENEMY_PALS + msxTypeEnemy[t]);
        for(Uint8 i = 0; i < 16; i++) {
            msxEnemyPal[t][i] = msxFadeColor(src[i], shades, BATTLE_FADE_SHADES);
        }
    }
}

static void drawEnemyGroup(Uint8 group, Uint8 xPos, Uint8 yPos) {
    if(!MSX_ON) {
        drawEnemy(picassignment[group], palassignment[group], xPos, yPos);
        return;
    }
    Uint8 size = picassignment[group] > 1 ? 6 : 4;
    Uint8 type = msxGroupType[group];
    Uint16 tile = ((MSX_ENEMY_FIRST_PAL + type) << 12) + msxTypeChar[type];

    for(Uint8 y = 0; y < size; y++) {
        for(Uint8 x = 0; x < size; x++) {
            menuPlane[yPos + y][xPos + CENTER_ADJUSTMENT - 1 + x] = tile++;
        }
    }
}

//Draws the MSX Fiend or Chaos over the same cells as drawFiendChaos.
static void msxDrawBoss(Uint8 width, Uint8 height, Uint8 nesX, Uint8 nesY) {
    for(Uint8 y = 0; y < height; y++) {
        for(Uint8 x = 0; x < width; x++) {
            Uint8 ch = msxBossChars[y * width + x];
            menuPlane[nesY + y][nesX + x + CENTER_ADJUSTMENT - 1] = ch ? (MSX_ENEMY_FIRST_PAL << 12) + ch : 0;
        }
    }
}

void enemy_bat_draw() {

    int i = 0;

    Uint8 startPosY = 10;
    Uint8 startPosX = 2;
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
            
            drawEnemyGroup(enemyNums[i], startPosX, startPosY);
            
            slotNum = battle_formation_0_order[i];
            slotToEnemyNum[slotNum] = i;
            if(enemies[i].hp > 0) {
                slotOccupied[slotNum] = TRUE;
                slotXCoords[slotNum] = startPosX;
                slotYCoords[slotNum] = startPosY;
            }  else {
                slotOccupied[slotNum] = FALSE;
                Uint8 enemy_num_to_check = enemyNums[i];
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
         //NES lut_EraseEnemyPPUAddress_4Large: rows 6/12, columns 2/10.
         startPosY = 6;
         for(i = 0; i < totalEnemies; i++) {
            
            drawEnemyGroup(enemyNums[i], startPosX, startPosY);


            slotNum = battle_formation_1_order[i];
            slotToEnemyNum[slotNum] = i;
            if(enemies[i].hp > 0) {
                slotOccupied[slotNum] = TRUE;
                slotXCoords[slotNum] = startPosX;
                slotYCoords[slotNum] = startPosY;
            } else {
                slotOccupied[slotNum] = FALSE;
                Uint8 enemy_num_to_check = enemyNums[i];
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

            if(i == 1) {
                startPosX += 8;
                startPosY -= 6;
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


        //Large enemies are indices 0-1, small enemies start at index 2.
        Uint8 current_slot = 0;

        //Same spots as the first two 4Large enemies.
        startPosY = 6;
        for(i = 0; i < totalLargeEnemies; i++) {

            current_slot = i;
            drawEnemyGroup(enemyNums[current_slot], startPosX, startPosY);

            slotNum = battle_formation_2_order[current_slot];

            slotToEnemyNum[slotNum] = current_slot;
            if(enemies[current_slot].hp > 0) {
                slotOccupied[slotNum] = TRUE;
                slotXCoords[slotNum] = startPosX;
                slotYCoords[slotNum] = startPosY;
            } else {
                slotOccupied[slotNum] = FALSE;
                Uint8 enemy_num_to_check = enemyNums[current_slot];
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

            startPosY += 6;
        }

        startPosY = 10;
        startPosX = 8;


        for(i = 0; i < totalSmallEnemies; i++) {
            current_slot = i + 2;
            drawEnemyGroup(enemyNums[current_slot], startPosX, startPosY);


            slotNum = battle_formation_2_order[current_slot];
            slotToEnemyNum[slotNum] = current_slot;
            if(enemies[current_slot].hp > 0) {
                slotOccupied[slotNum] = TRUE;
                slotXCoords[slotNum] = startPosX;
                slotYCoords[slotNum] = startPosY;
            } else {
                slotOccupied[slotNum] = FALSE;
                Uint8 enemy_num_to_check = enemyNums[current_slot];
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

    if(battle.battle_type == 3) {
        numOfFiendSlots = 1;
        if(MSX_ON) {
            msxDrawBoss(8, 8, 4, 8);
        } else {
            drawFiendChaos((Uint8 *) FIEND_TSA + (battle.pic_assignment * 0x50), 8, 8, 4, 8);
        }
    }
    if(battle.battle_type == 4) {
        numOfChaosSlots = 1;
        if(MSX_ON) {
            msxDrawBoss(14, 12, 2, 6);
        } else {
            drawFiendChaos((Uint8 *) CHAOS_TSA, 14, 12, 2, 6);
        }
    }
    if(battle.battle_type >= 3) {
        slotToEnemyNum[0] = 0;
        slotOccupied[0] = TRUE;
        //NES erase area for a dead Fiend, 12x12 tiles from $20C2.
        slotXCoords[0] = 2;
        slotYCoords[0] = 6;
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
    Uint8 *batRngPtr = (Uint8*)BATTLE_RNG_TABLE;
    Uint8 random = batRngPtr[battle_rng_seed];
    battle_rng_seed++;

    return random;
}

void setRngSeed(void) {
    //The original game relied on the randomness of NES RAM on 
    //Power On. The Saturn clears it's RAM on start up, so to
    //try and mimick the NES behavior I'm getting the 
    //seconds value from the RTC.
    rtcInit();
}

//Seconds into the current hour (BCD minutes/seconds), wrapped to a byte.
Uint8 readRtcSeed(void) {
    const Uint8 *time = rtcTime();
    Uint8 sec = (time[0] >> 4) * 10 + (time[0] & 0x0F);
    Uint8 min = (time[1] >> 4) * 10 + (time[1] & 0x0F);

    return (Uint8)(min * 60 + sec);
}

void seedBattleRng(void) {
    battle_rng_seed = readRtcSeed();
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
    
    Uint8 *battleBox_height = (Uint8 *)BATTLE_MAIN_BOX_HEIGHT;

    if (isSurprised) {
        msgBeginBlock();
        drawBox(info_box_luts[BOTTOM_MESSAGE_INDEX].xPos - 1, info_box_luts[BOTTOM_MESSAGE_INDEX].yPos + *battleBox_height, info_box_luts[BOTTOM_MESSAGE_INDEX].width, info_box_luts[BOTTOM_MESSAGE_INDEX].height, BATTLE_MENU_PALETTE);
        drawBattleMsg(info_box_luts[BOTTOM_MESSAGE_INDEX].text_xPos - 1, info_box_luts[BOTTOM_MESSAGE_INDEX].text_yPos + *battleBox_height + 1, BTLMSG_SURPRISED, BATTLE_MENU_PALETTE);
        msgEndBlock();
    } else if (isPreemptive) {
        msgBeginBlock();
        drawBox(info_box_luts[BOTTOM_MESSAGE_INDEX].xPos - 1, info_box_luts[BOTTOM_MESSAGE_INDEX].yPos  + *battleBox_height, info_box_luts[BOTTOM_MESSAGE_INDEX].width, info_box_luts[BOTTOM_MESSAGE_INDEX].height, BATTLE_MENU_PALETTE);
        drawBattleMsg(info_box_luts[BOTTOM_MESSAGE_INDEX].text_xPos - 1, info_box_luts[BOTTOM_MESSAGE_INDEX].text_yPos + *battleBox_height + 1, BTLMSG_STRIKEFIRST, BATTLE_MENU_PALETTE);
        msgEndBlock();
    }
}

void drawAttackerDefenderMessage(char * attacker, char * defender) {
    Uint8 *battleBox_height = (Uint8 *)BATTLE_MAIN_BOX_HEIGHT;

    msgBeginBlock();
    drawBox(info_box_luts[ATTACKER_MESSAGE_INDEX].xPos - 1, info_box_luts[ATTACKER_MESSAGE_INDEX].yPos + *battleBox_height, info_box_luts[ATTACKER_MESSAGE_INDEX].width, info_box_luts[ATTACKER_MESSAGE_INDEX].height, BATTLE_MENU_PALETTE);
    drawTextLine(info_box_luts[ATTACKER_MESSAGE_INDEX].text_xPos - 1, info_box_luts[ATTACKER_MESSAGE_INDEX].text_yPos + *battleBox_height + 1, attacker, BATTLE_MENU_PALETTE, 1);
    msgEndBlock();

    msgBeginBlock();
    drawBox(info_box_luts[DEFENDER_MESSAGE_INDEX].xPos - 1, info_box_luts[DEFENDER_MESSAGE_INDEX].yPos + *battleBox_height, info_box_luts[DEFENDER_MESSAGE_INDEX].width, info_box_luts[DEFENDER_MESSAGE_INDEX].height, BATTLE_MENU_PALETTE);
    drawTextLine(info_box_luts[DEFENDER_MESSAGE_INDEX].text_xPos - 1, info_box_luts[DEFENDER_MESSAGE_INDEX].text_yPos + *battleBox_height + 1, defender, BATTLE_MENU_PALETTE, 1);
    msgEndBlock();
}

void drawMonstersPerishedMessage() {
    msgBeginBlock();
    Uint8 *battleBox_height = (Uint8 *)BATTLE_MAIN_BOX_HEIGHT;
    drawBox(info_box_luts[BOTTOM_MESSAGE_INDEX].xPos - 1, info_box_luts[BOTTOM_MESSAGE_INDEX].yPos  + *battleBox_height, info_box_luts[BOTTOM_MESSAGE_INDEX].width, info_box_luts[BOTTOM_MESSAGE_INDEX].height, BATTLE_MENU_PALETTE);
    drawMonstersPerished(info_box_luts[BOTTOM_MESSAGE_INDEX].text_xPos - 1, info_box_luts[BOTTOM_MESSAGE_INDEX].text_yPos + *battleBox_height + 1, BATTLE_MENU_PALETTE);
    msgEndBlock();
}

void drawPartyPerishedMessage() {
    msgBeginBlock();
    Uint8 *battleBox_height = (Uint8 *)BATTLE_MAIN_BOX_HEIGHT;
    drawBox(info_box_luts[BOTTOM_MESSAGE_INDEX].xPos - 1, info_box_luts[BOTTOM_MESSAGE_INDEX].yPos + *battleBox_height, info_box_luts[BOTTOM_MESSAGE_INDEX].width, info_box_luts[BOTTOM_MESSAGE_INDEX].height, BATTLE_MENU_PALETTE);
    drawPartyPerished(info_box_luts[BOTTOM_MESSAGE_INDEX].text_xPos - 1, info_box_luts[BOTTOM_MESSAGE_INDEX].text_yPos + *battleBox_height + 1, battle_char_stats[0].name, BATTLE_MENU_PALETTE);
    msgEndBlock();
}

void drawCombatBoxText(Uint8 boxIndex, Uint8 *text) {
    msgBeginBlock();
    Uint8 *battleBox_height = (Uint8 *)BATTLE_MAIN_BOX_HEIGHT;
    BATTLE_INFO_BOX *box = &info_box_luts[boxIndex];
    drawBox(box->xPos - 1, box->yPos + *battleBox_height, box->width, box->height, BATTLE_MENU_PALETTE);
    drawTextLine(box->text_xPos - 1, box->text_yPos + *battleBox_height + 1, text, BATTLE_MENU_PALETTE, 1);
    msgEndBlock();
}

void drawAttackerName(char * attacker) {
    msgBeginBlock();
    Uint8 *battleBox_height = (Uint8 *)BATTLE_MAIN_BOX_HEIGHT;
    drawBox(info_box_luts[ATTACKER_MESSAGE_INDEX].xPos - 1, info_box_luts[ATTACKER_MESSAGE_INDEX].yPos + *battleBox_height, info_box_luts[ATTACKER_MESSAGE_INDEX].width, info_box_luts[ATTACKER_MESSAGE_INDEX].height, BATTLE_MENU_PALETTE);
    drawTextLine(info_box_luts[ATTACKER_MESSAGE_INDEX].text_xPos - 1, info_box_luts[ATTACKER_MESSAGE_INDEX].text_yPos + *battleBox_height + 1, attacker, BATTLE_MENU_PALETTE, 1);
    msgEndBlock();
}

void drawRunSuccessMessage() {
    msgBeginBlock();
    Uint8 *battleBox_height = (Uint8 *)BATTLE_MAIN_BOX_HEIGHT;
    drawBox(info_box_luts[BOTTOM_MESSAGE_INDEX].xPos - 1, info_box_luts[BOTTOM_MESSAGE_INDEX].yPos + *battleBox_height, info_box_luts[BOTTOM_MESSAGE_INDEX].width, info_box_luts[BOTTOM_MESSAGE_INDEX].height, BATTLE_MENU_PALETTE);
    drawBattleMsg(info_box_luts[BOTTOM_MESSAGE_INDEX].text_xPos - 1, info_box_luts[BOTTOM_MESSAGE_INDEX].text_yPos + *battleBox_height + 1, BTLMSG_CLOSECALL, BATTLE_MENU_PALETTE);
    msgEndBlock();
}

void drawLutBottomdMessage(Uint8 msg_indx) {
    msgBeginBlock();
    Uint8 *battleBox_height = (Uint8 *)BATTLE_MAIN_BOX_HEIGHT;
    drawBox(info_box_luts[BOTTOM_MESSAGE_INDEX].xPos - 1, info_box_luts[BOTTOM_MESSAGE_INDEX].yPos + *battleBox_height, info_box_luts[BOTTOM_MESSAGE_INDEX].width, info_box_luts[BOTTOM_MESSAGE_INDEX].height, BATTLE_MENU_PALETTE);
    drawBattleMsg(info_box_luts[BOTTOM_MESSAGE_INDEX].text_xPos - 1, info_box_luts[BOTTOM_MESSAGE_INDEX].text_yPos + *battleBox_height + 1, msg_indx, BATTLE_MENU_PALETTE);
    msgEndBlock();
}

void drawLutResultMessage(Uint8 msg_index){
    msgBeginBlock();
    Uint8 *battleBox_height = (Uint8 *)BATTLE_MAIN_BOX_HEIGHT;
    drawBox(info_box_luts[DAMAGE_MESSAGE_INDEX].xPos - 1, info_box_luts[DAMAGE_MESSAGE_INDEX].yPos + *battleBox_height, info_box_luts[DAMAGE_MESSAGE_INDEX].width, info_box_luts[DAMAGE_MESSAGE_INDEX].height, BATTLE_MENU_PALETTE);
    drawBattleMsg(info_box_luts[DAMAGE_MESSAGE_INDEX].text_xPos - 1, info_box_luts[DAMAGE_MESSAGE_INDEX].text_yPos + *battleBox_height + 1, msg_index, BATTLE_MENU_PALETTE);
    msgEndBlock();
}

void drawAttackMessage(Uint8 *msg){
    msgBeginBlock();
    Uint8 *battleBox_height = (Uint8 *)BATTLE_MAIN_BOX_HEIGHT;
    drawBox(info_box_luts[ATTACK_MESSAGE_INDEX].xPos - 1, info_box_luts[ATTACK_MESSAGE_INDEX].yPos + *battleBox_height, info_box_luts[ATTACK_MESSAGE_INDEX].width, info_box_luts[ATTACK_MESSAGE_INDEX].height, BATTLE_MENU_PALETTE);
    drawTextLine(info_box_luts[ATTACK_MESSAGE_INDEX].text_xPos - 1, info_box_luts[ATTACK_MESSAGE_INDEX].text_yPos + *battleBox_height + 1, msg, BATTLE_MENU_PALETTE, 1);
    msgEndBlock();
}

void drawResultMessage(Uint8 *msg) {
    msgBeginBlock();
    Uint8 *battleBox_height = (Uint8 *)BATTLE_MAIN_BOX_HEIGHT;
    drawBox(info_box_luts[DAMAGE_MESSAGE_INDEX].xPos - 1, info_box_luts[DAMAGE_MESSAGE_INDEX].yPos + *battleBox_height, info_box_luts[DAMAGE_MESSAGE_INDEX].width, info_box_luts[DAMAGE_MESSAGE_INDEX].height, BATTLE_MENU_PALETTE);
    drawTextLine(info_box_luts[DAMAGE_MESSAGE_INDEX].text_xPos - 1, info_box_luts[DAMAGE_MESSAGE_INDEX].text_yPos + *battleBox_height + 1, msg, BATTLE_MENU_PALETTE, 1);
    msgEndBlock();
}


//Undraws every block in the message area, newest first.
void clearMessageArea(){
    msgUndraw(msgGetBlockCount());
}

//Undraws the newest block, the bottom message.
void clearBottomMessage() {
    msgUndraw(1);
}

void drawMenu() {
    
    Uint8 *battleBox_height = (Uint8 *)BATTLE_MAIN_BOX_HEIGHT;

    BATTLE_BOX *enemyRosterBox = (BATTLE_BOX *)ENEMY_ROSTER_BOX_LUT;
    COMMAND_BOX *commandBox = (COMMAND_BOX *)COMMAND_BOX_LUT;

    msgBeginBlock();
    drawBox(enemyRosterBox[0].xPos - 1, enemyRosterBox[0].yPos + *battleBox_height, enemyRosterBox[0].width, enemyRosterBox[0].height, BATTLE_MENU_PALETTE);
    drawEnemyNames();
    msgEndBlock();

    msgBeginBlock();
    drawBox(commandBox[0].xPos - 1, commandBox[0].yPos + *battleBox_height, commandBox[0].width, commandBox[0].height, BATTLE_MENU_PALETTE);

    for(Uint8 i = 0; i < 5; i++) {

        Uint16 ptr = (commandBox[0].text[i].ptr_high << 8) | commandBox[0].text[i].ptr_low;
        Uint8 *text_ptr = ROM_PTR(0x0F, ptr);
        drawTextLine(commandBox[0].text[i].xPos - 1, commandBox[0].text[i].yPos + *battleBox_height + 1, text_ptr, BATTLE_MENU_PALETTE, 1);
    }
    msgEndBlock();
}

void drawMagicMenu(Uint8 playerIndex, Uint8 pageNum) {
    
    BATTLE_BOX *magicMenuBox = (BATTLE_BOX *)ITEM_AND_MAGIC_BOX_LUT;
    Uint8 *battleBox_height = (Uint8 *)BATTLE_MAIN_BOX_HEIGHT;

    msgBeginBlock();
    drawBox(magicMenuBox[0].xPos - 1, magicMenuBox[0].yPos + *battleBox_height, magicMenuBox[0].width, magicMenuBox[0].height, BATTLE_MENU_PALETTE);

    //NES DrawBattleMagicBox row: "L# NAME NAME NAME M", empty slots are blank.
    for(Uint8 i = 0; i < 4; i++) {
        Uint8 level = (pageNum * 4) + i;
        Uint8 row[20];

        for(Uint8 k = 0; k < 19; k++) {
            row[k] = 0xFF;
        }
        row[19] = 0x00;
        row[0] = 0x95;
        row[1] = 0x81 + level;

        for(Uint8 slot = 0; slot < 3; slot++) {
            Uint8 spell = battle_char_stats[playerIndex].magic[level][slot];
            if(spell != 0) {
                Uint8 *name = getSpellName(spell - 1);
                for(Uint8 k = 0; k < 4 && name[k] != 0x00; k++) {
                    row[3 + (slot * 5) + k] = name[k];
                }
            }
        }
        row[18] = battle_char_stats[playerIndex].current_mp[level] + 0x80;

        drawTextLine(magicMenuBox[0].xPos, magicMenuBox[0].yPos + *battleBox_height + ((i + 1) * 2), row, BATTLE_MENU_PALETTE, 1);
    }
    msgEndBlock();
}

//NES DrawBattleSubString_Max8.
static Uint8 appendItemName(Uint8 *row, Uint8 len, Uint8 itemId) {
    Uint8 *name = getItemName(itemId);
    for(Uint8 k = 0; k < 8 && name[k] != 0x00; k++) {
        row[len++] = name[k];
    }
    return len;
}

//NES DrawDrinkBox: " HEAL qty" and " PURE qty", blank when there are none.
void drawDrinkMenu(Uint8 healCount, Uint8 pureCount) {
    BATTLE_BOX *drinkBox = (BATTLE_BOX *)DRINK_BOX_LUT;
    Uint8 *battleBox_height = (Uint8 *)BATTLE_MAIN_BOX_HEIGHT;
    Uint8 counts[2] = {healCount, pureCount};

    msgBeginBlock();
    drawBox(drinkBox[0].xPos - 1, drinkBox[0].yPos + *battleBox_height, drinkBox[0].width, drinkBox[0].height, BATTLE_MENU_PALETTE);

    for(Uint8 i = 0; i < 2; i++) {
        Uint8 row[16];
        Uint8 len = 0;

        row[len++] = 0xFF;
        if(counts[i] != 0) {
            len = appendItemName(row, len, ITEM_HEAL + i);
            row[len++] = 0xFF;
            len = appendNumber(row, len, counts[i]);
        }
        row[len] = 0x00;

        drawTextLine(drinkBox[0].xPos, drinkBox[0].yPos + *battleBox_height + ((i + 1) * 2), row, BATTLE_MENU_PALETTE, 1);
    }
    msgEndBlock();
}

//NES DrawBattleItemBox: a row per equipment slot, " WEAPON  ARMOR". An empty weapon is 7 spaces.
void drawItemMenu(Uint8 playerIndex) {
    BATTLE_BOX *itemBox = (BATTLE_BOX *)ITEM_AND_MAGIC_BOX_LUT;
    Uint8 *battleBox_height = (Uint8 *)BATTLE_MAIN_BOX_HEIGHT;

    msgBeginBlock();
    drawBox(itemBox[0].xPos - 1, itemBox[0].yPos + *battleBox_height, itemBox[0].width, itemBox[0].height, BATTLE_MENU_PALETTE);

    for(Uint8 i = 0; i < 4; i++) {
        Uint8 row[24];
        Uint8 len = 0;
        Uint8 weapon = party[playerIndex].weapons[i] & 0x7F;
        Uint8 armor = party[playerIndex].armors[i] & 0x7F;

        row[len++] = 0xFF;
        if(weapon != 0) {
            len = appendItemName(row, len, ITEM_WEAPON_START - 1 + weapon);
        } else {
            for(Uint8 k = 0; k < 7; k++) {
                row[len++] = 0xFF;
            }
        }
        row[len++] = 0xFF;
        row[len++] = 0xFF;
        if(armor != 0) {
            len = appendItemName(row, len, ITEM_ARMOR_START - 1 + armor);
        }
        row[len] = 0x00;

        drawTextLine(itemBox[0].xPos, itemBox[0].yPos + *battleBox_height + ((i + 1) * 2), row, BATTLE_MENU_PALETTE, 1);
    }
    msgEndBlock();
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
            totalLargeEnemies = numOfLargeEnemySlots - numOfLargeEnemySlotsAvailable;
            totalSmallEnemies = numOfSmallEnemySlots - numOfSmallEnemySlotsAvailable;
            totalEnemies = totalLargeEnemies + totalSmallEnemies;
            break;
        case 3:
        case 4:
            totalEnemies = 1;
            break;
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
    //Not reached: target selection only runs with an enemy left.
    return start;
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
    //Not reached: target selection only runs with an enemy left.
    return start;
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

    if(BUGFIX_ON(BUGFIX_TURN_SHUFFLE)) {
        //Fisher-Yates.
        for(Uint8 i = 12; i > 0; i--) {
            swapIndexB = rand_min_max(0, i);
            swapValA = battleOrderBuffer[i];
            battleOrderBuffer[i] = battleOrderBuffer[swapIndexB];
            battleOrderBuffer[swapIndexB] = swapValA;
        }
        return;
    }

    for(Uint8 i = 0; i < 16; i++) {
        swapIndexA = rand_min_max(0, 12);
        swapIndexB = rand_min_max(0, 12);

        swapValA = battleOrderBuffer[swapIndexA];
        swapValB = battleOrderBuffer[swapIndexB];

        battleOrderBuffer[swapIndexA] = swapValB;
        battleOrderBuffer[swapIndexB] = swapValA;

    }
}

typedef struct {
    Uint8 strength;
    Uint8 crit_rate;
    Uint8 hit_rate;
    Uint8 num_hits;
    Uint8 num_hits_mult;
    Uint8 category;
    Uint8 element;
    Uint8 ailments;
    Uint8 attack_ailment;
    Uint8 def_is_player;
    Uint8 def_category;
    Uint8 def_elem_weak;
    Uint8 def_elem_resist;
    Uint8 def_mag_def;
    Uint8 def_ailments;
    Uint8 def_evade;
    Uint8 def_absorb;
} PHYSICAL_ATTACK;

//Ailment message order matches PrintPlayerAilmentMessageFromAttack (bit 7 down to bit 0).
static const Uint8 ailmentInflictMsgs[8] = {
    BTLMSG_CONFUSED, BTLMSG_SILENCED, BTLMSG_ASLEEP, BTLMSG_PARALYZED_A,
    BTLMSG_DARKNESS, BTLMSG_POISONED, BTLMSG_STOPPED, BTLMSG_SLAIN
};

static void queueAilmentMessages(Uint8 newAilments) {
    for(Uint8 i = 0; i < 8; i++) {
        if((newAilments & (0x80 >> i)) && ailmentMsgCount < 8) {
            ailmentMsgs[ailmentMsgCount] = ailmentInflictMsgs[i];
            ailmentMsgCount++;
        }
    }
}

static void doPhysicalAttack(PHYSICAL_ATTACK *atk) {
    Uint16 hitChance = 168;
    Uint16 baseDmg = atk->strength;
    Uint8 critChance = atk->crit_rate;

    if(atk->ailments & AIL_DARK) {
        hitChance -= 40;
    }
    if(atk->def_ailments & AIL_DARK) {
        hitChance += 40;
    }

    if((atk->category & atk->def_category) | (atk->element & atk->def_elem_weak)) {
        hitChance += 40;
        if(hitChance > 255) {
            hitChance = 255;
        }
        baseDmg += 4;
        if(baseDmg > 255) {
            baseDmg = 255;
        }
    }

    hitChance += atk->hit_rate;
    if(hitChance > 255) {
        hitChance = 255;
    }

    if(atk->def_ailments & (AIL_STUN | AIL_SLEEP)) {
        baseDmg += baseDmg >> 2;
        if(baseDmg > 255) {
            baseDmg = 255;
        }
    } else if(hitChance > atk->def_evade) {
        hitChance -= atk->def_evade;
    } else {
        hitChance = 0;
    }

    if(hitChance == 0) {
        hitChance = 1;
    }
    if(critChance > hitChance) {
        critChance = hitChance;
    }

    Uint8 numHits = atk->num_hits * atk->num_hits_mult;
    if(numHits == 0) {
        numHits = 1;
    }

    Uint16 totalDmg = 0;
    Uint8 hitsConnected = 0;
    Uint8 critsConnected = 0;
    curr_attack_ailments = atk->def_ailments;
    curr_attack_ailment_death = FALSE;
    ailmentMsgCount = 0;
    ailmentMsgIndex = 0;

    for(Uint8 i = 0; i < numHits; i++) {
        Uint8 randHit = rand_min_max(0, 200);
        if(randHit == 200) {
            continue;
        }

        Uint16 rollDmg = rand_min_max(0, baseDmg) + baseDmg;
        if(rollDmg > 255) {
            rollDmg = 255;
        }

        Uint16 hitDmg = 0;
        Uint8 hit = randHit <= hitChance;
        if(hit) {
            hitDmg = rollDmg > atk->def_absorb ? rollDmg - atk->def_absorb : 1;
            hitsConnected++;
        }
        if(randHit <= critChance) {
            hitDmg += rollDmg;
            critsConnected++;
        }
        totalDmg += hitDmg;

        //Only enemy attacks inflict ailments. The NES checks any earlier hit, not just this one.
        if(atk->def_is_player && atk->attack_ailment && (BUGFIX_ON(BUGFIX_AILMENT_ON_MISS) ? hit : hitsConnected)) {
            Uint8 ailChance = (atk->def_elem_resist & atk->element) ? 0 : 100;
            ailChance = ailChance > atk->def_mag_def ? ailChance - atk->def_mag_def : 0;
            if(ailChance == 0) {
                ailChance = 1;
            }

            Uint8 randAil = rand_min_max(0, 200);
            if(randAil != 200 && ailChance >= randAil) {
                Uint8 newAilments = ~curr_attack_ailments & atk->attack_ailment;
                queueAilmentMessages(newAilments);
                curr_attack_ailments |= atk->attack_ailment;
                if(newAilments & AIL_DEAD) {
                    curr_attack_ailment_death = TRUE;
                    break;
                }
            }
        }
    }

    curr_attack_dmg = totalDmg;
    curr_attack_miss = (totalDmg == 0);
    curr_attack_hits = hitsConnected;
    curr_attack_crits = critsConnected;
}

void takePlayersTurn(Uint8 playerIndex) {
    Uint8 targetIndex = playerActions[playerIndex].targetIndex;
    BATTLE_CHARACTER *player = &battle_char_stats[playerIndex];
    ENEMY_DATA *enemy = &enemies[targetIndex];
    PHYSICAL_ATTACK atk;

    atk.strength = player->damage;
    atk.crit_rate = player->crit_rate;
    atk.hit_rate = player->hit_percent;
    atk.num_hits = player->numhits;
    atk.num_hits_mult = player->numhitsmult;
    //The NES attacks with the player's category and weakness, which are always 0.
    if(BUGFIX_ON(BUGFIX_WEAPON_EFFECTS)) {
        atk.category = player->wep_category;
        atk.element = player->wep_element;
    } else {
        atk.category = player->category;
        atk.element = player->elem_weak;
    }
    atk.ailments = player->ailment;
    atk.attack_ailment = 0;
    atk.def_is_player = FALSE;
    atk.def_category = enemy->category;
    atk.def_elem_weak = enemy->elem_weak;
    atk.def_elem_resist = enemy->elem_resist;
    atk.def_mag_def = enemy->mag_def;
    atk.def_ailments = enemy->ailment;
    atk.def_evade = enemy->evade;
    atk.def_absorb = enemy->absorb;

    doPhysicalAttack(&atk);
    player->ib_ailment = player->ailment;
}

void takeEnemyTurn(Uint8 enemyIndex) {
    ENEMY_DATA *enemy = &enemies[enemyIndex];
    BATTLE_CHARACTER *player = &battle_char_stats[curr_enemy_target];
    PHYSICAL_ATTACK atk;

    atk.strength = enemy->strength;
    atk.crit_rate = enemy->crit_rate;
    atk.hit_rate = enemy->hit_rate;
    atk.num_hits = enemy->hit_num;
    atk.num_hits_mult = enemy->num_hits_mult;
    atk.category = enemy->category;
    //The NES attacks with the enemy's elemental weakness.
    atk.element = BUGFIX_ON(BUGFIX_ENEMY_ELEMENT) ? 0 : enemy->elem_weak;
    atk.ailments = enemy->ailment;
    atk.attack_ailment = enemy->atk_ailment;
    atk.def_is_player = TRUE;
    atk.def_category = player->category;
    atk.def_elem_weak = player->elem_weak;
    atk.def_elem_resist = player->elem_resist;
    atk.def_mag_def = player->mag_def;
    atk.def_ailments = player->ailment;
    atk.def_evade = player->evade_percent;
    atk.def_absorb = player->absorb;

    doPhysicalAttack(&atk);
}

//Runs if morale + rand[0,0x32] - 2*leader level is below 0x50.
Uint8 enemyRunsAway(ENEMY_DATA *enemy) {
    //The NES stores levels 0-based.
    Uint8 leaderLevel = (battle_char_stats[0].level - 1) * 2;

    if(enemy->morale < leaderLevel) {
        return TRUE;
    }

    Uint16 value = rand_min_max(0, 0x32) + (enemy->morale - leaderLevel);
    return value < 0x50;
}

//Returns a spell ID, a special attack ID + ENEMY_SPECIAL_ATTACK_OFFSET, or ENEMY_ACTION_PHYSICAL.
Uint8 enemyChooseAiAction(ENEMY_DATA *enemy) {
    if(enemy->ai_id == 0xFF) {
        return ENEMY_ACTION_PHYSICAL;
    }

    //16 bytes per AI: magic rate, special rate, spells at 2-9, special attacks at 0x0B-0x0E.
    Uint8 *ai = (Uint8*) (ENEMY_AI_LUT_LOCATION + enemy->ai_id * 0x10);
    Uint8 action;

    if(rand_min_max(0, 0x80) < ai[0]) {
        //Cycles through the spell slots, wrapping at an empty one.
        for(Uint8 tries = 0; tries < 9; tries++) {
            Uint8 pos = enemy->ai_mag_pos & 0x07;
            enemy->ai_mag_pos = pos + 1;
            action = ai[pos + 2];
            if(action != 0xFF) {
                return action;
            }
            enemy->ai_mag_pos = 0;
        }
    } else if(rand_min_max(0, 0x80) < ai[1]) {
        for(Uint8 tries = 0; tries < 5; tries++) {
            Uint8 pos = enemy->ai_atk_pos & 0x03;
            enemy->ai_atk_pos = pos + 1;
            action = ai[pos + 0x0B];
            if(action != 0xFF) {
                return action + ENEMY_SPECIAL_ATTACK_OFFSET;
            }
            enemy->ai_atk_pos = 0;
        }
    }

    return ENEMY_ACTION_PHYSICAL;
}

Uint8 determinePlayerTarget() {
    Uint8 playerIndex = 0;
    Uint8 playerDetermined = FALSE;
    while(!playerDetermined) {

        Uint8 rand = battleRNG();
        if(rand < 0x20) {
            playerIndex = 3;
        } else if(rand < 0x40) {
            playerIndex = 2;
        } else if(rand < 0x80) {
            playerIndex = 1;
        } else {
            playerIndex = 0;
        }
        if(!isPlayerDead(playerIndex) && !(battle_char_stats[playerIndex].ailment & AIL_STONE)) {
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

Uint8 isPlayerCrouching(Uint8 index, Uint16 maxHp) {
    Uint16 hp = battle_char_stats[index].hp;

    if(battle_char_stats[index].ailment & (AIL_POISON | AIL_STUN | AIL_SLEEP)) {
        return TRUE;
    }

    //The NES only compares against the low byte of 25% max HP.
    if(hp > 0xFF) {
        return FALSE;
    }

    return hp < ((maxHp / 4) & 0xFF);
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
        if(battle_char_stats[i].hp > 0 && !(battle_char_stats[i].ailment & AIL_STONE)) {
            return FALSE;
        }
    }

    return TRUE;
}

Uint8 canInputCommand(Uint8 index) {
    if(isPlayerDead(index)) {
        return FALSE;
    }
    return !(battle_char_stats[index].ailment & (AIL_DEAD | AIL_STONE | AIL_STUN | AIL_SLEEP));
}

void applyEndOfRoundEffects() {
    for(Uint8 i = 0; i < 4; i++) {
        BATTLE_CHARACTER *player = &battle_char_stats[i];
        //The NES drains a stoned character too.
        if((player->ailment & AIL_POISON) && !(BUGFIX_ON(BUGFIX_STONE_POISON) && (player->ailment & AIL_STONE))) {
            player->hp = player->hp > 2 ? player->hp - 2 : 0;
            if(player->hp == 0) {
                player->ailment = AIL_DEAD;
            }
        }
    }

    //The NES regenerates the first N enemy slots, where N is the enemy count.
    for(Uint8 i = 0; i < placedEnemies; i++) {
        ENEMY_DATA *enemy = &enemies[i];
        if(enemy->hp > 0 && (enemy->category & CATEGORY_REGEN)) {
            enemy->hp += 3;
            if(enemy->hp > enemy->hp_max) {
                enemy->hp = enemy->hp_max;
            }
        }
    }
}

Uint8 canRun(Uint8 index) {
    if(isPreemptive) {
        return TRUE;
    }
    if(battle.no_run) {
        return FALSE;
    }

    Uint8 level;
    if(BUGFIX_ON(BUGFIX_RUN_CHANCE)) {
        //0-based, like the NES OB level.
        level = battle_char_stats[index].level - 1;
    } else {
        //The NES reads $26 past the slot's $12-byte IB stats instead of the OB level.
        switch(index) {
            case 0:
                level = battle_char_stats[2].ib_ailment;
                break;
            case 1:
                level = battle_char_stats[3].ib_ailment;
                break;
            case 2:
                level = battleOrderBuffer[2];
                break;
            default:
                //Ones digit tile of slot 3's HP, last written by the status redraw.
                level = 0x80 | (battle_char_stats[3].hp % 10);
                break;
        }
    }
    Uint8 luck = battle_char_stats[index].luck;

    Uint8 randLvl15 = rand_min_max(0, (Uint8)(level + 15));

    if(luck > randLvl15) {
        return TRUE;
    }

    return FALSE;
}

void drawDmgString(Uint16 damage) {
    Uint8 dmgBuffer[12] = {0x00};
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


    Uint16 *battleMsg_lut = (Uint16 *)BATTLE_MSG_PTR_TABLE;
    
    Uint8 msg_id = BTLMSG_DMG;
    msg_id--;
    Uint16 tmpOffset = battleMsg_lut[msg_id];
    Uint16 btl_msg_offset = (tmpOffset >> 8) | (tmpOffset << 8);
    btl_msg_offset -= 0x8C40;
    Uint8 *dmg_msg_ptr = (Uint8*) BATTLE_MSG_TABLE + btl_msg_offset;

    for(Uint8 i = 0; dmg_msg_ptr[i] != 0 && i + dmgColStart < sizeof(dmgBuffer) - 1; i++){
        dmgBuffer[i + dmgColStart] = dmg_msg_ptr[i];
    }

    Uint8 *txt_ptr = dmgBuffer;

    drawResultMessage(txt_ptr);
}

//Draws "N Hits!" in the attack box.
void drawHitsString(Uint8 hits) {
    Uint8 hitsBuffer[16] = {0};
    Uint8 len = 0;

    if(hits >= 100) {
        hitsBuffer[len++] = (hits / 100) + 0x80;
    }
    if(hits >= 10) {
        hitsBuffer[len++] = ((hits / 10) % 10) + 0x80;
    }
    hitsBuffer[len++] = (hits % 10) + 0x80;

    Uint16 *battleMsg_lut = (Uint16 *)BATTLE_MSG_PTR_TABLE;
    Uint16 tmpOffset = battleMsg_lut[BTLMSG_HITS - 1];
    Uint16 btl_msg_offset = (tmpOffset >> 8) | (tmpOffset << 8);
    btl_msg_offset -= 0x8C40;
    Uint8 *hits_msg_ptr = (Uint8*) BATTLE_MSG_TABLE + btl_msg_offset;

    for(Uint8 i = 0; hits_msg_ptr[i] != 0 && len < sizeof(hitsBuffer) - 1; i++) {
        hitsBuffer[len++] = hits_msg_ptr[i];
    }

    drawAttackMessage(hitsBuffer);
}

//Clears one row of the enemy graphic per call. Returns TRUE once all rows are cleared.
Uint8 deleteEnemy(Uint8 xPos, Uint8 yPos, Uint8 xy) {

    if(lastDeletedEnemyRow >= xy) {
        return TRUE;
    }
    clearArea(xPos - 1, yPos + lastDeletedEnemyRow, xy, 1);
    lastDeletedEnemyRow++;
    return FALSE;
}

void setLastDeletedEnemyRow(Uint8 lastRow) {
    lastDeletedEnemyRow = lastRow;
}

Uint8 getEnemySlotNum(Uint8 enemy) {
    switch(battle.battle_type) {
        case 1:
            return battle_formation_1_order[enemy];
        case 2:
            return battle_formation_2_order[enemy];
        case 3:
        case 4:
            return 0;
        default:
            return battle_formation_0_order[enemy];
    }
}

void shakeScreen() {

    if(frameCount % 2 == 0) {
        Uint8 xVal = rand_min_max(0, 3);
        Uint8 yVal = rand_min_max(0, 3);

        static_move_to(FIXED(xVal), FIXED(yVal));
    }

    if(frameCount > 12) {
        static_move_to(FIXED(0), FIXED(0));
        battleSfxHold = FALSE;
        //Targets that were already dead or stone don't flash.
        if(battle_char_stats[curr_enemy_target].ailment & (AIL_DEAD | AIL_STONE)) {
            turn_exec_state = RESULT;
        } else {
            flashCounter = 0x10;
            turn_exec_state = FLASH_CHARACTER;
        }
    }

}

#define BATTLE_FADE_PAL_COUNT (9)
#define BATTLE_FADE_BACKDROP_PAL (2)
#define BATTLE_FADE_FIRST_SPRITE_PAL (5)

//The NES enemy palettes. MSX enemies use msxEnemyPal instead.
#define BATTLE_FADE_ENEMY_PAL_0 (3)
#define BATTLE_FADE_ENEMY_PAL_1 (4)

//NES color indices of each battle palette, kept so fades can be computed from them.
static Uint8 battleNesPal[BATTLE_FADE_PAL_COUNT][4];

//The boxes and cursor fade from their MSX colors in MSX mode.
#define BATTLE_FADE_BOX_PAL_0 (0)
#define BATTLE_FADE_BOX_PAL_1 (1)
#define BATTLE_FADE_CURSOR_PAL (7)
#define BATTLE_FADE_MSX_PAL(p) ((p) == BATTLE_FADE_BOX_PAL_0 || (p) == BATTLE_FADE_BOX_PAL_1 || (p) == BATTLE_FADE_CURSOR_PAL)
static Uint32 battleMsxPal[BATTLE_FADE_PAL_COUNT][4];
static Uint32 *battleFadePal[BATTLE_FADE_PAL_COUNT] = {
    menu_pal_0, menu_pal_1, menu_pal_2, menu_pal_3, menu_pal_4,
    player_pal_0, player_pal_1, cursor_pal_0, stone_pal
};

static void copyNesPalette(Uint8 *dest, Uint8 *src) {
    for(Uint8 i = 0; i < 4; i++) {
        dest[i] = src[i];
    }
}

void initBattleFadePalettes(Uint8 backdrop) {
    Uint8 *charPal = (Uint8*) BATTLE_CHAR_PALETTE_TABLE;

    copyNesPalette(battleNesPal[0], boarderPalette_black);
    copyNesPalette(battleNesPal[1], boarderPalette_blue);
    copyNesPalette(battleNesPal[2], (Uint8*) (BATTLE_BACKDROP_PALETTE_OFFSET + (4 * backdrop)));
    copyNesPalette(battleNesPal[3], (Uint8*) (BATTLE_PALETTE_TABLE + (4 * battle.palette_id_0)));
    copyNesPalette(battleNesPal[4], (Uint8*) (BATTLE_PALETTE_TABLE + (4 * battle.palette_id_1)));
    copyNesPalette(battleNesPal[5], charPal);
    copyNesPalette(battleNesPal[6], charPal + 4);
    copyNesPalette(battleNesPal[7], (Uint8*) CURSOR_PALETTE);
    copyNesPalette(battleNesPal[8], charPal + 12);
    if(MSX_ON) {
        msxBoxPalette(MSX_BOX_BATTLE, battleMsxPal[BATTLE_FADE_BOX_PAL_0]);
        msxBoxPalette(MSX_BOX_BATTLE, battleMsxPal[BATTLE_FADE_BOX_PAL_1]);
        msxCursorPalette((Uint8 *)CURSOR_PALETTE, battleMsxPal[BATTLE_FADE_CURSOR_PAL]);
    }
}

//NES FadeOutOneShade applied 'shades' times.
static Uint8 fadeNesColor(Uint8 color, Uint8 shades) {
    while(shades > 0) {
        if(!(color & 0x30)) {
            return 0x0F;
        }
        color -= 0x10;
        shades--;
    }
    return color;
}

static void shadeBattlePalettes(Uint8 first, Uint8 last, Uint8 shades) {
    for(Uint8 p = first; p <= last; p++) {
        if(MSX_ON && (p == BATTLE_FADE_ENEMY_PAL_0 || p == BATTLE_FADE_ENEMY_PAL_1 || p == BATTLE_FADE_BACKDROP_PAL)) {
            continue;
        }
        for(Uint8 i = 0; i < 4; i++) {
            battleFadePal[p][i] = MSX_ON && BATTLE_FADE_MSX_PAL(p) ? msxFadeColor(battleMsxPal[p][i], shades, BATTLE_FADE_SHADES)
                                : nes_pal[fadeNesColor(battleNesPal[p][i], shades)];
        }
    }
    if(MSX_ON && first <= BATTLE_FADE_ENEMY_PAL_0) {
        msxShadeEnemyPalettes(shades);
    }
    if(MSX_ON && first <= BATTLE_FADE_BACKDROP_PAL && last >= BATTLE_FADE_BACKDROP_PAL) {
        const Uint32 *src = msxPalette(MSX_ID_BATTLE_PAL);
        for(Uint8 i = 0; i < 16; i++) {
            menu_pal_2[i] = msxFadeColor(src[i], shades, BATTLE_FADE_SHADES);
        }
    }
    if(MSX_ON && last >= BATTLE_FADE_FIRST_SPRITE_PAL) {
        const Uint32 *src = msxPalette(MSX_ID_BATTLE_PAL);
        for(Uint8 i = 0; i < 16; i++) {
            msx_bat_pal[i] = msxFadeColor(src[i], shades, BATTLE_FADE_SHADES);
        }
    }
}

static void uploadBattlePalettes() {
    vidSetPalette(VID_NBG1, 0, 16, menu_pal_0);
    vidSetPalette(VID_NBG1, 16, 16, menu_pal_1);
    vidSetPalette(VID_NBG1, 32, 16, menu_pal_2);
    vidSetPalette(VID_NBG1, 48, 16, menu_pal_3);
    vidSetPalette(VID_NBG1, 64, 16, menu_pal_4);
    if(MSX_ON) {
        vidSetPalette(VID_NBG1, 80, 16, menu_pal_5);
        vidSetPalette(VID_NBG1, 96, 16, menu_pal_6);
    }
    vidSetPalette(VID_SPR, 16, 16, player_pal_0);
    vidSetPalette(VID_SPR, 32, 16, player_pal_1);
    vidSetPalette(VID_SPR, 64, 16, cursor_pal_0);
    vidSetPalette(VID_SPR, STONE_PALETTE_CRAM, 16, stone_pal);
    if(MSX_ON) {
        vidSetPalette(VID_SPR, MSX_BATTLE_CRAM, 16, msx_bat_pal);
    }
}

void setBattleShade(Uint8 shades) {
    shadeBattlePalettes(0, BATTLE_FADE_PAL_COUNT - 1, shades);
    uploadBattlePalettes();
    battleFadeShade = shades;
}

//Sprite palettes are only loaded at startup, so they're restored after a fade out.
//The menu layer palettes are reloaded by the next screen that uses them.
void restoreBattleSpritePalettes() {
    shadeBattlePalettes(BATTLE_FADE_FIRST_SPRITE_PAL, BATTLE_FADE_PAL_COUNT - 1, 0);
    uploadBattlePalettes();
}

Uint8 battleBoxTopShift() {
    return MSX_ON ? 1 : 0;
}

//The back screen shows through color 0, like the NES BG color. It takes RGB555.
void setBattleBackColor(Uint8 nesColor) {
    setBattleBackRgb(nes_pal[nesColor]);
}

void setBattleBackRgb(Uint32 color) {
    Uint8 r = color & 0xFF;
    Uint8 g = (color >> 8) & 0xFF;
    Uint8 b = (color >> 16) & 0xFF;

    battleBackColor = ((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3);
    battleBackColorPending = TRUE;
}
