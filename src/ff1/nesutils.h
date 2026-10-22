/*******************************************************************
 *
 *                      FF1 Graphics Builders
 *
 * Rearranges tiles converted by convertNesTilesToSatTiles into
 * VDP2 map cells and VDP1 sprite characters.
 *
 * All sources are Saturn 4bpp tiles (32 bytes each).
 *
*******************************************************************/
#ifndef NESUTILS_H
#define NESUTILS_H

#include    "types.h"
#include    "chr.h"

/**
 * @brief Builds 128 16x16 map tiles from a tileset's tile assembly tables.
 *
 * Each map tile is written as 4 cells in a row: top left, top right,
 * bottom left, bottom right.
 *
 * @param src Pointer to the converted tileset tiles.
 * @param dest Pointer to the map cells (128 bytes per map tile).
 * @param assignmentOffset Pointer to the NES tile assembly tables: 4 tables of
 * 128 tile numbers, in the order top left, top right, bottom left, bottom right.
 */
void buildMapTiles(Uint8 *src, Uint8 *dest, Uint8* assignmentOffset);

/**
 * @brief Joins each pair of tiles into a 16x8 sprite character.
 *
 * @param src Pointer to the converted tiles, in pairs (left, right).
 * @param dest Pointer to the sprite characters (64 bytes each).
 * @param size Length of the converted tile data in bytes.
 */
void buildMapSprites(Uint8 *src, Uint8 *dest, Uint16 size);

/**
 * @brief Builds 32 16x16 weapon and magic sprite characters.
 *
 * Each uses 4 tiles in a row: top left, top right, bottom left, bottom right.
 *
 * @param src Pointer to the converted tiles.
 * @param dest Pointer to the sprite characters (128 bytes each).
 */
void buildWeaponSprites(Uint8 *src, Uint8 *dest);

/**
 * @brief Builds the party's battle sprite characters, 6 for each of the 12 classes.
 *
 * The first 5 are 16x24. The 6th is 24x16 (the dead pose).
 *
 * @param src Pointer to the converted battle tiles (0x400 bytes per class).
 * @param dest Pointer to the sprite characters (192 bytes each).
 */
void buildBattleSprites(Uint8 *src, Uint8 *dest);

#endif
