#ifndef BATTLE_H
#define BATTLE_H
#include "datatypes.h"

#include <sega_def.h>
#include <sega_mth.h>
#include <sega_per.h>
#include "sprite.h"
#include "graphicrefs.h"

#define BATTLE_MENU_MAP_LOCATION (0x280000)
#define BATTLE_INVENTORY_MAP_LOCATION (0x2801E0)
#define BATTLE_MSG_MAP_LOCATION (0x280398)
#define BATTLE_TGT_MSG_MAP_LOCATION (0x280458)
#define BATTLE_ACT_MSG_MAP_LOCATION (0x2804A8)
#define BATTLE_CLEAR_MAP_LOCATION (0x280500)
#define BATTLE_ACT_MSG2_MAP_LOCATION (0x280710)
#define BATTLE_ORDER_DEFAULT_LOCATION (0x23216C)
#define WEAPON_LUT_LOCATION (0x230010)
#define ARMOR_LUT_LOCATION (0x230150)
#define MAGIC_LUT_LOCATION (0x2301F0)

#define AIL_DEAD (0x01)
#define AIL_STONE (0x02)
#define AIL_POISON (0x04)
#define AIL_DARK (0x08)
#define AIL_STUN (0x10)
#define AIL_SLEEP (0x20)
#define AIL_MUTE (0x040
#define AIL_CONF (0x80)

#define BTLMSG_HPUP             (0x01)
#define BTLMSG_ASLEEP           (0x04)

#define BTLMSG_SIGHTRECOVERED   (0x06)   // is this ever used?
#define BTLMSG_SILENCED         (0x07)

#define BTLMSG_DARKNESS         (0x09)

#define BTLMSG_NEUTRALIZED      (0x0E)

#define BTLMSG_BREAKSILENCE     (0x11)   // is this ever used??

#define BTLMSG_CONFUSED         (0x13)
#define BTLMSG_POISONED         (0x14)

#define BTLMSG_PARALYZED_A      (0x17)

#define BTLMSG_BROKENTOPIECES   (0x1A)

#define BTLMSG_SLAIN            (0x20)
#define BTLMSG_INEFFECTIVE      (0x21)
#define BTLMSG_STRIKEFIRST      (0x22)
#define BTLMSG_SURPRISED        (0x23)
#define BTLMSG_CANTRUN          (0x24)
#define BTLMSG_RUNAWAY          (0x25)
#define BTLMSG_CLOSECALL        (0x26)
#define BTLMSG_WOKEUP           (0x27)
#define BTLMSG_SLEEPING         (0x28)
#define BTLMSG_CURED            (0x29)
#define BTLMSG_PARALYZED_B      (0x2A)
#define BTLMSG_HITS             (0x2B)
#define BTLMSG_CRITICALHIT      (0x2C)

#define BTLMSG_DMG              (0x2E)
#define BTLMSG_STOPPED          (0x2F)

#define BTLMSG_STR              (0x33)
#define BTLMSG_AGI              (0x34)
#define BTLMSG_INT              (0x35)
#define BTLMSG_VIT              (0x36)
#define BTLMSG_LUCK             (0x37)
#define BTLMSG_UP               (0x38)

#define BTLMSG_PERISHED         (0x3C)
#define BTLMSG_MONSTERS_        (0x3D)
#define BTLMSG__PARTY           (0x3E)
#define BTLMSG_TERMINATED       (0x3F)
#define BTLMSG_MISSED           (0x40)

#define BTLMSG_INEFFECTIVENOW   (0x4A)

#define BTLMSG_NOTHINGHAPPENS   (0x4E)

enum	__battlemenustate__ {
    INIT,
    SELECT,
    TO_SELECT_ENEMY_TARGET,
    TO_SELECT_MAGIC,
    SELECT_MAGIC,
    SELECT_ENEMY_TARGET,
    SELECT_PARTY_TARGET,
    END_SELECT,
    SELECT_CANCEL,
    END
};

enum	__battleturnexecstate__ {
    BETWEEN_TURN,
    EXEC_INIT,
    ANIMATE1,
    CALCULATE,
    ANIMATE2,
    PREP_EXPLOSION,
    EXPLOSION,
    SCREEN_SHAKE,
    RESULT,
    RESULT_2,
    FINISH_TURN,
    EXEC_END,
    DELETING_ENEMY,
    ENEMY_DELETED,
    WAIT
};

enum __battlemenuoptions__ {
    FIGHT,
    MAGIC,
    DRINK,
    ITEM,
    RUN
};

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
    Uint8 evade;
    Uint8 absorb;
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
    Uint8 ailment;
    Uint8 name[8]; 
} ENEMY_DATA;

