#include <string.h>
#include <sega_def.h>
#include <sega_mth.h>
#include <sega_scl.h>

#include "nesutils.h"

#include "cd.h"
#include "overworld.h"
#include "player.h"
#include "print.h"
#include "scroll.h"
#include "sprite.h"

#define    MAP_OFFSET    (0x0800 *4)
#define    MAP_NUM        16

Fixed32 scrolls_x[]  = {0, 0, 0, 0};
Fixed32 scrolls_y[]  = {0, 0, 0, 0};
Sint32 map_tiles_x[] = {0, 0};
Sint32 map_tiles_y[] = {0, 0};
Uint32 copy_modes[]  = {0, 0};

int x_cords[] = {0, 32, 0, 32, 64, 96, 64, 96, 128, 160, 128, 160, 192, 224, 192, 224, 0, 32, 0, 32, 64, 96, 64, 96, 128, 160, 128, 160, 192, 224, 192, 224, 0, 32, 0, 32, 64, 96, 64, 96, 128, 160, 128, 160, 192, 224, 192, 224, 0, 32, 0, 32, 64, 96, 64, 96, 128, 160, 128, 160, 192, 224, 192, 224};
int y_cords[] = {0, 0, 32, 32, 0, 0, 32, 32, 0, 0, 32, 32, 0, 0, 32, 32, 64, 64, 96, 96, 64, 64, 96, 96, 64, 64, 96, 96, 64, 64, 96, 96, 128, 128, 160, 160, 128, 128, 160, 160, 128, 128, 160, 160, 128, 128, 160, 160, 192, 192, 224, 224, 192, 192, 224, 224, 192, 192, 224, 224, 192, 192, 224, 224};
Uint16 *maps[2]; //map locations in WRAM for the scrolling backgrounds
Uint16 **tileMapPtr;
int scroll_xsize = 0;
int scroll_ysize = 0;
Uint32 vram[] = {SCL_VDP2_VRAM_A0, SCL_VDP2_VRAM_A0 + 0x800, SCL_VDP2_VRAM_B1, SCL_VDP2_VRAM_B1 + 0x800}; //where in VRAM each tilemap is
#define VRAM_PTR(bg) ((Uint16 *)vram[bg])
int scroll_transition_state = TSTATE_NULL;


SclConfig Rbg0Scfg;


