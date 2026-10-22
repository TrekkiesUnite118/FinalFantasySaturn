/*******************************************************************
 *
 *                      Overworld Screen
 *
 * The overworld map on rotation layer RBG0, which can tilt for the
 * perspective view. NBG3 is the sky and haze shown past the horizon.
 *
 * Tiles are stored in VRAM bank A0. Map data is in bank A1.
 *
*******************************************************************/
#ifndef RBGSCROLL_H
#define RBGSCROLL_H

//Unused.
#define TILE_SIZE 16
//maximum # tiles i'll show onscreen
#define SCREEN_TILES_X 28
#define SCREEN_TILES_Y 20
#define BG_TILES 32

#include "datatypes.h"
#include "graphicrefs.h"

//Scroll numbers for scroll_move and scroll_set. Only SCROLL_BACKGROUND1 (the overworld) is used.
#define SCROLL_PLAYFIELD (0)
#define SCROLL_FOREGROUND (1)
#define SCROLL_BACKGROUND1 (2)
#define SCROLL_BACKGROUND2 (3)

//Scroll positions by scroll number.
extern Fixed32 scrolls_x[];
extern Fixed32 scrolls_y[];

/**
 * @brief Sets up RBG0 with the overworld's tiles, palettes and map, and the sky on NBG3.
 */
void scroll_init(void);

/**
 * @brief Moves a scroll by the given amounts.
 *
 * @param num Scroll number (SCROLL_).
 * @param x Pixels to move across.
 * @param y Pixels to move down.
 */
void scroll_move(int num, Fixed32 x, Fixed32 y);

/**
 * @brief Tilts the overworld map around the X axis for the perspective view.
 *
 * Also moves the sky to the new horizon.
 *
 * @param rotateAngle Angle in degrees. 0 is flat.
 */
void scroll_rotate_x(Fixed32 rotateAngle);

/**
 * @brief Finds where a map point shows on the tilted screen.
 *
 * @param x Map X relative to the scroll position.
 * @param y Map Y relative to the scroll position.
 * @param sx Set to the screen X.
 * @param sy Set to the screen Y.
 * @param scale Set to the screen pixels per map pixel at that point.
 * @return FALSE if the point is past the horizon.
 */
Uint8 scroll_project(Fixed32 x, Fixed32 y, Fixed32 *sx, Fixed32 *sy, Fixed32 *scale);

/**
 * @brief Restarts the sky's haze table. Call this at V-blank out.
 */
void scroll_vblank_out();

/**
 * @brief Moves a scroll to the given position.
 *
 * @param num Scroll number (SCROLL_).
 * @param x Position across.
 * @param y Position down.
 */
void scroll_set(int num, Fixed32 x, Fixed32 y);

/**
 * @brief Hides the overworld map and the sky.
 */
void scroll_disable();

/**
 * @brief Reloads the map palettes with NES color emphasis applied.
 *
 * @param emphasis Emphasis bits, as for emphasizeColor.
 */
void scroll_set_emphasis(Uint8 emphasis);

/**
 * @brief Starts the NES palette cycle on the map's colors.
 *
 * @param reverse FALSE to cycle out to black, TRUE to cycle in to the map's colors.
 */
void scroll_pal_cycle_init(Uint8 reverse);

/**
 * @brief Steps every map color one step of the palette cycle.
 *
 * Call scroll_pal_cycle_draw to show the result.
 *
 * @param reverse Same as given to scroll_pal_cycle_init.
 * @return Number of colors changed. 0 once the cycle is done.
 */
Uint8 scroll_pal_cycle_step(Uint8 reverse);

/**
 * @brief Loads the palette cycle's current colors into color RAM.
 */
void scroll_pal_cycle_draw();

#endif
