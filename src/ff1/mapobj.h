/*******************************************************************
 *
 *                      Map Objects
 *
 * The townspeople and other objects on standard maps: loading,
 * walking, blocking the player and drawing them as sprites.
 * Up to 15 objects per map, as on the NES.
 *
 *
*******************************************************************/
#ifndef MAPOBJ_H
#define MAPOBJ_H

#include "types.h"

#define LUT_MAP_OBJECTS (0x00203410) //$30 bytes per map: 15 objects of ID, X | flags, Y.
#define LUT_MAP_OBJ_GFX (0x00202E10)
#define LUT_MAP_OBJ_CHR (0x0020A210) //$100 bytes (16 tiles) per graphic.
#define LUT_MAP_OBJ_TALK (0x002395E5) //Bank $0E $95D5, 4 bytes per object ID.

#define SM_OBJ_COUNT (15)
#define SM_OBJ_CHAR_START (384) //8 16x8 halves per object.
#define SM_OBJ_TOP_CRAM (160) //NES sprite palette 2.
#define SM_OBJ_BOTTOM_CRAM (176) //NES sprite palette 3.

#define OBJ_FLAG_INROOM (0x80)
#define OBJ_FLAG_STILL (0x40)

#define OBJID_SKYWAR_FIRST (0x3A)
#define OBJID_SKYWAR_LAST (0x3E)
#define OBJID_BAT (0x57)

//NES mapobj.
typedef struct {
    Uint8 id; //0 when hidden by its game flag.
    Uint8 flgs;
    Uint8 physX; //Tile the object occupies, set when a step starts.
    Uint8 physY;
    Uint8 gfxX; //Tile the graphic is drawn from.
    Uint8 gfxY;
    Uint8 ctrX; //Pixels into the step.
    Uint8 ctrY;
    Sint8 spdX;
    Sint8 spdY;
    Uint8 rawid;
    Uint8 movectr; //Frames until the next step attempt.
    Uint8 face; //Nonzero while stepping.
    Uint8 pl; //Bit 7: talking to the player. Other bits: shoved by the player.
    Uint8 tsa; //Facing table: 0 right, 1 left, 2 up, 3 down.
} SM_MAPOBJ;

SM_MAPOBJ mapobj[SM_OBJ_COUNT];
Uint8 smObjCoverY; //NES dialogue box cutoff: object sprites above it go behind the BG. 0 when no box is open.

/**
 * @brief Loads cur_map's objects into mapobj.
 *
 * Objects whose game flag isn't set are hidden (id 0).
 */
void smLoadMapObjects(void);

/**
 * @brief Converts each object's graphics and registers them as sprite characters.
 */
void smLoadMapObjectChr(void);

/**
 * @brief Checks if an object blocks a tile.
 *
 * An object on the tile has its wait cut. An object stepping off the
 * tile is shoved to move faster.
 *
 * @param x Map tile X.
 * @param y Map tile Y.
 * @return TRUE if an object is on the tile.
 */
Uint8 smIsObjectInPath(Uint8 x, Uint8 y);

/**
 * @brief Finds an object the player can talk to on a tile.
 *
 * An object past the halfway point of a step counts as on the next tile.
 * The object found is marked as talking to the player.
 *
 * @param x Map tile X.
 * @param y Map tile Y.
 * @return The object's mapobj index, or -1 if there isn't one.
 */
Sint8 smCanTalkToMapObject(Uint8 x, Uint8 y);

/**
 * @brief Moves and animates every object and records them for drawing.
 *
 * @param playerFacing Direction the player faces.
 * @param playerCtrX Pixels into the player's step across.
 * @param playerCtrY Pixels into the player's step down.
 */
void smUpdateMapObjects(Uint8 playerFacing, Uint8 playerCtrX, Uint8 playerCtrY);

/**
 * @brief Records the objects without moving them, to show later with
 * smShowRecordedMapObjects.
 *
 * @param playerFacing Direction the player faces.
 */
void smRecordMapObjects(Uint8 playerFacing);

/**
 * @brief Shows the objects recorded by smRecordMapObjects.
 */
void smShowRecordedMapObjects(void);

/**
 * @brief Draws the recorded objects as sprites, below the player.
 */
void smDrawMapObjects(void);

#endif
