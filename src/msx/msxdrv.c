#include "types.h"
#include "msxchip.h"
#include "msxdrv.h"

#define MUSIC_CHANNELS 12
#define LOOP_DEPTH 5

//Channel ids: PSG A-C, OPLL melody 0-5, rhythm BD, SD, HH.
#define ID_PSG 0x80
#define ID_SFX 0x82

#define FLAG_KEY 0x10
#define FLAG_SUSTAIN 0x20
#define FLAG_TIE 0x40
#define FLAG_HOLD 0x80

#define LFO_ON 0x80
#define LFO_REINIT 0x04
#define LFO_PITCH 0x08
#define LFO_INVERT 0x10

typedef struct {
    Uint8 timer;
    const Uint8 *ptr;
    Uint8 len;
    Uint8 note;
    Uint16 freq;
    Uint8 oct;
    Uint8 vol;
    Uint8 level;
    Uint8 gate;
    Uint8 flags;
    Uint8 envCnt, envReload, envTarget, envStep, envStart;
    Uint8 swCnt, swReload;
    Uint16 swDelta;
    Uint8 inst;
    Uint8 lfoMode, lfoDepth, lfoValue;
    Uint16 lfoPeriod, lfoCount;
    Uint8 id;
    Uint8 loopCount[LOOP_DEPTH];
    const Uint8 *loopPtr[LOOP_DEPTH];
} MsxChannel;

static const Uint8 chanIds[MUSIC_CHANNELS] = {0x80, 0x81, 0x82, 0, 1, 2, 3, 4, 5, 6, 7, 8};

//MAIN.COM 0x7DB9: OPLL fnum, PSG period before the octave shift.
static const Uint16 noteTable[36][2] = {
    {0x00AA, 0x1AFC}, {0x00AC, 0x1AB7}, {0x00AD, 0x1A73}, {0x00B4, 0x197A},
    {0x00B6, 0x1939}, {0x00B7, 0x18F9}, {0x00BE, 0x1814}, {0x00C0, 0x17D6},
    {0x00C2, 0x179A}, {0x00CA, 0x16B3}, {0x00CC, 0x1679}, {0x00CE, 0x1640},
    {0x00D6, 0x156D}, {0x00D8, 0x1536}, {0x00DA, 0x1500}, {0x00E3, 0x1439},
    {0x00E5, 0x1405}, {0x00E7, 0x13D3}, {0x00F0, 0x1316}, {0x00F2, 0x12E5},
    {0x00F5, 0x12B5}, {0x00FE, 0x1204}, {0x0101, 0x11D6}, {0x0103, 0x11A9},
    {0x010D, 0x1101}, {0x0110, 0x10D6}, {0x0113, 0x10AB}, {0x011D, 0x100D},
    {0x0120, 0x0FE4}, {0x0123, 0x0FBB}, {0x012E, 0x0F26}, {0x0132, 0x0EFF},
    {0x0135, 0x0ED9}, {0x0140, 0x0E4C}, {0x0144, 0x0E28}, {0x0147, 0x0E04},
};

//MAIN.COM 0x7E51: silences a channel for one frame.
static const Uint8 stopSeq[] = {0xFE, 0x00, 0x32, 0x98, 0x29, 0x02, 0x00};

static MsxChannel sfx;
static MsxChannel music[MUSIC_CHANNELS];

static Uint8 bgmBusy, active, sfxBusy, psgMask;
static Uint8 fadeTick, fadeLevel, fadeCtl;
static Uint8 curId, mixer, userInst;
static Uint16 seed;
static Uint8 rhythmKey, rhythmSel, rhythmVol7, rhythmVol8;

static const Uint8 *volEnvTable, *sweepTable, *lfoTable, *patchTable, *sfxData;

static Uint8 rot3(Uint8 v) {
    return (Uint8)((v << 3) | (v >> 5));
}

static Uint16 readWord(const Uint8 *p) {
    return p[0] | (p[1] << 8);
}

static Uint8 readByte(MsxChannel *ch) {
    return *ch->ptr++;
}

