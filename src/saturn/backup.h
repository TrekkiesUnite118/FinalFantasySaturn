/*******************************************************************
 *
 *                      Backup Memory
 *
 * A wrapper around SBL's backup library (BUP) for System Memory, a
 * cartridge or the floppy drive.
 *
*******************************************************************/
#ifndef BACKUP_H
#define BACKUP_H

#include "types.h"

//Backup memory units: System Memory, cartridge, floppy.
#define BK_UNITS (3)

//bkStat results.
enum {
    BK_OK,
    BK_UNFORMAT,
    BK_ERROR
};

typedef struct {
    Uint32 blocksize; //Bytes per block.
    Uint32 freeblock;
    Uint32 datanum; //How many files of the asked size still fit.
} BkStat;

/**
 * @brief Turns the hot reset off, so backup memory isn't reset mid-write.
 */
void bkResetDisable(void);

/**
 * @brief Turns the hot reset back on.
 */
void bkResetEnable(void);

/**
 * @brief Starts the backup library and finds the connected units.
 */
void bkInit(void);

/**
 * @brief Checks if a backup unit is connected.
 *
 * @param unit Unit, 0 to BK_UNITS - 1.
 * @return TRUE if present.
 */
Uint8 bkUnitPresent(Uint8 unit);

/**
 * @brief Gets a unit's partition count.
 *
 * @param unit Unit.
 * @return Number of partitions.
 */
Uint8 bkPartitions(Uint8 unit);

/**
 * @brief Selects the partition a unit's reads and writes use.
 *
 * @param unit Unit.
 * @param part Partition.
 */
void bkSelectPartition(Uint8 unit, Uint8 part);

/**
 * @brief Gets a unit's free space.
 *
 * @param unit Unit.
 * @param size File size to count room for.
 * @param stat Set to the unit's block size, free blocks and room. Only set on BK_OK.
 * @return BK_OK, BK_UNFORMAT or BK_ERROR.
 */
Uint8 bkStat(Uint8 unit, Uint32 size, BkStat *stat);

/**
 * @brief Finds a file.
 *
 * @param unit Unit.
 * @param name File name.
 * @return The file's data size, or -1 if it isn't there.
 */
Sint32 bkFind(Uint8 unit, const char *name);

/**
 * @brief Reads a file.
 *
 * @param unit Unit.
 * @param name File name.
 * @param data Set to the file's data.
 * @return TRUE if read.
 */
Uint8 bkRead(Uint8 unit, const char *name, void *data);

/**
 * @brief Writes a file and verifies it.
 *
 * Uses BUP_Write with owsw OFF, then BUP_Verify. The date is taken from the RTC.
 *
 * @param unit Unit.
 * @param name File name, up to 11 characters.
 * @param comment Comment, up to 10 characters.
 * @param data Data to write.
 * @param size Data size in bytes.
 * @return TRUE if written and verified.
 */
Uint8 bkWrite(Uint8 unit, const char *name, const char *comment, void *data, Uint32 size);

#endif
