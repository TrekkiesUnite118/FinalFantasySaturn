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

typedef struct {
    Uint8 *tile_location; //location of tile data
    Uint16 rawlength; //length of tile data
    Uint16 numTiles; //numberOfTiles
    Uint8 *map_location; //location of map data
    Uint16 map_width; //width of map
    Uint16 map_height; //height of map
    Uint8 total_colors; //total number of colors
    Uint8 *palette; //pointer to palette data
    Uint8 num_palettes; //numberOfPalettes
} MAP_DATA;

#endif
