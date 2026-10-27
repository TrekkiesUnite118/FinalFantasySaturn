/*******************************************************************
 *
 *                      MSX Sound Chips
 *
 * The MSX2 PSG (AY-3-8910) and MSX-MUSIC (YM2413/OPLL) emulated
 * with SCSP slots by the sound 68K (msx68k.s, in MSXSND.BIN).
 * Register writes are queued for the 68K, which applies them once
 * per msxChipFrame.
 * 
 * This was written by Claude Opus 5.5
 *
*******************************************************************/
#ifndef MSXCHIP_H
#define MSXCHIP_H

#include "types.h"

/**
 * @brief Resets the register shadows and the mailbox. Call with MSXSND.BIN in sound RAM, before starting the 68K.
 */
void msxChipInit(void);

/**
 * @brief Writes a PSG register.
 *
 * @param reg Register 0-15.
 * @param value Value.
 */
void msxPsgWrite(Uint8 reg, Uint8 value);

/**
 * @brief Reads a PSG register.
 *
 * @param reg Register 0-15.
 * @return Last value written.
 */
Uint8 msxPsgRead(Uint8 reg);

/**
 * @brief Writes an OPLL register.
 *
 * @param reg Register 0x00-0x38.
 * @param value Value.
 */
void msxOpllWrite(Uint8 reg, Uint8 value);

/**
 * @brief Has the 68K apply the frame's register writes to the SCSP.
 */
void msxChipFrame(void);

#endif
