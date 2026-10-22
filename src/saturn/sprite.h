/*******************************************************************
 *
 *                      VDP1 Sprites
 *
 * A wrapper around SBL's VDP1 library: sprite characters, normal,
 * scaled, distorted and polygon draws, and SPRITE_INFO, which picks
 * the simplest draw for its scale and angle.
 *
 * Draws go between frameBegin and frameEnd.
 *
*******************************************************************/
#ifndef SPRITE_H
#define SPRITE_H

#include "types.h"

//Values for mirror.
#define MIRROR_HORIZ (1 << 4)
#define MIRROR_VERT (1 << 5)

//Same values as SBL's COLOR_0, ECDSPD_DISABLE and COMPO_REP.
#define SPRITE_4BPP (0x0000)
#define SPRITE_ECDSPD_OFF (0x00c0)
#define SPRITE_REPLACE (0x0000)

struct SpriteInfo;

typedef void (*IterateFunc)(struct SpriteInfo *);

//Unused, through SPRITE_MAXFALLSPEED.
//if the sprite shouldn't be displayed
#define OPTION_NODISP (1 << 0)
//if we're on a slope
#define OPTION_SLOPE (1 << 1)

#define SPRITE_GRAVITY (FIXED(0.5))
#define SPRITE_MAXFALLSPEED (FIXED(14))

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

//Same layout as SBL's XyInt.
typedef struct {
    Sint16 x;
    Sint16 y;
} SprPoint;

/**
 * @brief Sets up the VDP1 work area (SPR_2Initial).
 */
void sprInit(void);

/**
 * @brief Registers a sprite character.
 *
 * @param num Character number.
 * @param colorMode Color mode (SPRITE_4BPP).
 * @param color Sprite CRAM bank offset.
 * @param w Width in pixels.
 * @param h Height in pixels.
 * @param data Pixel data.
 */
void sprSetChar(Uint16 num, Uint16 colorMode, Uint16 color, Uint16 w, Uint16 h, Uint8 *data);

/**
 * @brief Draws a character at its own size.
 *
 * @param priority Draw priority.
 * @param mirror Mirror bits (MIRROR_).
 * @param colorMode Color mode (SPRITE_4BPP).
 * @param color Sprite CRAM bank offset, or 0xffff for the character's registered palette.
 * @param num Character number.
 * @param xy xy[0] is the top left.
 */
void sprNormal(Sint32 priority, Uint16 mirror, Uint16 colorMode, Uint16 color, Uint16 num, SprPoint *xy);

/**
 * @brief Draws a character stretched to a rectangle.
 *
 * @param priority Draw priority.
 * @param mirror Mirror bits (MIRROR_).
 * @param colorMode Color mode (SPRITE_4BPP).
 * @param color Sprite CRAM bank offset, or 0xffff for the character's registered palette.
 * @param num Character number.
 * @param xy xy[0] top left, xy[1] bottom right.
 */
void sprScaled(Sint32 priority, Uint16 mirror, Uint16 colorMode, Uint16 color, Uint16 num, SprPoint *xy);

/**
 * @brief Draws a character stretched to four corners.
 *
 * @param priority Draw priority.
 * @param mirror Mirror bits (MIRROR_).
 * @param colorMode Color mode (SPRITE_4BPP).
 * @param color Sprite CRAM bank offset, or 0xffff for the character's registered palette.
 * @param num Character number.
 * @param xy xy[0..3] clockwise from top left.
 */
void sprDistorted(Sint32 priority, Uint16 mirror, Uint16 colorMode, Uint16 color, Uint16 num, SprPoint *xy);

/**
 * @brief Draws a solid polygon.
 *
 * @param priority Draw priority.
 * @param drawMode Draw mode (SPRITE_ECDSPD_OFF, SPRITE_REPLACE).
 * @param color Color.
 * @param xy xy[0..3] clockwise from top left.
 */
void sprPolygon(Sint32 priority, Uint16 drawMode, Uint16 color, SprPoint *xy);

/**
 * @brief Draws a sprite with its character's registered palette.
 *
 * Uses a normal draw at scale 1 and angle 0, a scaled draw at angle 0,
 * and a distorted draw rotated around the sprite's center otherwise.
 *
 * @param info Sprite.
 */
void sprite_draw(SPRITE_INFO *info);

/**
 * @brief Draws a sprite like sprite_draw, with a given palette.
 *
 * @param info Sprite.
 * @param color Sprite CRAM bank offset, or 0xffff for the character's registered palette.
 */
void sprite_draw_pal(SPRITE_INFO *info, Uint16 color);

/**
 * @brief Sets up a sprite at scale 1, angle 0, not mirrored.
 *
 * xSize and ySize aren't set.
 *
 * @param tile_num Character number.
 * @param x Screen X.
 * @param y Screen Y.
 * @param priority Draw priority.
 * @param ptr Sprite to set up.
 */
void sprite_make(int tile_num, Fixed32 x, Fixed32 y, Sint32 priority, SPRITE_INFO *ptr);

#endif