static Uint8 octBits(MsxChannel *ch) {
    return (Uint8)(ch->oct << 1);
}

static void writeLevel(MsxChannel *ch, Uint8 c) {
    Uint8 id = curId;
    Uint8 a;

    if(fadeCtl >= 0x80) {
        if(ch->id & 0x80) {
            c = c < fadeLevel ? 0 : c - fadeLevel;
        } else {
            c = (Uint8)(c + fadeLevel);
            if(c >= 15) c = 15;
        }
    }
    if(id >= 0x80) {
        if(psgMask & (Uint8)(id << 1)) return;
        msxPsgWrite((Uint8)(id + 0x88), c);
    } else if(id < 7) {
        msxOpllWrite(0x30 + id, (Uint8)((ch->inst << 4) | c));
    } else {
        if(id == 7) {
            rhythmVol7 = c;
            a = (Uint8)(rhythmVol8 << 4) | c;
        } else {
            rhythmVol8 = c;
            a = (Uint8)(c << 4) | rhythmVol7;
        }
        msxOpllWrite(0x37, a);
        msxOpllWrite(0x38, (Uint8)((a >> 4) | (a << 4)));
    }
}

static void freqOut(MsxChannel *ch, Uint8 l, Uint8 h) {
    Uint8 id = curId;
    if(id >= 0x80) {
        Uint8 b = (Uint8)(id << 1);
        if(h & 0x80) {
            msxPsgWrite(6, l);
        } else if(!(psgMask & b)) {
            msxPsgWrite(b, l);
            msxPsgWrite(b + 1, h);
        }
    } else {
        Uint8 c = id;
        if(id >= 7) c = (rhythmSel & 9) ? 7 : 8;
        msxOpllWrite(0x10 + c, l);
        msxOpllWrite(0x20 + c, (ch->flags & FLAG_KEY) | h);
    }
}

static void freqWrite(MsxChannel *ch) {
    Uint8 h = ch->freq >> 8;
    if(curId < 0x80) h |= octBits(ch);
    freqOut(ch, ch->freq & 0xFF, h);
}

static void lfoApply(MsxChannel *ch, Uint8 a) {
    ch->lfoValue = a;
    if(!(ch->lfoMode & LFO_PITCH)) {
        Uint8 b = (Uint8)(a + ch->level);
        if(b < (curId >= 0x80 ? 0x10 : 0x80)) writeLevel(ch, b);
    } else {
        Uint16 hl = ch->freq + (Uint16)(Sint16)(Sint8)a;
        Uint8 h = hl >> 8;
        if(curId < 0x80) h |= octBits(ch);
        freqOut(ch, hl & 0xFF, h);
    }
}

//rand % (2 * depth) - depth, with the driver's 16-bit generator.
static Uint8 lfoRandom(MsxChannel *ch) {
    Uint16 d2 = (Uint16)(ch->lfoDepth * 2);
    Uint16 s = seed ? seed : 1;
    s = (Uint16)(s * 0x3D09) - 1;
    seed = (Uint16)((s >> 8) | (s << 8));
    return (Uint8)((d2 ? seed % d2 : seed) - ch->lfoDepth);
}

//Past the peak in the current direction.
static Uint8 lfoOver(MsxChannel *ch, Uint8 h) {
    Uint8 d = ch->lfoDepth;
    if(!(d & 0x80)) return !(h & 0x80) && h > d;
    return (h & 0x80) && h < d;
}

static Uint8 lfoFlip(MsxChannel *ch) {
    if(ch->lfoDepth & 0x80) return !(ch->lfoMode & LFO_INVERT);
    return (ch->lfoMode & LFO_INVERT) != 0;
}

