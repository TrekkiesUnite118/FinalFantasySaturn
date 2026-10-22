#include "types.h"

#include "cd.h"
#include "apu.h"
#include "slave.h"
#include "nesaudio.h"

#define NA_TABLE_FILE "SCSPAPU.BIN"
#define NA_TABLE_SIZE (0x80000)
#define NA_SOUND_RAM (0x25A00000)

//Samples written ahead of the DSP at the end of a frame (~29 ms).
#define NA_LEAD (1280)
//NES cycles between register writes within a frame.
#define NA_WRITE_CYCLES (4)
//apu_volume 1.0 in 16.16.
#define NA_VOLUME (65536)

//VDP2 TVSTAT, with VBLANK set during V-blank.
#define NA_TVSTAT (*(volatile Uint16 *)0x25F80004)
#define NA_TVSTAT_VBLANK (0x0008)
//Cache-through alias, since the CPUs' caches aren't kept in step.
#define NA_UNCACHED(v) (*(volatile Uint8 *)((Uint32)&(v) | 0x20000000))

static Uint8 naReady;
static Uint32 naTimestamp;
static Uint8 naSlaveRun;
static volatile Uint8 naOnSlave;
static void (*naFrameHook)(void);

void nesaudio_init(void *buffer, void (*init)(void), void (*frame)(void)) {
    Uint16 *src = (Uint16 *)buffer;
    volatile Uint16 *dst = (volatile Uint16 *)NA_SOUND_RAM;

    //apu_preinit clears sound RAM and moves the 68K into EFREG before the tables go in.
    apu_preinit();
    cd_load(NA_TABLE_FILE, buffer, NA_TABLE_SIZE);
    for(Uint32 i = 0; i < NA_TABLE_SIZE / 2; i++) {
        dst[i] = src[i];
    }

    apu_init(FALSE, NA_VOLUME);
    apu_power();
    apu_start_sync();
    naTimestamp = 0;
    init();
    naFrameHook = frame;
    naReady = TRUE;

    //NES reset: 5-step frame counter with IRQs off, all channels enabled.
    nesaudio_write(0x4017, 0xC0);
    nesaudio_write(0x4015, 0x0F);
}

void nesaudio_write(Uint16 addr, Uint8 value) {
    if(!naReady) {
        return;
    }
    apu_write(naTimestamp, addr, value);
    naTimestamp += NA_WRITE_CYCLES;
}

//Ends the previous APU frame at the length the SCSP actually played, then runs this frame's writes.
void nesaudio_frame() {
    Uint32 end;

    if(!naReady) {
        return;
    }
    end = apu_frame_timestamp(NA_LEAD);
    if(end < naTimestamp) {
        end = naTimestamp;
    }
    apu_frame(end);
    naTimestamp = 0;

    naFrameHook();
}

void nesaudio_vblank_in() {
    if(!naOnSlave) {
        nesaudio_frame();
    }
}

//Slave job: one audio frame per V-blank until nesaudio_slave_end.
static void naSlaveLoop(void *parm) {
    Uint8 inVblank = TRUE;

    while(NA_UNCACHED(naSlaveRun)) {
        Uint8 v = (NA_TVSTAT & NA_TVSTAT_VBLANK) != 0;

        if(v && !inVblank) {
            nesaudio_frame();
        }
        inVblank = v;
        //Keeps the polling off the bus most of the time.
        for(Uint16 i = 0; i < 256; i++) {
            __asm__ volatile("nop");
        }
    }
}

//The backup library holds off the master's interrupts while it waits on a drive, so the slave runs
//the audio frames meanwhile. The master mustn't touch music state until nesaudio_slave_end.
void nesaudio_slave_begin() {
    if(!naReady) {
        return;
    }
    NA_UNCACHED(naSlaveRun) = TRUE;
    naOnSlave = TRUE;
    slaveRun(naSlaveLoop, 0);
}

//Waits for the slave's last frame, then purges the master's cache of what the slave wrote.
void nesaudio_slave_end() {
    if(!naOnSlave) {
        return;
    }
    NA_UNCACHED(naSlaveRun) = FALSE;
    slaveWait();
    naOnSlave = FALSE;
}
