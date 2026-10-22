/*******************************************************************
 *
 *                      Bridge And Ending Scenes
 *
 * The NES story screens: the bridge scene with the opening credits,
 * and the ending after Chaos. Pages of text are shown in a box with
 * shutters that open and close over it, drawn as VDP1 polygons.
 *
 *
*******************************************************************/
#ifndef STORY_H
#define STORY_H

#include "types.h"

/**
 * @brief Picks which scene the TO_STORY state sets up.
 *
 * @param ending TRUE for the ending, FALSE for the bridge scene.
 */
void storySelect(Uint8 ending);

/**
 * @brief Loads the scene's graphics, picture and palettes and starts its music.
 */
void setupStoryScreen(void);

/**
 * @brief Runs one frame of the scene. Called each frame in the STORY state.
 */
void doStory(void);

/**
 * @brief Starts the palette cycle out of the scene back to the overworld.
 *
 * The minigame leaves through this too.
 */
void storyStartPalCycle(void);

/**
 * @brief Draws the closed part of the shutters.
 */
void story_draw(void);

/**
 * @brief Copies changed "The End" tiles into VRAM. Call this from V-blank in.
 */
void story_vblank_in(void);

#endif
