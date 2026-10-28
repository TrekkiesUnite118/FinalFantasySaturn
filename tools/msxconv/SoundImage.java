import java.io.ByteArrayOutputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

/**
 * Builds MSXSND.BIN, a sound RAM image loaded as is: the sound 68K's MSX chip emulation
 * (msx68k.s, from the jar), the waveform tables it plays, and the song bank the SH-2 driver reads.
 *
 * Sound RAM layout (byte addresses), mirrored in msx68k.s and src/ff1/msxsound.c:
 *   0x00000  68K program, then its mailbox ring at 0x3800
 *   0x04000  waveform tables (see buildWaves)
 *   0x37800  hi-hat tone tables (see hhTables)
 *   0x3C000  song bank, all values big-endian:
 *            char[4] "MSXS", Uint16 version, Uint16 entry count,
 *            entries, 8 bytes each: Uint32 sound RAM address, Uint32 size
 *   0x44000  band-limited PSG squares (see psgSquares)
 *
 * Song bank entries: 0 ORANYO.BIN, 1 FF.BGE, 2 MAIN.COM map -> song table (61 bytes, MSX directory
 * numbers), 3-23 the songs by MSX directory number 0x3C-0x50.
 * 
 * This was writtenby Claude Opus 5.5
 */
final class SoundImage {
    static final int VERSION = 1;

    static final int PROGRAM_END = 0x3000;
    static final int WAVE_BASE = 0x4000;
    static final int FM_CYCLE = 1024;
    static final int WAVE_SINE = WAVE_BASE + 0x00000 + 4 * FM_CYCLE * 2;
    static final int WAVE_HALFSINE = WAVE_BASE + 0x05000 + 4 * FM_CYCLE * 2;
    static final int WAVE_PSG_NOISE = WAVE_BASE + 0x0E000;
    static final int WAVE_OPLL_NOISE = WAVE_BASE + 0x1E000;
    static final int WAVE_NOISE_LEN = 0xFFFF;
    static final int WAVE_BIT_HH_A = WAVE_BASE + 0x2E000;
    static final int WAVE_BIT_HH_B = WAVE_BASE + 0x2E800;
    static final int WAVE_BIT_CYM_A = WAVE_BASE + 0x2F000;
    static final int WAVE_BIT_CYM_B = WAVE_BASE + 0x2F800;
    static final int WAVE_BIT_SD = WAVE_BASE + 0x30000;
    static final int WAVE_XOR_LOOKUP = WAVE_BASE + 0x30800 + 512 * 2;
    static final int WAVE_OR_LOOKUP = WAVE_BASE + 0x31000 + 512 * 2;
    static final int WAVE_SN_LOOKUP = WAVE_BASE + 0x31800 + 512 * 2;
    static final int WAVE_HH_LOOKUP = WAVE_BASE + 0x32000 + 512 * 2;
    static final int WAVE_SD_LOOKUP = WAVE_BASE + 0x32800 + 512 * 2;
    static final int WAVE_HH_RAMP = WAVE_BASE + 0x33000 + 512 * 2;
    static final int HH_TABLES = WAVE_BASE + 0x33800;
    static final int SONG_BANK = 0x3C000;
    static final int RAM_SIZE = 0x80000;

    //PSG squares: count, period limits, then the tables, in one 64K bank (msx68k.s PSG_SQUARES).
    static final int PSG_SQUARES = 0x44000;
    static final int PSG_SQ_TABLES = PSG_SQUARES + 0x40;
    static final int PSG_SQ_STRIDE = 0x804;
    //Odd harmonics kept by each table, 0 = silent. Each is used for the periods whose highest
    //kept harmonic stays below 22050 Hz.
    static final int[] PSG_SQ_HARMONICS = {0, 1, 3, 5, 7, 11, 15, 21, 31, 45, 63, 89, 127, 181, 255};
    static final double PSG_TONE_HZ = 1789772.5 / 16;
    //The sine table's peak (4/pi of this) stays inside 16 bits.
    static final double PSG_SQ_LEVEL = 25700;

