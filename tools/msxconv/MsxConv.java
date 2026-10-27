import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.util.TreeMap;

/**
 * Converts the MSX2 Final Fantasy graphics into MSXGFX.BIN for the Saturn port, and optionally
 * the music into the sound RAM image MSXSND.BIN (see SoundImage).
 *
 * Input is the output of the SAY-DOS extractor: the files folder (named files) and the
 * sectors folder (one 512-byte file per disk sector). The enemy graphics aren't SAY-DOS
 * files, so they're read from the rebuilt disk image.
 *
 * Entry ids (see Pack for the file layout):
 *   0-8      map tiles: MAINBG (256 tiles), then SUB0BG-SUB7BG (128 each), NES CHR order
 *   9-17     their palettes
 *   18       MAIN.COM map -> color set table (61 bytes)
 *   19       MAIN.COM color sets: 21 sets of palette entries 8-15
 *   20-143   enemy tiles, ENEMY_000-123 (Chaos cropped to 112x96)
 *   144-267  enemy palettes
 *   268-351  party battle frames, 7 per class in NES class order (base classes, then promoted):
 *            stand, walk, swing, cheer, crouch (16x24), dead (24x16), stone (16x24)
 *   352      party battle palette (BAT_L1)
 *   353      orbs, 5 of 16x16 as 10x2 tiles: 4 lit, then unlit
 *   354      menu palette (MENU_L1)
 *   355-370  battle backdrops, 4x3 tiles each, NES order
 *   371      shop tiles: shopType * 16 + k + 2 for NES shopkeeper image tile k (k = 0 unused)
 *   372      shop palette
 *   373      minimap dragon, 6x8 tiles in NES decor order
 *   374      minimap title, 14x5 tiles
 *   375      minimap palette
 *   376-377  bridge scene bitmap (256x212) and palette
 *   378-379  ending bitmap (256x212) and palette
 *   380-381  "The End" bitmap (128x80) and palette
 *   382      map sprite palette (JOBS_L1)
 *   383      prelude text palette (OPMSG)
 * 
 * This was written by Claude Opus 5.5
 */
public final class MsxConv {
    static final int SECTOR_SIZE = 512;
    static final int ENEMY_SECTOR = 0x44C;
    static final int ENEMY_COUNT = 124;
    static final int CHAOS = 123;
    static final int CLASS_COUNT = 12;
    static final int FRAMES_PER_CLASS = 7;

    static final int ID_MAP_TILES = 0;
    static final int ID_MAP_PALS = 9;
    static final int ID_MAP_COLOR_TABLE = 18;
    static final int ID_MAP_COLOR_SETS = 19;
    static final int ID_ENEMY_TILES = 20;
    static final int ID_ENEMY_PALS = ID_ENEMY_TILES + ENEMY_COUNT;
    static final int ID_BATTLE_FRAMES = ID_ENEMY_PALS + ENEMY_COUNT;
    static final int ID_BATTLE_PAL = ID_BATTLE_FRAMES + CLASS_COUNT * FRAMES_PER_CLASS;
    static final int ID_ORBS = ID_BATTLE_PAL + 1;
    static final int ID_MENU_PAL = ID_ORBS + 1;
    static final int ID_BACKDROPS = ID_MENU_PAL + 1;
    static final int ID_SHOP_TILES = ID_BACKDROPS + 16;
    static final int ID_SHOP_PAL = ID_SHOP_TILES + 1;
    static final int ID_MINIMAP_DRAGON = ID_SHOP_PAL + 1;
    static final int ID_MINIMAP_TITLE = ID_MINIMAP_DRAGON + 1;
    static final int ID_MINIMAP_PAL = ID_MINIMAP_TITLE + 1;
    static final int ID_BRIDGE = ID_MINIMAP_PAL + 1;
    static final int ID_ENDING = ID_BRIDGE + 2;
    static final int ID_THE_END = ID_ENDING + 2;
    static final int ID_MAP_SPRITE_PAL = ID_THE_END + 2;
    static final int ID_PRELUDE_PAL = ID_MAP_SPRITE_PAL + 1;
    static final int ID_COUNT = ID_PRELUDE_PAL + 1;

    static final int MAIN_COLOR_TABLE = 0x1143;
    static final int MAIN_COLOR_TABLE_SIZE = 61;
    static final int MAIN_COLOR_SETS = 0x1182;
    static final int MAIN_COLOR_SET_COUNT = 21;

    private final File filesDir;

    private MsxConv(File filesDir) {
        this.filesDir = filesDir;
    }

