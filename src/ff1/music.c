#include "types.h"

#include "cd.h"
#include "player.h"
#include "nesaudio.h"
#include "music.h"

//NES bank $0D ($8000-$BFFF) and fixed bank $0F ($C000-$FFFF) in the loaded ROM.
#define MUSIC_BANK_0D (LWRAM + 0x10 + (0x0D * 0x4000))
#define MUSIC_BANK_0F (LWRAM + 0x10 + (0x0F * 0x4000))

#define LUT_SCORE_DATA (0x8000)
#define LUT_OCTAVE_OFFSET (0xB2F3)
#define LUT_NOTE_FREQS (0xB2F9)
#define LUT_NOTE_LENGTHS (0xB359)
#define LUT_ENV_SPEEDS (0xB3B9)
#define LUT_ENV_PATTERNS (0xB3C9)
#define LUT_VEHICLE_MUSIC (0xC759)
#define LUT_TILESET_MUSIC (0xCFC3)

enum {
    CHAN_SQ1,
    CHAN_SQ2,
    CHAN_TRI,
    CHAN_COUNT
};

//NES ch_* block for one channel ($B0/$C0/$D0).
typedef struct {
    Uint16 scoreptr;
    Uint16 envptr;
    Uint8 envpos;
    Uint8 lenctr;
    Uint16 frqtblptr;
    Uint16 lentblptr;
    Uint8 envrate;
    Uint8 envrem;
    Uint8 vol;
    Uint8 loopctr;
    Uint8 freqLo;
    Uint8 freqHi;
} MUSIC_CHANNEL;

static MUSIC_CHANNEL chans[CHAN_COUNT];
static Uint8 chanPrimer;
//Register writes from game code, run at the start of the next musicFrame.
#define APU_QUEUE_SIZE (64)
static volatile Uint16 apuQueueAddr[APU_QUEUE_SIZE];
static volatile Uint8 apuQueueValue[APU_QUEUE_SIZE];
static volatile Uint8 apuQueueHead;
static volatile Uint8 apuQueueTail;

static Uint8 rom0D(Uint16 addr) {
    return *(Uint8 *)(MUSIC_BANK_0D + (addr & 0x3FFF));
}

static Uint8 rom0F(Uint16 addr) {
    return *(Uint8 *)(MUSIC_BANK_0F + (addr & 0x3FFF));
}

static Uint16 rom0DWord(Uint16 addr) {
    return rom0D(addr) | (rom0D(addr + 1) << 8);
}

void musicInit() {
    music_track = MUSIC_SILENCED;
    sq2_sfx = 0;
    btlFollowupMusic = 0;
    dlgMusicBackup = 0;
    chanPrimer = 0;
    musicHold = FALSE;
    sfxErrorFrames = 0;
    apuQueueHead = 0;
    apuQueueTail = 0;
    for(Uint8 i = 0; i < CHAN_COUNT; i++) {
        chans[i].freqHi = 0x80;
        chans[i].lenctr = 1;
    }
}

//Callable from the main loop or vblank, so the queue is filled with interrupts masked.
void musicApuWrite(Uint16 addr, Uint8 value) {
    Uint32 sr;

    __asm__ volatile("stc sr,%0" : "=r"(sr));
    __asm__ volatile("ldc %0,sr" : : "r"(sr | 0xF0) : "memory");
    if((Uint8)(apuQueueHead - apuQueueTail) < APU_QUEUE_SIZE) {
        apuQueueAddr[apuQueueHead % APU_QUEUE_SIZE] = addr;
        apuQueueValue[apuQueueHead % APU_QUEUE_SIZE] = value;
        apuQueueHead++;
    }
    __asm__ volatile("ldc %0,sr" : : "r"(sr) : "memory");
}

Uint8 musicVehicleTrack(Uint8 vehicle) {
    return rom0F(LUT_VEHICLE_MUSIC + vehicle);
}

Uint8 musicTilesetTrack(Uint8 tileset) {
    return rom0F(LUT_TILESET_MUSIC + tileset);
}

