/*******************************************************************
 *
 *                      NES Palette
 *
 * The NES's 64 colors as Saturn 24-bit colors, for building VDP1
 * and VDP2 palettes from NES palette indexes.
 *
 *
 *
*******************************************************************/
#ifndef NESPAL_H
#define NESPAL_H

#include "types.h"

/**
 * @brief The NES palette as VDP2 24-bit colors (0xBBGGRR).
 *
 * Index it with an NES color ($00-$3F).
 * Ex: nes_pal[0x0F] is black.
 */
extern Uint32 nes_pal[64];

/**
 * @brief Gets an NES color with the PPU's color emphasis applied.
 *
 * This is an approximation: channels that aren't emphasized are dimmed to
 * about 82%. With all three bits set, every channel is dimmed.
 * Colors $xE and $xF aren't affected.
 *
 * @param nesColor NES color ($00-$3F).
 * @param emphasis Emphasis bits. Bit 0 = red, bit 1 = green, bit 2 = blue.
 * @return The color as a VDP2 24-bit color (0xBBGGRR).
 */
Uint32 emphasizeColor(Uint8 nesColor, Uint8 emphasis);

/**
 * @brief Applies emphasis to a color, as emphasizeColor does to NES colors.
 *
 * @param color VDP2 24-bit color (0xBBGGRR).
 * @param emphasis Emphasis bits. Bit 0 = red, bit 1 = green, bit 2 = blue.
 * @return The color.
 */
Uint32 emphasizeRgb(Uint32 color, Uint8 emphasis);

#endif
