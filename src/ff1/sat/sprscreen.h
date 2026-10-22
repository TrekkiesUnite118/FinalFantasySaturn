/*******************************************************************
 *
 *                      Sprite Screen Setup
 *
 * Starts VDP1 and VDP2 and registers FF1's sprite characters and
 * sprite palettes.
 *
 *
 *
*******************************************************************/
#ifndef SPRSCREEN_H
#define SPRSCREEN_H

/**
 * @brief Initializes VDP2, VDP1 and the V-blank routines, then loads
 * the sprites.
 *
 * Registers the font, battle, weapon, cursor and explosion sprite
 * characters and loads the sprite palettes.
 * The converted sprite graphics must already be in LWRAM (see memmap.h).
 */
void sprite_init(void);

/**
 * @brief Switches sprites to type 5 with the sprite window enabled.
 */
void sprite_window_mode(void);

#endif
