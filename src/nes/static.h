/*******************************************************************
 * 
 *                      Static NBG Screen
 * 
 * This creates a static nonscrolling  4bpp screen on NBG1. 
 * 
 * Tiles are stored in VRAM bank A1. Tilemap data is in bank A0.
 * 
 * 
 * 
 * 
*******************************************************************/
#ifndef STATIC_H
#define STATIC_H

#include "types.h"

/**
 * @brief Sets up VDP2's NBG1 layer for the static tilemap screen.
 * 
 * @param tiles Pointer to the tile data.
 * @param palettes Pointer to the palette data.
 * @param count Number of palettes.
 * @param plane Pointer to tilemap data.
 */
void static_setup(Uint8 *tiles, Uint32 **palettes, Uint8 count, void *plane);

/**
 * @brief Disables the static screen. 
 */
void static_disable();
/** 
 * @brief Moves the static screen to the desired coordinates.
 * 
 * @param x Desired X Screen Coordinate.
 * @param y Desired Y Screen Coordinate.
 */
void static_move_to(Fixed32 x, Fixed32 y);

/**
 * @brief Updates the tilemap data in VRAM.
 * 
 * Use this if you update what's in the tilemap pointer in main RAM and you need to update.
 * Ex: Updated the tilemap to display a textbox and you need to update that in VRAM so it draws.
 * 
 */
void update_static();

#endif
