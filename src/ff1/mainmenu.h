/*******************************************************************
 *
 *                      Main Menu
 *
 * The NES main menu and its submenus: status, magic, weapon and
 * armor equip, and items.
 *
 * Each screen has a setup function that draws its boxes, an init
 * that sets up its sprites and cursor, a per-frame function and a
 * draw function for its sprites.
 *
*******************************************************************/
#ifndef MAINMENU_H
#define MAINMENU_H

#include "types.h"

//Set when a map opens the main menu, rather than a submenu returning to it (NES EnterMainMenu).
Uint8 mmFromMap;

/**
 * @brief Draws the main menu's orbs, text boxes and party boxes (NES EnterMainMenu and DrawMainMenu).
 */
void setupMainMenuScreen(void);

/**
 * @brief Sets up the party sprites and the cursor (NES ResumeMainMenu).
 */
void initMainMenu(void);

/**
 * @brief Runs one frame of the main menu.
 */
void doMainMenu(void);

/**
 * @brief Draws the party sprites and the cursor.
 */
void mainmenu_draw(void);

/**
 * @brief Draws the status screen for the chosen party member (NES EnterStatusMenu).
 */
void setupStatusScreen(void);

/**
 * @brief Sets up the status screen's character sprite.
 */
void initStatusMenu(void);

/**
 * @brief Runs one frame of the status screen. A, B or C go back to the main menu.
 */
void doStatusMenu(void);

/**
 * @brief Draws the status screen's character sprite.
 */
void status_draw(void);

/**
 * @brief Draws the magic screen for the chosen party member.
 */
void setupMagicScreen(void);

/**
 * @brief Sets up the magic screen's cursor.
 */
void initMagicMenu(void);

/**
 * @brief Runs one frame of the magic screen. B in the spell list goes back to the main menu.
 */
void doMagicMenu(void);

/**
 * @brief Draws the magic screen's cursor.
 */
void magic_draw(void);

/**
 * @brief Draws the weapon or armor equip screen (NES EnterEquipMenu).
 */
void setupEquipScreen(void);

/**
 * @brief Sets up the equip screen's cursor.
 */
void initEquipMenu(void);

/**
 * @brief Runs one frame of the equip screen.
 */
void doEquipMenu(void);

/**
 * @brief Draws the equip screen's cursors.
 *
 * Trading flickers between the two cursors every frame, like the NES.
 */
void equip_draw(void);

/**
 * @brief Removes a weapon or armor from a party member and fixes their stats if it was equipped.
 *
 * Used when selling in a shop.
 *
 * @param armor TRUE for armor, FALSE for a weapon.
 * @param member Party member.
 * @param slot Equipment slot.
 */
void removeEquipment(Uint8 armor, Uint8 member, Uint8 slot);

/**
 * @brief Draws the item screen.
 */
void setupItemScreen(void);

/**
 * @brief Sets up the item screen's cursor.
 */
void initItemMenu(void);

/**
 * @brief Runs one frame of the item screen. B in the item list goes back to the main menu.
 */
void doItemMenu(void);

/**
 * @brief Draws the item screen's cursor.
 */
void item_draw(void);

#endif
