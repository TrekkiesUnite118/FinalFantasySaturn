#ifndef OVERWORLD_H
#define OVERWORLD_H
#include "datatypes.h"


#define NORMAL_BATTLE (MTH_FIXED(0x40))
#define RIVER_BATTLE_1 (MTH_FIXED(0x41))
#define OCEAN_BATTLE (MTH_FIXED(0x42))
#define RIVER_BATTLE_2 (MTH_FIXED(0x43))

#define GRASS_BACKDROP (MTH_FIXED(0x00))
#define MARSH_CAVE_BACKDROP (MTH_FIXED(0x01))
#define CAVE_BACKDROP (MTH_FIXED(0x02))
#define WATER_BACKDROP (MTH_FIXED(0x03))
#define FOREST_BACKDROP (MTH_FIXED(0x04))
#define TEMPLE_BACKDROP (MTH_FIXED(0x05))
#define DESERT_BACKDROP (MTH_FIXED(0x06))
#define MARSH_BACKDROP (MTH_FIXED(0x07))
#define EARTH_CAVE_BACKDROP (MTH_FIXED(0x08))
#define CASTLE_BAKDROP (MTH_FIXED(0x09))
#define RIVER_BACKDROP (MTH_FIXED(0x0A))
#define TOWER_BACKDROP (MTH_FIXED(0x0B))
#define SHRINE_BACKDROP (MTH_FIXED(0x0C))
#define ICE_CAVE_BACKDROP (MTH_FIXED(0x0D))
#define VOLCANO_BACKDROP (MTH_FIXED(0x0E))
#define WATERFALL_BACKDROP (MTH_FIXED(0x0F))

extern Uint16 decompressedMap[256][256];

extern Uint16 mapPlane[32][32];;

extern const int OVERWORLDMAP_OFFSET;
extern const int OVERWORLDMAP_PTRADD;
extern const int OVERWORLDMAP_START;
extern const int OVERWORLDMAP_END; 

extern const int OVERWORLD_TILEDATA; 


//sets up initial overworld
void overworld_init(void);

//Decompresses Overworld Map 
void decomp_map();

//Updates Plane
void update_plane(int map_y, int map_x);

//Gets Tile from plane.
Uint16 getTile(int map_y, int map_x);

//Gets first tile proerty byte
BYTE getTileProperty1(int map_y, int map_x);

//Gets second tile proerty byte
BYTE getTileProperty2(int map_y, int map_x);

BYTE getTileBattleBackdrop(int map_y, int map_x);

int encounterBattle();

int determineBattleDomain(int pos_x, int pos_y, int property);

BYTE getBattleNum(int domain);

#endif