static void lfoInit(MsxChannel *ch) {
    switch(ch->lfoMode & 3) {
        case 0:
            ch->lfoCount = 0;
            ch->lfoValue = 0;
            break;
        case 1:
            ch->lfoCount = ch->lfoPeriod;
            if(lfoFlip(ch)) ch->lfoDepth = -ch->lfoDepth;
            ch->lfoValue = ch->lfoDepth;
            break;
        case 2:
            ch->lfoCount = 0;
            ch->lfoValue = 0;
            if(lfoFlip(ch)) {
                ch->lfoDepth = -ch->lfoDepth;
                ch->lfoPeriod = -ch->lfoPeriod;
            }
            break;
        default:
            ch->lfoValue = lfoRandom(ch);
            ch->lfoCount = ch->lfoPeriod;
            break;
    }
}

static void lfoTick(MsxChannel *ch) {
    Uint16 hl;
    Uint8 a;

    if(!(ch->lfoMode & LFO_ON)) return;
    switch(ch->lfoMode & 3) {
        case 0:
            //Sawtooth.
            hl = ch->lfoCount + ch->lfoPeriod;
            if(lfoOver(ch, hl >> 8)) hl = -(Uint16)(ch->lfoDepth << 8);
            if((hl >> 8) != ch->lfoValue) lfoApply(ch, hl >> 8);
            ch->lfoCount = hl;
            break;
        case 1:
            //Square.
            hl = ch->lfoCount - 1;
            if(hl == 0) {
                ch->lfoDepth = -ch->lfoDepth;
                lfoApply(ch, ch->lfoDepth);
                hl = ch->lfoPeriod;
            }
            ch->lfoCount = hl;
            break;
        case 2:
            //Triangle.
            hl = ch->lfoCount + ch->lfoPeriod;
            if(lfoOver(ch, hl >> 8)) {
                ch->lfoDepth = -ch->lfoDepth;
                ch->lfoPeriod = -ch->lfoPeriod;
            }
            if((hl >> 8) != ch->lfoValue) lfoApply(ch, hl >> 8);
            ch->lfoCount = hl;
            break;
        default:
            //Random.
            hl = ch->lfoCount - 1;
            if(hl == 0) {
                a = lfoRandom(ch);
                if(a != ch->lfoValue) lfoApply(ch, a);
                hl = ch->lfoPeriod;
            }
            ch->lfoCount = hl;
            break;
    }
}

static Uint8 rhythmBit(Uint8 id) {
    if(id == 6) return 0x10;
    if(id == 7) return rhythmSel & 0x0C;
    return rhythmSel & 0x03;
}

static void keyOn(MsxChannel *ch) {
    Uint8 id = curId;

    if(id >= 0x80) {
        Uint8 c = (Uint8)((id & 3) << 1);
        Uint8 m;
        if(!c) c = 1;
        if(ch->freq & 0x8000) {
            m = rot3(~c);
        } else {
            m = ~c;
            c = rot3(c);
        }
        mixer = (mixer & m) | c;
        msxPsgWrite(7, mixer);
    } else if(id >= 6) {
        rhythmKey |= rhythmBit(id);
        msxOpllWrite(0x0E, rhythmKey);
        return;
    } else {
        msxOpllWrite(0x10 + id, ch->freq & 0xFF);
        msxOpllWrite(0x20 + id, octBits(ch) | (ch->freq >> 8) | FLAG_KEY);
        ch->flags |= FLAG_KEY;
    }
    if(ch->lfoMode & LFO_ON) {
        if(ch->lfoMode & LFO_REINIT) lfoInit(ch);
        lfoApply(ch, ch->lfoValue);
    }
}

static void keyOff(MsxChannel *ch) {
    Uint8 id = curId;

    if(id >= 0x80) {
        Uint8 n = id & 3;
        Uint8 c = 9;
        if(n) {
            c <<= 1;
            if(n != 1) c <<= 1;
        }
        mixer |= c;
        msxPsgWrite(7, mixer);
    } else if(id >= 6) {
        rhythmKey &= ~rhythmBit(id);
        msxOpllWrite(0x0E, rhythmKey);
    } else {
        msxOpllWrite(0x20 + id, octBits(ch) | (ch->flags & FLAG_SUSTAIN));
        ch->flags &= ~FLAG_KEY;
    }
}

static void setGate(MsxChannel *ch) {
    ch->gate = (Uint8)((ch->len * (ch->flags & 7)) >> 3);
}

