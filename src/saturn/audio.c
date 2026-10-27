#include "types.h"

#include "slave.h"
#include "nesaudio.h"
#include "msxaudio.h"
#include "audio.h"

//VDP2 TVSTAT, with VBLANK set during V-blank.
#define AU_TVSTAT (*(volatile Uint16 *)0x25F80004)
#define AU_TVSTAT_VBLANK (0x0008)
//Cache-through alias, since the CPUs' caches aren't kept in step.
#define AU_UNCACHED(v) (*(volatile Uint8 *)((Uint32)&(v) | 0x20000000))

static volatile Uint8 auMode = AUDIO_OFF;
static Uint8 auMsxAvailable;
static void (*auFrameHook)(void);
static Uint8 auSlaveRun;
static volatile Uint8 auOnSlave;

static void auFrame() {
    switch(auMode) {
        case AUDIO_NES:
            nesaudio_frame();
            break;
        case AUDIO_MSX:
            msxaudio_frame();
            break;
    }
}

void audio_init(void (*init)(void), void (*frame)(void)) {
    auFrameHook = frame;
    auMsxAvailable = msxaudio_probe();
    nesaudio_init(init, frame);
    auMode = AUDIO_NES;
}

Uint8 audio_msx_available() {
    return auMsxAvailable;
}

Uint8 audio_mode() {
    return auMode;
}

Uint8 audio_set_mode(Uint8 mode) {
    if(mode == auMode) {
        return auMode;
    }
    //The V-blank runs no more frames from here, so the drivers can be stopped and reset.
    auMode = AUDIO_OFF;
    nesaudio_stop();
    msxaudio_stop();
    if(mode == AUDIO_MSX && auMsxAvailable && msxaudio_start(auFrameHook)) {
        auMode = AUDIO_MSX;
    } else {
        nesaudio_start();
        auMode = AUDIO_NES;
    }
    return auMode;
}

void audio_vblank_in() {
    if(!auOnSlave) {
        auFrame();
    }
}

//Slave job: one audio frame per V-blank until audio_slave_end.
static void auSlaveLoop(void *parm) {
    Uint8 inVblank = TRUE;

    while(AU_UNCACHED(auSlaveRun)) {
        Uint8 v = (AU_TVSTAT & AU_TVSTAT_VBLANK) != 0;

        if(v && !inVblank) {
            auFrame();
        }
        inVblank = v;
        //Keeps the polling off the bus most of the time.
        for(Uint16 i = 0; i < 256; i++) {
            __asm__ volatile("nop");
        }
    }
}

//The backup library holds off the master's interrupts while it waits on a drive, so the slave runs
//the audio frames meanwhile. The master mustn't touch music state until audio_slave_end.
void audio_slave_begin() {
    if(auMode == AUDIO_OFF) {
        return;
    }
    AU_UNCACHED(auSlaveRun) = TRUE;
    auOnSlave = TRUE;
    slaveRun(auSlaveLoop, 0);
}

//Waits for the slave's last frame, then purges the master's cache of what the slave wrote.
void audio_slave_end() {
    if(!auOnSlave) {
        return;
    }
    AU_UNCACHED(auSlaveRun) = FALSE;
    slaveWait();
    auOnSlave = FALSE;
}
