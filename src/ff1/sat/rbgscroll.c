#include <string.h>
#include <sega_def.h>
#include <sega_mth.h>
#include <sega_scl.h>
#include <sega_dma.h>
#include "battle.h"

#include "nesutils.h"

#include "cd.h"
#include "memmap.h"
#include "overworld.h"
#include "player.h"
#include "print.h"
#include "rbgscroll.h"
#include "sprite.h"

#define    MAP_OFFSET    (0x0800 *4)
#define    MAP_NUM        16

Fixed32 scrolls_x[]  = {0, 0, 0, 0};
Fixed32 scrolls_y[]  = {0, 0, 0, 0};

int x_cords[] = {0, 32, 0, 32, 64, 96, 64, 96, 128, 160, 128, 160, 192, 224, 192, 224, 0, 32, 0, 32, 64, 96, 64, 96, 128, 160, 128, 160, 192, 224, 192, 224, 0, 32, 0, 32, 64, 96, 64, 96, 128, 160, 128, 160, 192, 224, 192, 224, 0, 32, 0, 32, 64, 96, 64, 96, 128, 160, 128, 160, 192, 224, 192, 224};
int y_cords[] = {0, 0, 32, 32, 0, 0, 32, 32, 0, 0, 32, 32, 0, 0, 32, 32, 64, 64, 96, 96, 64, 64, 96, 96, 64, 64, 96, 96, 64, 64, 96, 96, 128, 128, 160, 160, 128, 128, 160, 160, 128, 128, 160, 160, 128, 128, 160, 160, 192, 192, 224, 224, 192, 192, 224, 224, 192, 192, 224, 224, 192, 192, 224, 224};


SclConfig Rbg0Scfg;

//Sky shown above the tilted map's horizon: the top three rows of the ocean battle backdrop over its solid sky
//tile, on NBG3 in VRAM B0 in front of RBG0. Haze rows under the horizon blend over the map with a ratio set per
//line by SCU DMA.
#define SKY_CHR (BATTLE_TILE_OFFSET + 3 * 0x800)
#define SKY_NES_PAL (BATTLE_BACKDROP_PALETTE_OFFSET + 3 * 4)
#define SKY_TILES (13)
#define SKY_CHAR_BLANK (SKY_TILES)
#define SKY_CHAR_HAZE (SKY_TILES + 1)
#define SKY_CHAR_ADDR (SCL_VDP2_VRAM_B0 + 0x1000)
#define SKY_CHAR_NUM (0x80) //SKY_CHAR_ADDR in chars, above the supplementary character bits.
#define SKY_PNC (0x0008) //Supplementary character bits selecting VRAM B0.
#define SKY_MAP_ADDR (SCL_VDP2_VRAM_B0 + 0x2000)
#define SKY_PAL (15) //A spare palette in RBG0's colour RAM.
#define SKY_HAZE_COLOR (4) //Its colour RAM MSB picks it for colour calculation.
#define SKY_HAZE_NES (0x3C)
#define SKY_BACK_NES (0x22) //The back screen behind the haze above the horizon, the backdrop's sky.
//Map rows, 8 lines each, with the horizon at the top of row 0: haze, blank over the map, solid sky, clouds, then
//haze again down to the horizon.
#define SKY_HAZE_ROWS (4)
#define SKY_SOLID_ROW (48)
#define SKY_CLOUD_ROW (59)
#define SKY_HAZE_UP_ROW (62)
#define SKY_LINES (512)
//Colour calculation ratios, 0 being nearly all haze and 31 none. The haze is thickest on the horizon line, held
//a few lines below it, and thins out above and below.
#define HAZE_RATIO_HORIZON (0)
#define HAZE_HOLD_LINES (4)
#define HAZE_LINES (SKY_HAZE_ROWS * 8)
#define HAZE_UP_LINES ((64 - SKY_HAZE_UP_ROW) * 8)
//H-blanks from V-blank out to the next, with spare.
#define HAZE_TABLE_LINES (272)
//SCU DMA level 1, as SBL's DMA_ScuMemCopy uses level 0.
#define HAZE_DMA_R ((volatile Uint32 *)0x25FE0020)
#define HAZE_DMA_EN ((volatile Uint32 *)0x25FE0030)
#define HAZE_DMA_ENABLE (1 << 8)
//VDP2 CCRNA/CCRNB, the NBG0-NBG3 colour calculation ratios.
#define HAZE_CCR_ADDR (0x25F80108)
static SclConfig skyScfg;
static Fixed32 skyScrollY = 0;
static Uint8 skyShown = FALSE;
//Per H-blank CCRNA and CCRNB values. Entry n is written in line n's H-blank, so it applies from line n + 1.
static Uint16 hazeTable[HAZE_TABLE_LINES][2];
static void sky_init();
static void sky_show(Uint8 show);
static void sky_place();
//SBL's, not in its header.
extern void SCL_SetColRamOffset(Uint32 Object, Uint32 Offset, Uint8 transparent);

