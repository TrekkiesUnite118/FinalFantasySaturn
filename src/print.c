#include "sprite.h"
#include "print.h"


//320x224, 8x16 font = 14 rows, 40 columns
#define ROWS 30
#define COLS 40
#define FONT_X 8
#define FONT_Y 8
Uint16 text[ROWS][COLS];


void print_init() {
    int i, j;
    for (i = 0; i < ROWS; i++) {
        for (j = 0; j < COLS; j++) {
            text[i][j] = 65535;
        }
    }
}

void print_num(Uint32 num, int row, int col) {
    int right_col = col + 9; //rightmost column
    int i;
    for (i = 0; i <= 9; i++) {
        text[row][right_col--] = (num % 10) + 16;
        num /= 10;
    }
}

void print_hp_num(Uint32 num, int row, int col) {
    int right_col = col + 2; //rightmost column
    int i;
    int numOfDigits = 2;
    if(num < 100) {
        numOfDigits = 1;
    }
    
    if(num < 10) {
        numOfDigits = 0;
    }


    for (i = 0; i <= 2; i++) {
        if(i <= numOfDigits) {
            text[row][right_col--] = (num % 10) + 16;
            num /= 10;
        } else {
            text[row][right_col--] = 0;
        }
    }
}

void print_mp_num(Uint8 num, int row, int col) {
    int right_col = col + 2; //rightmost column
    int i;
    int numOfDigits = 2;
    if(num < 100) {
        numOfDigits = 1;
    }
    
    if(num < 10) {
        numOfDigits = 0;
    }


    for (i = 0; i <= 2; i++) {
        if(i <= numOfDigits) {
            text[row][right_col--] = (num % 10) + 16;
            num /= 10;
        } else {
            text[row][right_col--] = 0;
        }
    }
}

void print_dmg(Uint16 num, int row, int col) {
    int i;
    int numOfDigits = 2;
    char dmg[3];
    dmg[0] = 'D';
    dmg[1] = 'M';
    dmg[2] = 'G';
    if(num < 100) {
        numOfDigits = 1;
    }
    
    if(num < 10) {
        numOfDigits = 0;
    }
    int right_col = col + numOfDigits;

    for (i = 0; i <= numOfDigits; i++) {
        text[row][right_col--] = (num % 10) + 16;
        num /= 10;
    }

    int index = 0;
    int dmgColStart = 0;

    if(numOfDigits == 2) {
        dmgColStart = col + 3;   
    } else if(numOfDigits == 1) {
        dmgColStart = col + 2;
    } else {
        dmgColStart = col + 1;
    }

    while(index < 3) {
        text[row][dmgColStart + index] = (Uint16)dmg[index] - 32;
        index++;
    }
}

void print_char_name(char *ch, int row, int col) {
    int index = 0;
    while (index < 6) {
        text[row][col + index] = (Uint16)ch[index] - 32;
        index++;
    }
}

void print_string(char *ch, int row, int col, int length) {
    int index = 0;
    while (index < length) {
        text[row][col + index] = (Uint16)ch[index] - 32;
        index++;
    }
}

void print_clear(int row, int col, int length) {
    int index = 0;
     while (index < length) {
        text[row][col + index] = 65535;
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
                    MTH_IntToFixed(j << 3), // * FONT_X
                    MTH_IntToFixed(i << 3), // * FONT_Y
                    (Sint32) CURSOR_PRIORITY, &text_spr);
                sprite_draw(&text_spr);
            }
        }
    }
}