    public static void main(String[] args) {
        if(args.length != 3 && args.length != 4) {
            System.err.println("Usage: java -jar msxconv.jar <SAY-DOS files folder> <SAY-DOS sectors folder> <output MSXGFX.BIN> [<output MSXSND.BIN>]");
            System.exit(2);
        }
        try {
            MsxConv conv = new MsxConv(new File(args[0]));
            long size = conv.run(new File(args[1]), args[2]);
            System.out.println("Wrote " + args[2] + ": " + size + " bytes (0x" + Long.toHexString(size).toUpperCase() + ")");
            if(args.length == 4) {
                size = conv.sound().write(args[3]);
                System.out.println("Wrote " + args[3] + ": " + size + " bytes (0x" + Long.toHexString(size).toUpperCase() + ")");
            }
        } catch(IOException e) {
            System.err.println("Error: " + e.getMessage());
            System.exit(1);
        }
    }

    private long run(File sectorsDir, String outPath) throws IOException {
        Pack pack = new Pack(ID_COUNT);

        String[] maps = {"MAINBG", "SUB0BG", "SUB1BG", "SUB2BG", "SUB3BG", "SUB4BG", "SUB5BG", "SUB6BG", "SUB7BG"};
        for(int i = 0; i < maps.length; i++) {
            Pic pic = pic(maps[i] + ".PIC", 256, -1);
            int rows = pic.height / 8;
            pack.put(ID_MAP_TILES + i, Pack.KIND_TILES, 256, rows * 8, pic.tiles(0, 0, 32, rows));
            pack.putPalette(ID_MAP_PALS + i, pic.palette);
        }

        byte[] main = file("MAIN.COM");
        if(main.length < MAIN_COLOR_SETS + MAIN_COLOR_SET_COUNT * 16) {
            throw new IOException("MAIN.COM is too short");
        }
        byte[] table = new byte[MAIN_COLOR_TABLE_SIZE];
        System.arraycopy(main, MAIN_COLOR_TABLE, table, 0, table.length);
        for(byte b : table) {
            if((b & 0xFF) >= MAIN_COLOR_SET_COUNT) {
                throw new IOException("MAIN.COM color table entry out of range: " + (b & 0xFF));
            }
        }
        pack.put(ID_MAP_COLOR_TABLE, Pack.KIND_RAW, table.length, 1, table);
        int[] sets = new int[MAIN_COLOR_SET_COUNT * 8];
        for(int i = 0; i < sets.length; i++) {
            sets[i] = Pic.color(main[MAIN_COLOR_SETS + i * 2] & 0xFF, main[MAIN_COLOR_SETS + i * 2 + 1] & 0xFF);
        }
        pack.putPalette(ID_MAP_COLOR_SETS, sets);

        Pic[] enemies = enemies(disk(sectorsDir));
        for(int i = 0; i < ENEMY_COUNT; i++) {
            Pic e = enemies[i];
            int w = e.width;
            int h = i == CHAOS ? 96 : e.height;
            if(w % 8 != 0 || h % 8 != 0 || h > e.height) {
                throw new IOException(e.name + ": unexpected size " + e.width + "x" + e.height);
            }
            pack.put(ID_ENEMY_TILES + i, Pack.KIND_TILES, w, h, e.tiles(0, 0, w / 8, h / 8));
            pack.putPalette(ID_ENEMY_PALS + i, e.palette);
        }

        String[] bats = {"BAT_L1.PIC", "BAT_L2.PIC"};
        for(int l = 0; l < 2; l++) {
            Pic bat = pic(bats[l], 256, 72);
            for(int c = 0; c < 6; c++) {
                int x = (c % 2) * 120;
                int y = (c / 2) * 24;
                int id = ID_BATTLE_FRAMES + (l * 6 + c) * FRAMES_PER_CLASS;
                for(int f = 0; f < 5; f++) {
                    pack.put(id + f, Pack.KIND_BITMAP, 16, 24, bat.bitmap(x + f * 16, y, 16, 24));
                }
                pack.put(id + 5, Pack.KIND_BITMAP, 24, 16, bat.bitmap(x + 80, y + 8, 24, 16));
                pack.put(id + 6, Pack.KIND_BITMAP, 16, 24, bat.bitmap(x + 104, y, 16, 24));
            }
            if(l == 0) {
                pack.putPalette(ID_BATTLE_PAL, bat.palette);
            }
        }

        Pic menu = pic("MENU_L1.PIC", 176, 72);
        pack.put(ID_ORBS, Pack.KIND_TILES, 80, 16, menu.tiles(96, 0, 10, 2));
        pack.putPalette(ID_MENU_PAL, menu.palette);

        Pic land = pic("LAND.PIC", 256, 48);
        for(int i = 0; i < 16; i++) {
            pack.put(ID_BACKDROPS + i, Pack.KIND_TILES, 32, 24, land.tiles((i % 8) * 32, (i / 8) * 24, 4, 3));
        }

        Pic shop = pic("SHOPBG.PIC", 256, 32);
        pack.put(ID_SHOP_TILES, Pack.KIND_TILES, 256, 32, shop.tiles(0, 0, 32, 4));
        pack.putPalette(ID_SHOP_PAL, shop.palette);

        Pic mapdsp = pic("MAPDSP.PIC", 192, 64);
        pack.put(ID_MINIMAP_DRAGON, Pack.KIND_TILES, 48, 64, mapdsp.tiles(0, 0, 6, 8));
        pack.put(ID_MINIMAP_TITLE, Pack.KIND_TILES, 112, 40, mapdsp.tiles(80, 0, 14, 5));
        pack.putPalette(ID_MINIMAP_PAL, mapdsp.palette);

        bitmap(pack, ID_BRIDGE, pic("OPENGRP.PIC", 256, 212));
        bitmap(pack, ID_ENDING, pic("ENDGRP.PIC", 256, 212));
        bitmap(pack, ID_THE_END, pic("ELOGO.PIC", 128, 80));

        pack.putPalette(ID_MAP_SPRITE_PAL, pic("JOBS_L1.PIC", 256, 48).palette);
        pack.putPalette(ID_PRELUDE_PAL, pic("OPMSG.PIC", 240, 68).palette);

        return pack.write(outPath);
    }

