/*******************************************************************
 *
 *                      System
 *
 * V-blank handlers, the per-frame VDP1 command list, the SMPC clock
 * and returning to the BIOS.
 *
*******************************************************************/
#ifndef SAT_SYS_H
#define SAT_SYS_H

#include "types.h"

/**
 * @brief Turns the display off (TVMD DISP) until VDP2 setup turns it back on.
 */
void sysDisplayOff(void);

/**
 * @brief Sets the V-blank handlers and turns the V-blank interrupts on.
 *
 * @param in Runs at V-blank in, before SBL's V-blank in work.
 * @param outFirst Runs at V-blank out, before SBL's V-blank out work.
 * @param out Runs at V-blank out, after SBL's V-blank out work.
 */
void sysSetVblank(void (*in)(void), void (*outFirst)(void), void (*out)(void));

/**
 * @brief Opens the frame's VDP1 command list and sets the 320x240 system clip.
 */
void frameBegin(void);

/**
 * @brief Closes the frame's VDP1 command list and waits for the frame to be shown.
 */
void frameEnd(void);

/**
 * @brief Starts SMPC time reads.
 */
void rtcInit(void);

/**
 * @brief Gets the SMPC clock. Call rtcInit first.
 *
 * @return BCD time: [0] seconds, [1] minutes, [2] hours, [3] day,
 *         [4] weekday (high nibble) and month (low nibble), [5] the year's
 *         last two digits, [6] its first two.
 */
const Uint8 *rtcTime(void);

/**
 * @brief Returns to the BIOS.
 */
void sysReset(void);

#endif
