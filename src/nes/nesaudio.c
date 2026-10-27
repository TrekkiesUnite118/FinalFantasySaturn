#include "types.h"

#include "cd.h"
#include "apu.h"
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

static Uint8 naReady;
static Uint32 naTimestamp;
static void (*naFrameHook)(void);

void nesaudio_init(void (*init)(void), void (*frame)(void)) {
    init();
    naFrameHook = frame;
    nesaudio_start();
}

void nesaudio_start() {
    naReady = FALSE;
    //apu_preinit resets the SCSP, clears sound RAM and moves the 68K into EFREG before the tables go in.
    apu_preinit();
    cd_load(NA_TABLE_FILE, (void *)NA_SOUND_RAM, NA_TABLE_SIZE);

    apu_init(FALSE, NA_VOLUME);
    apu_power();
    apu_start_sync();
    naTimestamp = 0;
    naReady = TRUE;

    //NES reset: 5-step frame counter with IRQs off, all channels enabled.
    nesaudio_write(0x4017, 0xC0);
    nesaudio_write(0x4015, 0x0F);
}

void nesaudio_stop() {
    naReady = FALSE;
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

