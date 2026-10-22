/*******************************************************************
 *
 *                      Battle
 *
 * Battle setup from the ROM's formations, enemy placement, turn
 * order, physical attacks, enemy AI and the battle message boxes.
 * Follows the NES battle code, including its bugs unless a bug-fix
 * toggle is on.
 *
*******************************************************************/
#ifndef BATTLE_H
#define BATTLE_H
#include "datatypes.h"

#include "types.h"
#include "sprdata.h"
#include "graphicrefs.h"

//ROM addresses.
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
#define ENEMY_AI_LUT_LOCATION (0x231030)

//enemyChooseAiAction result when the enemy does a physical attack.
#define ENEMY_ACTION_PHYSICAL (0xFF)
//Special attack IDs are offset past the spells and the two potions.
#define ENEMY_SPECIAL_ATTACK_OFFSET (0x42)

//NES ailment bits.
#define AIL_DEAD (0x01)
#define AIL_STONE (0x02)
#define AIL_POISON (0x04)
#define AIL_DARK (0x08)
#define AIL_STUN (0x10)
#define AIL_SLEEP (0x20)
#define AIL_MUTE (0x40)
#define AIL_CONF (0x80)

#define CATEGORY_REGEN (0x80)

//NES battle message numbers.
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

//States for bat_menu_state.
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
    CANCEL_ACTION,
    MAGIC_NOTHING, //"Nothing" box shown until any button is pressed.
    SELECT_DRINK,
    SELECT_ITEM,
    END
};

//States for turn_exec_state.
enum	__battleturnexecstate__ {
    BETWEEN_TURN,
    EXEC_INIT,
    ANIMATE1,
    CALCULATE,
    ANIMATE2,
    PREP_EXPLOSION,
    EXPLOSION,
    SCREEN_SHAKE,
    FLASH_CHARACTER,
    RESULT,
    RESULT_2,
    FINISH_TURN,
    EXEC_END,
    DELETING_ENEMY,
    ENEMY_DELETED,
    WAIT,
    RESULT_CLEAR,
    ENEMY_MSG_CLEAR,
    ENEMY_ATTACKER_CLEAR,
    ENEMY_END_DELAY,
    WALK_BACK,
    MAGIC_EFFECT,
    DRAW_STATUS,
    ROUND_END
};

//Battle commands, for TURN_ACTION action.
enum __battlemenuoptions__ {
    FIGHT,
    MAGIC,
    DRINK,
    ITEM,
    RUN,
    NO_ACTION
};

//A battle formation from the ROM.
typedef struct {
    Uint8 isFormation2; //TRUE for the B formation (formation ID bit 7).
    Uint8 battle_type; //0: 9 small, 1: 4 large, 2: 6 small and 2 large, 3 and 4: fiend or Chaos.
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
    Uint16 experience;
    Uint16 money_reward;
    Uint16 hp;
    Uint16 hp_max;
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
    Uint8 ai_mag_pos; //Next AI spell slot.
    Uint8 ai_atk_pos; //Next AI special attack slot.
    Uint8 num_hits_mult;
    Uint8 rom_absorb; //Magic reads absorb from the ROM stats.
    Uint8 name[8];
} ENEMY_DATA;

typedef struct {
    Uint8 slot_index; 
    Uint8 char_class; //Character Class
    Uint8 level;
    Uint8 luck; //Luck
    Uint8 ailment; //Character Ailments
    Uint8 ib_ailment; //NES IB ailments, which magic doesn't update. Only the NES run bug reads them.
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
    Uint8 wep_element; //Equipped weapon's element, used by the Weapon Effects fix.
    Uint8 wep_category; //Equipped weapon's category, used by the Weapon Effects fix.
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
    Uint8 mpLevel; //Spell level whose MP a magic action uses.
    Uint8 itemId; //NES item ID used by an ITEM action, for its name.
} TURN_ACTION;

ENEMY_DATA enemies[9];

BATTLE_DATA battle;

BATTLE_CHARACTER battle_char_stats[4];

Uint8 bat_menu_state;

Uint8 turn_exec_state;

TURN_ACTION playerActions[4];

Uint8 battleOrderBuffer[13];

Uint8 isSurprised;
Uint8 battleFormation; //Formation ID, including the B formation bit.
Uint8 isPreemptive;

Uint8 curr_enemy_target;