    //Two-bit logic lookups: inputs of BIT_UNIT, BIT_STEP samples per set bit at msx68k.s MDL_BIT.
    static final int BIT_UNIT = 16384;
    static final int BIT_STEP = 128;
    //Drum output lookups: noise is DRUM_UNIT, the snare bit and short_noise 2 units, DRUM_STEP
    //samples per unit at msx68k.s MDL_LOOKUP.
    static final int DRUM_UNIT = 8192;
    static final int DRUM_STEP = 128;

    static final int MAIN_SONG_TABLE = 0x057D;
    static final int MAP_COUNT = 61;
    static final int SONG_FIRST = 0x3C;
    static final String[] SONGS = {
        "OPENING", "PRERO", "SHIRO", "THEME", "KAOSS", "MATOOYA", "TOWN", "SHOP", "FUNE", "KAITEI",
        "RANCHON", "MENU", "HIKO_SEN", "GURUGU", "HUYU_JYO", "SENTOH", "SYO_RI", "ENDING", "DIED", "SAVE_", "GET",
    };
    //The driver's song buffer, MSX 0xD000-0xD8FF.
    static final int SONG_MAX = 0x900;

    //Hi-hat tone tables: channel 7 F-number and block of each.
    static final int[][] HH_PITCHES = {{257, 3}, {172, 4}};
    static final double HH_NATIVE_RATE = 3579545.0 / 72;
    static final double HH_OUT_RATE = 44100.0;
    //Raised cosine taper from here to the output Nyquist frequency.
    static final double HH_TAPER_START = 20000.0;

    private static final short[] SINE_QUARTER = {
        0, 201, 402, 603, 804, 1005, 1206, 1407, 1608, 1809, 2009, 2210,
        2410, 2611, 2811, 3012, 3212, 3412, 3612, 3811, 4011, 4210, 4410, 4609,
        4808, 5007, 5205, 5404, 5602, 5800, 5998, 6195, 6393, 6590, 6786, 6983,
        7179, 7375, 7571, 7767, 7962, 8157, 8351, 8545, 8739, 8933, 9126, 9319,
        9512, 9704, 9896, 10087, 10278, 10469, 10659, 10849, 11039, 11228, 11417, 11605,
        11793, 11980, 12167, 12353, 12539, 12725, 12910, 13094, 13279, 13462, 13645, 13828,
        14010, 14191, 14372, 14553, 14732, 14912, 15090, 15269, 15446, 15623, 15800, 15976,
        16151, 16325, 16499, 16673, 16846, 17018, 17189, 17360, 17530, 17700, 17869, 18037,
        18204, 18371, 18537, 18703, 18868, 19032, 19195, 19357, 19519, 19680, 19841, 20000,
        20159, 20317, 20475, 20631, 20787, 20942, 21096, 21250, 21403, 21554, 21705, 21856,
        22005, 22154, 22301, 22448, 22594, 22739, 22884, 23027, 23170, 23311, 23452, 23592,
        23731, 23870, 24007, 24143, 24279, 24413, 24547, 24680, 24811, 24942, 25072, 25201,
        25329, 25456, 25582, 25708, 25832, 25955, 26077, 26198, 26319, 26438, 26556, 26674,
        26790, 26905, 27019, 27133, 27245, 27356, 27466, 27575, 27683, 27790, 27896, 28001,
        28105, 28208, 28310, 28411, 28510, 28609, 28706, 28803, 28898, 28992, 29085, 29177,
        29268, 29358, 29447, 29534, 29621, 29706, 29791, 29874, 29956, 30037, 30117, 30195,
        30273, 30349, 30424, 30498, 30571, 30643, 30714, 30783, 30852, 30919, 30985, 31050,
        31113, 31176, 31237, 31297, 31356, 31414, 31470, 31526, 31580, 31633, 31685, 31736,
        31785, 31833, 31880, 31926, 31971, 32014, 32057, 32098, 32137, 32176, 32213, 32250,
        32285, 32318, 32351, 32382, 32412, 32441, 32469, 32495, 32521, 32545, 32567, 32589,
        32609, 32628, 32646, 32663, 32678, 32692, 32705, 32717, 32728, 32737, 32745, 32752,
        32757, 32761, 32765, 32766, 32767,
    };

