/*******************************************************************
 *
 *                      Audio
 *
 * Runs one audio frame per V-blank on whichever sound driver is
 * live: NES (nesaudio) or MSX (msxaudio). The frame runs on the
 * master SH-2, or on the slave SH-2 while the master is busy.
 *
 * Switching drivers resets the SCSP and loads the new driver's sound
 * RAM image from the CD, so sound is silent while it loads.
 *
*******************************************************************/
#ifndef AUDIO_H
#define AUDIO_H

#include "types.h"

enum {
    AUDIO_NES,
    AUDIO_MSX,
    //No frames run, while switching.
    AUDIO_OFF
};

/**
 * @brief Starts the NES driver and checks whether MSX audio is on the CD. Call after cd_init.
 *
 * @param init nesaudio_init's init callback.
 * @param frame The frame callback for either driver. Make sound driver calls here.
 */
void audio_init(void (*init)(void), void (*frame)(void));

/**
 * @brief Gives whether MSXSND.BIN is on the CD.
 */
Uint8 audio_msx_available(void);

/**
 * @brief Gives the live driver (AUDIO_).
 */
Uint8 audio_mode(void);

/**
 * @brief Switches drivers, loading the new one from the CD. Call from the main loop.
 *
 * Falls back to the NES driver if MSXSND.BIN is missing or bad.
 *
 * @param mode AUDIO_NES or AUDIO_MSX.
 * @return The live driver afterwards.
 */
Uint8 audio_set_mode(Uint8 mode);

/**
 * @brief Runs one audio frame. Call this from the V-blank in interrupt.
 *
 * Does nothing while the slave SH-2 is running the audio frames.
 */
void audio_vblank_in(void);

/**
 * @brief Moves the audio frames to the slave SH-2.
 *
 * Use this around library calls that hold off the master's interrupts.
 * Ex: Backup RAM reads and writes.
 * Don't touch the sound driver's state until audio_slave_end.
 */
void audio_slave_begin(void);

/**
 * @brief Waits for the slave SH-2's last audio frame and gives the audio
 * frames back to the V-blank interrupt.
 */
void audio_slave_end(void);

#endif
