/*******************************************************************
 *
 *                      Options
 *
 * The options menu (L from the main menu): the NES/MSX2 graphics and
 * sound switches. It's only reachable when MSXGFX.BIN loaded or
 * MSXSND.BIN is on the CD.
 *
*******************************************************************/
#ifndef GFXMENU_H
#define GFXMENU_H

/**
 * @brief Draws the options screen.
 */
void setupGfxScreen(void);

/**
 * @brief Puts the cursor on the first option.
 */
void initGfxMenu(void);

/**
 * @brief Runs one frame of the options menu.
 *
 * Up/down picks an option, A or C switches it, left/right picks a
 * setting, B goes back to the main menu. The sound switch loads the
 * new driver right away.
 */
void doGfxMenu(void);

/**
 * @brief Gives back the menu tiles the Japanese screen borrowed.
 *
 * Called when leaving the screen, including by soft reset.
 */
void leaveGfxMenu(void);

/**
 * @brief Draws the cursor.
 */
void gfxmenu_draw(void);

#endif