    private interface Wave {
        int value(int i);
    }

    private final byte[] ram = new byte[RAM_SIZE];
    private int end;

    /**
     * @param program msx68k.bin.
     * @param main MAIN.COM.
     * @param oranyo ORANYO.BIN.
     * @param sfx FF.BGE.
     * @param songs The .BGM files in SONGS order.
     */
    SoundImage(byte[] program, byte[] main, byte[] oranyo, byte[] sfx, byte[][] songs) throws IOException {
        if(program.length > PROGRAM_END) {
            throw new IOException("msx68k.bin is over 0x" + Integer.toHexString(PROGRAM_END) + " bytes");
        }
        System.arraycopy(program, 0, ram, 0, program.length);
        buildWaves();
        int hhEnd = hhTables();
        if(hhEnd > SONG_BANK) {
            throw new IOException("hi-hat tables run into the song bank");
        }

        if(main.length < MAIN_SONG_TABLE + MAP_COUNT) {
            throw new IOException("MAIN.COM is too short");
        }
        byte[] mapSongs = new byte[MAP_COUNT];
        System.arraycopy(main, MAIN_SONG_TABLE, mapSongs, 0, MAP_COUNT);
        for(byte b : mapSongs) {
            int s = b & 0xFF;
            if(s < SONG_FIRST || s >= SONG_FIRST + SONGS.length) {
                throw new IOException("MAIN.COM map song table entry out of range: 0x" + Integer.toHexString(s));
            }
        }
        for(int i = 0; i < songs.length; i++) {
            if(songs[i].length > SONG_MAX) {
                throw new IOException(SONGS[i] + ".BGM is over 0x" + Integer.toHexString(SONG_MAX) + " bytes");
            }
        }

        byte[][] entries = new byte[3 + songs.length][];
        entries[0] = oranyo;
        entries[1] = sfx;
        entries[2] = mapSongs;
        System.arraycopy(songs, 0, entries, 3, songs.length);
        int at = SONG_BANK;
        ram[at] = 'M';
        ram[at + 1] = 'S';
        ram[at + 2] = 'X';
        ram[at + 3] = 'S';
        put16(at + 4, VERSION);
        put16(at + 6, entries.length);
        int data = at + 8 + entries.length * 8;
        for(int i = 0; i < entries.length; i++) {
            data = (data + 3) & ~3;
            if(data + entries[i].length > PSG_SQUARES) {
                throw new IOException("song bank runs into the PSG squares");
            }
            put32(at + 8 + i * 8, data);
            put32(at + 12 + i * 8, entries[i].length);
            System.arraycopy(entries[i], 0, ram, data, entries[i].length);
            data += entries[i].length;
        }
        end = psgSquares();
    }

    //Square waves without the harmonics that would alias at 44.1 kHz, one per band of PSG periods.
    //Returns the end address.
    private int psgSquares() {
        int n = PSG_SQ_HARMONICS.length;
        put16(PSG_SQUARES, n);
        for(int t = 0; t < n; t++) {
            int h = PSG_SQ_HARMONICS[t];
            put16(PSG_SQUARES + 2 + t * 2, h == 0 ? 0 : (int)Math.floor(PSG_TONE_HZ * h / 22050) + 1);
            loopTable(PSG_SQ_TABLES + t * PSG_SQ_STRIDE, i -> {
                double v = 0;
                for(int k = 1; k <= h; k += 2) {
                    v += Math.sin(2 * Math.PI * k * i / 1024) / k;
                }
                return (int)Math.round(v * 4 / Math.PI * PSG_SQ_LEVEL);
            });
        }
        return PSG_SQ_TABLES + n * PSG_SQ_STRIDE;
    }

