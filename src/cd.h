#ifndef CD_H
#define CD_H

#define LWRAM    (0x200000)
#define LWRAM_ROM_END    (0x260000)
#define LWRAM_MAP_SPRITES    (0x262A40)
#define LWRAM_BATTLE_SPRITES    (0x268A40)
#define LWRAM_WEAPON_SPRITES    (0x26C040)
#define LWRAM_MENU_TILES    (0x26D040)
#define LWRAM_BUFFER    (0x26F040)
//init cd stuff
void cd_init(void);

/**
 * read data off the cd's root directory
 * filename: duh
 * dataBuf: where you want to copy the data to
 * read_size: # of bytes to read 
 */
void cd_load(char *filename, void *dataBuf, int read_size);

Sint32 cd_load_nosize(char *filename, void *dataBuf);

#endif
