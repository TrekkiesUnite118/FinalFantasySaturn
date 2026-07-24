#include <sega_def.h>
#include <sega_mth.h>
#include <machine.h>
#define _SPR2_
#include <sega_spr.h>
#include <sega_scl.h>
#include <string.h>

#include "cd.h"
#include "graphicrefs.h"
#include "print.h"
#include "scroll.h"
#include "sprite.h"
#include "vblank.h"

int num_sprites = 0;
SPRITE_INFO sprites[SPRITE_LIST_SIZE];
//normalize diagonal speed
#define DIAGONAL_MULTIPLIER (MTH_FIXED(0.8))

#define CommandMax    300
#define GourTblMax    300
#define LookupTblMax  100
#define CharMax       256 //CHANGE WHEN YOU INCREASE TILES BEYOND THIS POINT
#define DrawPrtyMax   256

#define BATTLE_CHAR_PALETTE_TABLE (0x0023EBB5)
#define CURSOR_PALETTE (0x00203234)

SPR_2DefineWork(work2D, CommandMax, GourTblMax, LookupTblMax, CharMax, DrawPrtyMax)

Uint32 image_buf[4096];

Uint16 large_enemy_num = 2;
Uint16 large_enemy_size = 1152;
Uint16 large_enemy_width = 48;
Uint16 large_enemy_height = 48;

Uint16 small_enemy_num = 2;
Uint16 small_enemy_size = 512;
Uint16 small_enemy_width = 32;
Uint16 small_enemy_height = 32;

Uint32 player_pal_0[] = {
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
};
Uint32 player_pal_1[] = {
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
};

Uint32 weapon_pal[] = {
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
};

Uint32 cursor_pal_0[] = {
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
};


void sprite_init() {
    int count, i;

    total_sprites = 0;
    mapSpriteStart = 0;
    mapSpriteCramStart = 0;
    

    SCL_Vdp2Init();
    SPR_2Initial(&work2D);
    SCL_SetFrameInterval(-1);
    count = 0;
    SCL_SetColRamMode(SCL_CRM24_1024);
    
    SetVblank(); //setup vblank routine
    set_imask(0);
    
    SPR_2FrameChgIntr(1); //wait until next frame to set color mode
    SCL_DisplayFrame();
    
    cd_load(font_name, image_buf, font_size * font_num);
    for (i = 0; i < font_num; i++) {
        SPR_2SetChar(i, COLOR_0, 0, font_width, font_height, (Uint8 *)(image_buf) + (i * font_size));
    }

    initCharBattlePalettes();
    initCursorPalette();
    updateWeaponPalettes(0x28, TRUE);

    Uint16 bsprite_num = 72;
    Uint16 bsprite_size = 192;
    Uint16 bsprite_width = 16;
    Uint16 bsprite_height = 24;
    total_sprites += font_num;
    Uint16 battlePalette = 16;
    for (i = 0; i < bsprite_num; i++) {
        if((i >= 6 && i < 18)  || (i >= 30 && i < 36) || (i >= 48 && i < 56) || i > 66) 
        {
            battlePalette = 16;
        } else {
            battlePalette = 32;
        }
        if( i == 5 || i == 11 || i == 17 || i == 23 || i == 29 || i == 35 || i == 41 
        || i == 47 || i == 53 || i == 59 || i == 65 || i == 71) {
            SPR_2SetChar(i + total_sprites, COLOR_0, battlePalette, bsprite_height, bsprite_width, (Uint8 *)(LWRAM_BATTLE_SPRITES) + (i * bsprite_size));
        }  else {
            SPR_2SetChar(i + total_sprites, COLOR_0, battlePalette, bsprite_width, bsprite_height, (Uint8 *)(LWRAM_BATTLE_SPRITES) + (i * bsprite_size));
        }
        
    }

    total_sprites += bsprite_num;
    Uint16 weaponSprite_num = 28;
    Uint16 weaponSprite_size = 128;
    Uint16 weaponSprite_width = 16;
    Uint16 weaponSprite_height = 16;

    weaponSpriteIndex = total_sprites;
    for(i = 0; i < weaponSprite_num; i++) {
         SPR_2SetChar(i + total_sprites, COLOR_0, 48, weaponSprite_width, weaponSprite_height, (Uint8 *)(LWRAM_WEAPON_SPRITES) + (i * weaponSprite_size));
    }
    total_sprites += weaponSprite_num;

    cursorSpriteIndex = total_sprites;
    Uint16 buffer = weaponSprite_num * weaponSprite_size;
    for (i = 0; i < 1; i++) {
        SPR_2SetChar(i + total_sprites, COLOR_0, 64, weaponSprite_width, weaponSprite_height, (Uint8 *)(LWRAM_WEAPON_SPRITES) + buffer + (i * weaponSprite_size));
    }
    total_sprites += 1;
    
    explosionSpriteIndex = total_sprites;
    buffer += weaponSprite_size;
    for (i = 0; i < 3; i++) {
        SPR_2SetChar(i + total_sprites, COLOR_0, 48, weaponSprite_width, weaponSprite_height, (Uint8 *)(LWRAM_WEAPON_SPRITES) + buffer + (i * weaponSprite_size));
    }
    
    total_sprites += 3;

    mapSpriteStart = total_sprites;
    total_sprites += 384; //make this a constant somewhere
    
   

    // SPR_2SetChar(0, COLOR_0, 0, owsprite_width, owsprite_height, (Uint8 *)(image_buf) + 256);
    SCL_AllocColRam(SCL_SPR, 16, OFF);
    SCL_SetColRam(SCL_SPR, 0, 16, &font_pal);
    SCL_SetColRam(SCL_SPR, 16, 16, &player_pal_0);
    SCL_SetColRam(SCL_SPR, 32, 16, &player_pal_1);
    SCL_SetColRam(SCL_SPR, 48, 16, &weapon_pal);
    SCL_SetColRam(SCL_SPR, 64, 16, &cursor_pal_0);
    mapSpriteCramStart = 112;

    sprite_deleteall();
    SCL_DisplayFrame();
    
    SPR_2SetTvMode(SPR_TV_NORMAL,SPR_TV_320X240,OFF);
    SPR_SetEraseData(RGB16_COLOR(0,0,0), 0, 0, 319, 239);
    SCL_SetFrameInterval(0); 
}

