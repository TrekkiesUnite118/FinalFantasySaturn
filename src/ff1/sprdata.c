#include "types.h"

#include "cd.h"
#include "memmap.h"
#include "graphicrefs.h"
#include "nesutils.h"
#include "sprdata.h"
#include "video.h"

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