//NBG3's character and pattern name reads in B0. RBG0's banks are assigned in the RAM control.
static Uint16 owCycleTb[] = {
    0xeeee, 0xeeee,
    0xeeee, 0xeeee,
    0x7733, 0xeeee,
    0xeeee, 0xeeee
};

//NES color indices of the current map's palettes, kept so emphasis can be applied to them.
static Uint8 *bgNesPalette;
static Uint8 bgTotalColors;


void scroll_init(void) {

    Uint8 *VramWorkP;
    Uint16 BackCol;
    SclVramConfig vram_cfg;

    SCL_SetDisplayMode(SCL_NON_INTER,SCL_240LINE,SCL_NORMAL_A);

    SCL_SetColRamMode(SCL_CRM24_1024);

    Uint32 numColors = 4 * 16;

    //Colour 0 drawn, so the sky behind only shows past the horizon.
    SCL_AllocColRam(SCL_RBG0, numColors, ON);
    Uint32 palette[8][16] = {{0}};
    Uint8 palette_iter = 0;
    Uint8 nes_val = 0;
    bgNesPalette = (Uint8 *)OVERWORLD_PALETTE;
    bgTotalColors = OVERWORLD_COLORS;
    for(int i = 0; i < bgTotalColors; i += 4) {

        nes_val = bgNesPalette[i];
        palette[palette_iter][0] = nes_pal[nes_val];
        nes_val = bgNesPalette[i + 1];
        palette[palette_iter][1] = nes_pal[nes_val];
        nes_val = bgNesPalette[i + 2];
        palette[palette_iter][2] = nes_pal[nes_val];
        nes_val = bgNesPalette[i + 3];
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

    convertNesTilesToSatTiles((Uint8 *)OVERWORLD_CHR, (Uint8 *)LWRAM_BUFFER, OVERWORLD_CHR_SIZE);
    buildMapTiles((Uint8 *)LWRAM_BUFFER, VramWorkP, (Uint8 *)OVERWORLD_TSA);

    int x = 0;
    int y = 0;
    update_plane(32, 32);
    for(int planeNum = 0; planeNum < 16; planeNum++){
        int offset = 0;
        for(int i = 0; i < 4; i++) {
            update_plane(y_cords[y], x_cords[x]);
            memcpy((void *)(Rbg0Scfg.plate_addr[planeNum] + offset), mapPlane, 0x800);
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
    sky_init();
}

static void sky_init() {
    Uint8 *src = (Uint8 *)SKY_CHR;
    Uint32 *chars = (Uint32 *)SKY_CHAR_ADDR;
    Uint16 *map = (Uint16 *)SKY_MAP_ADDR;
    Uint8 *nesPal = (Uint8 *)SKY_NES_PAL;
    //NES DrawBattleBackdropRow's column order, repeating every 8 columns.
    static const Uint8 order[8] = {1, 2, 3, 4, 3, 4, 1, 2};
    Uint32 palette[16] = {0};

    //NES 2bpp tiles to 4bpp.
    for(Uint8 t = 0; t < SKY_TILES; t++) {
        for(Uint8 y = 0; y < 8; y++) {
            Uint8 lo = src[t * 16 + y];
            Uint8 hi = src[t * 16 + 8 + y];
            Uint32 row = 0;

            for(Uint8 x = 0; x < 8; x++) {
                row = (row << 4) | ((lo >> (7 - x)) & 1) | (((hi >> (7 - x)) & 1) << 1);
            }
            chars[t * 8 + y] = row;
        }
    }
    for(Uint8 y = 0; y < 8; y++) {
        chars[SKY_CHAR_BLANK * 8 + y] = 0;
        chars[SKY_CHAR_HAZE * 8 + y] = SKY_HAZE_COLOR * 0x11111111;
    }
    for(Uint8 y = 0; y < 64; y++) {
        for(Uint8 x = 0; x < 64; x++) {
            Uint16 tile = SKY_CHAR_BLANK;

            if(y < SKY_HAZE_ROWS || y >= SKY_HAZE_UP_ROW) {
                tile = SKY_CHAR_HAZE;
            } else if(y >= SKY_CLOUD_ROW) {
                tile = order[x & 7] + (y - SKY_CLOUD_ROW) * 4;
            } else if(y >= SKY_SOLID_ROW) {
                tile = 1;
            }
            map[y * 64 + x] = (SKY_PAL << 12) | (SKY_CHAR_NUM + tile);
        }
    }
    for(Uint8 i = 0; i < 4; i++) {
        palette[i] = nes_pal[nesPal[i]];
    }
    palette[SKY_HAZE_COLOR] = nes_pal[SKY_HAZE_NES] | 0x80000000;
    SCL_SetColRam(SCL_RBG0, SKY_PAL * 16, 16, palette);

    SCL_InitConfigTb(&skyScfg);
        skyScfg.dispenbl      = OFF;
        skyScfg.charsize      = SCL_CHAR_SIZE_1X1;
        skyScfg.pnamesize     = SCL_PN1WORD;
        skyScfg.flip          = SCL_PN_12BIT;
        skyScfg.platesize     = SCL_PL_SIZE_1X1;
        skyScfg.coltype       = SCL_COL_TYPE_16;
        skyScfg.datatype      = SCL_CELL;
        skyScfg.mapover       = SCL_OVER_0;
        skyScfg.patnamecontrl = SKY_PNC;
        for(Uint8 i = 0; i < 4; i++)
        skyScfg.plate_addr[i] = SKY_MAP_ADDR;
    SCL_SetCycleTable(owCycleTb);
    SCL_SetColRamOffset(SCL_NBG3, SCL_GetColRamOffset(SCL_RBG0), OFF);
    SCL_SetPriority(SCL_NBG3, 5);
    skyShown = TRUE;
    sky_show(FALSE);
}

//Colour calculation on NBG3's dots whose colour RAM MSB is set, by the ratio of NBG3 as the top image.
static void sky_show(Uint8 show) {
    DmaScuPrm prm;

    if(show == skyShown) {
        return;
    }
    skyShown = show;
    skyScfg.dispenbl = show ? ON : OFF;
    SCL_SetConfig(SCL_NBG3, &skyScfg);
    //Sets the back screen, written in V-blank in.
    setBattleBackColor(show ? SKY_BACK_NES : 0x0F);
    *HAZE_DMA_EN = 0;
    SCL_SET_N3CCEN(show);
    if(!show) {
        return;
    }
    SCL_SET_N3SCCM(3);
    SCL_SET_CCMD(0);
    SCL_SET_CCRTMD(0);
    //Each H-blank writes the next entry; the read address carries on until restarted.
    prm.dxr = (Uint32)hazeTable;
    prm.dxw = HAZE_CCR_ADDR;
    prm.dxc = sizeof(hazeTable[0]);
    prm.dxad_r = DMA_SCU_R4;
    prm.dxad_w = DMA_SCU_W2;
    prm.dxmod = DMA_SCU_DIR;
    prm.dxrup = DMA_SCU_REN;
    prm.dxwup = DMA_SCU_KEEP;
    prm.dxft = DMA_SCU_F_HBLK_IN;
    prm.msk = DMA_SCU_M_DXR | DMA_SCU_M_DXW;
    DMA_ScuSetPrm(&prm, DMA_SCU_CH1);
    *HAZE_DMA_EN = HAZE_DMA_ENABLE;
}

//Restarts the haze ratios from the top of the table. Called at V-blank out.
void scroll_vblank_out() {
    if(skyShown) {
        *HAZE_DMA_EN = 0;
        *HAZE_DMA_R = (Uint32)hazeTable;
        *HAZE_DMA_EN = HAZE_DMA_ENABLE;
    }
}

void scroll_move(int num, Fixed32 x, Fixed32 y) {
    scrolls_x[num] += x;
    scrolls_y[num] += y;

    switch (num) {
        case SCROLL_BACKGROUND1:
            //The sky scrolls slower across than the map does at the horizon, and sits on the horizon.
            SCL_Open(SCL_NBG3);
            SCL_MoveTo(scrolls_x[num] >> 4, skyScrollY, 0);
            SCL_Close();
            //Opened last since the rotation calls act on the open layer.
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

extern SclRotreg *SclRotregBuff;
extern Uint16 SclK_TableBuff[2][820];

#define ROT_LINES (240)
//Per screen line of RBG0: map X at the left edge and map Y, relative to the scroll position, and the map pixels per
//screen pixel. A step of 0 means the line shows no map.
static Fixed32 rotRowX[ROT_LINES];
static Fixed32 rotRowY[ROT_LINES];
static Fixed32 rotRowStep[ROT_LINES];
static Sint16 rotFirstRow;

//VDP2 rotation per line from the SBL parameter table and coefficient table. The matrix's D and the H delta must
//leave Y constant across the line, which holds for X axis rotation.
static void scroll_build_projection() {
    SclRotreg *r = &SclRotregBuff[0];
    Fixed32 px = MTH_IntToFixed(r->viewp.x);
    Fixed32 py = MTH_IntToFixed(r->viewp.y);
    Fixed32 pz = MTH_IntToFixed(r->viewp.z);
    Fixed32 cx = MTH_IntToFixed(r->rotatecenter.x);
    Fixed32 cy = MTH_IntToFixed(r->rotatecenter.y);
    Fixed32 cz = MTH_IntToFixed(r->rotatecenter.z);
    Fixed32 xp = MTH_Mul(r->matrix_a, px - cx) + MTH_Mul(r->matrix_b, py - cy) + MTH_Mul(r->matrix_c, pz - cz) + cx;
    Fixed32 yp = MTH_Mul(r->matrix_d, px - cx) + MTH_Mul(r->matrix_e, py - cy) + MTH_Mul(r->matrix_f, pz - cz) + cy;
    Fixed32 dx = MTH_Mul(r->matrix_a, r->delta.x) + MTH_Mul(r->matrix_b, r->delta.y);
    Sint32 v;

    rotFirstRow = ROT_LINES;
    for(v = 0; v < ROT_LINES; v++) {
        Fixed32 xst = r->screenst.x + r->screendlt.x * v - px;
        Fixed32 yst = r->screenst.y + r->screendlt.y * v - py;
        Fixed32 zst = r->screenst.z - pz;
        Uint16 entry = SclK_TableBuff[0][MTH_FixedToInt(r->k_tab + r->k_delta.x * v)];
        //1-word coefficients: bit 15 = transparent, 10 fraction bits.
        Fixed32 k = (entry & 0x8000) ? 0 : (Fixed32)(entry & 0x7FFF) << 6;

        rotRowStep[v] = MTH_Mul(k, dx);
        if(rotRowStep[v] <= 0) {
            rotRowStep[v] = 0;
            continue;
        }
        if(rotFirstRow == ROT_LINES) {
            rotFirstRow = v;
        }
        rotRowX[v] = MTH_Mul(k, MTH_Mul(r->matrix_a, xst) + MTH_Mul(r->matrix_b, yst) + MTH_Mul(r->matrix_c, zst)) + xp;
        rotRowY[v] = MTH_Mul(k, MTH_Mul(r->matrix_d, xst) + MTH_Mul(r->matrix_e, yst) + MTH_Mul(r->matrix_f, zst)) + yp;
    }
}

//rotateAngle is in fixed point degrees.
//Viewpoint distance in front of the screen. Smaller is a closer camera with stronger perspective.
Uint16 scroll_view_z = 168;

void scroll_rotate_x(Fixed32 rotateAngle) {
    SclRotregBuff[0].viewp.z = scroll_view_z;
    SCL_Open(SCL_RBG0);
    SCL_RotateTo(rotateAngle, FIXED(0), FIXED(0), SCL_X_AXIS);
    SCL_Close();
    scroll_build_projection();
    sky_place();
}

//Puts the haze and cloud rows on the line where the map starts, which is above the screen at small tilts.
static void sky_place() {
    SclRotreg *r = &SclRotregBuff[0];
    Sint32 horizon;

    if(r->matrix_f <= MTH_FIXED(0.01)) {
        sky_show(FALSE);
        return;
    }
    if(rotFirstRow > 0) {
        horizon = rotFirstRow;
    } else {
        //SBL hides lines scaled past 4, from viewp.y - 0.75 * Pz * E / F.
        Fixed32 pz = MTH_IntToFixed(r->viewp.z - r->rotatecenter.z) / 4 * 3;

        horizon = r->viewp.y - MTH_FixedToInt(MTH_Div(MTH_Mul(pz, r->matrix_e), r->matrix_f));
    }
    //Past this the solid sky rows would wrap onto the bottom of the screen.
    if(horizon <= ROT_LINES - SKY_SOLID_ROW * 8) {
        sky_show(FALSE);
        return;
    }
    skyScrollY = MTH_IntToFixed((SKY_LINES - horizon) & (SKY_LINES - 1));
    for(Sint32 i = 0; i < HAZE_TABLE_LINES; i++) {
        Sint32 d = i + 1 - horizon;
        Uint16 ratio = 31;

        if(d >= -HAZE_UP_LINES && d < 0) {
            ratio = HAZE_RATIO_HORIZON + (31 - HAZE_RATIO_HORIZON) * -d / HAZE_UP_LINES;
        } else if(d >= 0 && d < HAZE_HOLD_LINES) {
            ratio = HAZE_RATIO_HORIZON;
        } else if(d >= HAZE_HOLD_LINES && d < HAZE_LINES) {
            ratio = HAZE_RATIO_HORIZON + (31 - HAZE_RATIO_HORIZON) * (d - HAZE_HOLD_LINES) / (HAZE_LINES - HAZE_HOLD_LINES);
        }
        hazeTable[i][0] = 0;
        hazeTable[i][1] = ratio << 8;
    }
    sky_show(TRUE);
}

//Where the RBG0 map point (x, y), relative to the scroll position, shows on screen. scale is screen pixels per map
//pixel. Points past the bottom line are extrapolated from the last two lines. Returns FALSE past the horizon.
Uint8 scroll_project(Fixed32 x, Fixed32 y, Fixed32 *sx, Fixed32 *sy, Fixed32 *scale) {
    Sint32 lo = rotFirstRow;
    Sint32 hi = ROT_LINES - 1;
    Fixed32 frac, step, x0;

    if(lo >= hi || y < rotRowY[lo]) {
        return FALSE;
    }
    //The lines' map Y rises down the screen; finds the last line at or above y.
    while(hi - lo > 1) {
        Sint32 mid = (lo + hi) >> 1;

        if(rotRowY[mid] <= y) {
            lo = mid;
        } else {
            hi = mid;
        }
    }
    frac = MTH_Div(y - rotRowY[lo], rotRowY[hi] - rotRowY[lo]);
    step = rotRowStep[lo] + MTH_Mul(rotRowStep[hi] - rotRowStep[lo], frac);
    x0 = rotRowX[lo] + MTH_Mul(rotRowX[hi] - rotRowX[lo], frac);
    if(step <= 0) {
        return FALSE;
    }
    *sx = MTH_Div(x - x0, step);
    *sy = MTH_IntToFixed(lo) + frac;
    *scale = MTH_Div(MTH_FIXED(1), step);
    return TRUE;
}

void scroll_set_emphasis(Uint8 emphasis) {
    Uint32 palette[16] = {0};

    for(Uint8 p = 0; p * 4 < bgTotalColors; p++) {
        for(Uint8 i = 0; i < 4; i++) {
            palette[i] = emphasizeColor(bgNesPalette[(p * 4) + i], emphasis);
        }
        SCL_SetColRam(SCL_RBG0, p * 16, 16, (void *)palette);
    }
}

//NES CyclePalettes. Only the 16 BG colors cycle, like the NES.
static Uint8 palCycColors[16];

void scroll_pal_cycle_draw() {
    Uint32 palette[16] = {0};

    for(Uint8 p = 0; p < 4; p++) {
        for(Uint8 i = 0; i < 4; i++) {
            palette[i] = nes_pal[palCycColors[(p * 4) + i]];
        }
        SCL_SetColRam(SCL_RBG0, p * 16, 16, (void *)palette);
    }
}

//Cycling in starts each non-black color at chroma $C of its brightness.
void scroll_pal_cycle_init(Uint8 reverse) {
    for(Uint8 i = 0; i < 16; i++) {
        palCycColors[i] = bgNesPalette[i];
        if(reverse && palCycColors[i] != 0x0F) {
            palCycColors[i] = (palCycColors[i] & 0x30) | 0x0C;
        }
    }
    if(reverse) {
        scroll_pal_cycle_draw();
    }
}

//NES PalCyc_Step. Returns the number of colors changed, 0 once the cycle is done.
Uint8 scroll_pal_cycle_step(Uint8 reverse) {
    Uint8 changed = 0;

    for(Uint8 i = 0; i < 16; i++) {
        Uint8 color = palCycColors[i];
        if(reverse) {
            if(color != bgNesPalette[i]) {
                palCycColors[i] = color - 1;
                changed++;
            }
        } else if(color != 0x0F) {
            Uint8 chroma = (color & 0x0F) + 1;
            palCycColors[i] = (chroma >= 0x0D) ? 0x0F : ((color & 0x30) | chroma);
            changed++;
        }
    }
    return changed;
}

void scroll_disable() {
    Rbg0Scfg.dispenbl      = OFF;
    SCL_SetConfig(SCL_RBG0, &Rbg0Scfg);
    sky_show(FALSE);
}