    /**
     * @brief Reads msx68k.bin from the jar.
     */
    static byte[] program() throws IOException {
        try(InputStream in = SoundImage.class.getResourceAsStream("/msx68k.bin")) {
            if(in == null) {
                throw new IOException("msx68k.bin isn't in the jar");
            }
            ByteArrayOutputStream out = new ByteArrayOutputStream();
            byte[] buf = new byte[4096];
            int n;
            while((n = in.read(buf)) > 0) {
                out.write(buf, 0, n);
            }
            return out.toByteArray();
        }
    }

    long write(String path) throws IOException {
        try(FileOutputStream f = new FileOutputStream(path)) {
            f.write(ram, 0, end);
        }
        return end;
    }

    private void put16(int addr, int v) {
        ram[addr] = (byte)(v >> 8);
        ram[addr + 1] = (byte)v;
    }

    private void put32(int addr, int v) {
        put16(addr, v >>> 16);
        put16(addr + 2, v);
    }

    //Sine table value for an OPLL 10-bit phase, full scale 32767.
    static int sine(int phase) {
        phase &= 1023;
        if(phase < 256) return SINE_QUARTER[phase];
        if(phase < 512) return SINE_QUARTER[512 - phase];
        if(phase < 768) return -SINE_QUARTER[phase - 512];
        return -SINE_QUARTER[1024 - phase];
    }

    //Loop tables are 1024 samples plus a copy of sample 0.
    private void loopTable(int addr, Wave w) {
        for(int i = 0; i <= 1024; i++) {
            put16(addr + i * 2, w.value(i & 1023));
        }
    }

    //8-bit loop table of one phase bit, so interpolation only blends 0 and 1.
    private void bitTable(int addr, Wave bit, int level) {
        for(int i = 0; i <= 1024; i++) {
            ram[addr + i] = (byte)(bit.value(i & 1023) != 0 ? level : 0);
        }
    }

    //Input step k (and -k) from the middle reads value(k), steps past the last repeat it.
    private void lookup(int center, int stepSize, int steps, Wave w) {
        for(int i = -512; i < 512; i++) {
            int k = (Math.abs(i) + stepSize / 2) / stepSize;
            if(k >= steps) k = steps - 1;
            put16(center + i * 2, w.value(k));
        }
    }

    //The hi-hat output phases are linear in noise and short_noise, so a ramp passes the
    //precomputed short_noise tone straight through. Half scale for headroom.
    private void hhRamp(int center) {
        int top = sine(0xD0), step = top - sine(0x34);
        for(int i = -512; i < 512; i++) {
            int v = (top - step * i / DRUM_STEP) / 2;
            if(v > 32767) v = 32767;
            if(v < -32767) v = -32767;
            put16(center + i * 2, v);
        }
    }

