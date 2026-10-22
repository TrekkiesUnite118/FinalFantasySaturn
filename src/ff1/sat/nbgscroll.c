#include <string.h>
#include <sega_def.h>
#include <sega_mth.h>
#include <sega_scl.h>

#include "cd.h"
#include "memmap.h"
#include "graphicrefs.h"
#include "nesutils.h"
#include "menu.h"
#include "stdmap.h"
#include "dialog.h"
#include "wipe.h"
#include "nbgscroll.h"

static Uint16 smCycleTb[] = {
    0x4fee, 0xeeee,
    0x06ee, 0xeeee,
    0x2eee, 0xeeee,
    0xfeee, 0xeeee
};

static SclConfig nbg0Scfg;
static SclConfig nbg2Scfg;

void smScreenInit(Uint8 reenter, void (*wipeDone)(void)) {
    SclVramConfig vram_cfg;

    SCL_FreeColRam(SCL_RBG0);
    SCL_FreeColRam(SCL_NBG1);
    SCL_SetColRamMode(SCL_CRM24_1024);
    SCL_AllocColRam(SCL_NBG0, 64, OFF);
    SCL_AllocColRam(SCL_NBG2, 80, OFF);
    SCL_SetColRam(SCL_NBG2, 0, 16, menu_pal_0);
    SCL_SetColRam(SCL_NBG2, 16, 16, menu_pal_1);
    SCL_SetColRam(SCL_NBG2, 32, 16, menu_pal_2);
    SCL_SetColRam(SCL_NBG2, 48, 16, menu_pal_3);
    SCL_SetColRam(SCL_NBG2, 64, 16, menu_pal_4);
    smUploadPalettes();

    SCL_InitVramConfigTb(&vram_cfg);
        vram_cfg.vramModeA = ON;
        vram_cfg.vramModeB = ON;
    SCL_SetVramConfig(&vram_cfg);

    //SBL writes the rotation parameter table every vblank. The overworld left it at B0, where the NBG2 plane is.
    SCL_InitRotateTable(SCL_VDP2_VRAM_B1, 1, SCL_RBG0, SCL_NON);

    //PPU $00-$7F is the tileset, $80-$FF the menu CHR.
    convertNesTilesToSatTiles((Uint8 *)(SM_TILESET_CHR + cur_tileset * 0x800), (Uint8 *)LWRAM_BUFFER, 0x800);
    memcpy((Uint8 *)LWRAM_BUFFER + 0x1000, (Uint8 *)LWRAM_MENU_TILES + 0x1000, 0x1000);
    buildMapTiles((Uint8 *)LWRAM_BUFFER, (Uint8 *)SM_NBG0_CHAR, (Uint8 *)(LUT_SM_TILESET_TSA + cur_tileset * 0x200));
    smFullRedraw();

    memcpy((Uint8 *)SM_NBG2_CHAR, (Uint8 *)LWRAM_MENU_TILES, 0x2000);
    clearMenuPlane();
    memcpy((Uint8 *)SM_NBG2_PN, &menuPlane, 0x2000);

    SCL_InitConfigTb(&nbg0Scfg);
        nbg0Scfg.dispenbl      = ON;
        nbg0Scfg.charsize      = SCL_CHAR_SIZE_2X2;
        nbg0Scfg.pnamesize     = SCL_PN1WORD;
        nbg0Scfg.flip          = SCL_PN_12BIT;
        nbg0Scfg.platesize     = SCL_PL_SIZE_2X2;
        nbg0Scfg.coltype       = SCL_COL_TYPE_16;
        nbg0Scfg.datatype      = SCL_CELL;
        nbg0Scfg.mapover       = SCL_OVER_0;
        nbg0Scfg.patnamecontrl = 0x0004;
        for(int i = 0; i < 4; i++)
        nbg0Scfg.plate_addr[i] = SM_NBG0_PN;

    SCL_InitConfigTb(&nbg2Scfg);
        nbg2Scfg.dispenbl      = ON;
        nbg2Scfg.charsize      = SCL_CHAR_SIZE_1X1;
        nbg2Scfg.pnamesize     = SCL_PN1WORD;
        nbg2Scfg.flip          = SCL_PN_12BIT;
        nbg2Scfg.platesize     = SCL_PL_SIZE_1X1;
        nbg2Scfg.coltype       = SCL_COL_TYPE_16;
        nbg2Scfg.datatype      = SCL_CELL;
        nbg2Scfg.mapover       = SCL_OVER_0;
        nbg2Scfg.patnamecontrl = 0x0004;
        for(int i = 0; i < 4; i++)
        nbg2Scfg.plate_addr[i] = SM_NBG2_PN;

    SCL_SetCycleTable(smCycleTb);
    //NBG2 cell x,y is NES tile x,y: -8 x lines it up with the map, 1 y matches SM_VIEW_Y.
    SCL_Open(SCL_NBG2);
        SCL_MoveTo(FIXED(512 - 8), FIXED(1), 0);
    SCL_Close();
    dlgSetVisible(0);
    SCL_SetPriority(SCL_SPR,  6);
    SCL_SetPriority(SCL_NBG2, 5);
    SCL_SetPriority(SCL_NBG0, 4);
    SCL_SetPriority(SCL_SP1,  3); //NES BG priority sprites.
    if(reenter) {
        SCL_SetWindow(SCL_W1, 0, 0, 0, 0, 0, 0, 0);
    } else {
        wipeStart(TRUE, SCL_NBG0, wipeDone);
    }
    SCL_SetConfig(SCL_NBG0, &nbg0Scfg);
    SCL_SetConfig(SCL_NBG2, &nbg2Scfg);
}

