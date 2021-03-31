#include <string.h>
#include <sega_def.h>
#include <sega_mth.h>
#include <sega_scl.h>

#include "cd.h"
#include "player.h"
#include "print.h"
#include "static.h"
#include "sprite.h"

#define    MAP_OFFSET    (0x0800 *4)
#define    MAP_NUM        16

Uint32 static_screen_vram[] = {SCL_VDP2_VRAM_A0, SCL_VDP2_VRAM_A0 + 0x800, SCL_VDP2_VRAM_B1, SCL_VDP2_VRAM_B1 + 0x800}; //where in VRAM each tilemap is
#define VRAM_PTR(bg) ((Uint16 *)static_screen_vram[bg])
Uint8 **tileMapPtr;
Uint16 *lwram_ptr = (Uint16 *)LWRAM_BUFFER;

/*
 * 0: NBG0 Pattern Name
 * 1: NBG1 Pattern Name
 * 2: NBG2 Pattern Name
 * 3: NBG3 Pattern Name
 * 4: NBG0 Character Pattern
 * 5: NBG1 Character Pattern
 * 6: NBG2 Character Pattern
 * 7: NBG3 Character Pattern
 * C: NBG0 Vertical Scroll Table
 * D: NBG1 Vertical Scroll Table
 * E: CPU Read/Write
 * F: No Access
 */

/*
 Data Type            # Accesses required
 Pattern name data          1
 16-color tiles                  1
 256-color tiles              2
 2048-color tiles              4
 32K-color tiles              4
 16M-color tiles              8
 Vertical scroll data         1
 */

// There's also numerous read restrictions, see SOA technical bulletin #6 for more information

Uint16    CycleTb[]={
    0x0011,0xeeee,
    0x5555,0x4444,
    0x6677,0xffff,
    0x23ff,0xeeee
};


SclConfig nbg1Scfg;

void static_init(STATIC_SCREEN *screen) {
    
    Uint8 *VramWorkP;
    Uint16 *TilemapVram;
    Uint16 BackCol;
    SclVramConfig vram_cfg;
    
    SCL_SetDisplayMode(SCL_NON_INTER,SCL_224LINE,SCL_NORMAL_A);

    SCL_SetColRamMode(SCL_CRM24_1024);

    SCL_AllocColRam(SCL_NBG1, 16, OFF);
    SCL_SetColRam(SCL_NBG1, 0, 16, (void *)(screen->bg.palette));        

    BackCol = 0x0000; //set the background color to black
    SCL_SetBack(SCL_VDP2_VRAM+0x80000-2,1,&BackCol);

    
    SCL_InitRotateTable(SCL_VDP2_VRAM_B1,1,SCL_NBG1,SCL_NON);

    //setup VRAM configuration
    SCL_InitVramConfigTb(&vram_cfg);
        vram_cfg.vramModeA = ON; //separate VRAM A into A0 & A1
        vram_cfg.vramModeB = ON; //separate VRAM B into B0 & B1
    SCL_SetVramConfig(&vram_cfg);

    SCL_InitConfigTb(&nbg1Scfg);
        nbg1Scfg.dispenbl      = ON;
        nbg1Scfg.charsize      = SCL_CHAR_SIZE_1X1;
        nbg1Scfg.pnamesize     = SCL_PN1WORD;
        nbg1Scfg.flip          = SCL_PN_12BIT;
        nbg1Scfg.platesize     = SCL_PL_SIZE_1X1;
        nbg1Scfg.coltype       = SCL_COL_TYPE_16;
        nbg1Scfg.datatype      = SCL_CELL;
        nbg1Scfg.mapover       = SCL_OVER_0;
        nbg1Scfg.patnamecontrl = 0x0004;
        for(int i=0;i<MAP_NUM;i++)
        nbg1Scfg.plate_addr[i] = SCL_VDP2_VRAM_A0 + i * MAP_OFFSET;

    //---nbg1: bg---
    VramWorkP = (Uint8 *)SCL_VDP2_VRAM_A1;
    cd_load(screen->bg.tile_name, lwram_ptr, 128 * screen->bg.tile_num);
    memcpy(VramWorkP, (void *)LWRAM_BUFFER, 128 * screen->bg.tile_num);
    TilemapVram = (Uint8 *)SCL_VDP2_VRAM_A0;
    tileMapPtr = &(screen->bg.map_name);

    cd_load(screen->bg.map_name, lwram_ptr, (screen->bg.map_width * screen->bg.map_height) * 2);
    int tileHPos = 0;
    int lwramPos = 0;
    for (int i = 0; i < 64 * 64; i++) {
        if(tileHPos < 40) {
            TilemapVram[i] = lwram_ptr[lwramPos];
            lwramPos++;
        } else {
            TilemapVram[i] = 0;
        }
        
        tileHPos++;
        if(tileHPos == 64) {
            tileHPos = 0;
        }
    }
    //setup VRAM configuration
    SCL_InitVramConfigTb(&vram_cfg);
        vram_cfg.vramModeA = ON; //separate VRAM A into A0 & A1
        vram_cfg.vramModeB = ON; //separate VRAM B into B0 & B1
    SCL_SetVramConfig(&vram_cfg);

    //setup vram access pattern
    SCL_SetCycleTable(CycleTb);
    SCL_Open(SCL_NBG1);
        SCL_MoveTo(FIXED(0), FIXED(0), 0);
    SCL_Close();
     //set layer priorities
    SCL_SetPriority(SCL_SPR,  6);
    SCL_SetPriority(SCL_NBG1, 4);
    SCL_SetConfig(SCL_NBG1, &nbg1Scfg);
}
void static_enable() {
     nbg1Scfg.dispenbl      = ON;
    SCL_SetConfig(SCL_NBG1, &nbg1Scfg);
}
void static_disable() {
    nbg1Scfg.dispenbl      = OFF;
    SCL_SetConfig(SCL_NBG1, &nbg1Scfg);
}
