/*******************************************************************
 *
 *                      Player And Game State
 *
 * The party, the game state, button input, the overworld player and
 * vehicles, and the party's side of battles: commands, turns, sprites,
 * transitions and the battle end sequences.
 *
 * Also new game setup and soft reset.
 * 
 * Yes I realize this is kind of sloppy. Originally this just handled
 * the player input and sprite movement logic on the main map but
 * as the game evolved more player input and state stuff kept getting
 * shoved in here. Could probably be refactored and cleaned up.
 *
*******************************************************************/
#ifndef PLAYER_H
#define PLAYER_H
#include "types.h"
#include "sprdata.h"
#include "romaddr.h"

//Map sprite halves: player is the top, playerb the bottom.
extern SPRITE_INFO player;
extern SPRITE_INFO playerb;
extern SPRITE_INFO cursor;

#define PLAYER_SPRITE_X (FIXED(144))
#define PLAYER_SPRITE_Y (FIXED(96))

#define FALSE 0
#define TRUE 1

//Game states, for state.
enum	__gamestate__ {
	OW_STATE,
	TO_BAT_STATE,
	BAT_STATE,
	TO_OW_STATE,
    TITLE_STATE,
    CHAR_SELECT_STATE,
    CHAR_NAME_STATE,
    TO_TITLE_STATE,
    TO_CHAR_SELECT_STATE,
    TO_CHAR_NAME_STATE,
    TO_BAT_MENU_STATE,
    BAT_MENU_STATE,
    TO_BAT_EXECUTE_STATE,
    BAT_EXECUTE_STATE,
    TO_BAT_VICTORY_STATE,
    BAT_VICTORY_STATE,
    TO_BAT_RUN_AWAY,
    BAT_RUN_AWAY,
    TO_GAME_OVER,
    GAME_OVER,
    GAME_RESTART,
    TO_SAD,
    SAD,
    TO_PRELUDE,
    PRELUDE,
    BAT_FADE_IN,
    BAT_DRAW_STATUS,
    BAT_FADE_OUT,
    BAT_TRANSITION,
    BAT_INIT_MSG,
    BAT_CHAOS_EXIT,
    OW_PAL_CYCLE,
    TO_MAIN_MENU,
    MAIN_MENU,
    TO_LINEUP_MENU,
    LINEUP_MENU,
    TO_MINIMAP,
    MINIMAP,
    TO_STATUS_MENU,
    STATUS_MENU,
    TO_MAGIC_MENU,
    MAGIC_MENU,
    TO_EQUIP_MENU,
    EQUIP_MENU,
    TO_ITEM_MENU,
    ITEM_MENU,
    TO_SM_STATE,
    SM_STATE,
    TO_SHOP,
    SHOP,
    WIPE,
    SM_PAL_CYCLE,
    TO_SAVE_MENU,
    SAVE_MENU,
    TO_STORY,
    STORY,
    TO_MINIGAME,
    MINIGAME,
    TO_BOOT_MSG,
    BOOT_MSG,
    TO_BUGFIX_MENU,
    BUGFIX_MENU,
    TO_GFX_MENU,
    GFX_MENU
};

//A party member's stats outside battle.
typedef struct {
    Uint8 char_class; //Character Class
    Uint8 ailment; //Character Ailments
    Uint8 name[7]; //Character Name
    Uint32 exp;
    Uint16 current_hp; //Current Health Points
    Uint16 max_hp; //Maximum Health Points
    Uint8 str; //Strength Points
    Uint8 agi; //Agility Points
    Uint8 intel; //Intelligence
    Uint8 vit; //Vitality Points
    Uint8 luck; //Luck
    Uint16 exp_to_next_lvl; 
    Uint8 weapons[4]; //Each Character can hold 4 weapons
    Uint8 armors[4]; //Each character can hold 4 armors
    Uint8 damage; //Damage
    Uint8 hit_percent; //Hit Percent
    Uint8 absorb; //Absorb
    Uint8 evade_percent; //Evade Percent
    Uint8 resist; //Resistence
    Uint8 mag_def; //Magic Defense
    Uint8 level;
    Uint8 current_mp[8]; //Current MP Per 8 Levels
    Uint8 max_mp[8]; //Max MP Per 8 Levels
    Uint8 magic[8][3]; //8 Magic Levels with 3 slots each
    
} PLAYER_CHARACTER;

