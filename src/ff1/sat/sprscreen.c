#include <sega_def.h>
#include <sega_mth.h>
#include <machine.h>
#define _SPR2_
#include <sega_spr.h>
#include <sega_scl.h>
#include <string.h>

#include "cd.h"
#include "memmap.h"
#include "graphicrefs.h"
#include "sprdata.h"
#include "sprscreen.h"
#include "vblank.h"

Uint32 image_buf[4096];

void sprite_init() {
    int count, i;

    total_sprites = 0;
    mapSpriteStart = 0;
    mapSpriteCramStart = 0;


    SCL_Vdp2Init();
    //Black back screen before V-blank starts copying menuPlane into VRAM A0.
    Uint16 backCol = 0x0000;
    SCL_SetBack(SCL_VDP2_VRAM + 0x80000 - 2, 1, &backCol);
    sprInit();
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
    total_sprites += 384;

    SCL_AllocColRam(SCL_SPR, 16, OFF);
    SCL_SetColRam(SCL_SPR, 0, 16, &font_pal);
    SCL_SetColRam(SCL_SPR, 16, 16, &player_pal_0);
    SCL_SetColRam(SCL_SPR, 32, 16, &player_pal_1);
    SCL_SetColRam(SCL_SPR, 48, 16, &weapon_pal);
    SCL_SetColRam(SCL_SPR, 64, 16, &cursor_pal_0);
    SCL_SetColRam(SCL_SPR, STONE_PALETTE_CRAM, 16, &stone_pal);
    mapSpriteCramStart = 112;

    SCL_DisplayFrame();

    SPR_2SetTvMode(SPR_TV_NORMAL,SPR_TV_320X240,OFF);
    SPR_SetEraseData(RGB16_COLOR(0,0,0), 0, 0, 319, 239);
    SCL_SetFrameInterval(0);
}

void sprite_window_mode(void) {
    SCL_SetSpriteMode(SCL_TYPE5,SCL_MIX,SCL_SP_WINDOW);
}