    private SoundImage sound() throws IOException {
        byte[][] songs = new byte[SoundImage.SONGS.length][];
        for(int i = 0; i < songs.length; i++) {
            songs[i] = file(SoundImage.SONGS[i] + ".BGM");
        }
        return new SoundImage(SoundImage.program(), file("MAIN.COM"), file("ORANYO.BIN"), file("FF.BGE"), songs);
    }

    private static void bitmap(Pack pack, int id, Pic pic) {
        pack.put(id, Pack.KIND_BITMAP, pic.width, pic.height, pic.bitmap(0, 0, pic.width, pic.height));
        pack.putPalette(id + 1, pic.palette);
    }

    private byte[] file(String name) throws IOException {
        File f = new File(filesDir, name);
        if(!f.isFile()) {
            throw new IOException("missing " + f.getPath());
        }
        return Files.readAllBytes(f.toPath());
    }

    //Height -1 accepts any height that's a multiple of 8.
    private Pic pic(String name, int width, int height) throws IOException {
        Pic pic = Pic.decode(name, file(name), 0);
        boolean heightOk = height < 0 ? pic.height % 8 == 0 : pic.height == height;
        if(pic.width != width || !heightOk) {
            throw new IOException(name + ": unexpected size " + pic.width + "x" + pic.height);
        }
        return pic;
    }

    //Concatenates the sector files (named by decimal sector number) into a disk image.
    private static byte[] disk(File sectorsDir) throws IOException {
        File[] list = sectorsDir.listFiles();
        if(list == null) {
            throw new IOException("can't read " + sectorsDir.getPath());
        }
        TreeMap<Integer, File> sectors = new TreeMap<>();
        for(File f : list) {
            String n = f.getName();
            if(n.matches("\\d+\\.bin")) {
                sectors.put(Integer.parseInt(n.substring(0, n.length() - 4)), f);
            }
        }
        if(sectors.isEmpty() || sectors.firstKey() != 0 || sectors.lastKey() != sectors.size() - 1) {
            throw new IOException(sectorsDir.getPath() + ": expected sector files 0000.bin onward with none missing");
        }
        byte[] disk = new byte[sectors.size() * SECTOR_SIZE];
        for(int i = 0; i < sectors.size(); i++) {
            byte[] s = Files.readAllBytes(sectors.get(i).toPath());
            if(s.length != SECTOR_SIZE) {
                throw new IOException(sectors.get(i).getPath() + ": not " + SECTOR_SIZE + " bytes");
            }
            System.arraycopy(s, 0, disk, i * SECTOR_SIZE, SECTOR_SIZE);
        }
        return disk;
    }

    //The enemy PICs run from sector 0x44C, each starting on the first sector boundary after the previous one.
    private static Pic[] enemies(byte[] disk) throws IOException {
        Pic[] out = new Pic[ENEMY_COUNT];
        int pos = ENEMY_SECTOR * SECTOR_SIZE;
        for(int i = 0; i < ENEMY_COUNT; i++) {
            if(pos >= disk.length) {
                throw new IOException("disk ends before enemy " + i);
            }
            out[i] = Pic.decode(String.format("ENEMY_%03d (sector 0x%X)", i, pos / SECTOR_SIZE), disk, pos);
            pos = (out[i].compressedEnd + SECTOR_SIZE - 1) / SECTOR_SIZE * SECTOR_SIZE;
        }
        return out;
    }
}