typedef struct {
    Uint8 slot_index; 
    Uint8 char_class; //Character Class
    Uint8 level;
    Uint8 luck; //Luck
    Uint8 ailment; //Character Ailments 
    Uint16 hp; //Health Points
    Uint8 hit_percent; //Hit Percent
    Uint8 mag_def; //Magic Defense
    Uint8 evade_percent; //Evade Percent
    Uint8 absorb; //Absorb
    Uint8 damage; //Damage
    Uint8 elem_resist; //Elemental Resistence
    Uint8 numhitsmult; //Number of Hits Multiplier
    Uint8 numhits; // Number of Hits
    Uint8 category; // Category - Always 0
    Uint8 elem_weak; // Elemental Weakness - Always 0
    Uint8 crit_rate; // Critical Hit Rate
    Uint8 wep_gfx; // Weapon Graphic Id
    Uint8 wep_pal; // Weapon Palette.
    Uint8 name[8];
    Uint8 current_mp[8]; //Current MP Per 8 Levels
    Uint8 max_mp[8]; //Max MP Per 8 Levels
    Uint8 magic[8][3]; //8 Magic Levels with 3 slots each
} BATTLE_CHARACTER;

typedef struct {
    Uint8 turnOwnerIndex; //Index of the turn owner 0-3 for players, 0-8 for enemies.
    Uint8 isPlayer; //Is this a player or an enemy turn.
    Uint8 targetIndex; //Index of the target. 0-3 for players, 0-8 for enemies.
    Uint8 targetIsPlayer; //Is the target a player or an enemy?
    Uint8 action; //Index for the action.
    Uint8 secondaryActionId; //If the action is Drink, Item, or Magic, this is the ID for what item or spell to use.
} TURN_ACTION;

ENEMY_DATA enemies[9];

BATTLE_DATA battle;

BATTLE_CHARACTER battle_char_stats[4];

Uint8 bat_menu_state;

Uint8 turn_exec_state;

TURN_ACTION playerActions[4];

Uint8 battleOrderBuffer[13];

Uint8 isSurprised;
Uint8 isPreemptive;

Uint8 curr_enemy_target;

Uint8 curr_attack_miss;
Uint16 curr_attack_dmg;

Uint8 slotOccupied[9];
Uint8 slotToEnemyNum[9];

Uint16 slotXCoords[9];
Uint16 slotYCoords[9];

void battle_init(Uint8 id);

void enemy_init(ENEMY_DATA *enemy);

void parseEnemyName(char *enemy, int id);

void parseEnemyData(ENEMY_DATA *enemy, int id, int picNum, int palNum);

void parseBattleData(BATTLE_DATA *battle, int id);

void parseBattleGroup(int numOfEnemies, int picNum, int palNum, int enemyId, int seed);

void enemy_bat_draw(void);

Uint8 rand_min_max(int min, int max);

Uint8 battleRNG(void);

void setRngSeed(void);

void createEnemySprite(SPRITE_INFO *enemySprite, Uint8 enemy_num, Uint16 xPos, Uint16 yPos) ;

void drawEnemyNames();

void drawInitMessage();

void clearMessageArea();

void drawMenu();

void drawMagicMenu(Uint8 playerIndex, Uint8 pageNum);

void determineTotalEnemies();

Uint8 getNextOccupiedSlot(Sint8 start);

Uint8 getPrevOccupiedSlot(Sint8 start);

Uint16 getXSlotCoord(Uint8 index);

Uint16 getYSlotCoord(Uint8 index);

void updatePlayerBattleAction(Uint8 partyNum, Uint8 SlotNum, Uint8 action, Uint8 subAction, Uint8 isPlayerTarget);

void determineBattleOrder();

void takePlayersTurn(Uint8 playerIndex);

void takeEnemyTurn(Uint8 enemyIndex);

void determineWeaponMagicPalette(Uint32 *palette, Uint8 pal_id, Uint8 isMagic);

Uint16 calculateBasePhysicalDamage(Uint8 attackerIndex);

void drawAttackerDefenderMessage(char * attacker, char * defender);

Uint8 determinePlayerTarget();
Uint8 isPlayerDead(Uint8 index);

Uint8 isPartyDead();

Uint8 isPlayerLowHealth(Uint8 index, Uint16 maxHp);

Uint8 allEnemiesDead();

Uint8 canRun(Uint8 index);

void drawAttemptRunMessage(char * attacker);

void drawLutAttackMessage(Uint8 msg_index);

void drawLutResultMessage(Uint8 msg_index);

void drawAttackMessage(Uint8 *msg);

void drawResultMessage(Uint8 *msg);

void drawRunSuccessMessage();

void drawRunFailedMessage();

void drawMonstersPerishedMessage();

void drawPartyPerishedMessage();

void waitForRespondRate();

void drawDmgString(Uint16 damage);

void deleteEnemy(Uint8 xPos, Uint8 yPos, Uint8 xy);

void setLastDeletedEnemyRow(Uint8 lastRow);

Uint8 getEnemySlotNum(Uint8 enemy);

void shakeScreen();

#endif
