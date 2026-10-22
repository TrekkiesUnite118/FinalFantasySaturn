/*******************************************************************
 *
 *                      World Minimap
 *
 * The NES world map screen, opened with L on the overworld. The map
 * is drawn a pixel row at a time like the NES, then the party's
 * position and the map markers are shown as blinking sprites.
 *
 *
*******************************************************************/
#ifndef MINIMAP_H
#define MINIMAP_H

#include "types.h"

/**
 * @brief Sets up the minimap screen and starts drawing the map.
 *
 * The music waits until the drawing is done.
 */
void initMinimap(void);

/**
 * @brief Runs one frame of the minimap. Called each frame in the MINIMAP state.
 *
 * A, B or C closes it once the map is drawn.
 */
void doMinimap(void);

/**
 * @brief Draws the party and marker sprites once they're shown.
 */
void minimap_draw(void);

/**
 * @brief Copies the tiles changed this frame into VRAM. Call this from V-blank in.
 */
void minimap_vblank_in(void);

#endif
