/*******************************************************************
 *
 *                      NES Audio
 *
 * Plays NES APU sound on the SCSP using CroNSF's APU emulation.
 *
 * The game's sound driver writes APU registers with nesaudio_write.
 * audio runs one APU frame per V-blank with nesaudio_frame.
 *
*******************************************************************/
#ifndef NESAUDIO_H
#define NESAUDIO_H

#include "types.h"

/**
 * @brief Sets up the sound driver, then starts the APU (nesaudio_start).
 *
 * @param init Called first, once. Set up the sound driver here.
 * @param frame Called after each APU frame. Run the sound driver here.
 */
void nesaudio_init(void (*init)(void), void (*frame)(void));

/**
 * @brief Resets the SCSP, loads the APU tables and powers on the APU.
 *
 * Loads SCSPAPU.BIN from the CD straight into sound RAM, so call it after cd_init.
 * Sound is silent from the reset until the APU starts. Keeps the sound driver's state.
 */
void nesaudio_start(void);

/**
 * @brief Stops APU frames and writes. The SCSP keeps playing until it's reset.
 */
void nesaudio_stop(void);

/**
 * @brief Writes an APU register.
 *
 * Writes are queued in the current APU frame and played in order.
 * Does nothing before nesaudio_init.
 *
 * @param addr APU register address ($4000-$4017).
 * @param value Value to write.
 */
void nesaudio_write(Uint16 addr, Uint8 value);

/**
 * @brief Ends the current APU frame, then calls the frame callback.
 *
 * The frame is ended at the length the SCSP actually played, so the
 * audio stays in step with the SCSP.
 */
void nesaudio_frame(void);

#endif