//Note, rest and PSG noise end the channel's frame: these return 1.
static Uint8 noteStart(MsxChannel *ch) {
    Uint8 a;

    freqWrite(ch);
    writeLevel(ch, ch->level);
    a = ch->flags;
    if(a < 0x80) {
        a += 0x40;
        if(a & 0x80) ch->flags = a;
        keyOff(ch);
        keyOn(ch);
    }
    ch->timer = ch->len;
    return 1;
}

static Uint8 noteCmd(MsxChannel *ch, Uint8 n) {
    ch->note = n;
    if(curId >= 0x80) {
        Uint16 de = noteTable[n][1];
        Uint8 b, carry = 0;
        for(b = ch->oct + 1; b; b--) {
            carry = de & 1;
            de >>= 1;
        }
        if(carry) de++;
        ch->freq = de;
    } else {
        ch->freq = noteTable[n][0];
    }
    return noteStart(ch);
}

static Uint8 restCmd(MsxChannel *ch) {
    Uint8 a;
    keyOff(ch);
    ch->timer = ch->len;
    a = ch->flags + 0x40;
    if(a & 0x80) ch->flags = a & 0x7F;
    return 1;
}

static void volumeCmd(MsxChannel *ch, Uint8 v) {
    ch->vol = v & 15;
    ch->envReload = 0;
    ch->envCnt = 0;
    ch->level = (ch->id & 0x80) ? ch->vol : 15 - ch->vol;
}

static Uint8 instCmd(MsxChannel *ch, Uint8 n) {
    Uint8 b;
    const Uint8 *p;

    if(ch->id & 0x80) {
        ch->freq = 0x8000 | n;
        return noteStart(ch);
    }
    if(n < 16) {
        ch->inst = n;
        return 0;
    }
    ch->inst = 0;
    userInst = n - 16;
    p = patchTable + userInst * 8;
    for(b = 0; b < 8; b++) msxOpllWrite(b, p[b]);
    return 0;
}

static void loopStart(MsxChannel *ch, Uint8 count) {
    Uint8 i;
    for(i = LOOP_DEPTH - 1; i; i--) {
        ch->loopCount[i] = ch->loopCount[i - 1];
        ch->loopPtr[i] = ch->loopPtr[i - 1];
    }
    ch->loopCount[0] = count;
    ch->loopPtr[0] = ch->ptr;
}

static void loopEnd(MsxChannel *ch) {
    Uint8 c = ch->loopCount[0];
    Uint8 i;
    if(c) {
        if(--c == 0) {
            for(i = 0; i < LOOP_DEPTH - 1; i++) {
                ch->loopCount[i] = ch->loopCount[i + 1];
                ch->loopPtr[i] = ch->loopPtr[i + 1];
            }
            return;
        }
        ch->loopCount[0] = c;
    }
    ch->ptr = ch->loopPtr[0];
}

static void volEnvCmd(MsxChannel *ch, Uint8 n) {
    const Uint8 *p = volEnvTable + n * 3;
    ch->envCnt = ch->envReload = p[0];
    ch->envTarget = p[1];
    ch->envStep = p[2];
    ch->envStart = ch->level;
}

static void sweepCmd(MsxChannel *ch, Uint8 n) {
    const Uint8 *p = sweepTable + n * 3;
    ch->swCnt = ch->swReload = p[0];
    ch->swDelta = p[1] | (p[2] << 8);
}

static void lfoCmd(MsxChannel *ch, Uint8 n) {
    const Uint8 *p = lfoTable + n * 4;
    ch->lfoMode = p[0];
    ch->lfoDepth = p[1];
    ch->lfoPeriod = p[2] | (p[3] << 8);
    lfoInit(ch);
}

