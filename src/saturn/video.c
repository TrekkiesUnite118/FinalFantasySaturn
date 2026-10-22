#include <sega_def.h>
#include <sega_scl.h>

#include "video.h"

//Compile error if the VID_* copies drift from SBL.
typedef char vidCheck[(VID_RBG0 == SCL_RBG0 && VID_RBG1 == SCL_RBG1 && VID_NBG0 == SCL_NBG0 &&
                       VID_NBG1 == SCL_NBG1 && VID_NBG2 == SCL_NBG2 && VID_NBG3 == SCL_NBG3 &&
                       VID_SPR == SCL_SPR && VID_W0 == SCL_W0 && VID_W1 == SCL_W1 &&
                       VID_VRAM_A0 == SCL_VDP2_VRAM_A0 && VID_VRAM_A1 == SCL_VDP2_VRAM_A1 &&
                       VID_VRAM_B0 == SCL_VDP2_VRAM_B0 && VID_VRAM_B1 == SCL_VDP2_VRAM_B1) ? 1 : -1];

void vidSetPalette(Uint32 layer, Uint32 index, Uint32 n, void *colors) {
    SCL_SetColRam(layer, index, n, colors);
}

void vidSetWindow(Uint8 win, Uint32 logic, Uint32 enable, Uint32 area,
                  Uint16 sx, Uint16 sy, Uint16 ex, Uint16 ey) {
    SCL_SetWindow(win, logic, enable, area, sx, sy, ex, ey);
}
