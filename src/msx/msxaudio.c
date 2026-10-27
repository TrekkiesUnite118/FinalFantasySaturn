#include "types.h"

#include "cd.h"
#include "cronsf.h"
#include "msxchip.h"
#include "msxdrv.h"
#include "msxaudio.h"

#define MA_FILE "MSXSND.BIN"
#define MA_RAM_SIZE (0x80000)
#define MA_RAM8(addr) (*(volatile Uint8 *)(0x25A00000 + (addr)))
#define MA_RAM16(addr) (*(volatile Uint16 *)(0x25A00000 + (addr)))
//SCSP MEM4MB set, master volume 15.
#define MA_SCSP_MIXER (*(volatile Uint16 *)0x25B00400)
#define MA_MIXER_ON (0x020F)

//Song bank (tools/msxconv SoundImage): "MSXS", version, count, then (address, size) per entry.
#define MA_BANK (0x3C000)
#define MA_BANK_VERSION (1)
#define MA_ENTRY_ORANYO (0)
#define MA_ENTRY_SFX (1)
#define MA_ENTRY_MAP_SONGS (2)
#define MA_ENTRY_SONGS (3)
#define MA_ENTRY_COUNT (MA_ENTRY_SONGS + MSX_SONG_COUNT)
#define MA_MAP_COUNT (61)

//MSX RAM the driver data had: ORANYO.BIN at 0xCE00, FF.BGE at 0xD900, songs at 0xD000.
#define MA_ORANYO_MAX (0x200)
#define MA_SFX_MAX (0x700)
#define MA_SONG_MAX (0x900)

static Uint8 maOranyo[MA_ORANYO_MAX];
static Uint8 maSfx[MA_SFX_MAX];
static Uint8 maSong[MA_SONG_MAX];
static Uint8 maMapSongs[MA_MAP_COUNT];
static Uint8 maReady;
static void (*maFrameHook)(void);

static Uint32 maRam32(Uint32 addr) {
    return ((Uint32)MA_RAM16(addr) << 16) | MA_RAM16(addr + 2);
}

static Uint32 maEntryAddr(Uint8 id) {
    return maRam32(MA_BANK + 8 + id * 8);
}

static Uint32 maEntrySize(Uint8 id) {
    return maRam32(MA_BANK + 12 + id * 8);
}

static void maCopy(Uint8 *dst, Uint32 addr, Uint32 size) {
    for(Uint32 i = 0; i < size; i++) {
        dst[i] = MA_RAM8(addr + i);
    }
}

//Checks the bank in sound RAM against the file size and the buffers.
static Uint8 maBankOk(Sint32 fileSize) {
    static const Uint32 maxSize[MA_ENTRY_SONGS] = {MA_ORANYO_MAX, MA_SFX_MAX, MA_MAP_COUNT};

    if(fileSize < MA_BANK + 8 || MA_RAM8(MA_BANK) != 'M' || MA_RAM8(MA_BANK + 1) != 'S' || MA_RAM8(MA_BANK + 2) != 'X'
       || MA_RAM8(MA_BANK + 3) != 'S' || MA_RAM16(MA_BANK + 4) != MA_BANK_VERSION || MA_RAM16(MA_BANK + 6) != MA_ENTRY_COUNT
       || fileSize < MA_BANK + 8 + MA_ENTRY_COUNT * 8) {
        return FALSE;
    }
    for(Uint8 i = 0; i < MA_ENTRY_COUNT; i++) {
        Uint32 addr = maEntryAddr(i), size = maEntrySize(i);
        Uint32 max = i < MA_ENTRY_SONGS ? maxSize[i] : MA_SONG_MAX;
        if(addr < MA_BANK || addr > (Uint32)fileSize || size > (Uint32)fileSize - addr || size > max) {
            return FALSE;
        }
    }
    if(maEntrySize(MA_ENTRY_MAP_SONGS) != MA_MAP_COUNT) {
        return FALSE;
    }
    return TRUE;
}

Uint8 msxaudio_probe() {
    Sint32 size = cd_size(MA_FILE);
    return size > 0 && size <= MA_RAM_SIZE;
}

Uint8 msxaudio_start(void (*frame)(void)) {
    Sint32 size;

    maReady = FALSE;
    //Stops the 68K, keys off and mutes every slot, clears the DSP and sound RAM.
    scsp_init();
    size = cd_load_max(MA_FILE, (void *)0x25A00000, MA_RAM_SIZE);
    if(!maBankOk(size)) {
        return FALSE;
    }
    maCopy(maOranyo, maEntryAddr(MA_ENTRY_ORANYO), maEntrySize(MA_ENTRY_ORANYO));
    maCopy(maSfx, maEntryAddr(MA_ENTRY_SFX), maEntrySize(MA_ENTRY_SFX));
    maCopy(maMapSongs, maEntryAddr(MA_ENTRY_MAP_SONGS), MA_MAP_COUNT);

    msxChipInit();
    MA_SCSP_MIXER = MA_MIXER_ON;
    smpc_sound_on();
    msxDrvInit(maOranyo, maSfx);
    msxDrvStopBgm();
    maFrameHook = frame;
    maReady = TRUE;
    return TRUE;
}

void msxaudio_stop() {
    maReady = FALSE;
}

void msxaudio_frame() {
    if(!maReady) {
        return;
    }
    maFrameHook();
    msxDrvTick();
    msxChipFrame();
}

const Uint8 *msxaudio_song(Uint8 song) {
    if(song < MSX_SONG_FIRST || song >= MSX_SONG_FIRST + MSX_SONG_COUNT) {
        return NULL;
    }
    Uint8 id = MA_ENTRY_SONGS + song - MSX_SONG_FIRST;
    maCopy(maSong, maEntryAddr(id), maEntrySize(id));
    return maSong;
}

Uint8 msxaudio_map_song(Uint8 map) {
    return maMapSongs[map < MA_MAP_COUNT ? map : 0];
}
