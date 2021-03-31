#include "battle.h"
#include "print.h"

#define ENEMY_NAME_LOOKUP_TABLE (0x0022D4F0)
#define ENEMY_NAME_PTR_ADD (0x00224010)
#define ENEMY_DATA_TABLE (0x00230530)
#define BATTLE_DATA_TABLE (0x0022C410)

#define UPPER_CASE_LETTER_VALUE (0x8A)
#define UPPER_CASE_ASCII_DIFF (0x49)
#define NUMERICAL_SYMBOL_VALUE (0x80)
#define NUMERICAL_ASCII_DIFF (0x50)
#define LOWER_CASE_LETTER_VALUE (0xA4)
#define LOWER_CASE_ASCII_DIFF (0x43)



void battle_init(int id) {
    parseBattleData(&battle, id);

    parseEnemyData(&enemies[0], battle.enemy_id_0);
    parseEnemyName(&enemies[0], battle.enemy_id_0);
    parseEnemyData(&enemies[1], battle.enemy_id_1);
    parseEnemyName(&enemies[1], battle.enemy_id_1);
    parseEnemyData(&enemies[2], battle.enemy_id_2);
    parseEnemyName(&enemies[2], battle.enemy_id_2);
    parseEnemyData(&enemies[3], battle.enemy_id_3);
    parseEnemyName(&enemies[3], battle.enemy_id_3);

}

void parseEnemyData(ENEMY_DATA *enemy, int id) {

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
}

void parseEnemyName(ENEMY_DATA *enemy, int id) {
    
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
        
        enemy->name[i] = name_char;
        i++;
        current_char = enemy_name_ptr[i];
    }

    for(;i < 8; i++) {
        enemy->name[i] = 0x20;
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

    battle->surprise_rate = battle_ptr[13];
    Uint8 b14 = battle_ptr[14];
    Uint8 palette_assignment = b14 >> 4;
    Uint8 runFlag = b14 & 0x01;

    battle->palette_assignment = palette_assignment;
    battle->no_run = runFlag;
    battle->min_max_en0_b = battle_ptr[15];
    battle->min_max_en1_b = battle_ptr[16];

}

void enemy_bat_draw() {

	print_string(enemies[0].name, 19, 6, 8);
	print_string(enemies[1].name, 21, 6, 8);
	print_string(enemies[2].name, 23, 6, 8);
	print_string(enemies[3].name, 25, 6, 8);
	
}
