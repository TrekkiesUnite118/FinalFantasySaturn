#include    <machine.h>
#include    <sega_mth.h>
#define     _SPR2_
#include    <sega_spr.h>
#include    <sega_scl.h>
#include    <sega_int.h>
#include    <sega_per.h>
#include    <sega_sys.h>
#include    "sys.h"

static void (*vblIn)(void);
static void (*vblOutFirst)(void);
static void (*vblOut)(void);

static Uint32 rtcWorkArea[7];

#define TVMD_DISP (0x8000)

//V-blanks left until sysShowDisplay turns the display back on, 0 when not waiting.
//Counted in V-blank: after a long frame, the next frameEnd calls don't wait.
static volatile Uint8 showDisplayFrames = 0;

void sysDisplayOff(void) {
    *(volatile Uint16 *)0x25F80000 = 0x0000;
}

//SCL rewrites TVMD from Scl_s_reg, so DISP is changed there too.
static void sysSetDisp(Uint8 on) {
    if(on) {
        Scl_s_reg.tvmode |= TVMD_DISP;
    } else {
        Scl_s_reg.tvmode &= ~TVMD_DISP;
    }
    *(volatile Uint16 *)0x25F80000 = Scl_s_reg.tvmode;
    if(SclProcess == 0) {
        SclProcess = 1;
    }
}

void sysBlankDisplay(void) {
    showDisplayFrames = 0;
    sysSetDisp(FALSE);
}

void sysShowDisplay(Uint8 frames) {
    showDisplayFrames = frames;
}

static void sysVblankIn(void) {
    vblIn();
    if(showDisplayFrames && --showDisplayFrames == 0) {
        sysSetDisp(TRUE);
    }
    SCL_VblankStart();
}

static void sysVblankOut(void) {
    vblOutFirst();
    SCL_VblankEnd();
    vblOut();
}

void sysSetVblank(void (*in)(void), void (*outFirst)(void), void (*out)(void)) {
    vblIn = in;
    vblOutFirst = outFirst;
    vblOut = out;
    INT_ChgMsk(INT_MSK_NULL,INT_MSK_VBLK_IN | INT_MSK_VBLK_OUT);
    INT_SetScuFunc(INT_SCU_VBLK_IN,sysVblankIn);
    INT_SetScuFunc(INT_SCU_VBLK_OUT,sysVblankOut);
    INT_ChgMsk(INT_MSK_VBLK_IN | INT_MSK_VBLK_OUT,INT_MSK_NULL);
}

void frameBegin(void) {
    static XyInt sysClip = {319, 239};
    SPR_2OpenCommand(SPR_2DRAW_PRTY_ON);
    //Nothing else sets the VDP1 system clip; the BIOS leaves it at 224 lines.
    SPR_2SysClip(0, &sysClip);
}

void frameEnd(void) {
    SPR_2CloseCommand();
    SCL_DisplayFrame();
}

void rtcInit(void) {
    PER_LInit(PER_KD_PERTIM,2,PER_SIZE_DGT,(Uint8 *)rtcWorkArea,0);
}

const Uint8 *rtcTime(void) {
    return PER_GET_TIM();
}

void sysReset(void) {
    SYS_Exit(0);
}
