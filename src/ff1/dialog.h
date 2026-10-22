/*******************************************************************
 *
 *                      Map Dialogue Box
 *
 * The standard map dialogue box and treasure chests. The box is
 * drawn on NBG2 and shown through VDP2 window W0, which opens it
 * line by line like the NES.
 *
 *
*******************************************************************/
#ifndef DIALOG_H
#define DIALOG_H

#include "types.h"

#define LUT_DIALOGUE_PTR (0x00228010) //Bank $0A $8000, 16-bit pointers into the same bank.
#define DIALOGUE_BANK_ADD (0x00220010) //LWRAM address of bank $0A minus $8000.
#define LUT_TREASURE (0x00203110) //Bank $00 $B100, chest contents by chest ID.
#define LUT_ITEM_PRICES (0x00237C10) //Bank $0D $BC00, 16-bit prices by item ID.

#define DLGID_NOTHING (0x00)
#define DLGID_DONTBEGREEDY (0x3A)
#define DLGID_TCGET (0xF0)
#define DLGID_CANTCARRY (0xF1)
#define DLGID_EMPTYTC (0xF2)

#define GMFLG_TCOPEN (0x04)

#define TCITYPE_WEPSTART (0x1C)
#define TCITYPE_ARMSTART (0x44)
#define TCITYPE_GPSTART (0x6C)

#define DLG_BOX_PAL (0) //NBG2 palette holding the map's BG palette 3.
#define DLG_LINES_OPEN (0x60) //NES tmp+11 once the box is fully open.

//NES dlgsfx: 1 plays the fanfare when the box opens, more than 1 the treasure jingle.
Uint8 dlgSfx;

/**
 * @brief Adds gold to the party, capped at 999999 like the NES.
 *
 * Keeps the NES cap check's bug unless BUGFIX_GOLD_CAP is on.
 *
 * @param gp Gold to add.
 */
void dlgAddGold(Uint16 gp);

/**
 * @brief Finds the first empty weapon or armor slot in the party.
 *
 * @param armor TRUE for armor, FALSE for weapons.
 * @return Pointer to the empty slot, or 0 if every slot is full.
 */
Uint8 *dlgFindEmptySlot(Uint8 armor);

/**
 * @brief Gives the party a chest's item, equipment or gold and marks the chest open.
 *
 * Also sets dlgSfx for the jingle.
 *
 * @param chest Chest ID.
 * @return The dialogue ID to show (DLGID_TCGET or DLGID_CANTCARRY).
 */
Uint8 dlgOpenTreasureChest(Uint8 chest);

/**
 * @brief Draws the dialogue box and its text into the NBG2 plane.
 *
 * The box isn't shown until dlgSetVisible opens it.
 *
 * @param dlgId Dialogue ID.
 * @return Number of frames the NES spends drawing the box.
 */
Uint8 dlgDraw(Uint8 dlgId);

/**
 * @brief Sets how much of the dialogue box is shown.
 *
 * @param lines NES lines open. Below 10 the box is hidden.
 */
void dlgSetVisible(Uint8 lines);

#endif
