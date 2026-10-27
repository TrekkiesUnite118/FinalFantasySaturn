import java.io.IOException;

/**
 * A decoded MSX2 PIC image: 4bpp, left pixel in the high nibble, with a 16 colour palette.
 * 
 * This was written by Claude Opus 5.5
 */
final class Pic {
    static final int HEADER_SIZE = 0x30;

    final String name;
    final int width;
    final int height;
    final byte[] pixels; //width / 2 bytes per row.
    final int[] palette; //16 colors, 0x00BBGGRR.
    final int compressedEnd; //Offset just past the compressed data.

    private Pic(String name, int width, int height, byte[] pixels, int[] palette, int compressedEnd) {
        this.name = name;
        this.width = width;
        this.height = height;
        this.pixels = pixels;
        this.palette = palette;
        this.compressedEnd = compressedEnd;
    }

    //MSX2 3-bit levels as an MSX2 emulator shows them.
    private static final int[] LEVELS = {0, 39, 82, 115, 155, 186, 224, 255};

    /**
     * Converts an MSX2 palette entry (0RRR0BBB 00000GGG) to 0x00BBGGRR.
     */
    static int color(int b0, int b1) {
        int r = LEVELS[(b0 >> 4) & 7];
        int b = LEVELS[b0 & 7];
        int g = LEVELS[b1 & 7];
        return (b << 16) | (g << 8) | r;
    }

    /**
     * Decodes a PIC starting at start in data.
     *
     * Header byte 6 is the width in bytes, byte 8 the rows, 0x10-0x2F the palette.
     * RLE: a byte is written; if the next byte is equal, the byte after it is the run's total count.
     * Runs never cross rows.
     */
    static Pic decode(String name, byte[] data, int start) throws IOException {
        if(start + HEADER_SIZE > data.length) {
            throw new IOException(name + ": truncated header");
        }
        int rowBytes = data[start + 6] & 0xFF;
        int rows = data[start + 8] & 0xFF;
        if(rowBytes == 0 || rowBytes > 128 || rows == 0 || rows > 212) {
            throw new IOException(name + ": bad header (" + rowBytes * 2 + "x" + rows + ")");
        }

        int[] palette = new int[16];
        for(int i = 0; i < 16; i++) {
            palette[i] = color(data[start + 0x10 + i * 2] & 0xFF, data[start + 0x11 + i * 2] & 0xFF);
        }

        byte[] out = new byte[rowBytes * rows];
        int pos = start + HEADER_SIZE;
        try {
            int n = 0;
            for(int row = 0; row < rows; row++) {
                int left = rowBytes;
                int a = data[pos++] & 0xFF;
                while(true) {
                    out[n++] = (byte)a;
                    left--;
                    if(left == 0) {
                        break;
                    }
                    int b = data[pos++] & 0xFF;
                    if(b != a) {
                        a = b;
                        continue;
                    }
                    int count = (data[pos++] & 0xFF) - 1;
                    if(count < 0 || count > left) {
                        throw new IOException(name + ": bad run length");
                    }
                    for(int i = 0; i < count; i++) {
                        out[n++] = (byte)a;
                    }
                    left -= count;
                    if(left == 0) {
                        break;
                    }
                    a = data[pos++] & 0xFF;
                }
            }
        } catch(ArrayIndexOutOfBoundsException e) {
            throw new IOException(name + ": compressed data runs past the end");
        }
        return new Pic(name, rowBytes * 2, rows, out, palette, pos);
    }

    int pixel(int x, int y) {
        int v = pixels[y * (width / 2) + (x / 2)] & 0xFF;
        return (x & 1) == 0 ? v >> 4 : v & 0x0F;
    }

    /**
     * Cuts a region into 8x8 Saturn 4bpp cells (32 bytes each), row-major.
     */
    byte[] tiles(int x0, int y0, int tilesWide, int tilesHigh) {
        byte[] out = new byte[tilesWide * tilesHigh * 32];
        int n = 0;
        for(int ty = 0; ty < tilesHigh; ty++) {
            for(int tx = 0; tx < tilesWide; tx++) {
                for(int y = 0; y < 8; y++) {
                    for(int x = 0; x < 8; x += 2) {
                        int px = x0 + tx * 8 + x;
                        int py = y0 + ty * 8 + y;
                        out[n++] = (byte)((pixel(px, py) << 4) | pixel(px + 1, py));
                    }
                }
            }
        }
        return out;
    }

    /**
     * Cuts a region into a linear 4bpp bitmap, width / 2 bytes per row.
     */
    byte[] bitmap(int x0, int y0, int w, int h) {
        byte[] out = new byte[(w / 2) * h];
        int n = 0;
        for(int y = 0; y < h; y++) {
            for(int x = 0; x < w; x += 2) {
                out[n++] = (byte)((pixel(x0 + x, y0 + y) << 4) | pixel(x0 + x + 1, y0 + y));
            }
        }
        return out;
    }
}