PLAYER_CHARACTER party[4];
Uint32 partyGold;
Uint8 items[0x1C]; //NES items: key items $01-$11, orbs $12-$15, then quantities from $16.
Uint8 gameFlags[0x100]; //NES game_flags, indexed by object ID.
Uint8 shipVisible;
Uint8 shipX;
Uint8 shipY;
Uint8 airshipVisible; //Counts up to $1F while the Floater raises it.
Uint8 airshipX;
Uint8 airshipY;
Uint8 bridgeVisible;
Uint8 bridgeX;
Uint8 bridgeY;
Uint8 canalVisible; //Nonzero while the canal is blocked.
Uint8 canalX;
Uint8 canalY;
Uint8 hasCanoe;
Uint8 bridgeScene; //NES bridgescene: 1 once the bridge is crossed, $80 after the scene.
Uint8 state;
Uint8 currentBackDrop;
Uint8 owWipeIn; //Enter the overworld with the NES screen wipe.

Uint8 drawBattleCursor; //Set while the battle cursor is shown.
Uint8 drawTargetNames;

/**
 * @brief Sets up the cursor sprite.
 *
 * On the party creation screen the position comes from the current slot instead.
 *
 * @param startX Screen X.
 * @param startY Screen Y.
 */
void player_cursor_init(Fixed32 startX, Fixed32 startY);

/**
 * @brief Draws the cursor sprite.
 */
void player_cursor_draw(void);

/**
 * @brief Draws the respond rate number on the title screen.
 */
void draw_response_rate(void);

/**
 * @brief Draws the name being entered on the name entry screen.
 */
void draw_char_name(void);

/**
 * @brief Loads the map sprite and palettes of the first living party member's class.
 */
void update_player_sprites();

/** * 
 * @brief Sets up the map sprites and puts the party at the start position.
 * 
 * @param startX Player starting X Position
 * @param startY plaeyr starting Y Position.
 */
void player_ow_init(Fixed32 startX, Fixed32 startY);

/**
 * @brief Sets up the party's battle sprites and clears their commands.
 */
void player_bat_init();

/**
 * @brief Reads the pad and handles button presses for the current state.
 *
 * A+B+C+Start goes to the CD player from the boot messages, prelude,
 * title and bad ROM screen, and soft resets anywhere else.
 */
void player_input(void);

/**
 * @brief Scrolls the overworld to the party and draws the party, vehicles, bridge and canal.
 */
void player_ow_draw(void);

/**
 * @brief Sets up the party on the overworld once the map is ready (NES PrepOverworld).
 */
void player_ow_enter(void);

/**
 * @brief Runs one frame of an overworld palette cycle (NES CyclePalettes).
 */
void doOwPalCycle(void);

/**
 * @brief Leaves a menu or battle, back to the standard map or the overworld it came from.
 */
void exitOwMenu(void);

/**
 * @brief Sets the map sprite frame (NES lut_PlayerMapmanSprTbl).
 *
 * @param facing NES direction bits: 1 right, 2 left, 4 down, 8 up.
 * @param anim TRUE for the second walk frame.
 */
void setMapmanSprite(Uint8 facing, Uint8 anim);

/**
 * @brief Puts the party on an overworld tile.
 *
 * @param x Map tile X.
 * @param y Map tile Y.
 */
void setOWPosition(Uint8 x, Uint8 y);

/**
 * @brief Gets the party's vehicle.
 *
 * @return Vehicle (VEH_).
 */
Uint8 getOWVehicle(void);

/**
 * @brief Sets the party's vehicle.
 *
 * @param v Vehicle (VEH_).
 */
void setOWVehicle(Uint8 v);

//Party's overworld tile.
extern Uint8 current_x_pos;
extern Uint8 current_y_pos;

/**
 * @brief Draws a party member's menu sprite (NES DrawOBSprite).
 *
 * Dead members aren't drawn. Poisoned members crouch, and stoned members
 * crouch in the stone palette.
 *
 * @param sprite Sprite to draw with.
 * @param member Party member.
 * @param x Screen X.
 * @param y Screen Y.
 */
