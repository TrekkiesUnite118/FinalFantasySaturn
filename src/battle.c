
#include <sega_spr.h>
#include <sega_scl.h>
#include "battle.h"
#include "print.h"

#define ENEMY_NAME_LOOKUP_TABLE (0x0022D4F0)
#define ENEMY_NAME_PTR_ADD (0x00224010)
#define ENEMY_DATA_TABLE (0x00230530)
#define BATTLE_DATA_TABLE (0x0022C410)
#define BATTLE_RNG_TABLE (0x0023FD01)
#define BATTLE_PALETTE_TABLE (0x00230F30)

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

Uint32 pal_0[] = {
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
};
Uint32 pal_1[] = {
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
};

int numOfSmallEnemySlots = 0;
int numOfLargeEnemySlots = 0;
int numOfFiendSlots = 0;
int numOfChaosSlots = 0;

int numOfSmallEnemySlotsAvailable = 9;
int numOfLargeEnemySlotsAvailable = 4;
int numOfFiendSlotsAvailable = 1;
int numOfChaosSlotsAvailable = 1;

int enemy0_num = 0;
int enemy1_num = 0;
int enemy2_num = 0;
int enemy3_num = 0;

char enemy_name_0[8];
char enemy_name_1[8];
char enemy_name_2[8];
char enemy_name_3[8];

int picassignment[4];
int palassignment[4];

Uint8 enemyNums[9];

Uint8 battle_rng_seed = 0;
int rtc_offset = 0;
int rng_init = 0;
int start_index = 0;

static Uint32	PadWorkArea[7];