void sprite_draw(SPRITE_INFO *info) {
    XyInt xy[4];
    Fixed32 xOffset, yOffset, sin, cos, scaledX, scaledY;
    int i;
    
    if (info->scale == MTH_FIXED(1) && info->angle == 0) {
        xy[0].x = (Sint16)MTH_FixedToInt(info->xPos);
        xy[0].y = (Sint16)MTH_FixedToInt(info->yPos);
        SPR_2NormSpr(info->priority, info->mirror, COLOR_0, 0xffff, info->char_num, xy, NO_GOUR); //4bpp normal sprite
    }
    
    else if (info->angle == 0){
        xy[0].x = (Sint16)MTH_FixedToInt(info->xPos);
        xy[0].y = (Sint16)MTH_FixedToInt(info->yPos);
        //the way scale works is by giving the x/y coordinates of the top left and
        //bottom right corner of the sprite
        xy[1].x = (Sint16)(MTH_FixedToInt(MTH_Mul(info->xSize, info->scale) + info->xPos));
        xy[1].y = (Sint16)(MTH_FixedToInt(MTH_Mul(info->ySize, info->scale) + info->yPos));
        SPR_2ScaleSpr(info->priority, info->mirror, COLOR_0, 0xffff, info->char_num, xy, NO_GOUR); //4bpp scaled sprite
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
        SPR_2DistSpr(info->priority, info->mirror, COLOR_0, 0xffff, info->char_num, xy, NO_GOUR); //4bpp distorted sprite
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

void sprite_draw_all() {
    int i;
    SPRITE_INFO tmp;
    for (i = 0; i < num_sprites; i++) {
        if (!(sprites[i].options & OPTION_NODISP) && sprites[i].iterate != NULL) {
            sprites[i].iterate(&sprites[i]);
        }
    }
    for (i = 0; i < num_sprites; i++) {
        if (!(sprites[i].options & OPTION_NODISP)) {
            memcpy((void *)&tmp, (void *)&sprites[i], sizeof(SPRITE_INFO));
            tmp.xPos -=scrolls_x[0];
            tmp.yPos -=scrolls_y[0];
            sprite_draw(&tmp);
        }
    }
}

SPRITE_INFO *sprite_next() {
    int i;
    for (i = 0; i < SPRITE_LIST_SIZE; i++) {
        if (sprites[i].options & OPTION_NODISP) {
            num_sprites++;
            sprites[i].index = i;
            sprites[i].iterate = NULL;
            return &sprites[i];
        }
    }
    return NULL;
}

void sprite_delete(SPRITE_INFO *sprite) {
    sprite->options |= OPTION_NODISP;
    sprite->iterate = NULL;
    num_sprites--;
}

void sprite_deleteall() {
    for (int i = 0; i < SPRITE_LIST_SIZE; i++) {
        sprites[i].options = OPTION_NODISP;
    }
    num_sprites = 0;
}

void initBattleSpriteData(Uint8 *patternData) {
    
    convertNesTilesToSatTiles(patternData, (Uint8 *)LWRAM_BUFFER, 0x1800);
    buildBattleSprites((Uint8 *)LWRAM_BUFFER, (Uint8 *)(LWRAM_BATTLE_SPRITES));
}

void initWeaponSpriteData(Uint8 *patternData) {
    
    convertNesTilesToSatTiles(patternData, (Uint8 *)LWRAM_BUFFER, 0x1800);
    buildWeaponSprites((Uint8 *)LWRAM_BUFFER, (Uint8 *)(LWRAM_WEAPON_SPRITES));
}

void initMapSpriteData(Uint8 *patternData) {

    convertNesTilesToSatTiles(patternData, (Uint8 *)LWRAM_BUFFER, 0x3000);
    buildMapSprites((Uint8 *)LWRAM_BUFFER, (Uint8 *)(LWRAM_MAP_SPRITES), 0x6000);

}

void initCharBattlePalettes() {
    Uint32 pal_offset = BATTLE_CHAR_PALETTE_TABLE;

    Uint8 *palPtr = (Uint8*) pal_offset;
    player_pal_0[0] = nes_pal[palPtr[0]];
    player_pal_0[1] = nes_pal[palPtr[1]];
    player_pal_0[2] = nes_pal[palPtr[2]];
    player_pal_0[3] = nes_pal[palPtr[3]];

    player_pal_1[0] = nes_pal[palPtr[4]];
    player_pal_1[1] = nes_pal[palPtr[5]];
    player_pal_1[2] = nes_pal[palPtr[6]];
    player_pal_1[3] = nes_pal[palPtr[7]];
}

void initCursorPalette() {
    Uint32 pal_offset = CURSOR_PALETTE;

    Uint8 *palPtr = (Uint8*) pal_offset;
    cursor_pal_0[0] = nes_pal[palPtr[0]];
    cursor_pal_0[1] = nes_pal[palPtr[1]];
    cursor_pal_0[2] = nes_pal[palPtr[2]];
    cursor_pal_0[3] = nes_pal[palPtr[3]];
}

void updateWeaponPalettes(Uint8 paletteSeed, Uint8 isWeapon) {
    Uint8 paletteVaue = paletteSeed - 0x20;
    Uint16 weaponSprite_width = 16;
    Uint16 weaponSprite_height = 16;
    Uint16 weaponSprite_size = 128;
    Uint16 weaponSprite_num = 28;

    weapon_pal[0] = nes_pal[0x0F];
    weapon_pal[1] = nes_pal[paletteVaue + 0x20];
    weapon_pal[2] = nes_pal[paletteVaue + 0x10];
    if(isWeapon) {
        weapon_pal[3] = nes_pal[paletteVaue];
    } else {
        weapon_pal[3] = nes_pal[0x30];
    }
    
    SCL_SetColRam(SCL_SPR, 48, 16, &weapon_pal);

    for(Uint8 i = 0; i < weaponSprite_num; i++) {
         SPR_2SetChar(i + weaponSpriteIndex, COLOR_0, 48, weaponSprite_width, weaponSprite_height, (Uint8 *)(LWRAM_WEAPON_SPRITES) + (i * weaponSprite_size));
    }
    
}
