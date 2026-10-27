#include "types.h"
#include "video.h"
#include "player.h"
#include "wipe.h"
#include "music.h"

//NES ScreenWipe line counts start 11 lines before the picture, and the map is 1 line higher on the Saturn.
#define WIPE_LINE_OFFSET (11 + 1)

static Uint8 wipeOpen;
static Uint8 wipeTop; //NES tmp+4.
static Uint8 wipeLines; //NES tmp+5.
static Uint32 wipeLayers;
static void (*wipeDone)(void);

//The layers only show inside the window, like the NES turning BG rendering on for those lines.
static void wipeSetWindow() {
    Sint16 top = wipeTop - WIPE_LINE_OFFSET;
    Sint16 bottom = top + wipeLines - 1;

    if(top < 0) {
        top = 0;
    }
    vidSetWindow(VID_W1, 0, wipeLayers, wipeLayers, 0, top, 319, bottom);
}

//NES StartScreenWipe: square 1 alone plays the wipe sound, sweeping down.
static void wipeSoundStart() {
    musicApuWrite(0x4015, 0x01);
    musicApuWrite(0x4000, 0x38);
    musicApuWrite(0x4001, 0x8A);
}

//NES ScreenWipeFrame: every 4th frame the period is set to ~lines * 8.
static void wipeSoundFrame() {
    Uint16 period;

    if(wipeLines & 0x0C) {
        return;
    }
    period = (Uint8)~wipeLines << 3;
    musicApuWrite(0x4002, period & 0xFF);
    musicApuWrite(0x4003, period >> 8);
}

//NES ScreenWipe_Finalize, which runs into StartScreenWipe again.
static void wipeSoundEnd() {
    musicApuWrite(0x4002, 0);
    musicApuWrite(0x4003, 0);
    wipeSoundStart();
}

//NES ScreenWipe_Open and ScreenWipe_Close. Sprites aren't drawn in the WIPE state, like the NES clearing OAM.
void wipeStart(Uint8 open, Uint32 layers, void (*done)(void)) {
    wipeOpen = open;
    wipeTop = open ? 122 + 11 : 10 + 11;
    wipeLines = open ? 1 : 220 + 1;
    wipeLayers = layers;
    wipeDone = done;
    wipeSetWindow();
    wipeSoundStart();
    wipeSoundFrame();
    //The MSX game stops its music for both wipes. The next map's song waits for the wipe to end.
    musicMsxStopSong();
    musicMsxSfx(open ? MSX_SFX_WIPE_OPEN : MSX_SFX_WIPE_CLOSE);
    state = WIPE;
}

//NES ScreenWipeFrame loop, one frame per call.
void doWipe() {
    if(wipeOpen) {
        wipeTop -= 2;
        wipeLines += 4;
        if(wipeLines >= 224) {
            vidSetWindow(VID_W1, 0, 0, 0, 0, 0, 0, 0);
            wipeSoundEnd();
            wipeDone();
            return;
        }
    } else {
        wipeTop += 2;
        wipeLines -= 4;
        if(wipeLines == 1) {
            //A start line that's never reached leaves the window empty, hiding the layers.
            vidSetWindow(VID_W1, 0, wipeLayers, wipeLayers, 0, 0x1FF, 0, 0x1FF);
            wipeSoundEnd();
            wipeDone();
            return;
        }
    }
    wipeSetWindow();
    wipeSoundFrame();
}