void battle_init(int id) {
    start_index = 0;

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

    initPalette(&pal_0, battle.palette_id_0);
    initPalette(&pal_1, battle.palette_id_1);

    int pic = 0;
	for(int co = 1, co2 = 128; co <= 128; co <<= 1, co2 >>= 1) {
		if(battle.pic_assignment & co2) {
            pic |= co;
        } 
    }
   
    for(int i = 3; i >= 0; i--, pic >>= 2) {
		picassignment[i] = pic & 3;
    }

    int pal = battle.palette_assignment;
    for(int j = 3; j >= 0; j--, pal >>= 1) {
		palassignment[j] = (pal & 1) == 1;
    }

    if(battle.isFormation2) {
        enemy0_num = rand_min_max(battle.min_max_en0_b >> 4, battle.min_max_en0_b & 0x0F);
        enemy1_num = rand_min_max(battle.min_max_en1_b >> 4, battle.min_max_en1_b & 0x0F);
        parseBattleGroup(enemy0_num, picassignment[0], palassignment[0], battle.enemy_id_0);
        parseBattleGroup(enemy1_num, picassignment[1], palassignment[1], battle.enemy_id_1);
        
        parseEnemyName(&enemy_name_0, battle.enemy_id_0);
        parseEnemyName(&enemy_name_1, battle.enemy_id_1);

    } else {
        enemy0_num = rand_min_max(battle.min_max_en0_a >> 4, battle.min_max_en0_a & 0x0F);
        enemy1_num = rand_min_max(battle.min_max_en1_a >> 4, battle.min_max_en1_a & 0x0F);
        enemy2_num = rand_min_max(battle.min_max_en2_a >> 4, battle.min_max_en2_a & 0x0F);
        enemy3_num = rand_min_max(battle.min_max_en3_a >> 4, battle.min_max_en3_a & 0x0F);
        parseBattleGroup(enemy0_num, picassignment[0], palassignment[0], battle.enemy_id_0);
        parseBattleGroup(enemy1_num, picassignment[1], palassignment[1], battle.enemy_id_1);
        parseBattleGroup(enemy2_num, picassignment[2], palassignment[2], battle.enemy_id_2);
        parseBattleGroup(enemy3_num, picassignment[3], palassignment[3], battle.enemy_id_3);

        parseEnemyName(&enemy_name_0, battle.enemy_id_0);
        parseEnemyName(&enemy_name_1, battle.enemy_id_1);
        parseEnemyName(&enemy_name_2, battle.enemy_id_2);
        parseEnemyName(&enemy_name_3, battle.enemy_id_3);
    } 

    /**
     * 
     * This loads in the Enemy Pattern Table depending on the battle we encounter.
     * Each table has 2 small enemies 2 large enemies and 2 diffrent palettes. 
     * 
     * Yes this is hideous copy/pasta, but I'm awful at C and Pointers adding loops to this made it confusing.
     * 
     * ...don't judge me....
     * 
     **/
    switch(battle.pattern_selection) {
        case 0:
            initSmallEnemySprites(&small_0_name, enemyCramStart, enemySpriteStart);
            SCL_SetColRam(SCL_SPR, enemyCramStart, 16, &pal_0);
            initSmallEnemySprites(&small_0_name, enemyCramStart + 16, enemySpriteStart + 2);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 16, 16, &pal_1);
            initLargeEnemySprites(&large_0_name, enemyCramStart + 32, enemySpriteStart + 4);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 32, 16, &pal_0);
            initLargeEnemySprites(&large_0_name, enemyCramStart + 48, enemySpriteStart + 6);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 48, 16, &pal_1);
            break;
        case 1:
            initSmallEnemySprites(&small_1_name, enemyCramStart, enemySpriteStart);
            SCL_SetColRam(SCL_SPR, enemyCramStart, 16, &pal_0);
            initSmallEnemySprites(&small_1_name, enemyCramStart + 16, enemySpriteStart + 2);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 16, 16, &pal_1);
            initLargeEnemySprites(&large_1_name, enemyCramStart + 32, enemySpriteStart + 4);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 32, 16, &pal_0);
            initLargeEnemySprites(&large_1_name, enemyCramStart + 48, enemySpriteStart + 6);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 48, 16, &pal_1);
            break;
        case 2:
            initSmallEnemySprites(&small_2_name, enemyCramStart, enemySpriteStart);
            SCL_SetColRam(SCL_SPR, enemyCramStart, 16, &pal_0);
            initSmallEnemySprites(&small_2_name, enemyCramStart + 16, enemySpriteStart + 2);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 16, 16, &pal_1);
            initLargeEnemySprites(&large_2_name, enemyCramStart + 32, enemySpriteStart + 4);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 32, 16, &pal_0);
            initLargeEnemySprites(&large_2_name, enemyCramStart + 48, enemySpriteStart + 6);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 48, 16, &pal_1);
            break;            
        case 3:
            initSmallEnemySprites(&small_3_name, enemyCramStart, enemySpriteStart);
            SCL_SetColRam(SCL_SPR, enemyCramStart, 16, &pal_0);
            initSmallEnemySprites(&small_3_name, enemyCramStart + 16, enemySpriteStart + 2);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 16, 16, &pal_1);
            initLargeEnemySprites(&large_3_name, enemyCramStart + 32, enemySpriteStart + 4);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 32, 16, &pal_0);
            initLargeEnemySprites(&large_3_name, enemyCramStart + 48, enemySpriteStart + 6);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 48, 16, &pal_1);
            break;
        case 4:
            initSmallEnemySprites(&small_4_name, enemyCramStart, enemySpriteStart);
            SCL_SetColRam(SCL_SPR, enemyCramStart, 16, &pal_0);
            initSmallEnemySprites(&small_4_name, enemyCramStart + 16, enemySpriteStart + 2);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 16, 16, &pal_1);
            initLargeEnemySprites(&large_4_name, enemyCramStart + 32, enemySpriteStart + 4);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 32, 16, &pal_0);
            initLargeEnemySprites(&large_4_name, enemyCramStart + 48, enemySpriteStart + 6);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 48, 16, &pal_1);
            break;    
        case 5:
            initSmallEnemySprites(&small_5_name, enemyCramStart, enemySpriteStart);
            SCL_SetColRam(SCL_SPR, enemyCramStart, 16, &pal_0);
            initSmallEnemySprites(&small_5_name, enemyCramStart + 16, enemySpriteStart + 2);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 16, 16, &pal_1);
            initLargeEnemySprites(&large_5_name, enemyCramStart + 32, enemySpriteStart + 4);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 32, 16, &pal_0);
            initLargeEnemySprites(&large_5_name, enemyCramStart + 48, enemySpriteStart + 6);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 48, 16, &pal_1);
            break;    
        case 6:
            initSmallEnemySprites(&small_6_name, enemyCramStart, enemySpriteStart);
            SCL_SetColRam(SCL_SPR, enemyCramStart, 16, &pal_0);
            initSmallEnemySprites(&small_6_name, enemyCramStart + 16, enemySpriteStart + 2);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 16, 16, &pal_1);
            initLargeEnemySprites(&large_6_name, enemyCramStart + 32, enemySpriteStart + 4);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 32, 16, &pal_0);
            initLargeEnemySprites(&large_6_name, enemyCramStart + 48, enemySpriteStart + 6);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 48, 16, &pal_1);
            break;    
        case 7:
            initSmallEnemySprites(&small_7_name, enemyCramStart, enemySpriteStart);
            SCL_SetColRam(SCL_SPR, enemyCramStart, 16, &pal_0);
            initSmallEnemySprites(&small_7_name, enemyCramStart + 16, enemySpriteStart + 2);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 16, 16, &pal_1);
            initLargeEnemySprites(&large_7_name, enemyCramStart + 32, enemySpriteStart + 4);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 32, 16, &pal_0);
            initLargeEnemySprites(&large_7_name, enemyCramStart + 48, enemySpriteStart + 6);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 48, 16, &pal_1);
            break;    
        case 8:
            initSmallEnemySprites(&small_8_name, enemyCramStart, enemySpriteStart);
            SCL_SetColRam(SCL_SPR, enemyCramStart, 16, &pal_0);
            initSmallEnemySprites(&small_8_name, enemyCramStart + 16, enemySpriteStart + 2);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 16, 16, &pal_1);
            initLargeEnemySprites(&large_8_name, enemyCramStart + 32, enemySpriteStart + 4);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 32, 16, &pal_0);
            initLargeEnemySprites(&large_8_name, enemyCramStart + 48, enemySpriteStart + 6);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 48, 16, &pal_1);
            break;
        case 9:
            initSmallEnemySprites(&small_9_name, enemyCramStart, enemySpriteStart);
            SCL_SetColRam(SCL_SPR, enemyCramStart, 16, &pal_0);
            initSmallEnemySprites(&small_9_name, enemyCramStart + 16, enemySpriteStart + 2);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 16, 16, &pal_1);
            initLargeEnemySprites(&large_9_name, enemyCramStart + 32, enemySpriteStart + 4);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 32, 16, &pal_0);
            initLargeEnemySprites(&large_9_name, enemyCramStart + 48, enemySpriteStart + 6);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 48, 16, &pal_1);
            break;
        case 10:
            initSmallEnemySprites(&small_10_name, enemyCramStart, enemySpriteStart);
            SCL_SetColRam(SCL_SPR, enemyCramStart, 16, &pal_0);
            initSmallEnemySprites(&small_10_name, enemyCramStart + 16, enemySpriteStart + 2);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 16, 16, &pal_1);
            initLargeEnemySprites(&large_10_name, enemyCramStart + 32, enemySpriteStart + 4);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 32, 16, &pal_0);
            initLargeEnemySprites(&large_10_name, enemyCramStart + 48, enemySpriteStart + 6);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 48, 16, &pal_1);
            break;
        case 11:
            initSmallEnemySprites(&small_11_name, enemyCramStart, enemySpriteStart);
            SCL_SetColRam(SCL_SPR, enemyCramStart, 16, &pal_0);
            initSmallEnemySprites(&small_11_name, enemyCramStart + 16, enemySpriteStart + 2);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 16, 16, &pal_1);
            initLargeEnemySprites(&large_11_name, enemyCramStart + 32, enemySpriteStart + 4);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 32, 16, &pal_0);
            initLargeEnemySprites(&large_11_name, enemyCramStart + 48, enemySpriteStart + 6);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 48, 16, &pal_1);
            break;
        case 12:
            initSmallEnemySprites(&small_12_name, enemyCramStart, enemySpriteStart);
            SCL_SetColRam(SCL_SPR, enemyCramStart, 16, &pal_0);
            initSmallEnemySprites(&small_12_name, enemyCramStart + 16, enemySpriteStart + 2);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 16, 16, &pal_1);
            initLargeEnemySprites(&large_12_name, enemyCramStart + 32, enemySpriteStart + 4);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 32, 16, &pal_0);
            initLargeEnemySprites(&large_12_name, enemyCramStart + 48, enemySpriteStart + 6);
            SCL_SetColRam(SCL_SPR, enemyCramStart + 48, 16, &pal_1);
            break;

    }

}