Uint8 curr_attack_miss;
Uint16 curr_attack_dmg;
Uint8 curr_attack_ailments; //Defender ailments after the attack.
Uint8 curr_attack_ailment_death;
Uint8 curr_attack_hits;
Uint8 curr_attack_crits;

Uint8 flashCounter; //Frames left in the hit target's flash.

//Back screen color, written in vblank. Used for the magic flash.
volatile Uint16 battleBackColor;
volatile Uint8 battleBackColorPending;

//Frames each fade shade is shown.
#define BATTLE_FADE_STEP_FRAMES (3)
Uint8 battleFadeShade; //Shades currently faded out, 0-4.
Uint8 battleFadeFrame;
Uint8 battleFadeNextState;

Uint8 ailmentMsgs[8];
Uint8 ailmentMsgCount;
Uint8 ailmentMsgIndex;
Uint8 ailmentMsgShown;

Uint8 slotOccupied[9];
Uint8 slotToEnemyNum[9];

Uint16 slotXCoords[9];
Uint16 slotYCoords[9];

/**
 * @brief Sets up a battle: loads the formation, rolls and places the enemies, and loads their graphics.
 *
 * @param id Formation ID, with bit 7 set for the B formation.
 */
void battle_init(Uint8 id);

/**
 * @brief Clears an enemy's stats.
 *
 * @param enemy Enemy to clear.
 */
void enemy_init(ENEMY_DATA *enemy);

/**
 * @brief Copies an enemy's name from the ROM.
 *
 * @param enemy Set to the name, 8 bytes.
 * @param id Enemy ID.
 */
void parseEnemyName(Uint8 *enemy, int id);

/**
 * @brief Loads an enemy's stats from the ROM.
 *
 * @param enemy Enemy to fill.
 * @param id Enemy ID.
 * @param picNum Enemy graphic, as for drawEnemy.
 * @param palNum Enemy palette, 0 or 1.
 */
void parseEnemyData(ENEMY_DATA *enemy, int id, int picNum, int palNum);

/**
 * @brief Loads a formation from the ROM.
 *
 * @param battle Formation to fill.
 * @param id Formation ID, with bit 7 set for the B formation.
 */
void parseBattleData(BATTLE_DATA *battle, int id);

/**
 * @brief Rolls how many of an enemy group appear and places them in free slots.
 *
 * Mix formations skip the roll if there's no free slot of the group's size.
 *
 * @param minMax Minimum in the high nibble, maximum in the low nibble.
 * @param picNum Enemy graphic, as for drawEnemy.
 * @param palNum Enemy palette, 0 or 1.
 * @param enemyId Enemy ID.
 * @param group Group number, 0-3.
 * @return Number placed.
 */
Uint8 parseBattleGroup(Uint8 minMax, int picNum, int palNum, int enemyId, int group);

/**
 * @brief Draws the enemies in their slots and records each slot's position.
 */
void enemy_bat_draw(void);

/**
 * @brief Gets a random number in a range from battleRNG.
 *
 * @param min Lowest value.
 * @param max Highest value.
 * @return Random number from min to max.
 */
Uint8 rand_min_max(int min, int max);

/**
 * @brief Gets the next number from the ROM's battle random number table.
 *
 * @return Random number.
 */
Uint8 battleRNG(void);

/**
 * @brief Starts the RTC used for random seeds.
 *
 * The NES seeds from its RAM at power on, which the Saturn clears.
 */
void setRngSeed(void);

/**
 * @brief Gets a seed from the RTC.
 *
 * @return Seconds into the current hour, wrapped to a byte.
 */
Uint8 readRtcSeed(void);

/**
 * @brief Seeds battleRNG from the RTC.
 */
void seedBattleRng(void);

/**
 * @brief Draws the enemy group names in the enemy roster box.
 */
void drawEnemyNames();

/**
 * @brief Shows the surprised or strike first message, if the battle starts with one.
 */
void drawInitMessage();

/**
 * @brief Removes every message block, newest first.
 */
void clearMessageArea();

/**
 * @brief Removes the newest message block, the bottom message.
 */
void clearBottomMessage();

/**
 * @brief Checks if an enemy runs from the party.
 *
 * Runs if morale + rand[0, $32] - 2 * the leader's level is below $50.
 *
 * @param enemy Enemy.
 * @return TRUE if it runs.
 */
Uint8 enemyRunsAway(ENEMY_DATA *enemy);

