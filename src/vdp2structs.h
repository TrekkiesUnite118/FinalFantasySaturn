#ifndef VDP2STRUCTS_H
#define VDP2STRUCTS_H

#include "datatypes.h"

typedef struct {
    Uint8 *tile_name; //filename of .TLE file
    Uint16 tile_num; //number of tiles
    Uint32 *palette; //pointer to palette
    Uint8 *map_name; //filename of .MAP file
    Uint16 map_width; //width of map
    Uint16 map_height; //height of map
} LAYER;

#endif