void parseBattleGroup(int numOfEnemies, int picNum, int palNum, int enemyId) {

    int isLarge = 0;
    if(picNum > 1) {
        isLarge = 1;
    }

    for(int i = 0; i < numOfEnemies; i++, start_index++) {
        if(isLarge) {
            if(numOfLargeEnemySlotsAvailable > 0) {
                parseEnemyData(&enemies[start_index], enemyId, picNum, palNum);
                numOfLargeEnemySlotsAvailable--;
            }
        } else {
            if(numOfSmallEnemySlotsAvailable > 0) {
                parseEnemyData(&enemies[start_index], enemyId, picNum, palNum);
                numOfSmallEnemySlotsAvailable--;
            }
        }
    }

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
    Uint8 agility = statPtr[8];
    Uint8 def = statPtr[9];
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
    enemy->agility = agility;
    enemy->defense = def;
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
        char name_char = 0x20;
        if(current_char >= UPPER_CASE_LETTER_VALUE && current_char < LOWER_CASE_LETTER_VALUE) {
             name_char = current_char - UPPER_CASE_ASCII_DIFF;
        } else if(current_char >= LOWER_CASE_LETTER_VALUE) {
            name_char = current_char - LOWER_CASE_ASCII_DIFF;
        }
        
        enemy[i] = name_char;
        i++;
        current_char = enemy_name_ptr[i];
    }

    for(;i < 8; i++) {
        enemy[i] = 0x20;
    }

}