//NES Music_ApplyTone.
static void applyTone(MUSIC_CHANNEL *ch, Uint8 code) {
    Uint8 y = (code >> 3) & 0x1E;

    ch->freqLo = rom0D(ch->frqtblptr + y);
    ch->freqHi = rom0D(ch->frqtblptr + y + 1);
}

//NES Music_ApplyLength. Rests keep the envelope where it is.
static void applyLength(MUSIC_CHANNEL *ch, Uint8 code, Uint8 restartEnv) {
    ch->lenctr = rom0D(ch->lentblptr + (code & 0x0F));
    if(restartEnv) {
        ch->envpos = 0;
        ch->envrem = 0;
    }
}

//NES Music_LoopCode. $D0 loops forever, $Dx repeats x more times.
static void loopCode(MUSIC_CHANNEL *ch, Uint8 code) {
    if(ch->loopctr == 0) {
        ch->loopctr = code & 0x0F;
    } else if(--ch->loopctr == 0) {
        ch->scoreptr += 3;
        return;
    }
    ch->scoreptr = rom0DWord(ch->scoreptr + 1);
}

//NES Music_DoScore: runs control codes until a note or rest sets the channel's length.
static void doScore(Uint8 chan) {
    MUSIC_CHANNEL *ch = &chans[chan];
    Uint8 code;

    while(1) {
        code = rom0D(ch->scoreptr);
        if(code < 0xC0) {
            applyTone(ch, code);
            applyLength(ch, code, TRUE);
            ch->scoreptr++;
            return;
        } else if(code < 0xD0) {
            applyLength(ch, code, FALSE);
            if(chan == CHAN_TRI) {
                ch->freqHi = 0xFF;
            }
            ch->scoreptr++;
            return;
        } else if(code < 0xD8) {
            loopCode(ch, code);
        } else if(code < 0xE0) {
            //NES Music_SetOctave. $DE/$DF read past lut_OctaveOffset like the NES.
            ch->frqtblptr = LUT_NOTE_FREQS + rom0D(LUT_OCTAVE_OFFSET + (code & 0x07));
            ch->scoreptr++;
        } else if(code < 0xF0) {
            //NES Music_SetEnvPattern.
            ch->envptr = LUT_ENV_PATTERNS + ((code & 0x0F) << 5);
            ch->scoreptr++;
        } else if(code == 0xFF) {
            music_track = MUSIC_ENDED;
            return;
        } else if(code == 0xF8) {
            ch->envrate = rom0D(LUT_ENV_SPEEDS + (rom0D(ch->scoreptr + 1) & 0x0F));
            ch->scoreptr += 2;
        } else if(code >= 0xF9) {
            //NES Music_SetTempo.
            ch->lentblptr = LUT_NOTE_LENGTHS + (((code - 0x09) & 0x0F) << 4);
            ch->scoreptr++;
        } else {
            ch->scoreptr++;
        }
    }
}

//NES Music_NewSong: halts the channels, then primes one channel per frame.
static void newSong() {
    static const Uint8 primerToChan[4] = {CHAN_SQ1, CHAN_SQ2, CHAN_TRI, CHAN_COUNT};
    Uint8 chan;
    Uint8 ptr;

    if(music_track >= 0x40) {
        music_track &= 0x3F;
        chanPrimer = 0;
        sq2_sfx = 0;
        nesaudio_write(0x4002, 0);
        nesaudio_write(0x4003, 0);
        nesaudio_write(0x4006, 0);
        nesaudio_write(0x4007, 0);
        nesaudio_write(0x400A, 0);
        nesaudio_write(0x400B, 0);
        nesaudio_write(0x400E, 0);
        nesaudio_write(0x4000, 0x30);
        nesaudio_write(0x4004, 0x30);
        nesaudio_write(0x4008, 0x30);
        nesaudio_write(0x400C, 0x30);
    }

    //The pointer table index is 8 bit, like the NES.
    ptr = (Uint8)(((music_track - 1) << 3) | chanPrimer);
    chan = primerToChan[chanPrimer >> 1];
    chans[chan].scoreptr = rom0DWord(LUT_SCORE_DATA + ptr);
    doScore(chan);

    chanPrimer += 2;
    if(chanPrimer >= 2 * 3) {
        music_track = 0;
        chans[CHAN_SQ1].loopctr = 0;
        chans[CHAN_SQ2].loopctr = 0;
        chans[CHAN_TRI].loopctr = 0;
        nesaudio_write(0x4015, 0x0F);
        sq2_sfx = 0;
        nesaudio_write(0x4002, 0);
        nesaudio_write(0x4003, 0);
        nesaudio_write(0x4006, 0);
        nesaudio_write(0x4007, 0);
        nesaudio_write(0x400A, 0);
        nesaudio_write(0x400B, 0);
    }
}

