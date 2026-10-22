#include <sega_def.h>
#include <sega_mth.h>
#define _SPR2_
#include <sega_spr.h>

#include "sprite.h"

//normalize diagonal speed
#define DIAGONAL_MULTIPLIER (MTH_FIXED(0.8))

#define CommandMax    300
#define GourTblMax    300
#define LookupTblMax  100
#define CharMax       512 //CHANGE WHEN YOU INCREASE TILES BEYOND THIS POINT
#define DrawPrtyMax   256

SPR_2DefineWork(work2D, CommandMax, GourTblMax, LookupTblMax, CharMax, DrawPrtyMax)

//Compile error if the copies in sprite.h drift from SBL.
typedef char sprCheck[(sizeof(SprPoint) == sizeof(XyInt) && SPRITE_4BPP == COLOR_0 &&
                       SPRITE_ECDSPD_OFF == ECDSPD_DISABLE && SPRITE_REPLACE == COMPO_REP) ? 1 : -1];

void sprInit(void) {
    SPR_2Initial(&work2D);
}

void sprSetChar(Uint16 num, Uint16 colorMode, Uint16 color, Uint16 w, Uint16 h, Uint8 *data) {
    SPR_2SetChar(num, colorMode, color, w, h, data);
}

void sprNormal(Sint32 priority, Uint16 mirror, Uint16 colorMode, Uint16 color, Uint16 num, SprPoint *xy) {
    SPR_2NormSpr(priority, mirror, colorMode, color, num, (XyInt *)xy, NO_GOUR);
}

void sprScaled(Sint32 priority, Uint16 mirror, Uint16 colorMode, Uint16 color, Uint16 num, SprPoint *xy) {
    SPR_2ScaleSpr(priority, mirror, colorMode, color, num, (XyInt *)xy, NO_GOUR);
}

void sprDistorted(Sint32 priority, Uint16 mirror, Uint16 colorMode, Uint16 color, Uint16 num, SprPoint *xy) {
    SPR_2DistSpr(priority, mirror, colorMode, color, num, (XyInt *)xy, NO_GOUR);
}

void sprPolygon(Sint32 priority, Uint16 drawMode, Uint16 color, SprPoint *xy) {
    SPR_2Polygon(priority, drawMode, color, (XyInt *)xy, NO_GOUR);
}

void sprite_draw(SPRITE_INFO *info) {
    sprite_draw_pal(info, 0xffff);
}

//color is a sprite CRAM bank offset; 0xffff uses the char's registered palette.
void sprite_draw_pal(SPRITE_INFO *info, Uint16 color) {
    XyInt xy[4];
    Fixed32 xOffset, yOffset, sin, cos, scaledX, scaledY;
    int i;

    if (info->scale == MTH_FIXED(1) && info->angle == 0) {
        xy[0].x = (Sint16)MTH_FixedToInt(info->xPos);
        xy[0].y = (Sint16)MTH_FixedToInt(info->yPos);
        SPR_2NormSpr(info->priority, info->mirror, COLOR_0, color, info->char_num, xy, NO_GOUR); //4bpp normal sprite
    }

    else if (info->angle == 0){
        xy[0].x = (Sint16)MTH_FixedToInt(info->xPos);
        xy[0].y = (Sint16)MTH_FixedToInt(info->yPos);
        //the way scale works is by giving the x/y coordinates of the top left and
        //bottom right corner of the sprite
        xy[1].x = (Sint16)(MTH_FixedToInt(MTH_Mul(info->xSize, info->scale) + info->xPos));
        xy[1].y = (Sint16)(MTH_FixedToInt(MTH_Mul(info->ySize, info->scale) + info->yPos));
        SPR_2ScaleSpr(info->priority, info->mirror, COLOR_0, color, info->char_num, xy, NO_GOUR); //4bpp scaled sprite
    }

    else {
        //offset of top left sprite corner from the origin
        xOffset = -(MTH_Mul(info->xSize >> 1, info->scale));
        yOffset = -(MTH_Mul(info->ySize >> 1, info->scale));
        sin = MTH_Sin(info->angle);
        cos = MTH_Cos(info->angle);
        scaledX = info->xPos + MTH_Mul(info->xSize >> 1, info->scale);
        scaledY = info->yPos + MTH_Mul(info->ySize >> 1, info->scale);
        //formula from
        //https://gamedev.stackexchange.com/questions/86755/
        for (i = 0; i < 4; i++) {
            if (i == 1) xOffset = -xOffset; //upper right
            if (i == 2) yOffset = -yOffset; //lower right
            if (i == 3) xOffset = -xOffset; //lower left
            xy[i].x = (Sint16)MTH_FixedToInt(MTH_Mul(xOffset, cos) -
                MTH_Mul(yOffset, sin) + scaledX);
            xy[i].y = (Sint16)MTH_FixedToInt(MTH_Mul(xOffset, sin) +
                MTH_Mul(yOffset, cos) + scaledY);
        }
        SPR_2DistSpr(info->priority, info->mirror, COLOR_0, color, info->char_num, xy, NO_GOUR); //4bpp distorted sprite
    }
}

void sprite_make(int tile_num, Fixed32 x, Fixed32 y, Sint32 priority, SPRITE_INFO *ptr) {
    ptr->char_num = tile_num;
    ptr->options = 0;
    ptr->xPos = x;
    ptr->yPos = y;
    ptr->mirror = 0;
    ptr->dx = 0;
    ptr->dy = 0;
    ptr->scale = MTH_FIXED(1);
    ptr->angle = 0;
    ptr->animTimer = 0;
    ptr->moveTimer = 0;
    ptr->animCursor = 0;
    ptr->priority = priority;
    ptr->iterate = NULL;
}
