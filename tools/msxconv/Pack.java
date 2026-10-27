import java.io.ByteArrayOutputStream;
import java.io.FileOutputStream;
import java.io.IOException;

/**
 * Writes the packed MSX graphics file.
 *
 * Layout, all values big-endian:
 *   char[4] "MSXG", Uint16 version, Uint16 entry count
 *   entries, 16 bytes each: Uint16 id, Uint8 kind, Uint8 0, Uint16 width, Uint16 height,
 *                           Uint32 offset from file start, Uint32 size
 *   data, each entry 4-byte aligned
 *
 * An entry's id is its index, so the runtime can index the table directly.
 * 
 * This was written by Claude Opus 5.5
 */
final class Pack {
    static final int VERSION = 1;

    static final int KIND_TILES = 0; //8x8 4bpp cells, row-major. Width and height in pixels.
    static final int KIND_BITMAP = 1; //Linear 4bpp, width / 2 bytes per row.
    static final int KIND_PALETTE = 2; //Uint32 0x00BBGGRR colors. Width is the color count.
    static final int KIND_RAW = 3;

    private final int[] kinds;
    private final int[] widths;
    private final int[] heights;
    private final byte[][] data;

    Pack(int count) {
        kinds = new int[count];
        widths = new int[count];
        heights = new int[count];
        data = new byte[count][];
    }

    void put(int id, int kind, int width, int height, byte[] bytes) {
        if(data[id] != null) {
            throw new IllegalStateException("entry " + id + " set twice");
        }
        kinds[id] = kind;
        widths[id] = width;
        heights[id] = height;
        data[id] = bytes;
    }

    void putPalette(int id, int[] colors) {
        byte[] out = new byte[colors.length * 4];
        for(int i = 0; i < colors.length; i++) {
            out[i * 4 + 1] = (byte)(colors[i] >> 16);
            out[i * 4 + 2] = (byte)(colors[i] >> 8);
            out[i * 4 + 3] = (byte)colors[i];
        }
        put(id, KIND_PALETTE, colors.length, 1, out);
    }

    long write(String path) throws IOException {
        int count = data.length;
        for(int i = 0; i < count; i++) {
            if(data[i] == null) {
                throw new IllegalStateException("entry " + i + " missing");
            }
        }
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        out.write('M');
        out.write('S');
        out.write('X');
        out.write('G');
        u16(out, VERSION);
        u16(out, count);

        int offset = 8 + count * 16;
        int[] offsets = new int[count];
        for(int i = 0; i < count; i++) {
            offset = (offset + 3) & ~3;
            offsets[i] = offset;
            offset += data[i].length;
        }
        for(int i = 0; i < count; i++) {
            u16(out, i);
            out.write(kinds[i]);
            out.write(0);
            u16(out, widths[i]);
            u16(out, heights[i]);
            u32(out, offsets[i]);
            u32(out, data[i].length);
        }
        for(int i = 0; i < count; i++) {
            while(out.size() < offsets[i]) {
                out.write(0);
            }
            out.write(data[i]);
        }
        try(FileOutputStream f = new FileOutputStream(path)) {
            out.writeTo(f);
        }
        return out.size();
    }

    private static void u16(ByteArrayOutputStream out, int v) {
        out.write(v >> 8);
        out.write(v);
    }

    private static void u32(ByteArrayOutputStream out, int v) {
        out.write(v >> 24);
        out.write(v >> 16);
        out.write(v >> 8);
        out.write(v);
    }
}
