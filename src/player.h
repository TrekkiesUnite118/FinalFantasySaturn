#ifndef PLAYER_H
#define PLAYER_H
#include "sprite.h"

extern SPRITE_INFO player;
extern SPRITE_INFO playerb;

#define PLAYER_SPRITE_X (MTH_FIXED(144))
#define PLAYER_SPRITE_Y (MTH_FIXED(96))

#define MAPMAN_SPRITE_OFFSET (0x00209010)
#define BATTLE_SPRITE_OFFSET (0x00225010)
#define WEAPON_SPRITE_OFFSET (0x00226810)
#define MAPMAN_SPRITE_PALETTE_OFFSET (0x002003A0)
#define MAPMAN_SPRITE_PALETTE_OFFSET_2 (0x002003B0)
#define LUT_SMALL_FORMATION_EXPLOSION_LUT (0x00233D2C)
#define LUT_LARGE_FORMATION_EXPLOSION_LUT (0x00233D92)
#define LUT_MIX_SMALL_FORMATION_EXPLOSION_LUT (0x00233DDC)

#define BATTLE_CHAR_DEFAULT_XPOS (0x00231751)
#define BATTLE_CHAR_DEFAULT_YPOS_1 (0x0023175F)

#define FALSE 0
#define TRUE 1

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
    PRELUDE
};

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
Uint8 state;
Uint8 currentBackDrop;

Uint8 drawBattleCursor;
Uint8 drawTargetNames;

void player_cursor_init(Fixed32 startX, Fixed32 startY);
void player_cursor_draw(void);
void draw_response_rate(void);
void draw_char_name(void);
//void player_select_init();
void update_player_sprites();
void player_ow_init();
void player_bat_init();
void player_animate(void);
void player_input(void);
void player_ow_draw(void);
void player_bat_draw(void);
//void player_select_draw(void);

void char_init(int classValue, int partyNum, char newName[6]);

void movePlayerLeft();
void movePlayerRight();
void movePlayerUp();
void movePlayerDown();

void moveBattlePlayerLeft(int partyNum);
void moveBattlePlayerRight(int partyNum);

void player_animate_battle_walk(int partyNum);

void doBattleMenu();
void doBattleExec();

Uint8 canMove(int);
Uint8 inForest();
int doBattle();

void determineSurprise();

void copyPlayerStatsToBattleStats();

void copyBattleStatsToPlayerStats();

void applyEquippedWeapon(Uint8 index);

void applyEquippedArmor(Uint8 index);

void party_animate_battle_cheer();

void player_bat_stand();

void player_crouch(int partyNum);

void player_stand(int partyNum);

void player_dead(int partyNum);

void player_flip(int partyNum);

void party_flip();

void party_dead();

void party_crouch();

void reset();

void determine_curr_char(); 

Uint8 determine_curr_char_class();

void initMapManPalette(Uint32 *palette_a, Uint32 *palette_b, Uint8 *palPtr, Uint8 *palPtr2);

void performActionButtonInput();

void performCancelButtonInput();

Uint8 getResponseRate();

void drawSmallFormationExplosion(Uint8 slotNum);

void drawSmallMixFormationExplosion(Uint8 slotNum);

void drawLargeFormationExplosion(Uint8 slotNum);

void drawExplosions(Uint8 minX, Uint8 maxX, Uint8 minY, Uint8 maxY);

#endif
