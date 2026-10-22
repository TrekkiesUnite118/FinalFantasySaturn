/*******************************************************************
 *
 *                      Graphic References
 *
 * The sprite font's size, file name and palette (font.c).
 * The font is loaded from the CD by sprite_init.
 *
 *
 *
*******************************************************************/
#ifndef GRAPHICREFS_H
#define GRAPHICREFS_H

#include "nespal.h"

//---sprite graphics---
//font.c
extern Uint16 font_num;
extern Uint16 font_size;
extern Uint16 font_width;
extern Uint16 font_height;
extern char font_name[];
extern Uint32 font_pal[];

#endif