void sm_disable() {
    nbg0Scfg.dispenbl = OFF;
    nbg2Scfg.dispenbl = OFF;
    SCL_SetConfig(SCL_NBG0, &nbg0Scfg);
    SCL_SetConfig(SCL_NBG2, &nbg2Scfg);
    SCL_SetWindow(SCL_W0, 0, 0, 0, 0, 0, 0, 0);
    SCL_FreeColRam(SCL_NBG0);
    SCL_FreeColRam(SCL_NBG2);
}

Uint8 smScreenShown() {
    return nbg0Scfg.dispenbl == ON;
}

//Called from the vblank handler. SBL skips its register copy while an SCL_Open is unclosed,
//so scrolling from the free-running main loop drops frames.
void smScreenScroll(Uint16 x, Uint16 y) {
    SCL_Open(SCL_NBG0);
        SCL_MoveTo(MTH_FIXED(x), MTH_FIXED(y), 0);
    SCL_Close();
}

//The altar beam's monochrome. With VDP2 in hi-res, color RAM mode 0 looks up the second image's palette
//colors in the second half of CRAM, so grey copies go there. NBG2 goes on top inside W0 at ratio 31
//(0/32 of itself), so the map and sprites under it show their grey colors.
#define SM_CRAM32 ((volatile Uint32 *)SCL_COLRAM_ADDR)
#define SM_CRAM16 ((volatile Uint16 *)SCL_COLRAM_ADDR)
#define SM_CRAM_COLORS (1024)
#define SM_RAMCTL ((volatile Uint16 *)0x25F8000E)
//Two line window tables, so the one being displayed isn't rewritten.
#define SM_ALTAR_LWT (SCL_VDP2_VRAM_B0 + 0x4000)
#define SM_ALTAR_LWT_SIZE (0x400)

enum {
    SM_CRAM_NONE,
    SM_CRAM_GRAY,
    SM_CRAM_RESTORE
};

static Uint32 smCramSave[SM_CRAM_COLORS];
static Uint16 smCramGray[SM_CRAM_COLORS * 2];
static Uint8 smCramPending = SM_CRAM_NONE;
static SclLineWindowTb smAltarWin[240];
static Uint8 smAltarLwt;

//CRAM's layout depends on the mode, so it's set now instead of in the next V-blank.
static void smSetCramMode(Uint32 mode) {
    SCL_SetColRamMode(mode);
    *SM_RAMCTL = Scl_s_reg.ramcontrl;
}

void smScreenCramReset() {
    smCramPending = SM_CRAM_NONE;
    smSetCramMode(SCL_CRM24_1024);
}

//NES monochrome keeps the brightness row of a color. Colors not in the NES palette use luminance.
static Uint32 smGrayColor(Uint32 c) {
    Uint32 rgb = c & 0xFFFFFF;
    Uint32 y;

    for(Uint8 i = 0; i < 64; i++) {
        if(nes_pal[i] == rgb) {
            return (c & 0x80000000) | nes_pal[i & 0x30];
        }
    }
    y = ((c & 0xFF) * 77 + ((c >> 8) & 0xFF) * 150 + ((c >> 16) & 0xFF) * 29) >> 8;
    return (c & 0x80000000) | (y << 16) | (y << 8) | y;
}

//Mode 2 color to mode 0/1.
static Uint16 smTo15(Uint32 c) {
    return ((c >> 16) & 0x8000) | ((c >> 9) & 0x7C00) | ((c >> 6) & 0x03E0) | ((c >> 3) & 0x001F);
}

