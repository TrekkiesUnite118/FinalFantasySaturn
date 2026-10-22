/*******************************************************************
 *
 *                      Screen Wipe
 *
 * The NES screen wipe used when entering and leaving standard maps.
 * VDP2 window W1 shows a band of the screen that grows or shrinks
 * from the middle, with the NES wipe sound.
 *
 *
*******************************************************************/
#ifndef WIPE_H
#define WIPE_H

#include "types.h"

/**
 * @brief Starts a screen wipe and switches to the WIPE state.
 *
 * Sprites aren't drawn during the wipe.
 *
 * @param open TRUE to open (reveal) the screen, FALSE to close it.
 * @param layers The VID_ layers the window hides.
 * @param done Called once the wipe finishes. Set the next state here.
 */
void wipeStart(Uint8 open, Uint32 layers, void (*done)(void));

/**
 * @brief Runs one frame of the wipe. Called each frame in the WIPE state.
 */
void doWipe(void);

#endif
