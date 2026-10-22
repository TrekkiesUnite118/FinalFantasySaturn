/*******************************************************************
 *
 *                      VDP2
 *
 * VDP2 layer and VRAM names for code without SBL headers, and
 * wrappers for setting color RAM and windows.
 *
*******************************************************************/
#ifndef VIDEO_H
#define VIDEO_H

#include "types.h"

//VDP2 layer bits, same values as SBL's SCL_*.
#define VID_RBG0 (0x00000001)
#define VID_RBG1 (0x00000002)
#define VID_NBG0 (0x00000004)
#define VID_NBG1 (0x00000008)
#define VID_NBG2 (0x00000010)
#define VID_NBG3 (0x00000020)
#define VID_SPR  (0x00000100)

//Windows, same values as SBL's SCL_W0/W1.
#define VID_W0 (0)
#define VID_W1 (1)

//VRAM bank addresses.
#define VID_VRAM_A0 (0x25e00000)
#define VID_VRAM_A1 (0x25e20000)
#define VID_VRAM_B0 (0x25e40000)
#define VID_VRAM_B1 (0x25e60000)

/**
 * @brief Writes colors to a layer's color RAM area (SCL_SetColRam).
 *
 * @param layer Layer (VID_).
 * @param index First color in the layer's area.
 * @param n Number of colors.
 * @param colors Colors.
 */
void vidSetPalette(Uint32 layer, Uint32 index, Uint32 n, void *colors);

/**
 * @brief Sets a rectangle window (SCL_SetWindow).
 *
 * @param win Window (VID_W0 or VID_W1).
 * @param logic As SCL_SetWindow, with VID_ layer bits.
 * @param enable As SCL_SetWindow, with VID_ layer bits.
 * @param area As SCL_SetWindow, with VID_ layer bits.
 * @param sx Left edge.
 * @param sy Top edge.
 * @param ex Right edge.
 * @param ey Bottom edge.
 */
void vidSetWindow(Uint8 win, Uint32 logic, Uint32 enable, Uint32 area,
                  Uint16 sx, Uint16 sy, Uint16 ex, Uint16 ey);

#endif
