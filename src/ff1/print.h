/*******************************************************************
 *
 *                      Sprite Text
 *
 * A 40x30 grid of 8x8 characters drawn with the sprite font.
 * Only the SATURN SAD bad ROM screen writes text into it.
 *
 *
*******************************************************************/
#ifndef PRINT_H
#define PRINT_H

/**
 * @brief Clears the text grid.
 */
void print_init();

/**
 * @brief Writes ASCII text into the text grid.
 *
 * @param ch Pointer to the text.
 * @param row Row (0-29).
 * @param col Column of the first character (0-39).
 * @param length Number of characters to write.
 */
void print_string(char *ch, int row, int col, int length);

/**
 * @brief Draws the text grid as sprites. Call it each frame the text should show.
 */
void print_display();
#endif