static void writeSq2(MUSIC_CHANNEL *ch, Uint8 freq) {
    nesaudio_write(0x4004, ch->vol | 0x30);
    nesaudio_write(0x4005, 0x7F);
    if(freq) {
        nesaudio_write(0x4006, ch->freqLo);
        nesaudio_write(0x4007, ch->freqHi & 0x0F);
        ch->freqHi = (ch->freqHi & 0x0F) | 0x80;
    }
}

//NES MusicPlay. Called once per frame.
void musicPlay() {
    MUSIC_CHANNEL *ch;
    Uint8 pos;

    if(music_track & 0x80) {
        if(music_track == MUSIC_ENDED) {
            nesaudio_write(0x4000, 0x70);
            nesaudio_write(0x4004, 0x70);
            nesaudio_write(0x4008, 0x70);
            music_track++;
        }
        return;
    }
    if(music_track != 0) {
        newSong();
        return;
    }

    //Square 1, fixed at 25% duty. Writing $4003 resets the duty, so the freq is only written when it changes.
    ch = &chans[CHAN_SQ1];
    nesaudio_write(0x4000, ch->vol | 0x70);
    nesaudio_write(0x4001, 0x7F);
    if(!(ch->freqHi & 0x80)) {
        nesaudio_write(0x4002, ch->freqLo);
        nesaudio_write(0x4003, ch->freqHi);
        ch->freqHi = 0x80;
    }

    //Square 2 is skipped while a sound effect has it, then fully rewritten when the effect ends.
    ch = &chans[CHAN_SQ2];
    if(sq2_sfx) {
        if(--sq2_sfx == 0) {
            writeSq2(ch, TRUE);
        }
    } else {
        writeSq2(ch, !(ch->freqHi & 0x80));
    }

    //Triangle. A freq high byte of $FF marks a rest.
    ch = &chans[CHAN_TRI];
    if(!(ch->freqHi & 0x80)) {
        nesaudio_write(0x4008, 0x8F);
        nesaudio_write(0x400A, ch->freqLo);
        nesaudio_write(0x400B, ch->freqHi);
        ch->freqHi = 0x80;
    } else if(ch->freqHi == 0xFF) {
        nesaudio_write(0x4008, 0x80);
    }

    for(Uint8 i = 0; i < CHAN_COUNT; i++) {
        ch = &chans[i];
        if(--ch->lenctr == 0) {
            doScore(i);
            pos = ch->envpos;
        } else {
            ch->envrem += ch->envrate;
            if(ch->envrem < 8) {
                continue;
            }
            pos = (ch->envrem >> 3) + ch->envpos;
            ch->envrem &= 7;
        }
        if(pos >= 0x20) {
            pos = 0x1F;
        }
        ch->envpos = pos;
        ch->vol = rom0D(ch->envptr + pos);
    }
}

#define BATTLE_BANK_0C (LWRAM + 0x10 + (0x0C * 0x4000))
#define LUT_BATTLE_SFX (0xBFA4)

//NES btlsfx_* variables.
static Uint8 *btlSfxSq2Ptr;
static Uint8 *btlSfxNsePtr;
static Uint8 btlSfxFrameCtr;
static Uint8 btlSfxSq2Len;
static Uint8 btlSfxNseLen;
static Uint8 btlSfxSq2FrameCtr;
static Uint8 btlSfxNseFrameCtr;

