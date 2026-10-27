/*******************************************************************
 *
 *                      Low Work RAM Layout
 *
 * Where FF1 keeps its data in Low Work RAM. The NES ROM is loaded at
 * LWRAM, followed by the converted sprite and menu graphics and a
 * scratch buffer.
 *
 *
*******************************************************************/
#ifndef MEMMAP_H
#define MEMMAP_H

#include "cd.h"

#define LWRAM_ROM_END    (0x260000)
#define LWRAM_MAP_SPRITES    (0x262A40)
#define LWRAM_BATTLE_SPRITES    (0x268A40)
#define LWRAM_WEAPON_SPRITES    (0x26C040)
#define LWRAM_MENU_TILES    (0x26D040)
#define LWRAM_BUFFER    (0x26F040)
//MSXGFX.BIN, the optional MSX2 graphics from tools/msxconv.
#define LWRAM_MSX_GFX    (0x275040)
#define LWRAM_MSX_GFX_END    (0x300000)

#endif