    private void buildWaves() {
        for(int i = 0; i < 9 * FM_CYCLE; i++) {
            int s = sine(i * (1024 / FM_CYCLE));
            put16(WAVE_SINE - 4 * FM_CYCLE * 2 + i * 2, s);
            put16(WAVE_HALFSINE - 4 * FM_CYCLE * 2 + i * 2, s > 0 ? s : 0);
        }
        //emu2413 short_noise = (h2 ^ h7) | (h3 ^ c5) | (c3 ^ c5), h = hi-hat phase, c = cymbal phase.
        bitTable(WAVE_BIT_HH_A, i -> ((i >> 2) ^ (i >> 7)) & 1, BIT_UNIT >> 8);
        bitTable(WAVE_BIT_HH_B, i -> (i >> 3) & 1, BIT_UNIT >> 8);
        bitTable(WAVE_BIT_CYM_A, i -> ((i >> 3) ^ (i >> 5)) & 1, BIT_UNIT >> 8);
        bitTable(WAVE_BIT_CYM_B, i -> (i >> 5) & 1, BIT_UNIT >> 8);
        bitTable(WAVE_BIT_SD, i -> (i >> 8) & 1, (2 * DRUM_UNIT) >> 8);
        //Logic lookup input step = number of set input bits.
        lookup(WAVE_XOR_LOOKUP, BIT_STEP, 3, k -> k == 1 ? BIT_UNIT : 0);
        lookup(WAVE_OR_LOOKUP, BIT_STEP, 3, k -> k != 0 ? BIT_UNIT : 0);
        lookup(WAVE_SN_LOOKUP, BIT_STEP, 3, k -> k != 0 ? 2 * DRUM_UNIT : 0);
        //Input step = noise + 2 * short_noise.
        final int[] hhPhase = {0xD0, 0x34, 0x234, 0x2D0};
        lookup(WAVE_HH_LOOKUP, DRUM_STEP, 4, k -> sine(hhPhase[k]));
        //Input step = noise + 2 * snare phase bit 8.
        final int[] sdPhase = {0x100, 0x000, 0x200, 0x300};
        lookup(WAVE_SD_LOOKUP, DRUM_STEP, 4, k -> sine(sdPhase[k]));
        hhRamp(WAVE_HH_RAMP);

        //AY 17-bit LFSR, one sample per shift.
        int lfsr = 1;
        for(int i = 0; i <= WAVE_NOISE_LEN; i++) {
            ram[WAVE_PSG_NOISE + i] = (byte)((lfsr & 1) != 0 ? 0x6B : 0x95);
            lfsr = (lfsr >> 1) | (((lfsr ^ (lfsr >> 3)) & 1) << 16);
        }
        //emu2413 noise bit as the snare/hi-hat see it (14 of 18 steps into each OPLL sample),
        //resampled to 44.1kHz so it plays at step 1.0 with no interpolation between bits.
        //One OPLL sample per 72 clocks; 3579545 clocks per second.
        lfsr = 1;
        int acc = 72 * 44100, bit = 0;
        for(int i = 0; i <= WAVE_NOISE_LEN; i++) {
            for(; acc >= 72 * 44100; acc -= 72 * 44100) {
                for(int n = 0; n < 18; n++) {
                    if(n == 14) bit = lfsr & 1;
                    if((lfsr & 1) != 0) lfsr ^= 0x800200;
                    lfsr >>>= 1;
                }
            }
            ram[WAVE_OPLL_NOISE + i] = (byte)(bit != 0 ? DRUM_UNIT >> 8 : 0);
            acc += 3579545;
        }
    }

    /**
     * @brief Writes the hi-hat tone tables at HH_TABLES.
     *
     * The OPLL hi-hat's short_noise tone for each HH_PITCHES pitch, as emu2413 computes it at
     * 49716Hz, resampled band-limited to 44100Hz. Point-sampling the phase bits at 44100Hz instead
     * folds their high harmonics to different frequencies than the real chip does. Assumes the
     * cymbal phase is 0 (channel 8 never keyed or pitched), so short_noise = (h2 ^ h7) | h3.
     *
     * Layout (big-endian): count.w, pad.w, then per table key.w (fnum | block << 9), length.w,
     * offset.l from HH_TABLES; each table is length signed 8-bit samples plus a copy of sample 0.
     *
     * @return The end address.
     */
    private int hhTables() {
        //Hi-hat output phases 0xD0/0x234 (noise 0) and 0x34/0x2D0 (noise 1) for short_noise 0/1:
        //output is linear in noise and short_noise with this short_noise / noise weight ratio.
        double ratio = (sineValue(0xD0) - sineValue(0x234)) / (sineValue(0xD0) - sineValue(0x34));
        int count = HH_PITCHES.length;
        int offset = 4 + 8 * count;
        put16(HH_TABLES, count);
        put16(HH_TABLES + 2, 0);
        for(int i = 0; i < count; i++) {
            int fnum = HH_PITCHES[i][0], blk = HH_PITCHES[i][1];
            byte[] t = hhTable(fnum, blk, (DRUM_UNIT >> 8) * ratio);
            int len = t.length - 1;
            put16(HH_TABLES + 4 + i * 8, fnum | (blk << 9));
            put16(HH_TABLES + 6 + i * 8, len);
            put32(HH_TABLES + 8 + i * 8, offset);
            System.arraycopy(t, 0, ram, HH_TABLES + offset, t.length);
            offset += (len + 2) & ~1;
        }
        return HH_TABLES + offset;
    }

