/*******************************************************************
 *
 *                      Save And Load
 *
 * Saving and loading games in Saturn backup memory: system memory,
 * a cartridge or the floppy drive, 3 save slots each.
 *
 * Also the boot messages for unformatted or full backup memory.
 *
*******************************************************************/
#ifndef SAVE_H
#define SAVE_H

#include "types.h"

//Modes for saveOpen.
enum {
    SV_MODE_SAVE,
    SV_MODE_LOAD
};

Uint8 saveBusy; //Set while backup memory is being written, so soft reset is held off.

/**
 * @brief Starts the backup memory library.
 */
void saveInit(void);

/**
 * @brief Checks backup memory at boot and queues any boot messages.
 */
void saveBootCheck(void);

/**
 * @brief Checks if a boot message is still to be shown.
 *
 * @return TRUE if one is waiting.
 */
Uint8 saveBootMessagePending(void);

/**
 * @brief Draws the next boot message.
 */
void setupBootMessage(void);

/**
 * @brief Runs one frame of the boot message. Any button goes to the next
 * message, or to the prelude after the last.
 */
void doBootMessage(void);

/**
 * @brief Checks if the main menu can offer saving.
 *
 * @return TRUE on the overworld and on maps using tileset 0 (towns).
 */
Uint8 saveAllowed(void);

/**
 * @brief Picks whether the save screen saves or loads. Call before setupSaveScreen.
 *
 * @param mode SV_MODE_SAVE or SV_MODE_LOAD.
 */
void saveOpen(Uint8 mode);

/**
 * @brief Draws the save screen's boxes and sets up the slot sprites.
 */
void setupSaveScreen(void);

/**
 * @brief Runs one frame of the save screen.
 *
 * B leaves: to the main menu when saving, to the title when loading.
 */
void doSaveMenu(void);

/**
 * @brief Draws the save screen's sprites and does any waiting backup
 * memory read or write.
 *
 * Saves and loads only happen from here, in the main loop.
 */
void save_draw(void);

#endif
