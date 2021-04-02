#ifndef BATTLE_H
#define BATTLE_H
#include "datatypes.h"

#include <sega_def.h>
#include <sega_mth.h>
#include <sega_per.h>
#include "sprite.h"
#include "graphicrefs.h"

#define BATTLE_TYPE_3X3 (MTH_FIXED(0x00))
#define BATTLE_TYPE_2X2 (MTH_FIXED(0x01))
#define BATTLE_TYPE_MIX (MTH_FIXED(0x02))
#define BATTLE_TYPE_FIEND (MTH_FIXED(0x03))
#define BATTLE_TYPE_CHAOS (MTH_FIXED(0x04))

typedef struct {
    Uint8 isFormation2;
    Uint8 battle_type;
    Uint8 pattern_selection;
    Uint8 pic_assignment;
    Uint8 enemy_id_0;
    Uint8 enemy_id_1;
    Uint8 enemy_id_2;
    Uint8 enemy_id_3;
    Uint8 min_max_en0_a;
    Uint8 min_max_en1_a;
    Uint8 min_max_en2_a;
    Uint8 min_max_en3_a;
    Uint8 palette_id_0;
    Uint8 palette_id_1;
    Uint8 surprise_rate;
    Uint8 palette_assignment;
    Uint8 no_run;
    Uint8 min_max_en0_b;
    Uint8 min_max_en1_b;
} BATTLE_DATA;

typedef struct {
    Uint8 experience;
    Uint8 money_reward;
    Uint8 hp;
    Uint8 morale;
    Uint8 ai_id;
    Uint8 agility;
    Uint8 defense;
    Uint8 hit_num;
    Uint8 hit_rate;
    Uint8 strength;
    Uint8 crit_rate;
    Uint8 unknown;
    Uint8 atk_ailment;
    Uint8 category;
    Uint8 mag_def;
    Uint8 elem_weak;
    Uint8 elem_resist;
    Uint8 pic_num;
    Uint8 pal_num;
} ENEMY_DATA;

ENEMY_DATA enemies[9];

BATTLE_DATA battle;

void battle_init(int id);

void parseEnemyName(char *enemy, int id);

void parseEnemyData(ENEMY_DATA *enemy, int id, int picNum, int palNum);

void parseBattleData(BATTLE_DATA *battle, int id);

void parseBattleGroup(int numOfEnemies, int picNum, int palNum, int enemyId);

void enemy_bat_draw(void);

Uint8 rand_min_max(int min, int max);

Uint8 battleRNG(void);

void setRngSeed(void);

void initPalette(Uint32 *palette, Uint8 pal_id);

void createEnemySprite(SPRITE_INFO *enemySprite, int enemy_num, int xPos, int yPos) ;

#endif