//Called at V-blank in, before SBL copies the registers set up with it.
void sm_vblank_in() {
    if(smCramPending == SM_CRAM_GRAY) {
        //Mode 1 doesn't mirror writes, so the halves can differ.
        smSetCramMode(SCL_CRM15_2048);
        for(Uint16 i = 0; i < SM_CRAM_COLORS * 2; i++) {
            SM_CRAM16[i] = smCramGray[i];
        }
        smSetCramMode(SCL_CRM15_1024);
    } else if(smCramPending == SM_CRAM_RESTORE) {
        smSetCramMode(SCL_CRM24_1024);
        for(Uint16 i = 0; i < SM_CRAM_COLORS; i++) {
            SM_CRAM32[i] = smCramSave[i];
        }
    }
    smCramPending = SM_CRAM_NONE;
}

//The CRAM switch waits for V-blank in. The register changes are copied then too.
void smAltarGray(Uint8 on) {
    if(on) {
        for(Uint16 i = 0; i < SM_CRAM_COLORS; i++) {
            smCramSave[i] = SM_CRAM32[i];
            smCramGray[i] = smTo15(smCramSave[i]);
            smCramGray[SM_CRAM_COLORS + i] = smTo15(smGrayColor(smCramSave[i]));
        }
        smCramPending = SM_CRAM_GRAY;
        //NBG0 and NBG2 transparent dots drawn, so NBG0's backdrop color also comes from CRAM.
        Scl_s_reg.dispenbl |= 0x0500;
        SCL_SetPriority(SCL_NBG2, 7);
        SCL_SET_CCMD(0);
        SCL_SET_CCRTMD(0);
        SCL_SET_N2SCCM(0);
        SCL_SET_N2CCRT(31);
        SCL_SET_N2CCEN(1);
        //VDP1 keeps its resolution, so NBG0 is doubled across to match it.
        SCL_SetDisplayMode(SCL_NON_INTER, SCL_240LINE, SCL_HIRESO_A);
        SCL_Open(SCL_NBG0);
            SCL_Scale(MTH_FIXED(2), MTH_FIXED(1));
        SCL_Close();
        //A start line that's never reached leaves the window empty.
        SCL_SetWindow(SCL_W0, 0, SCL_NBG2, SCL_NBG2, 0, 0x1FF, 0, 0x1FF);
    } else {
        smCramPending = SM_CRAM_RESTORE;
        Scl_s_reg.dispenbl &= ~0x0500;
        SCL_SetPriority(SCL_NBG2, 5);
        SCL_SET_N2CCEN(0);
        SCL_SetDisplayMode(SCL_NON_INTER, SCL_240LINE, SCL_NORMAL_A);
        SCL_Open(SCL_NBG0);
            SCL_Scale(MTH_FIXED(1), MTH_FIXED(1));
        SCL_Close();
        dlgSetVisible(0);
    }
}

//A map entered mid-effect keeps the altar's VDP2 setup otherwise. The new map's palettes replace CRAM.
void smAltarReset() {
    smCramPending = SM_CRAM_NONE;
    smSetCramMode(SCL_CRM24_1024);
    Scl_s_reg.dispenbl &= ~0x0500;
    SCL_SET_N2CCEN(0);
    SCL_SetDisplayMode(SCL_NON_INTER, SCL_240LINE, SCL_NORMAL_A);
    SCL_Open(SCL_NBG0);
        SCL_Scale(MTH_FIXED(1), MTH_FIXED(1));
    SCL_Close();
}

//NES DoAltarScanline. Counting from the NES delay loop's start, NES line n (delay + beam line) turns
//monochrome on 342n - 5174 dots after the start of the first visible line, and off 120 dots later.
//That's x n - 60 to n + 59 on NES line n - 15, so the beam slants 1 dot right per line. In screen
//coordinates (NES x + 40, NES line - 1) it's x n - 20 to n + 99 on line n - 16, carried on past
//the left edge of the NES screen. Window X is in hi-res dots.
void smAltarDraw(Uint8 delay, Uint8 lines) {
    Uint16 count = 0;
    Sint16 top = -1;

    for(Uint8 k = 0; k < lines; k++) {
        Sint16 n = delay + k;
        Sint16 x = n - 20;

        if(n < 16) {
            continue;
        }
        if(top < 0) {
            top = n - 16;
        }
        smAltarWin[count].start = (x < 0 ? 0 : x) * 2;
        smAltarWin[count].end = (n + 100) * 2 - 1;
        count++;
    }
    if(!count) {
        SCL_SetWindow(SCL_W0, 0, SCL_NBG2, SCL_NBG2, 0, 0x1FF, 0, 0x1FF);
        return;
    }
    smAltarLwt ^= 1;
    SCL_SetLineWindow(SCL_W0, 0, SCL_NBG2, SCL_NBG2, SM_ALTAR_LWT + smAltarLwt * SM_ALTAR_LWT_SIZE,
                      top, count, smAltarWin);
}