//NES UpdateBattleSFX_Square.
static void battleSfxSquare() {
    if(btlSfxSq2Len == 0 || --btlSfxSq2FrameCtr != 0) {
        return;
    }
    btlSfxSq2FrameCtr = btlSfxSq2Ptr[4];
    nesaudio_write(0x4004, btlSfxSq2Ptr[0]);
    nesaudio_write(0x4005, btlSfxSq2Ptr[1]);
    nesaudio_write(0x4006, btlSfxSq2Ptr[2]);
    nesaudio_write(0x4007, btlSfxSq2Ptr[3]);
    btlSfxSq2Ptr += 8;
    btlSfxSq2Len--;
}

//NES UpdateBattleSFX_Noise.
static void battleSfxNoise() {
    if(btlSfxNseLen == 0 || --btlSfxNseFrameCtr != 0) {
        return;
    }
    btlSfxNseFrameCtr = btlSfxNsePtr[2];
    nesaudio_write(0x400C, btlSfxNsePtr[0]);
    nesaudio_write(0x400E, btlSfxNsePtr[1]);
    nesaudio_write(0x400F, 0xFF);
    btlSfxNsePtr += 8;
    btlSfxNseLen--;
}

//NES PlayBattleSFX, run before MusicPlay so sq2_sfx is set first.
static void battleSfxStart(Uint8 id) {
    Uint8 *lut = (Uint8 *)BATTLE_BANK_0C + (LUT_BATTLE_SFX & 0x3FFF) + (id * 2);
    Uint8 *sfx = (Uint8 *)BATTLE_BANK_0C + ((lut[0] | (lut[1] << 8)) & 0x3FFF);

    btlSfxSq2Ptr = sfx + 3;
    btlSfxNsePtr = sfx + 8;
    sq2_sfx = sfx[0];
    btlSfxFrameCtr = sfx[0];
    btlSfxSq2Len = sfx[1];
    btlSfxNseLen = sfx[2];
    btlSfxSq2FrameCtr = 1;
    btlSfxNseFrameCtr = 1;
    battleSfxSquare();
    battleSfxNoise();
    nesaudio_write(0x4015, 0x0F);
}

//NES UpdateBattleSFX.
static void battleSfxUpdate() {
    if(btlSfxFrameCtr) {
        battleSfxSquare();
        battleSfxNoise();
        btlSfxFrameCtr--;
    }
}

void battleSfx(Uint8 id) {
    battleSfxRequest = id + 1;
}

