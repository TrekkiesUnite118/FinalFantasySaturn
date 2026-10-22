/*******************************************************************
 *
 *                      NES Audio
 *
 * Plays NES APU sound on the SCSP using CroNSF's APU emulation.
 *
 * The game's sound driver writes APU registers with nesaudio_write.
 * One APU frame is played per V-blank, on the master SH-2, or on the
 * slave SH-2 while the master is busy.
 *
*******************************************************************/
#ifndef NESAUDIO_H
#define NESAUDIO_H

#include "types.h"

/**
 * @brief Starts the SCSP, loads the APU tables and powers on the APU.
 *
 * Loads SCSPAPU.BIN from the CD, so call it after cd_init.
 *
 * @param buffer Pointer to 0x80000 bytes of scratch RAM for loading the tables.
 * @param init Called once the APU is running. Set up the sound driver here.
 * @param frame Called after each APU frame. Run the sound driver here.
 */
void nesaudio_init(void *buffer, void (*init)(void), void (*frame)(void));

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

/**
 * @brief Runs nesaudio_frame. Call this from the V-blank in interrupt.
 *
 * Does nothing while the slave SH-2 is running the audio frames.
 */
void nesaudio_vblank_in(void);

/**
 * @brief Moves the audio frames to the slave SH-2.
 *
 * Use this around library calls that hold off the master's interrupts.
 * Ex: Backup RAM reads and writes.
 * Don't touch the sound driver's state until nesaudio_slave_end.
 */
void nesaudio_slave_begin(void);

/**
 * @brief Waits for the slave SH-2's last audio frame and gives the audio
 * frames back to the V-blank interrupt.
 */
void nesaudio_slave_end(void);

#endif