static void controlCmd(MsxChannel *ch, Uint8 n) {
    switch(n) {
        case 0:
            ch->flags |= FLAG_TIE;
            break;
        case 1:
            ch->flags &= 0x37;
            keyOff(ch);
            break;
        case 2:
            ch->flags &= ~FLAG_SUSTAIN;
            break;
        case 3:
            ch->flags |= FLAG_SUSTAIN;
            break;
        case 4:
            //SD and HH.
            if(curId == 7) rhythmSel = (rhythmSel & ~0x04) | 0x08;
            else rhythmSel = (rhythmSel & ~0x02) | 0x01;
            break;
        default:
            //TOM and CYM.
            if(curId == 7) rhythmSel = (rhythmSel & ~0x08) | 0x04;
            else rhythmSel = (rhythmSel & ~0x01) | 0x02;
            break;
    }
}

//Opcode table at MAIN.COM 0x773F.
static Uint8 command(MsxChannel *ch, Uint8 op) {
    Uint8 a;

    if(op <= 0x24) return noteCmd(ch, op - 0x01);
    if(op == 0x25) return restCmd(ch);
    if(op <= 0x27) {
        ch->oct = (ch->oct + (op == 0x26 ? 0xFF : 1)) & 7;
    } else if(op <= 0x2F) {
        ch->oct = op - 0x28;
    } else if(op <= 0x31) {
        volumeCmd(ch, ch->vol + (op == 0x30 ? 0xFF : 1));
    } else if(op <= 0x41) {
        volumeCmd(ch, op - 0x32);
    } else if(op <= 0x61) {
        return instCmd(ch, op - 0x42);
    } else if(op <= 0x71) {
        loopStart(ch, op - 0x62);
    } else if(op == 0x72) {
        loopEnd(ch);
    } else if(op <= 0x82) {
        volEnvCmd(ch, op - 0x73);
    } else if(op <= 0x92) {
        sweepCmd(ch, op - 0x83);
    } else if(op <= 0x96) {
        //Length +-1/2: the driver falls through to FA, so the next byte replaces the length.
        ch->len = readByte(ch);
        setGate(ch);
    } else if(op <= 0xF1) {
        ch->len = op - 0x97;
        setGate(ch);
    } else if(op <= 0xF9) {
        ch->flags = (ch->flags & 0xF0) | (op - 0xF2);
        setGate(ch);
    } else if(op == 0xFA) {
        ch->len = readByte(ch);
        setGate(ch);
    } else if(op == 0xFB) {
        loopStart(ch, readByte(ch));
    } else if(op == 0xFC) {
        return instCmd(ch, readByte(ch));
    } else if(op == 0xFD) {
        a = readByte(ch);
        msxOpllWrite(a, readByte(ch));
    } else if(op == 0xFE) {
        lfoCmd(ch, readByte(ch));
    } else {
        controlCmd(ch, readByte(ch));
    }
    return 0;
}

static void channelTick(MsxChannel *ch, Uint8 id) {
    Uint8 op;

    curId = id;
    ch->id = id;
    if(!ch->timer) return;
    active = 1;
    lfoTick(ch);
    ch->timer--;
    if(!(ch->flags & FLAG_HOLD) && ch->gate == ch->timer) {
        //PSG notes with no gate are never released.
        if(ch->gate || !(id & 0x80)) keyOff(ch);
    }

    if(ch->timer) {
        Uint8 stepped = 0;
        if(ch->envCnt && !--ch->envCnt) {
            Uint16 sum = ch->envStep + ch->level;
            Uint8 a = (Uint8)sum;
            if(sum > 0xFF ? a >= ch->envTarget : a <= ch->envTarget) {
                ch->level = a;
                writeLevel(ch, a);
                ch->envCnt = ch->envReload;
                stepped = 1;
            }
        }
        if(!stepped && (fadeCtl & 0x80)) writeLevel(ch, ch->level);
        if(!ch->swCnt || --ch->swCnt) return;
        ch->swCnt = ch->swReload;
        ch->freq += ch->swDelta;
        freqWrite(ch);
        return;
    }

    ch->swCnt = 0;
    if(ch->envReload) {
        ch->envCnt = ch->envReload;
        ch->level = ch->envStart;
    }
    for(;;) {
        op = *ch->ptr;
        if(!op) {
            keyOff(ch);
            return;
        }
        ch->ptr++;
        if(command(ch, op)) return;
    }
}