    private static double sineValue(int phase) {
        return Math.floor(32767.0 * Math.sin(2 * Math.PI * phase / 1024) + 0.5);
    }

    //One tone table of length + 1 samples, scale 8-bit units per short_noise unit.
    private static byte[] hhTable(int fnum, int blk, double scale) {
        //Hi-hat modulator ML 1: 19-bit phase step fnum << blk. Only phase bits 11-16 matter.
        long inc = (long)fnum << blk;
        int period = 1 << 17;
        //The bit pattern repeats every 2^17 / gcd(inc, 2^17) samples.
        for(long t = inc; period > 1 && (t & 1) == 0; t >>= 1) period >>= 1;
        int len = (int)Math.floor(period * HH_OUT_RATE / HH_NATIVE_RATE + 0.5);
        int top = len / 2;

        double[] x = new double[period];
        double[] re = new double[top + 1];
        double[] im = new double[top + 1];
        double[] cosT = new double[Math.max(period, len)];
        double[] sinT = new double[Math.max(period, len)];
        for(int m = 0; m < period; m++) {
            int h = (int)(((m * inc) & 0x7FFFF) >> 9);
            x[m] = (((h >> 2) ^ (h >> 7)) | (h >> 3)) & 1;
        }
        for(int m = 0; m < period; m++) {
            cosT[m] = Math.cos(2 * Math.PI * m / period);
            sinT[m] = Math.sin(2 * Math.PI * m / period);
        }
        for(int k = 0; k <= top; k++) {
            double r = 0, i = 0;
            int idx = 0;
            for(int m = 0; m < period; m++) {
                r += x[m] * cosT[idx];
                i -= x[m] * sinT[idx];
                idx += k;
                if(idx >= period) idx -= period;
            }
            re[k] = r / period;
            im[k] = i / period;
        }
        //Bin k of the period plays at k * HH_OUT_RATE / len Hz.
        for(int k = 1; k <= top; k++) {
            double f = k * HH_OUT_RATE / len, w = 1;
            if(2 * k == len) w = 0;
            else if(f > HH_TAPER_START) w = 0.5 + 0.5 * Math.cos(Math.PI * (f - HH_TAPER_START) / (HH_OUT_RATE / 2 - HH_TAPER_START));
            re[k] *= 2 * w;
            im[k] *= 2 * w;
        }
        for(int m = 0; m < len; m++) {
            cosT[m] = Math.cos(2 * Math.PI * m / len);
            sinT[m] = Math.sin(2 * Math.PI * m / len);
        }
        byte[] out = new byte[len + 1];
        for(int j = 0; j < len; j++) {
            double v = re[0];
            int idx = 0;
            for(int k = 1; k <= top; k++) {
                idx += j;
                if(idx >= len) idx -= len;
                v += re[k] * cosT[idx] - im[k] * sinT[idx];
            }
            long q = (long)Math.floor(v * scale + 0.5);
            if(q > 127) q = 127;
            if(q < -128) q = -128;
            out[j] = (byte)q;
        }
        out[len] = out[0];
        return out;
    }
}