void scroll_init(LEVEL *level) {
    
    Uint8 *VramWorkP;
    Uint16*TilemapVram;
    Uint16 BackCol;
    SclVramConfig vram_cfg;
    
    SCL_SetDisplayMode(SCL_NON_INTER,SCL_240LINE,SCL_NORMAL_A);

    SCL_SetColRamMode(SCL_CRM24_1024);

    Uint32 numColors = 4 * 16;

    SCL_AllocColRam(SCL_RBG0, numColors, OFF);
    Uint32 palette[8][16] = {{0}};
    Uint8 palette_iter = 0;
    Uint8 nes_val = 0;
    for(int i = 0; i < level->bg.total_colors; i += 4) {

        nes_val = level->bg.palette[i];
        palette[palette_iter][0] = nes_pal[nes_val];
        nes_val = level->bg.palette[i + 1];
        palette[palette_iter][1] = nes_pal[nes_val];
        nes_val = level->bg.palette[i + 2];
        palette[palette_iter][2] = nes_pal[nes_val];
        nes_val = level->bg.palette[i + 3];
        palette[palette_iter][3] = nes_pal[nes_val];

        SCL_SetColRam(SCL_RBG0, palette_iter * 16 , 16, (void *)palette[palette_iter]);   
        palette_iter++;
    } 

    BackCol = nes_pal[0x0F]; //set the background color to black
    SCL_SetBack(SCL_VDP2_VRAM+0x80000-2,1,&BackCol);
   
    SCL_InitRotateTable(SCL_VDP2_VRAM_B0,1,SCL_RBG0,SCL_NON);

    //setup VRAM configuration
    SCL_InitVramConfigTb(&vram_cfg);
        vram_cfg.vramModeA = ON; //separate VRAM A into A0 & A1
        vram_cfg.vramModeB = ON; //separate VRAM B into B0 & B1
        vram_cfg.vramA0     = SCL_RBG0_CHAR;    
        vram_cfg.vramA1     = SCL_RBG0_PN;    
        vram_cfg.vramB1     = SCL_RBG0_K;
    SCL_SetVramConfig(&vram_cfg);

    SCL_InitConfigTb(&Rbg0Scfg);
        Rbg0Scfg.dispenbl      = ON;
        Rbg0Scfg.charsize      = SCL_CHAR_SIZE_2X2;
        Rbg0Scfg.pnamesize     = SCL_PN1WORD;
        Rbg0Scfg.flip          = SCL_PN_12BIT;
        Rbg0Scfg.platesize     = SCL_PL_SIZE_2X2;
        Rbg0Scfg.coltype       = SCL_COL_TYPE_16;
        Rbg0Scfg.datatype      = SCL_CELL;
        Rbg0Scfg.mapover       = SCL_OVER_0;
        Rbg0Scfg.patnamecontrl = 0x0004;
        for(int i=0;i<MAP_NUM;i++)
        Rbg0Scfg.plate_addr[i] = SCL_VDP2_VRAM_A1 + i * MAP_OFFSET;

    //---rbg0: far bg---
    VramWorkP = (Uint8 *)SCL_VDP2_VRAM_A0;

    convertNesTilesToSatTiles(level->bg.tile_location, (Uint8 *)LWRAM_BUFFER, level->bg.rawlength);
    buildMapTiles((Uint8 *)LWRAM_BUFFER, VramWorkP, (Uint8 *)LWRAM + 0x110);

    TilemapVram = (Uint8 *)SCL_VDP2_VRAM_A1;
    tileMapPtr = &(level->bg.map_location);

    int x = 0;
    int y = 0;
    update_plane(32, 32);
    for(int planeNum = 0; planeNum < 16; planeNum++){
        int offset = 0;
        for(int i = 0; i < 4; i++) {
            update_plane(y_cords[y], x_cords[x]);
            memcpy(Rbg0Scfg.plate_addr[planeNum] + offset, level->bg.map_location, 0x800);
            x++;
            y++;
            offset += 0x800;
        }
    }

    SCL_Open(SCL_RBG_TB_A);
    SCL_MoveTo(FIXED(0), FIXED(0), FIXED(0));/* Home Position */
    
    SCL_Scale(FIXED(1.0), FIXED(1.0));
    SCL_Close();
     //set layer priorities
    SCL_SetPriority(SCL_SPR,  6);
    SCL_SetPriority(SCL_RBG0, 4);
    SCL_SetConfig(SCL_RBG0, &Rbg0Scfg);
}

void scroll_move(int num, Fixed32 x, Fixed32 y) {
    scrolls_x[num] += x;
    scrolls_y[num] += y;
    
    switch (num) {
        case SCROLL_BACKGROUND1:
            SCL_Open(SCL_RBG0);
            break;
    }
    SCL_MoveTo(scrolls_x[num], scrolls_y[num], 0);
    SCL_Close();
}

//moves scroll absolutely to coordinates
void scroll_set(int num, Fixed32 x, Fixed32 y) {
    scroll_move(num, x - scrolls_x[num], y - scrolls_y[num]);
}

void scroll_rotate_x(Fixed32 rotateAngle) {
    SCL_RotateTo(FIXED(rotateAngle), FIXED(0), FIXED(0), SCL_X_AXIS);
}

//gets the value at the given coordinates for a map
Uint16 scroll_get(int map, int x, int y) {
    
    if ( x >= scroll_xsize || x < 0 || y >= scroll_ysize || y < 0) {
        return 0;
    }
    Uint16 (*tileMap)[32][32] = &tileMapPtr;

    return (*tileMap)[y][x];
}

void scroll_enable() {
     Rbg0Scfg.dispenbl      = ON;
    SCL_SetConfig(SCL_RBG0, &Rbg0Scfg);
}
void scroll_disable() {
    Rbg0Scfg.dispenbl      = OFF;
    SCL_SetConfig(SCL_RBG0, &Rbg0Scfg);
}