static void clearChannel(MsxChannel *ch) {
    Uint8 *p = (Uint8 *)ch;
    Uint32 i;
    for(i = 0; i < sizeof(MsxChannel); i++) p[i] = 0;
}

//MAIN.COM 0x752F.
static void driverReset() {
    Uint8 i;

    clearChannel(&sfx);
    for(i = 0; i < MUSIC_CHANNELS; i++) clearChannel(&music[i]);
    bgmBusy = active = sfxBusy = psgMask = 0;
    fadeTick = fadeLevel = fadeCtl = 0;
    curId = userInst = 0;
    seed = 0;
    rhythmVol7 = rhythmVol8 = 0;
    mixer = msxPsgRead(7) | 0x3F;
    msxPsgWrite(7, mixer);
    rhythmKey = 0x20;
    rhythmSel = 9;
    bgmBusy = 1;
}

static void startSilence() {
    Uint8 i;
    for(i = 0; i < MUSIC_CHANNELS; i++) {
        music[i].timer = 1;
        music[i].ptr = stopSeq;
    }
}

void msxDrvInit(const Uint8 *oranyo, const Uint8 *sfxBank) {
    volEnvTable = oranyo;
    sweepTable = oranyo + 0x30;
    lfoTable = oranyo + 0x60;
    patchTable = oranyo + 0xE0;
    sfxData = sfxBank;
    driverReset();
}

void msxDrvPlayBgm(const Uint8 *bgm) {
    const Uint8 *base, *hl, *swap;
    MsxChannel *ch = music;

    driverReset();
    if(*bgm == 0xFE) bgm += 7;
    base = bgm;
    hl = bgm + 2;
    do {
        ch->timer = 1;
        ch->ptr = hl + 2;
        ch++;
        hl = base + readWord(hl);
    } while(readWord(hl) && ch < music + MUSIC_CHANNELS);
    //Music PSG B and C swap so that effects, on C, take over the second part.
    swap = music[1].ptr;
    music[1].ptr = music[2].ptr;
    music[2].ptr = swap;
}

void msxDrvStopBgm() {
    driverReset();
    startSilence();
}

void msxDrvFade(Uint8 speed) {
    fadeCtl = 0x80 | speed;
}

Uint8 msxDrvBgmBusy() {
    return bgmBusy;
}

Uint8 msxDrvPlaySfx(Uint8 id) {
    const Uint8 *p;
    Uint8 a;

    if((Uint8)(id - 1) >= sfxData[0]) return 0xFF;
    p = sfxData + readWord(sfxData + id * 2);
    a = *p;
    //Multi-part entries: only the last part is kept, as in the driver.
    do {
        sfx.timer = 1;
        p++;
        sfx.ptr = p;
        p += a;
        a = *p;
    } while(a);
    sfxBusy = 4;
    return 0;
}

void msxDrvStopSfx() {
    if(!sfxBusy) return;
    sfx.timer = 0;
    curId = ID_SFX;
    sfx.id = ID_SFX;
    keyOff(&sfx);
    sfxBusy = 0;
}

Uint8 msxDrvSfxBusy() {
    return sfxBusy;
}

Uint8 msxDrvSfxCount() {
    return sfxData[0];
}

void msxDrvTick() {
    Uint8 i;

    if(fadeCtl >= 0x80) {
        if((fadeCtl & 0x7F) != fadeTick) {
            fadeTick++;
        } else {
            fadeTick = 0;
            if(++fadeLevel >= 16) {
                bgmBusy = 0;
                driverReset();
                startSilence();
            }
        }
    }

    active = 0;
    psgMask = sfxBusy;
    for(i = 0; i < MUSIC_CHANNELS; i++) channelTick(&music[i], chanIds[i]);
    if(sfxBusy) {
        active = 0;
        psgMask = 0;
        channelTick(&sfx, ID_SFX);
        if(!active) sfxBusy = 0;
    } else if(!active) {
        bgmBusy = 0;
    }
}
