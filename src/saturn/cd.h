/*******************************************************************
 *
 *                      CD Loading
 *
 * Reads files from the CD's root directory with SBL's file system
 * library (GFS).
 *
*******************************************************************/
#ifndef CD_H
#define CD_H

//Low Work RAM base.
#define LWRAM    (0x200000)

/**
 * @brief Starts the file system library and reads the root directory.
 */
void cd_init(void);

/**
 * @brief Reads a file from the CD's root directory.
 *
 * @param filename File name.
 * @param dataBuf Where the data goes.
 * @param read_size Bytes to read.
 */
void cd_load(char *filename, void *dataBuf, int read_size);

/**
 * @brief Reads a whole file from the CD's root directory.
 *
 * @param filename File name.
 * @param dataBuf Where the data goes.
 * @return File size in bytes.
 */
Sint32 cd_load_nosize(char *filename, void *dataBuf);

#endif