/**
 * @brief Picks an enemy's action from its AI.
 *
 * @param enemy Enemy.
 * @return Spell ID, special attack ID + ENEMY_SPECIAL_ATTACK_OFFSET, or ENEMY_ACTION_PHYSICAL.
 */
Uint8 enemyChooseAiAction(ENEMY_DATA *enemy);

/**
 * @brief Draws the enemy roster and command boxes.
 */
void drawMenu();

/**
 * @brief Draws a page of a party member's spells (NES DrawBattleMagicBox).
 *
 * @param playerIndex Party member.
 * @param pageNum Page, 4 spell levels each.
 */
void drawMagicMenu(Uint8 playerIndex, Uint8 pageNum);

/**
 * @brief Draws the HEAL and PURE potion box (NES DrawDrinkBox).
 *
 * @param healCount HEAL potions. 0 leaves the row blank.
 * @param pureCount PURE potions. 0 leaves the row blank.
 */
void drawDrinkMenu(Uint8 healCount, Uint8 pureCount);

/**
 * @brief Draws a party member's weapons and armor (NES DrawBattleItemBox).
 *
 * @param playerIndex Party member.
 */
void drawItemMenu(Uint8 playerIndex);

/**
 * @brief Counts the placed enemies from the free slots.
 */
void determineTotalEnemies();

/**
 * @brief Finds the next occupied enemy slot, wrapping around.
 *
 * @param start Slot to start from.
 * @return Slot number.
 */
Uint8 getNextOccupiedSlot(Sint8 start);

/**
 * @brief Finds the previous occupied enemy slot, wrapping around.
 *
 * @param start Slot to start from.
 * @return Slot number.
 */
Uint8 getPrevOccupiedSlot(Sint8 start);

/**
 * @brief Gets an enemy slot's cursor X.
 *
 * @param index Slot number.
 * @return X in pixels.
 */
Uint16 getXSlotCoord(Uint8 index);

/**
 * @brief Gets an enemy slot's cursor Y.
 *
 * @param index Slot number.
 * @return Y in pixels.
 */
Uint16 getYSlotCoord(Uint8 index);

/**
 * @brief Records a party member's command for the round.
 *
 * @param partyNum Party member.
 * @param SlotNum Target: enemy slot, or party member.
 * @param action Command (FIGHT, MAGIC, ...).
 * @param subAction Spell or item for MAGIC, DRINK and ITEM.
 * @param isPlayerTarget TRUE if the target is a party member.
 */
void updatePlayerBattleAction(Uint8 partyNum, Uint8 SlotNum, Uint8 action, Uint8 subAction, Uint8 isPlayerTarget);

/**
 * @brief Shuffles the turn order of the 4 party members and 9 enemies into battleOrderBuffer.
 *
 * The NES does 16 random swaps. BUGFIX_TURN_SHUFFLE does a full shuffle.
 */
void determineBattleOrder();

/**
 * @brief Does a party member's physical attack on their target.
 *
 * @param playerIndex Party member.
 */
void takePlayersTurn(Uint8 playerIndex);

/**
 * @brief Does an enemy's physical attack on curr_enemy_target.
 *
 * @param enemyIndex Enemy.
 */
void takeEnemyTurn(Uint8 enemyIndex);

/**
 * @brief Shows the attacker and defender name boxes.
 *
 * @param attacker Attacker's name.
 * @param defender Defender's name.
 */
void drawAttackerDefenderMessage(char * attacker, char * defender);

/**
 * @brief Picks a party member for an enemy to attack.
 *
 * The first member is picked half the time, the second a quarter, the
 * last two an eighth each. Dead and stoned members are rerolled.
 *
 * @return Party member.
 */
Uint8 determinePlayerTarget();

/**
 * @brief Checks if a party member has 0 HP.
 *
 * @param index Party member.
 * @return TRUE if dead.
 */
Uint8 isPlayerDead(Uint8 index);

/**
 * @brief Checks if every party member is dead or stoned.
 *
 * @return TRUE if the party has lost.
 */
Uint8 isPartyDead();

/**
 * @brief Checks if a party member shows the crouching pose.
 *
 * Poisoned, stunned or asleep, or HP under a quarter of max. Like the
 * NES, only the low byte of the quarter is compared.
 *
 * @param index Party member.
 * @param maxHp Party member's max HP.
 * @return TRUE if crouching.
 */
