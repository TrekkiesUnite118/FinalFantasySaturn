#ifndef SPRITE_H
#define SPRITE_H

#include <sega_def.h>
#include <sega_mth.h>

#define MIRROR_HORIZ (1 << 4)
#define MIRROR_VERT (1 << 5)

struct SpriteInfo;

typedef void (*IterateFunc)(struct SpriteInfo *);

//if the sprite shouldn't be displayed
#define OPTION_NODISP (1 << 0)
//if we're on a slope
#define OPTION_SLOPE (1 << 1)

#define SPRITE_GRAVITY (MTH_FIXED(0.5))
#define SPRITE_MAXFALLSPEED (MTH_FIXED(14))
#define CURSOR_PRIORITY (3)
#define EFFECT_PRIORITY (2)
#define CHARACTER_PRIORITY (1)
#define WEAPON_PRIORITY (0)

typedef struct SpriteInfo {
    Uint16 char_num; //tile number
    Uint16 index; //where the sprite is in the sprites array
    Uint16 options;
    Uint16 state;
    Uint16 collision;
    Fixed32 xPos;
    Fixed32 yPos;
    Fixed32 xSize;
    Fixed32 ySize;
    Fixed32 dx;
    Fixed32 dy;
    Fixed32 scale;
    Fixed32 angle;
    Sint32 priority;
    Uint16 mirror;
    Uint16 animTimer; //timer for animations
    Uint16 moveTimer; //timer for movements
    Uint16 animCursor; //where we are in animation array
    IterateFunc iterate;
} SPRITE_INFO;

Uint16 total_sprites;
Uint16 cursorSpriteIndex;
Uint16 weaponSpriteIndex;
Uint16 explosionSpriteIndex;
Uint16 mapSpriteStart;
Uint16 mapSpriteCramStart;

#define SPRITE_LIST_SIZE (20)
extern SPRITE_INFO sprites[];

//sets up initial sprite display
void sprite_init(void);
//automatically picks the simplest SBL function for drawing the sprite depending
//on required features
//needs command to be opened before calling
void sprite_draw(SPRITE_INFO *info);
//inits the SPRITE_INFO pointer given
void sprite_make(int tile_num, Fixed32 x, Fixed32 y, Sint32 priority, SPRITE_INFO *ptr);
//draws all sprites in the sprite list
void sprite_draw_all(void);
//gets a pointer to the next sprite in the list
SPRITE_INFO *sprite_next(void);
//deletes the given sprite from the sprite list
void sprite_delete(SPRITE_INFO *sprite);
//deletes all sprites from the list
void sprite_deleteall(void);
//utility function that moves a sprite based on its state
//collision: 1 to do collision, 0 to not
//returns the tile the sprite is over
Uint16 sprite_move(SPRITE_INFO *sprite, int collision);

void initBattleSpriteData(Uint8 *patternData);
void initMapSpriteData(Uint8 *patternData);
void initWeaponSpriteData(Uint8 *patternData);
void initCharBattlePalettes();
void initCursorPalette();

void updateWeaponPalettes(Uint8 paletteSeed, Uint8 isWeapon);

#endif
