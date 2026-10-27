#include "types.h"

#include "cd.h"
#include "memmap.h"
#include "graphicrefs.h"
#include "nesutils.h"
#include "sprdata.h"
#include "video.h"
#include "msxgfx.h"

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

Uint32 stone_pal[] = {
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
};

Uint32 msx_bat_pal[16];

#define BATTLE_CLASS_FRAMES (6)
#define BATTLE_FRAME_DEAD (5)
#define BATTLE_FRAME_SIZE (192)

//MSX mode the battle characters were last registered in, -1 before the first.
static Sint8 battleSpritesMsx = -1;

//Which of the NES battle palettes each class's frames use.
static Uint16 nesBattleCram(Uint8 i) {
    return ((i >= 6 && i < 18) || (i >= 30 && i < 36) || (i >= 48 && i < 56) || i > 66) ? 16 : 32;
}

void registerBattleSprites() {
    for(Uint8 i = 0; i < BATTLE_SPRITE_COUNT; i++) {
        Uint8 frame = i % BATTLE_CLASS_FRAMES;
        Uint16 w = frame == BATTLE_FRAME_DEAD ? 24 : 16;
        Uint16 h = frame == BATTLE_FRAME_DEAD ? 16 : 24;
        if(MSX_ON) {
            sprSetChar(BATTLE_CHAR_START + i, SPRITE_4BPP, MSX_BATTLE_CRAM, w, h,
                (Uint8 *)msxData(MSX_ID_BATTLE_FRAMES + (i / BATTLE_CLASS_FRAMES) * MSX_BATTLE_CLASS_FRAMES + frame));
        } else {
            sprSetChar(BATTLE_CHAR_START + i, SPRITE_4BPP, nesBattleCram(i), w, h,
                (Uint8 *)LWRAM_BATTLE_SPRITES + i * BATTLE_FRAME_SIZE);
        }
    }
    if(MSX_ON) {
        for(Uint8 c = 0; c < BATTLE_SPRITE_COUNT / BATTLE_CLASS_FRAMES; c++) {
            sprSetChar(MSX_STONE_CHAR_START + c, SPRITE_4BPP, MSX_BATTLE_CRAM, 16, 24,
                (Uint8 *)msxData(MSX_ID_BATTLE_FRAMES + c * MSX_BATTLE_CLASS_FRAMES + MSX_BATTLE_FRAME_STONE));
        }
        const Uint32 *pal = msxPalette(MSX_ID_BATTLE_PAL);
        for(Uint8 i = 0; i < 16; i++) {
            msx_bat_pal[i] = pal[i];
        }
        vidSetPalette(VID_SPR, MSX_BATTLE_CRAM, 16, msx_bat_pal);
    }
    initCursorPalette();
    vidSetPalette(VID_SPR, CURSOR_PALETTE_CRAM, 16, cursor_pal_0);
    battleSpritesMsx = MSX_ON;
}

void updateBattleSprites() {
    if(battleSpritesMsx != MSX_ON) {
        registerBattleSprites();
    }
}

void drawStoneSprite(SPRITE_INFO *sprite, Uint8 charClass) {
    if(MSX_ON) {
        sprite->char_num = MSX_STONE_CHAR_START + charClass;
        sprite_draw(sprite);
    } else {
        sprite->char_num = BATTLE_CHAR_START + charClass * BATTLE_CLASS_FRAMES + 4;
        sprite_draw_pal(sprite, STONE_PALETTE_CRAM);
    }
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

    //NES sprite palette 3, used for stoned characters.
    stone_pal[0] = nes_pal[palPtr[12]];
    stone_pal[1] = nes_pal[palPtr[13]];
    stone_pal[2] = nes_pal[palPtr[14]];
    stone_pal[3] = nes_pal[palPtr[15]];
}

void initCursorPalette() {
    Uint32 pal_offset = CURSOR_PALETTE;

    Uint8 *palPtr = (Uint8*) pal_offset;
    if(MSX_ON) {
        msxCursorPalette(palPtr, cursor_pal_0);
        return;
    }
    cursor_pal_0[0] = nes_pal[palPtr[0]];
    cursor_pal_0[1] = nes_pal[palPtr[1]];
    cursor_pal_0[2] = nes_pal[palPtr[2]];
    cursor_pal_0[3] = nes_pal[palPtr[3]];
}

//NES UpdateVariablePalette. NES palette entries are 6 bits, so any seed is masked into range.
void updateWeaponPalettes(Uint8 paletteSeed, Uint8 isWeapon) {
    Uint16 weaponSprite_width = 16;
    Uint16 weaponSprite_height = 16;
    Uint16 weaponSprite_size = 128;
    Uint16 weaponSprite_num = 28;

    weapon_pal[0] = nes_pal[0x0F];
    weapon_pal[1] = nes_pal[paletteSeed & 0x3F];
    weapon_pal[2] = nes_pal[(Uint8)(paletteSeed - 0x10) & 0x3F];
    if(isWeapon) {
        weapon_pal[3] = nes_pal[(Uint8)(paletteSeed - 0x20) & 0x3F];
    } else {
        weapon_pal[3] = nes_pal[0x30];
    }

    vidSetPalette(VID_SPR, 48, 16, &weapon_pal);

    for(Uint8 i = 0; i < weaponSprite_num; i++) {
         sprSetChar(i + weaponSpriteIndex, SPRITE_4BPP, 48, weaponSprite_width, weaponSprite_height, (Uint8 *)(LWRAM_WEAPON_SPRITES) + (i * weaponSprite_size));
    }

}