Uint8 isPlayerCrouching(Uint8 index, Uint16 maxHp);

/**
 * @brief Checks if every enemy has 0 HP.
 *
 * @return TRUE if the party has won.
 */
Uint8 allEnemiesDead();

/**
 * @brief Rolls a party member's run attempt.
 *
 * Always works on a preemptive strike, never in a no run battle. The
 * NES reads the wrong level, unless BUGFIX_RUN_CHANCE is on.
 *
 * @param index Party member.
 * @return TRUE if the party gets away.
 */
Uint8 canRun(Uint8 index);

/**
 * @brief Shows the attacker name box.
 *
 * @param attacker Attacker's name.
 */
void drawAttackerName(char * attacker);

/**
 * @brief Checks if a party member can be given a command.
 *
 * @param index Party member.
 * @return FALSE if dead, stoned, stunned or asleep.
 */
Uint8 canInputCommand(Uint8 index);

/**
 * @brief Poison damage on the party and regeneration on enemies.
 */
void applyEndOfRoundEffects();

/**
 * @brief Shows a battle message in the result box.
 *
 * @param msg_index Message number (BTLMSG_).
 */
void drawLutResultMessage(Uint8 msg_index);

/**
 * @brief Shows a battle message in the bottom box.
 *
 * @param msg_indx Message number (BTLMSG_).
 */
void drawLutBottomdMessage(Uint8 msg_indx);

/**
 * @brief Shows a string in the attack box.
 *
 * @param msg String.
 */
void drawAttackMessage(Uint8 *msg);

/**
 * @brief Shows a string in the result box.
 *
 * @param msg String.
 */
void drawResultMessage(Uint8 *msg);

/**
 * @brief Shows "Close call" in the bottom box, after running away.
 */
void drawRunSuccessMessage();

/**
 * @brief Shows "Monsters perished" in the bottom box.
 */
void drawMonstersPerishedMessage();

/**
 * @brief Shows "<Name> party perished" in the bottom box, with the first party member's name.
 */
void drawPartyPerishedMessage();

/**
 * @brief Shows a string in one of the ROM's battle message boxes.
 *
 * @param boxIndex Box number in the ROM's box table.
 * @param text String.
 */
void drawCombatBoxText(Uint8 boxIndex, Uint8 *text);

/**
 * @brief Shows "N DMG" in the result box.
 *
 * @param damage Damage.
 */
void drawDmgString(Uint16 damage);

/**
 * @brief Shows "N Hits!" in the attack box.
 *
 * @param hits Number of hits.
 */
void drawHitsString(Uint8 hits);

/**
 * @brief Clears one row of an enemy graphic per call.
 *
 * @param xPos Enemy's left column.
 * @param yPos Enemy's top row.
 * @param xy Enemy's size in tiles.
 * @return TRUE once every row is cleared.
 */
Uint8 deleteEnemy(Uint8 xPos, Uint8 yPos, Uint8 xy);

/**
 * @brief Sets the row deleteEnemy clears next.
 *
 * @param lastRow Row, 0 to start over.
 */
void setLastDeletedEnemyRow(Uint8 lastRow);

/**
 * @brief Gets the slot an enemy is placed in.
 *
 * @param enemy Enemy number.
 * @return Slot number.
 */
Uint8 getEnemySlotNum(Uint8 enemy);

/**
 * @brief Runs one frame of the screen shake when an enemy hits.
 *
 * Moves the screen every other frame. Once frameCount passes 12 it goes
 * to the target's flash, or straight to the result if they were dead or stoned.
 */
void shakeScreen();

/**
 * @brief Copies the battle's NES palettes for fading.
 *
 * @param backdrop Backdrop number.
 */
void initBattleFadePalettes(Uint8 backdrop);

/**
 * @brief Fades the battle palettes out by some shades (NES FadeOutOneShade).
 *
 * @param shades Shades to fade, 0 for none.
 */
void setBattleShade(Uint8 shades);

/**
 * @brief Restores the sprite palettes after a fade out.
 *
 * Sprite palettes are only loaded at startup.
 */
void restoreBattleSpritePalettes();

/**
 * @brief Sets the back screen color, written at the next V-blank in.
 *
 * The back screen shows through color 0, like the NES BG color.
 *
 * @param nesColor NES color.
 */
void setBattleBackColor(Uint8 nesColor);

#endif
