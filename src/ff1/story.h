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
 *
 * @param msx TRUE for the MSX picture, which fades in once it's loaded.
 */
void setupStoryScreen(Uint8 msx);

/**
 * @brief Runs one frame of the scene. Called each frame in the STORY state.
 */
void doStory(void);

/**
 * @brief Starts the palette cycle out of the scene back to the overworld, a fade in MSX mode.
 *
 * The minigame leaves through this too.
 */
void storyStartPalCycle(void);

/**
 * @brief Runs a frame of the MSX picture's load and fade in.
 *
 * @return TRUE once it's done, or right away for the NES picture.
 */
Uint8 storyFadeInFrame(void);

/**
 * @brief Gets how far up the scene's layer is scrolled, to move sprites with it.
 *
 * @return Lines scrolled, 0 for the NES picture.
 */
Sint16 storyScrollY(void);

/**
 * @brief Gets a color of the MSX scene's box.
 *
 * @param color NES box color 0-3.
 * @return The color (0x00BBGGRR).
 */
Uint32 storyMsxBoxColor(Uint8 color);

/**
 * @brief Draws the closed part of the shutters.
 */
void story_draw(void);

/**
 * @brief Copies changed "The End" tiles into VRAM. Call this from V-blank in.
 */
void story_vblank_in(void);

#endif
