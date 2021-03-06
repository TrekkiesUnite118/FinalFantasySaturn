#ifndef CD_H
#define CD_H

#define LWRAM    (2097152)
#define LWRAM_BUFFER    (2490368)
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
