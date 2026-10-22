/*******************************************************************
 *
 *                      NES Tile Conversion
 *
 * Converts NES CHR tiles into Saturn 4bpp cell data.
 *
 * Each converted pixel keeps its NES color index (0-3), so the
 * palette is chosen by the tilemap entry, the same as on the NES.
 *
 *
*******************************************************************/
#ifndef CHR_H
#define CHR_H

#include "types.h"

/**
 * @brief Converts NES 2bpp planar tiles to Saturn 4bpp tiles.
 *
 * An NES tile is 16 bytes and becomes a 32 byte Saturn tile.
 *
 * @param src Pointer to source NES tiles.
 * @param dest Pointer to converted 4bpp tiles. Must hold twice the length.
 * @param length Length of 2bpp tile data in bytes (16 per tile).
 */
void convertNesTilesToSatTiles(Uint8 *src, Uint8 *dest, Uint16 length);

#endif