void parseBattleData(BATTLE_DATA *battle, int id) {
    if(id >= 0x80) {
        battle->isFormation2 = 1;
        id = id - 0x80;
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
    //I don't think I'll use this.
    battle->pic_assignment = battle_ptr[1];

    battle->enemy_id_0 = battle_ptr[2];
    battle->enemy_id_1 = battle_ptr[3];
    battle->enemy_id_2 = battle_ptr[4];
    battle->enemy_id_3 = battle_ptr[5];
    battle->min_max_en0_a = battle_ptr[6];
    battle->min_max_en1_a = battle_ptr[7];
    battle->min_max_en2_a = battle_ptr[8];
    battle->min_max_en3_a = battle_ptr[9];
    //Again probably wont use these palette values.
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

    Uint16 startPosY = MTH_FIXED(72);
    Uint16 startPosX = MTH_FIXED(40);
    Uint8 sizeToSkip = 0;

    
    int spriteIndex = 0;

    for(i = 0; i < enemy0_num; i++) {
        enemyNums[spriteIndex] = 0;
        spriteIndex++;
    }

    for(i = 0; i < enemy1_num; i++) {
        enemyNums[spriteIndex] = 1;
        spriteIndex++;
    }

    for(i = 0; i < enemy2_num; i++) {
        enemyNums[spriteIndex] = 2;
        spriteIndex++;
    }

    for(i = 0; i < enemy3_num; i++) {
        enemyNums[spriteIndex] = 3;
        spriteIndex++;
    }

    SPRITE_INFO enemySprite0;
    SPRITE_INFO enemySprite1;
    SPRITE_INFO enemySprite2;
    SPRITE_INFO enemySprite3;
    SPRITE_INFO enemySprite4;
    SPRITE_INFO enemySprite5;
    SPRITE_INFO enemySprite6;
    SPRITE_INFO enemySprite7;
    SPRITE_INFO enemySprite8;

    SPRITE_INFO *spritePtrs[9] = {&enemySprite0, &enemySprite1, &enemySprite2, &enemySprite3, &enemySprite4, &enemySprite5,
                                    &enemySprite6, &enemySprite7, &enemySprite8};

    int yChange = 0;
    int xChange = 0;
    int totalEnemies = 0;

    if(battle.battle_type == 0) {
    
        /**
         * Battle Arrangement is like this:
         * 
         * SMALL2    SMALL5    SMALL8
         * SMALL1    SMALL4    SMALL7
         * SMALL3    SMALL6    SMALL9
         * 
         **/

        numOfSmallEnemySlots = 9;

        totalEnemies = numOfSmallEnemySlots - numOfSmallEnemySlotsAvailable;
        
        for(i = 0; i < totalEnemies; i++) {
            createEnemySprite(spritePtrs[i], enemyNums[i], startPosX, startPosY);

            SPRITE_INFO temp;
            memcpy(&temp, spritePtrs[i], sizeof(temp));
            temp.xPos = MTH_FIXED(40 + xChange);
            temp.yPos = MTH_FIXED(72 - yChange);
            
            if(i == 2 || i == 5 ) {
                temp.yPos = MTH_FIXED(104);
                yChange = 0;
                xChange += 32;;
            } else {
                yChange = 32;
            }
            sprite_draw(&temp);
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


        numOfLargeEnemySlots = 4;
        totalEnemies = numOfLargeEnemySlots - numOfLargeEnemySlotsAvailable;
        yChange = 0;
        xChange = 0;


        for(i = 0; i < totalEnemies; i++) {
            createEnemySprite(spritePtrs[i], enemyNums[i], startPosX, startPosY);

            SPRITE_INFO temp;
            memcpy(&temp, spritePtrs[i], sizeof(temp));
            temp.xPos = MTH_FIXED(40 + xChange);
            temp.yPos = MTH_FIXED(40 + yChange);
            
            if(i == 2 || i == 5 ) {
                temp.yPos = MTH_FIXED(40);
                yChange = 0;
                xChange += 48;
            } else {
                yChange = 48;
            }
            sprite_draw(&temp);
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

        numOfSmallEnemySlots = 6;
        numOfLargeEnemySlots = 2;
        int totalLargeEnemies = numOfLargeEnemySlots - (numOfLargeEnemySlotsAvailable - 2);
        int totalSmallEnemies = numOfSmallEnemySlots - (numOfSmallEnemySlotsAvailable - 3);
        yChange = 0;
        xChange = 0;
        totalEnemies = totalLargeEnemies + totalSmallEnemies;

        Uint8 smallEnemyNums[9];
        Uint8 largeEnemyNums[9];
        int largeIndex = 0;
        int smallIndex = 0;
        spriteIndex = 0;
        for(int j = 0; j < 9; j++) {
                if(picassignment[enemyNums[j]] > 1) {
                    largeEnemyNums[largeIndex] = enemyNums[j];
                    largeIndex++;
                } else {
                    smallEnemyNums[smallIndex] = enemyNums[j];
                    smallIndex++;
                }
        }

        for(i = 0; i < totalLargeEnemies; i++) {
            createEnemySprite(spritePtrs[spriteIndex], largeEnemyNums[i], startPosX, startPosY);
            
            SPRITE_INFO temp;
            memcpy(&temp, spritePtrs[spriteIndex], sizeof(temp));
            temp.xPos = MTH_FIXED(40);
            temp.yPos = MTH_FIXED(40 + yChange);
            yChange = 48;
            
            sprite_draw(&temp);
            spriteIndex++;
        }
        yChange = 0;
        xChange = 0;


        for(i = 0; i < totalSmallEnemies; i++) {
            createEnemySprite(spritePtrs[spriteIndex], smallEnemyNums[i], startPosX, startPosY);

            SPRITE_INFO temp;
            memcpy(&temp, spritePtrs[spriteIndex], sizeof(temp));
            temp.xPos = MTH_FIXED(88 + xChange);
            temp.yPos = MTH_FIXED(72 - yChange);
            
            if(i == 2 || i == 5 ) {
                temp.yPos = MTH_FIXED(104);
                yChange = 0;
                xChange += 32;;
            } else {
                yChange = 32;
            }
            sprite_draw(&temp);
            spriteIndex++;
        }
    }

    if(battle.battle_type == 3) {
        numOfFiendSlots = 1;
    }
    if(battle.battle_type == 4) {
        numOfChaosSlots = 1;
    }


    int startYText = 19;

    if(enemy0_num > 0) {
        print_string(enemy_name_0, startYText, 6, 8);
        startYText +=2;
    }
    if(enemy1_num > 0) {
        print_string(enemy_name_1, startYText, 6, 8);
        startYText +=2;
    } else {
        print_string("        ", startYText, 6, 8);
        startYText++;
    }
    if(enemy2_num > 0) {
        print_string(enemy_name_2, startYText, 6, 8);
        startYText +=2;
    } else {
        print_string("        ", startYText, 6, 8);
        startYText++;
    }
    if(enemy3_num > 0) {
        print_string(enemy_name_3, startYText, 6, 8);
        startYText +=2;
    } else {
        print_string("        ", startYText, 6, 8);
        startYText++;
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

void initPalette(Uint32 *palette, Uint8 pal_id) {
    int pal_index = pal_id * 4;

    int pal_offset = BATTLE_PALETTE_TABLE + pal_index;

    Uint8 *palPtr = (Uint8*) pal_offset;

    palette[0] = nes_pal[palPtr[0]];
    palette[1] = nes_pal[palPtr[1]];
    palette[2] = nes_pal[palPtr[2]];
    palette[3] = nes_pal[palPtr[3]];
}

void createEnemySprite(SPRITE_INFO *enemySprite, int enemy_num, int xPos, int yPos) {

    /**
     * Enemy sprites are organized as follows:
     * Small 1, Small2, 
     * Small 1 Palete 2, Small 2 Palette 2,
     * Large 1, Large 2,
     * Large 1 Palette 1, Large 2 Palette 2.
     *  
     */

    if(picassignment[enemy_num] > 1) {
        sprite_make(enemySpriteStart + 4 +(palassignment[enemy_num] * 2) + (picassignment[enemy_num] - 2), xPos,  yPos, enemySprite);
    } else {
        sprite_make(enemySpriteStart + (palassignment[enemy_num] * 2) + picassignment[enemy_num], xPos,  yPos, enemySprite);
    }
    
    if(picassignment[enemy_num] > 1) {
        enemySprite->xSize = MTH_FIXED(48);
        enemySprite->ySize = MTH_FIXED(48);
    } else {
        enemySprite->xSize = MTH_FIXED(32);
        enemySprite->ySize = MTH_FIXED(32);
    }
   
}


