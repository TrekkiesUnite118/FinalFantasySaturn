#include "types.h"
#include "sprdata.h"
#include "print.h"

//Text grid of 8x8 sprite font characters, 65535 = empty.
#define ROWS 30
#define COLS 40
Uint16 text[ROWS][COLS];


void print_init() {
    int i, j;
    for (i = 0; i < ROWS; i++) {
        for (j = 0; j < COLS; j++) {
            text[i][j] = 65535;
        }
    }
}

void print_string(char *ch, int row, int col, int length) {
    int index = 0;
    while (index < length) {
        text[row][col + index] = (Uint16)ch[index] - 32;
        index++;
    }
}

void print_display() {
    int i, j;
    SPRITE_INFO text_spr;
    for (i = 0; i < ROWS; i++) {
        for (j = 0; j < COLS; j++) {
            if (text[i][j] != 65535) {
                sprite_make(text[i][j],
                    intToFixed(j << 3),
                    intToFixed(i << 3),
                    (Sint32) CURSOR_PRIORITY, &text_spr);
                sprite_draw(&text_spr);
            }
        }
    }
}
