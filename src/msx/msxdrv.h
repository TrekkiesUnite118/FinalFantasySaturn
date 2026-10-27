/*******************************************************************
 *
 *                      MSX Sound Driver
 *
 * C port of the FF1 MSX2 music driver (MAIN.COM 0x752F-0x7EC3).
 * Plays .BGM songs on the PSG and OPLL and FF.BGE effects on PSG
 * channel C, writing registers through msxchip. msxDrvTick runs
 * once per frame; the other calls must not interrupt it.
 * 
 * This was written by Claude Opus 5.5
 *
*******************************************************************/
#ifndef MSXDRV_H
#define MSXDRV_H

#include "types.h"

/**
 * @brief Sets the data tables and resets the driver.
 *
 * @param oranyo ORANYO.BIN (envelopes, sweeps, LFOs, FM patches).
 * @param sfxBank FF.BGE.
 */
void msxDrvInit(const Uint8 *oranyo, const Uint8 *sfxBank);

/**
 * @brief Starts a song (driver call 2). Also stops any effect.
 *
 * @param bgm .BGM file data, kept until the song is stopped.
 */
void msxDrvPlayBgm(const Uint8 *bgm);

/**
 * @brief Stops the song and effect (driver call 3).
 */
void msxDrvStopBgm(void);

/**
 * @brief Fades the song out (driver call 4).
 *
 * @param speed Frames per volume step.
 */
void msxDrvFade(Uint8 speed);

/**
 * @brief Gives whether music is playing (driver call 5).
 */
Uint8 msxDrvBgmBusy(void);

/**
 * @brief Starts an effect (driver call 6).
 *
 * @param id Effect 1 to the bank's count.
 * @return 0, or 0xFF if the id is out of range.
 */
Uint8 msxDrvPlaySfx(Uint8 id);

/**
 * @brief Stops the effect. Not in the original driver.
 */
void msxDrvStopSfx(void);

/**
 * @brief Gives whether an effect is playing (driver call 7).
 */
Uint8 msxDrvSfxBusy(void);

/**
 * @brief Gives the number of effects in the bank.
 */
Uint8 msxDrvSfxCount(void);

/**
 * @brief Runs one frame of the driver (MAIN.COM 0x75D4).
 */
void msxDrvTick(void);

#endif
