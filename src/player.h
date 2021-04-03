#ifndef PLAYER_H
#define PLAYER_H
#include "sprite.h"

extern SPRITE_INFO player;
extern SPRITE_INFO playerb;

#define PLAYER_SPRITE_X (MTH_FIXED(144))
#define PLAYER_SPRITE_Y (MTH_FIXED(96))

#define FALSE (MTH_FIXED(0))
#define TRUE (MTH_FIXED(1))

#define OW_STATE (MTH_FIXED(0x01))
#define TO_BAT_STATE (MTH_FIXED(0x02))
#define BAT_STATE (MTH_FIXED(0x03))
#define TO_OW_STATE (MTH_FIXED(0x04))
#define TITLE_STATE (MTH_FIXED(0x05))
#define CHAR_SELECT_STATE (MTH_FIXED(0x06))
#define CHAR_NAME_STATE (MTH_FIXED(0x07))
#define TO_TITLE_STATE (MTH_FIXED(0x08))
#define TO_CHAR_SELECT_STATE (MTH_FIXED(0x09))
#define TO_CHAR_NAME_STATE (MTH_FIXED(0x0A))

typedef struct {
    Uint8 isAlive; //Is the character alive?
    Uint8 useMagic; //Is the character able to use Magic?
    Uint8 char_class; //Character Class
    Uint16 current_hp; //Current Health Points
    Uint16 max_hp; //Maximum Health Points
    Uint16 str; //Strength Points
    Uint16 vit; //Vitality Points
    Uint16 agi; //Agility Points
    Uint16 unknown; //unknown
    Uint16 hit_percent; //Hit Percent
    Uint16 evade_percent; //Evade Percent
    Uint16 luck; //Luck
    Uint16 hit_percent_up; //Hit Percent Up
    Uint16 damage; //Damage
    Uint16 mag_def; //Magic Defense
    Uint16 intel; //Intelligence
    Uint8 level;
    Uint32 exp;
    Uint32 exp_to_next_lvl; 
    Uint8 current_mp[8]; //Current MP Per 8 Levels
    Uint8 max_mp[8]; //Max MP Per 8 Levels
    Uint8 weapons[4]; //Each Character can hold 4 weapons
    Uint8 armors[4]; //Each character can hold 4 armors
    Uint8 magic[8][3]; //8 Magic Levels with 3 slots each
    char name[6]; //Character Name
    
} PLAYER_CHARACTER;

PLAYER_CHARACTER party[4];
int state;
int currentBackDrop;

void player_cursor_init(Fixed32 startX, Fixed32 startY);
void player_cursor_draw(void);
void draw_response_rate(void);
void draw_char_name(void);
void player_select_init();
void player_ow_init();
void player_bat_init();
void player_animate(void);
void player_input(void);
void player_ow_draw(void);
void player_bat_draw(void);
void player_select_draw(void);

void char_init(int classValue, int partyNum, char newName[6]);

void movePlayerLeft();
void movePlayerRight();
void movePlayerUp();
void movePlayerDown();

int canMove(int);
int inForest();
int doBattle();

#endif
