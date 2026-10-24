/*******************************************************************
 *
 *                      Overworld Map
 *
 * The 256x256 overworld map decompressed from the ROM, its tile
 * properties and the random battle lookups.
 *
*******************************************************************/
#ifndef OVERWORLD_H
#define OVERWORLD_H
#include "datatypes.h"
#include "types.h"
#include "romaddr.h"


//Battle types for determineBattleDomain.
#define NORMAL_BATTLE (FIXED(0x40))
#define RIVER_BATTLE_1 (FIXED(0x41))
#define OCEAN_BATTLE (FIXED(0x42))
#define RIVER_BATTLE_2 (FIXED(0x43))

#define GRASS_BACKDROP (FIXED(0x00))
#define MARSH_CAVE_BACKDROP (FIXED(0x01))
#define CAVE_BACKDROP (FIXED(0x02))
#define WATER_BACKDROP (FIXED(0x03))
#define FOREST_BACKDROP (FIXED(0x04))
#define TEMPLE_BACKDROP (FIXED(0x05))
#define DESERT_BACKDROP (FIXED(0x06))
#define MARSH_BACKDROP (FIXED(0x07))
#define EARTH_CAVE_BACKDROP (FIXED(0x08))
#define CASTLE_BAKDROP (FIXED(0x09))
#define RIVER_BACKDROP (FIXED(0x0A))
#define TOWER_BACKDROP (FIXED(0x0B))
#define SHRINE_BACKDROP (FIXED(0x0C))
#define ICE_CAVE_BACKDROP (FIXED(0x0D))
#define VOLCANO_BACKDROP (FIXED(0x0E))
#define WATERFALL_BACKDROP (FIXED(0x0F))

//Overworld tile numbers, [y][x].
extern Uint16 decompressedMap[256][256];

//RBG0 map plane filled by update_plane.
extern Uint16 mapPlane[32][32];

#define OVERWORLD_COLORS (32)
#define OVERWORLD_CHR_SIZE (0x1000)


/**
 * @brief Sets up the overworld. Decompresses the map.
 */
void overworld_init(void);

/**
 * @brief Seeds the battle formation counter and battleStepRNG's direction from the RTC.
 */
void seedEncounterCounter(void);

/**
 * @brief Decompresses the overworld map from the ROM into decompressedMap.
 */
void decomp_map();

/**
 * @brief Fills mapPlane with 32x32 map tiles and their palettes.
 *
 * Wraps around the map's edges.
 *
 * @param map_y Map tile Y of the top row.
 * @param map_x Map tile X of the left column.
 */
void update_plane(int map_y, int map_x);

/**
 * @brief Gets a map tile's first property byte.
 *
 * @param map_y Map tile Y.
 * @param map_x Map tile X.
 * @return Property byte.
 */
BYTE getTileProperty1(int map_y, int map_x);

/**
 * @brief Gets a map tile's second property byte.
 *
 * @param map_y Map tile Y.
 * @param map_x Map tile X.
 * @return Property byte.
 */
BYTE getTileProperty2(int map_y, int map_x);

/**
 * @brief Gets the battle backdrop for a map tile.
 *
 * @param map_y Map tile Y.
 * @param map_x Map tile X.
 * @return Backdrop (_BACKDROP).
 */
BYTE getTileBattleBackdrop(int map_y, int map_x);

/**
 * @brief Gets the next step's random number (NES BattleStepRNG).
 *
 * Seeded from the RTC on the first call.
 *
 * @return Random number.
 */
Uint8 battleStepRNG();

/**
 * @brief Gets the battle domain for a map position.
 *
 * Normal battles use an 8x8 grid of 32x32 tile areas. Rivers use
 * domain $40 or $41 by map half, the ocean $42.
 *
 * @param pos_x Map tile X.
 * @param pos_y Map tile Y.
 * @param property Battle type (_BATTLE).
 * @return Domain number.
 */
Uint8 determineBattleDomain(Uint8 pos_x, Uint8 pos_y, int property);

/**
 * @brief Picks a battle formation from a domain.
 *
 * @param domain Domain number.
 * @return Formation number.
 */
BYTE getBattleNum(int domain);

#endif
