#ifndef NESUTILS_H
#define NESUTILS_H

#include    <sega_def.h>

void convertNesTilesToSatTiles(Uint8 *src, Uint8 *dest, Uint16 length);

void buildMapTiles(Uint8 *src, Uint8 *dest, Uint8* assignmentOffset);

void buildMapSprites(Uint8 *src, Uint8 *dest, Uint16 size);

void buildWeaponSprites(Uint8 *src, Uint8 *dest);

void buildBattleSprites(Uint8 *src, Uint8 *dest);

#endif