//Runs the queued writes, then MusicPlay where the NES frame routine for this state calls it.
void musicFrame() {
    Uint8 battle = FALSE;

    while(apuQueueTail != apuQueueHead) {
        nesaudio_write(apuQueueAddr[apuQueueTail % APU_QUEUE_SIZE], apuQueueValue[apuQueueTail % APU_QUEUE_SIZE]);
        apuQueueTail++;
    }

    //NES PlaySFX_Error @Frame, without MusicPlay.
    if(sfxErrorFrames) {
        nesaudio_write(0x4004, 0x8C);
        nesaudio_write(0x4005, 0x89);
        nesaudio_write(0x4006, 0x80);
        nesaudio_write(0x4007, 0x00);
        if(--sfxErrorFrames == 0) {
            nesaudio_write(0x4004, 0x30);
            nesaudio_write(0x4006, 0x00);
        }
        return;
    }

    if(musicHold) {
        return;
    }
    switch(state) {
        //Map loading with the PPU off, ScreenWipeFrame and BattleTransition don't call MusicPlay.
        case TO_OW_STATE:
        case TO_SM_STATE:
        case WIPE:
        case BAT_TRANSITION:
            return;
        //NES MenuFrame restarts the menu music once a jingle ends.
        case MAIN_MENU:
        case STATUS_MENU:
        case MAGIC_MENU:
        case ITEM_MENU:
        case SAVE_MENU:
        case BUGFIX_MENU:
            if(music_track & 0x80) {
                music_track = MUSIC_MENU;
            }
            break;
        //NES BattleUpdateAudio.
        case TO_BAT_STATE:
        case BAT_STATE:
        case TO_BAT_MENU_STATE:
        case BAT_MENU_STATE:
        case TO_BAT_EXECUTE_STATE:
        case BAT_EXECUTE_STATE:
        case TO_BAT_VICTORY_STATE:
        case BAT_VICTORY_STATE:
        case TO_BAT_RUN_AWAY:
        case BAT_RUN_AWAY:
        case TO_GAME_OVER:
        case GAME_OVER:
        case BAT_FADE_IN:
        case BAT_DRAW_STATUS:
        case BAT_FADE_OUT:
        case BAT_INIT_MSG:
        case BAT_CHAOS_EXIT:
            if(music_track & 0x80) {
                music_track = btlFollowupMusic;
            }
            battle = TRUE;
            if(battleSfxRequest) {
                battleSfxStart(battleSfxRequest - 1);
                battleSfxRequest = 0;
                //The NES starts the effect mid-frame, so its next update is a frame later.
                battle = FALSE;
            }
            break;
        default:
            break;
    }
    musicPlay();
    if(battle && !battleSfxHold) {
        battleSfxUpdate();
    }
}

//NES PlaySFX_MenuSel.
void sfxMenuSel() {
    musicApuWrite(0x4004, 0xBA);
    musicApuWrite(0x4005, 0xBA);
    musicApuWrite(0x4006, 0x40);
    musicApuWrite(0x4007, 0x00);
    sq2_sfx = 0x1F;
}

//NES PlaySFX_MenuMove.
void sfxMenuMove() {
    musicApuWrite(0x4004, 0x7A);
    musicApuWrite(0x4005, 0x9B);
    musicApuWrite(0x4006, 0x20);
    musicApuWrite(0x4007, 0x10);
    sq2_sfx = 0x10;
}

//NES PlaySFX_Error. The NES stays in this routine for its 16 frames, so the game waits on sfxErrorFrames.
void sfxError() {
    sq2_sfx = 1;
    musicApuWrite(0x4000, 0x30);
    musicApuWrite(0x4008, 0x30);
    musicApuWrite(0x400C, 0x30);
    sfxErrorFrames = 16;
}

//NES DialogueBox_Sfx: $8E sweeps up for opening, $95 down for closing.
void sfxDialogueBox(Uint8 sweep, Uint8 frames) {
    Uint8 lo = (sweep >> 2) | ((sweep & 0x01) << 7);

    sq2_sfx = frames;
    musicApuWrite(0x4004, 0x38);
    musicApuWrite(0x4005, sweep);
    musicApuWrite(0x4006, lo ^ 0xFF);
    musicApuWrite(0x4007, 0x00);
}

//NES MiniGameLoop's sound for a piece that can't slide.
void sfxMinigameError() {
    musicApuWrite(0x4004, 0x3C);
    musicApuWrite(0x4005, 0x8C);
    musicApuWrite(0x4006, 0x70);
    musicApuWrite(0x4007, 0x00);
    sq2_sfx = 0x14;
}

//NES PlayDoorSFX.
void sfxDoor() {
    musicApuWrite(0x400C, 0x0C);
    musicApuWrite(0x400E, 0x0E);
    musicApuWrite(0x400F, 0x30);
}

//NES MapPoisonDamage sound.
void sfxPoison() {
    musicApuWrite(0x4004, 0x3A);
    musicApuWrite(0x4005, 0x81);
    musicApuWrite(0x4006, 0x60);
    musicApuWrite(0x4007, 0x60);
    sq2_sfx = 6;
}

//NES MapTileDamage sound.
void sfxTileDamage() {
    musicApuWrite(0x400C, 0x0F);
    musicApuWrite(0x400E, 0x0D);
    musicApuWrite(0x400F, 0x00);
}
