/*******************************************************************
 *
 *                      FF1 Sprite Data
 *
 * FF1's sprite priorities, sprite palettes and the character numbers
 * where each group of sprites starts. Also converts the NES sprite
 * graphics into LWRAM (see memmap.h) for sprite_init to register.
 *
 *
*******************************************************************/
#ifndef SPRDATA_H
#define SPRDATA_H

#include "types.h"
#include "sprite.h"
#include "romaddr.h"

//Sprite priority numbers for sprite_make.
#define CURSOR_PRIORITY (3)
#define EFFECT_PRIORITY (2)
#define CHARACTER_PRIORITY (1)
#define WEAPON_PRIORITY (0)

#define STONE_PALETTE_CRAM (80)

//Sprite palettes, 16 colors each.
extern Uint32 player_pal_0[];
extern Uint32 player_pal_1[];
extern Uint32 weapon_pal[];
extern Uint32 cursor_pal_0[];
extern Uint32 stone_pal[];

//Sprite character numbers, set by sprite_init.
Uint16 total_sprites;
Uint16 cursorSpriteIndex;
Uint16 weaponSpriteIndex;
Uint16 explosionSpriteIndex;
Uint16 mapSpriteStart;
//First sprite color RAM entry for map sprite palettes.
Uint16 mapSpriteCramStart;

/**
 * @brief Converts the NES battle sprite graphics into LWRAM_BATTLE_SPRITES.
 *
 * @param patternData Pointer to the NES battle sprite CHR.
 */
void initBattleSpriteData(Uint8 *patternData);

/**
 * @brief Converts the NES map sprite graphics into LWRAM_MAP_SPRITES.
 *
 * @param patternData Pointer to the NES map sprite CHR.
 */
void initMapSpriteData(Uint8 *patternData);

/**
 * @brief Converts the NES weapon and magic sprite graphics into LWRAM_WEAPON_SPRITES.
 *
 * @param patternData Pointer to the NES weapon and magic sprite CHR.
 */
void initWeaponSpriteData(Uint8 *patternData);

/**
 * @brief Loads the party's battle sprite palettes and the stone palette from the ROM.
 */
void initCharBattlePalettes();

/**
 * @brief Loads the cursor palette from the ROM.
 */
void initCursorPalette();

/**
 * @brief Sets the weapon and magic sprite palette, like the NES UpdateVariablePalette.
 *
 * @param paletteSeed NES color for color 1. Colors 2 and 3 are $10 and $20 darker.
 * @param isWeapon FALSE makes color 3 white, for magic.
 */
void updateWeaponPalettes(Uint8 paletteSeed, Uint8 isWeapon);

#endif
