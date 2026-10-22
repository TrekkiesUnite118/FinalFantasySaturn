/*******************************************************************
 *
 *                      Sliding Puzzle Minigame
 *
 * The NES 15 puzzle, opened on the ship by holding A and B after
 * pressing B 55 times. Pieces are VDP1 sprites.
 *
 *
 *
*******************************************************************/
#ifndef MINIGAME_H
#define MINIGAME_H

#include "types.h"

/**
 * @brief Sets up the puzzle box, piece graphics and shuffled pieces.
 *
 * setupStoryScreen must be run first for the bridge scene graphics.
 */
void setupMinigame(void);

/**
 * @brief Runs one frame of the minigame. Called each frame in the MINIGAME state.
 */
void doMinigame(void);

/**
 * @brief Draws the puzzle pieces as sprites.
 */
void minigame_draw(void);

#endif
