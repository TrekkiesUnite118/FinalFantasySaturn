/*******************************************************************
 *
 *                      Standard Maps
 *
 * Towns, castles and dungeons: loading maps from the ROM, moving
 * the player, teleports, tile effects, palette cycles and the altar
 * effect. Follows the NES StandardMapLoop.
 *
*******************************************************************/
#ifndef STDMAP_H
#define STDMAP_H

#include "types.h"
#include "video.h"

//ROM tables.
#define LUT_ENTR_TELE_X (0x00202C10)
#define LUT_ENTR_TELE_Y (0x00202C30)
#define LUT_ENTR_TELE_MAP (0x00202C50)
#define LUT_EXIT_TELE_X (0x00202C70)
#define LUT_EXIT_TELE_Y (0x00202C80)
#define LUT_TILESETS (0x00202CD0)
#define LUT_NORM_TELE_X (0x00202D10)
#define LUT_NORM_TELE_Y (0x00202D50)
#define LUT_NORM_TELE_MAP (0x00202D90)
#define LUT_SM_PTR_TBL (0x00210010) //16-bit offsets from the start of bank 4.
#define LUT_SM_TILESET_ATTR (0x00200410)
#define LUT_SM_TILESET_PROP (0x00200810)
#define LUT_SM_TILESET_TSA (0x00201010)
#define LUT_SM_PALETTES (0x00202010)
#define SM_TILESET_CHR (0x0020C010)
//A0: NBG0 char. A1: NBG2 char, NBG0 PN. B0: NBG2 PN.
#define SM_NBG0_CHAR (VID_VRAM_A0)
#define SM_NBG2_CHAR (VID_VRAM_A1)
#define SM_NBG0_PN (VID_VRAM_A1 + 0x2000)
#define SM_NBG2_PN (VID_VRAM_B0)
#define LUT_BATTLE_RATES (0x0022CC10) //Bank $0B $8C00, X/256 encounter chance per map, +1 for the overworld entry.
#define SM_DOMAIN_START (0x40) //Standard map battle domains follow the 8x8 overworld ones.

//NES standard map tile property bits.
#define TP_NOMOVE (0x01)
#define TP_SPEC_DOOR (0x02)
#define TP_SPEC_LOCKED (0x04)
#define TP_SPEC_CLOSEROOM (0x06)
#define TP_SPEC_TREASURE (0x08)
#define TP_SPEC_BATTLE (0x0A)
#define TP_SPEC_DAMAGE (0x0C)
#define TP_SPEC_CROWN (0x0E)
#define TP_SPEC_CUBE (0x10)
#define TP_SPEC_4ORBS (0x12)
#define TP_SPEC_USEROD (0x14)
#define TP_SPEC_USELUTE (0x16)
#define TP_SPEC_EARTHORB (0x18)
#define TP_SPEC_FIREORB (0x1A)
#define TP_SPEC_WATERORB (0x1C)
#define TP_SPEC_AIRORB (0x1E)
#define TP_SPEC_MASK (0x1E)
#define TP_NOTEXT_MASK (0xC2) //Talking to a tile with any of these shows "Nothing here".
#define TP_BATTLEMARKER (0x20)
#define TP_TELE_WARP (0x40)
#define TP_TELE_NORM (0x80)
#define TP_TELE_EXIT (0xC0)
#define TP_TELE_MASK (0xC0)

//Maps left by normal teleports, returned to by warps.
#define SM_STACK_MAX (16)

typedef struct {
    Uint8 scrollX;
    Uint8 scrollY;
    Uint8 inroom;
    Uint8 map;
    Uint8 tileset;
} SM_STACK_ENTRY;

Uint8 smMap[64][64]; //Decompressed map tiles, [y][x].
Uint8 cur_map;
Uint8 cur_tileset;
Uint8 sm_scroll_x; //NES sm_scroll_x/y. The player's tile is 7 right and 7 down from it.
Uint8 sm_scroll_y;
Uint8 inroom;
Uint8 sm_player_x; //NES sm_player_x/y, the tile the player is on or stepping to.
Uint8 sm_player_y;
Uint8 altareffect;
Uint8 shop_id;
Uint8 battleReturnSM; //The battle was started from a standard map.
Uint8 menuReturnSM; //The menu was opened from a standard map.
Uint8 smTilepropNow; //NES tileprop_now, the special bits of the tile the player stands on.

/**
 * @brief Sets the map and position for entering from the overworld (NES EntrTele).
 *
 * @param teleId Entrance teleport number.
 */
void smTeleportIn(Uint8 teleId);

/**
 * @brief Sets up the current map and its screen.
 *
 * Loads the map and its objects, which then wipe open. When coming
 * back from a menu or shop the loaded map is kept and cycles in.
 */
void sm_init(void);

/**
 * @brief Runs one frame of a standard map palette cycle, one step every 4th frame.
 */
void doSmPalCycle(void);

/**
 * @brief Takes 1 HP from party members, but not below 1 (NES MapPoisonDamage and AssignMapTileDamage).
 *
 * @param poisonOnly TRUE for only poisoned members, FALSE for all (damage floors).
 */
void smStepDamage(Uint8 poisonOnly);

/**
 * @brief Plays the poison sound if anyone is poisoned.
 */
void smPoisonSfx(void);

/**
 * @brief Draws the map objects and the player.
 */
void sm_draw(void);

/**
 * @brief Hides the standard map layers and frees their color RAM.
 */
void sm_disable(void);

/**
 * @brief Ends any altar effect and turns the map off, for soft reset.
 */
void sm_reset(void);

/**
 * @brief Runs one frame of the standard map (NES StandardMapLoop).
 */
void doStandardMap(void);

/**
 * @brief Does a pending altar color RAM switch. Call this at V-blank in.
 */
void sm_vblank_in(void);

/**
 * @brief Reloads the map palettes with NES color emphasis applied.
 *
 * @param emphasis Emphasis bits, as for emphasizeColor.
 */
void smSetEmphasis(Uint8 emphasis);

//For nbgscroll.c.
/**
 * @brief Loads the map, dialogue box and player palettes into color RAM.
 */
void smUploadPalettes(void);

/**
 * @brief Redraws the whole map plane (NES DrawFullMap).
 */
void smFullRedraw(void);

/**
 * @brief Goes back to the map, keeping it and its objects (NES ReenterStandardMap).
 */
void smReenterMap(void);

/**
 * @brief Uses the Rod or Lute on its plate (NES UseItem_Rod/UseItem_Lute).
 *
 * @param objId Plate's map object ID.
 * @param spec Tile property the player has to stand on (TP_SPEC_).
 * @return TRUE if used. The plate is hidden.
 */
Uint8 smUsePlateItem(Uint8 objId, Uint8 spec);

/**
 * @brief Leaves the map from the menu with WARP or EXIT.
 *
 * WARP goes back to the map before the last teleport, or to the
 * overworld if there isn't one. EXIT goes to the overworld.
 *
 * @param warp TRUE for WARP, FALSE for EXIT.
 */
void smMenuExit(Uint8 warp);

#endif
