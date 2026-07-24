#ifndef STATIC_H
#define STATIC_H

#define TILE_SIZE 16
//maximum # tiles i'll show onscreen
#define SCREEN_TILES_X 28
#define SCREEN_TILES_Y 20
#define BG_TILES 32

#include "datatypes.h"
#include "vdp2structs.h"

typedef struct {
    LAYER bg;
} STATIC_SCREEN;
int staticEnable;

void static_init();
void static_enable();
void static_disable();
void static_move(Fixed32 x, Fixed32 y);
void update_static();
void update_palettes();
void update_static_portion(Uint16 *lwram_map_ptr, Uint16 vram_offset, Uint8 height, Uint8 width);
void memsetDword(Uint32 *dst, Uint32 value, Sint32 cnt);

#endif