void drawOBSprite(SPRITE_INFO *sprite, Uint8 member, Uint16 x, Uint16 y);

/**
 * @brief Sets up the lineup screen's sprites, names and cursor.
 */
void initLineupMenu(void);

/**
 * @brief Runs one frame of the lineup screen.
 */
void doLineupMenu(void);

/**
 * @brief Draws the lineup screen's sprites and cursor.
 */
void lineup_draw(void);

/**
 * @brief Draws the party's battle sprites, weapon and explosions.
 */
void player_bat_draw(void);

/**
 * @brief Sets up a party member with a class's starting stats.
 *
 * @param classValue Class.
 * @param partyNum Party member.
 * @param newName Name, 6 characters and a 0.
 */
void char_init(int classValue, int partyNum, char newName[6]);

/**
 * @brief Sets the party's gold to the new game amount.
 */
void initPartyGold();

/**
 * @brief Sets the items, game flags and vehicles to their new game values.
 */
void initPartyItems();


/**
 * @brief Runs one frame of a party member's battle sprite walking left.
 *
 * Moves the battle on once the walk is done.
 *
 * @param partyNum Party member.
 */
void moveBattlePlayerLeft(int partyNum);

/**
 * @brief Runs one frame of a party member's battle sprite walking back right.
 *
 * Moves the battle on once the walk is done.
 *
 * @param partyNum Party member.
 */
void moveBattlePlayerRight(int partyNum);

/**
 * @brief Runs one frame of command input.
 */
void doBattleMenu();

/**
 * @brief Runs one frame of the round's turns.
 */
void doBattleExec();

/**
 * @brief Moves dead, then stoned, then poisoned members to the back (NES ReSortPartyByAilment).
 *
 * Like the NES, a bad swap can scramble the order of the others.
 */
void resortPartyByAilment();

/**
 * @brief Starts the transition into a battle.
 */
void startBattleTransition();

/**
 * @brief Runs one frame of the battle transition (NES BattleTransition).
 */
void doBattleTransition();

/**
 * @brief Runs one frame of the battle fade in or out (NES BattleFadeIn/BattleFadeOut).
 */
void doBattleFade();

/**
 * @brief Draws one party member's status box per frame after the fade in.
 */
void doBattleDrawStatus();

/**
 * @brief Starts the battle fade out.
 *
 * @param nextState State after the fade.
 */
void startBattleFadeOut(Uint8 nextState);

/**
 * @brief Gets the special bits of the overworld tile the party is on (NES tileprop_now).
 *
 * @return Tile property bits $C0.
 */
Uint8 getOWTileSpecial();

/**
 * @brief Rolls whether the party is surprised or strikes first.
 *
 * Neither happens in a no run battle.
 */
void determineSurprise();

/**
 * @brief Copies the party's stats into battle_char_stats.
 */
void copyPlayerStatsToBattleStats();

/**
 * @brief Copies the battle results back to the party.
 *
 * Only dead, stone or poison stay after the battle.
 */
void copyBattleStatsToPlayerStats();

//Phases for initBattleEnd.
enum __battleendphase__ {
    BATTLE_END_CHEER,
    BATTLE_END_WAIT_INPUT, //Input, then respond delay.
    BATTLE_END_DELAY,
    BATTLE_END_DELAY_INPUT, //Respond delay, then input.
    BATTLE_END_INPUT,
    BATTLE_END_RUN_FLIP,
    BATTLE_END_CHAOS_WAIT,
    BATTLE_END_CHAOS_DISSOLVE,
    BATTLE_END_CHAOS_PAUSE,
    BATTLE_END_CHAOS_EXIT
};

enum __victorystep__ {
    VICTORY_PERISHED,
    VICTORY_REWARDS,
    VICTORY_STAT_UP,
    VICTORY_LEVEL_DONE
};

/**
 * @brief Sets the living party members to the cheer or stand frame.
 *
 * @param cheer Nonzero to cheer.
 */
void party_cheer_pose(Uint8 cheer);

/**
 * @brief Starts a battle end sequence.
 *
 * @param phase First phase (BATTLE_END_).
 */
void initBattleEnd(Uint8 phase);

/**
 * @brief Runs one frame of the victory, run away or game over sequence.
 */
