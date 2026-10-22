/*******************************************************************
 *
 *                      Standard Map Screen
 *
 * The VDP2 setup for towns, castles and dungeons. NBG0 is the
 * scrolling map of 16x16 tiles, NBG2 is the dialogue box layer.
 *
 * Also the altar effect's grey beam, which uses VDP2 hi-res mode.
 *
*******************************************************************/
#ifndef NBGSCROLL_H
#define NBGSCROLL_H

#include "types.h"

/**
 * @brief Sets up color RAM, VRAM and the NBG0 map and NBG2 dialogue layers.
 *
 * Builds the map tiles for cur_tileset and draws the whole map.
 *
 * @param reenter TRUE to show the map at once, FALSE to open it with a wipe.
 * @param wipeDone Called when the wipe finishes. Unused when reentering.
 */
void smScreenInit(Uint8 reenter, void (*wipeDone)(void));

/**
 * @brief Checks if the standard map layers are on.
 *
 * @return TRUE if NBG0 is displayed.
 */
Uint8 smScreenShown(void);

/**
 * @brief Scrolls the map layer. Call this from the V-blank handler.
 *
 * @param x Scroll X in pixels.
 * @param y Scroll Y in pixels.
 */
void smScreenScroll(Uint16 x, Uint16 y);

/**
 * @brief Drops a pending altar CRAM switch and puts CRAM back in its normal mode now.
 */
void smScreenCramReset(void);

/**
 * @brief Turns the altar beam's grey on or off.
 *
 * The color RAM switch happens at the next V-blank in.
 *
 * @param on TRUE to switch to hi-res with grey colors, FALSE to switch back.
 */
void smAltarGray(Uint8 on);

/**
 * @brief Undoes the altar effect's VDP2 setup at once, for entering a map mid-effect.
 */
void smAltarReset(void);

/**
 * @brief Sets the window for this frame's part of the altar beam.
 *
 * @param delay NES delay line count where the beam starts.
 * @param lines Number of beam lines.
 */
void smAltarDraw(Uint8 delay, Uint8 lines);

#endif
