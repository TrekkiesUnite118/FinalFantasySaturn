/*******************************************************************
 *
 *                      MSX2 Graphics
 *
 * Optional graphics from the MSX2 version of the game, loaded from
 * MSXGFX.BIN (made by tools/msxconv). Without the file the option
 * isn't offered and the NES graphics are used.
 *
 * Game code checks the option with MSX_ON.
 *
*******************************************************************/
#ifndef MSXGFX_H
#define MSXGFX_H

#include "types.h"

//Entry ids, as tools/msxconv writes them.
#define MSX_ID_MAP_TILES (0) //MAINBG, then SUB0BG-SUB7BG.
#define MSX_ID_MAP_PALS (9)
#define MSX_ID_MAP_COLOR_TABLE (18)
#define MSX_ID_MAP_COLOR_SETS (19)
#define MSX_ID_ENEMY_TILES (20)
#define MSX_ID_ENEMY_PALS (144)
#define MSX_ID_BATTLE_FRAMES (268)
#define MSX_ID_BATTLE_PAL (352)
#define MSX_ID_ORBS (353)
#define MSX_ID_MENU_PAL (354)
#define MSX_ID_BACKDROPS (355) //4x3 tiles each.
#define MSX_BACKDROP_ROWS (3)
#define MSX_ID_SHOP_TILES (371)
#define MSX_ID_SHOP_PAL (372)
#define MSX_ID_MINIMAP_DRAGON (373)
#define MSX_ID_MINIMAP_TITLE (374)
#define MSX_ID_MINIMAP_PAL (375)
#define MSX_ID_BRIDGE (376) //256x212 bitmap, then its palette.
#define MSX_ID_BRIDGE_PAL (377)
#define MSX_ID_ENDING (378) //256x212 bitmap, then its palette.
#define MSX_ID_ENDING_PAL (379)
#define MSX_ID_THE_END (380) //128x80 bitmap, then its palette.
#define MSX_ID_THE_END_PAL (381)
#define MSX_ID_MAP_SPRITE_PAL (382)
#define MSX_ID_PRELUDE_PAL (383)
#define MSX_ID_COUNT (384)

//Party battle frames per class: stand, walk, swing, cheer, crouch, dead (24x16), stone.
#define MSX_BATTLE_CLASS_FRAMES (7)
#define MSX_BATTLE_FRAME_DEAD (5)
#define MSX_BATTLE_FRAME_STONE (6)

//Palette steps of the MSX fade that stands in for the NES CyclePalettes.
#define MSX_FADE_STEPS (4)

//Nonzero when the MSX graphics are on.
#define MSX_ON (gfxMsx)

//TRUE when MSXGFX.BIN loaded and checked out.
extern Uint8 msxAvailable;
//The graphics option. Only TRUE when msxAvailable.
extern Uint8 gfxMsx;

/**
 * @brief Loads and checks MSXGFX.BIN. Sets msxAvailable.
 */
void msxInit(void);

/**
 * @brief Gets an entry's data.
 *
 * @param id MSX_ID_ entry.
 * @return Pointer to the entry's data.
 */
const Uint8 *msxData(Uint16 id);

/**
 * @brief Gets a 16 color palette entry.
 *
 * @param id MSX_ID_ palette entry.
 * @return Pointer to its 16 colors (0x00BBGGRR).
 */
const Uint32 *msxPalette(Uint16 id);

/**
 * @brief Builds a standard map's palette: its tileset's PIC palette with the map's colors 8-15.
 *
 * @param tileset NES tileset.
 * @param map NES standard map.
 * @param dst Where the 16 colors go.
 */
void msxMapPalette(Uint8 tileset, Uint8 map, Uint32 *dst);

/**
 * @brief Hides room interiors, as MAIN.COM does outside rooms on castle and dungeon maps.
 *
 * Colors 12-15 all become color 10, one level brighter.
 *
 * @param palette A standard map's 16 colors.
 */
void msxHideRooms(Uint32 *palette);

/**
 * @brief Applies NES $2001 effects to an MSX color.
 *
 * @param color Color (0x00BBGGRR).
 * @param emphasis Emphasis bits, as for emphasizeColor.
 * @param gray Nonzero for monochrome.
 * @return The color.
 */
Uint32 msxShadeColor(Uint32 color, Uint8 emphasis, Uint8 gray);

/**
 * @brief Darkens a color for a step of a fade to black.
 *
 * @param color Color (0x00BBGGRR).
 * @param step 0 (unchanged) to steps (black).
 * @param steps Steps in the fade.
 * @return The color.
 */
Uint32 msxFadeColor(Uint32 color, Uint8 step, Uint8 steps);

//Map object graphics with MSX colors.
#define MSX_OBJECT_GFX (30)

/**
 * @brief Gets the JOBS palette the MSX map sprites share, with color 0 transparent.
 *
 * @param dst Where the 16 colors go.
 */
void msxSpritePalette(Uint32 *dst);

/**
 * @brief Builds the party map sprite's palettes in its class's MSX colors.
 *
 * @param charClass Class of the party's lead.
 * @param top Where the top half's 16 colors go.
 * @param bottom Where the bottom half's 16 colors go.
 */
void msxMapManPalette(Uint8 charClass, Uint32 *top, Uint32 *bottom);

/**
 * @brief Changes a map object's NES colors 1-3 to its MSX colors in the shared sprite palette.
 *
 * @param gfx NES map object graphic.
 * @param halves Its 8 16x8 halves, as buildMapSprites leaves them.
 */
void msxRecolorObject(Uint8 gfx, Uint8 *halves);

//Box colors by screen, as the MSX picture palettes have them.
enum {
    MSX_BOX_MAP,    //Blue inside, orange border.
    MSX_BOX_MENU,   //Slate inside, light orange border.
    MSX_BOX_BATTLE, //Black inside, light orange border.
    MSX_BOX_BRIDGE, //Green inside, dark blue border.
    MSX_BOX_ENDING  //Green inside, blue border.
};

/**
 * @brief Gets the MSX colors for the NES box and text colors 0-3.
 *
 * @param style MSX_BOX_ style.
 * @param dst Where the 4 colors go.
 */
void msxBoxPalette(Uint8 style, Uint32 *dst);

/**
 * @brief Gets the MSX cursor colors: white, peach and a blue outline.
 *
 * @param nesPal The cursor's 4 NES colors.
 * @param dst Where the 4 colors go.
 */
void msxCursorPalette(const Uint8 *nesPal, Uint32 *dst);

#define MSX_ORB_UNLIT (4)

/**
 * @brief Copies an MSX orb as 4 tiles in NES order (top left, top right, bottom left, bottom right),
 * for the MENU palette.
 *
 * @param orb 0-3 lit in orbPos order, or MSX_ORB_UNLIT.
 * @param dst Where the 128 bytes of tiles go.
 */
void msxOrbTiles(Uint8 orb, Uint8 *dst);

/**
 * @brief Gets the MSX image for an NES enemy. The NES fiend copies share one.
 *
 * @param nesEnemy NES enemy id.
 * @return MSX enemy, the offset from MSX_ID_ENEMY_TILES and MSX_ID_ENEMY_PALS.
 */
Uint8 msxEnemy(Uint8 nesEnemy);

#endif