void doBattleEnd();

/**
 * @brief Sets the living party members to the stand frame.
 */
void player_bat_stand();

/**
 * @brief Sets a party member's battle sprite to the crouch frame.
 *
 * @param partyNum Party member.
 */
void player_crouch(int partyNum);

/**
 * @brief Sets a party member's battle sprite to the stand frame, unless isMoving is set.
 *
 * @param partyNum Party member.
 */
void player_stand(int partyNum);

/**
 * @brief Sets a party member's battle sprite to the dead pose.
 *
 * @param partyNum Party member.
 */
void player_dead(int partyNum);

/**
 * @brief Clears every party member's command.
 */
void resetPlayerActions();

/**
 * @brief Mirrors a party member's battle sprite.
 *
 * @param partyNum Party member.
 */
void player_flip(int partyNum);

/**
 * @brief Mirrors the living party members' battle sprites.
 */
void party_flip();

/**
 * @brief Sets party members with 0 HP to the dead pose.
 */
void party_dead();

/**
 * @brief Crouches the party members isPlayerCrouching picks.
 */
void party_crouch();

/**
 * @brief Puts party creation back on the first slot.
 */
void reset();

/**
 * @brief Sets up a new game's party, gold, items and position.
 */
void newGameInit(void);

//Set by player_input. A soft reset waits for the main loop.
extern volatile Uint8 softResetPending;

/**
 * @brief Turns off the screens and sound and goes back to the prelude with new game data.
 */
void softReset(void);

/**
 * @brief Picks the map sprite of the first living party member's class.
 */
void determine_curr_char();

/**
 * @brief Gets the first living party member's class.
 *
 * @return Class, or 0 if everyone is dead.
 */
Uint8 determine_curr_char_class();

/**
 * @brief Builds the two map sprite palettes.
 *
 * @param palette_a Set to the first palette.
 * @param palette_b Set to the second palette.
 * @param palPtr NES sprite palette.
 * @param palPtr2 Class colors, one for each palette.
 */
void initMapManPalette(Uint32 *palette_a, Uint32 *palette_b, Uint8 *palPtr, Uint8 *palPtr2);

/**
 * @brief Handles A or C for the current state.
 */
void performActionButtonInput();

/**
 * @brief Handles B for the current state.
 */
void performCancelButtonInput();

/**
 * @brief Gets the respond rate setting.
 *
 * @return Respond rate, 0-7.
 */
Uint8 getResponseRate();

//NES respond rate delays in frames, by respond rate.
extern Uint8 *response_rate_lut;

//VRAM row the Chaos dissolve clears at the next V-blank, 0 for none.
extern volatile Uint32 *chaosEraseAddr;

/**
 * @brief Places an explosion sprite at random on a small enemy.
 *
 * @param slotNum Enemy slot.
 * @param spriteSlot Explosion sprite, 0-2.
 */
void drawSmallFormationExplosion(Uint8 slotNum, Uint8 spriteSlot);

/**
 * @brief Places an explosion sprite at random on a small enemy in a mix formation.
 *
 * @param slotNum Enemy slot.
 * @param spriteSlot Explosion sprite, 0-2.
 */
void drawSmallMixFormationExplosion(Uint8 slotNum, Uint8 spriteSlot);

/**
 * @brief Places an explosion sprite at random on a large enemy.
 *
 * @param slotNum Enemy slot.
 * @param spriteSlot Explosion sprite, 0-2.
 */
void drawLargeFormationExplosion(Uint8 slotNum, Uint8 spriteSlot);

/**
 * @brief Gets the number of 3-sprite explosion rounds drawn on an enemy (NES DrawEnemyEffect).
 *
 * @param enemyIndex Enemy.
 * @return Number of rounds.
 */
Uint8 getExplosionCount(Uint8 enemyIndex);

/**
 * @brief Places an explosion sprite at a random point in an area, with a random frame.
 *
 * @param spriteSlot Explosion sprite, 0-2.
 * @param minX Left edge.
 * @param maxX Right edge.
 * @param minY Top edge.
 * @param maxY Bottom edge.
 */
void drawExplosions(Uint8 spriteSlot, Uint8 minX, Uint8 maxX, Uint8 minY, Uint8 maxY);

#endif
