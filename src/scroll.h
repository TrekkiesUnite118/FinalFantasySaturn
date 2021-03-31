#ifndef SCROLL_H
#define SCROLL_H

#define TILE_SIZE 16
//maximum # tiles i'll show onscreen
#define SCREEN_TILES_X 28
#define SCREEN_TILES_Y 20
#define BG_TILES 32

#include "datatypes.h"
#include "vdp2structs.h"

extern Sint32 map_tiles_x[], map_tiles_y[];

#define COPY_MODE_LCOL 1 
#define COPY_MODE_RCOL 2 
#define COPY_MODE_TROW 4 
#define COPY_MODE_BROW 8

#define SCROLL_PLAYFIELD (0)
#define SCROLL_FOREGROUND (1)
#define SCROLL_BACKGROUND1 (2)
#define SCROLL_BACKGROUND2 (3)

#define SCROLL_HMIRROR (0x400)
#define SCROLL_VMIRROR (0x800)

extern Uint32 copy_modes[]; //what to copy to VRAM from the map
extern Fixed32 scrolls_x[];
extern Fixed32 scrolls_y[];

typedef struct {
    Fixed32 player_startx; //where the player starts in the level
    Fixed32 player_starty;
    LAYER playfield; //map the player does stuff on
    LAYER bg_near; //near bg
    LAYER bg_far; //far bg
} LEVEL;

enum transition_states {
    TSTATE_NULL = 0,
    TSTATE_PRESETUP,
    TSTATE_ZOOM,
    TSTATE_POSTSETUP
};
extern int scroll_transition_state;
int scrollEnable;

void scroll_init(LEVEL *level);
//translate scroll by x,y amounts
void scroll_move(int num, Fixed32 x, Fixed32 y);
//moves scroll absolutely to coordinates
void scroll_set(int num, Fixed32 x, Fixed32 y);
//gets the value at the given coordinates for a square map
void copyPlanes(int x, int y, int offset);
//copies scroll to VRAM after position has been changed by move/set scroll
void scroll_copy(int num);
void scroll_enable();
void scroll_disable();
#endif
