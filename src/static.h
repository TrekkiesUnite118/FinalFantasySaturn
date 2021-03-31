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

void static_init(STATIC_SCREEN *screen);
void static_enable();
void static_disable();

#endif
