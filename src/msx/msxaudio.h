/*******************************************************************
 *
 *                      MSX Audio
 *
 * Plays the MSX2 music and sound effects with the MSX driver port
 * (msxdrv) on the emulated PSG and OPLL (msxchip).
 *
 * MSXSND.BIN (made by tools/msxconv) is a sound RAM image: the 68K
 * chip emulation, its waveform tables and the song bank. Songs are
 * copied out of sound RAM when they start.
 * 
 * This was written by Claude Opus 5.5
 *
*******************************************************************/
#ifndef MSXAUDIO_H
#define MSXAUDIO_H

#include "types.h"

//MSX directory numbers of the songs.
#define MSX_SONG_FIRST (0x3C)
#define MSX_SONG_COUNT (21)

/**
 * @brief Checks MSXSND.BIN is on the CD without loading it.
 *
 * @return TRUE if it's there and fits in sound RAM.
 */
Uint8 msxaudio_probe(void);

/**
 * @brief Resets the SCSP, loads MSXSND.BIN into sound RAM and starts the driver.
 *
 * Sound is silent from the reset until the driver starts.
 *
 * @param frame Called before each driver frame. Make driver calls here.
 * @return TRUE if the file loaded and checked out. FALSE leaves the SCSP silent.
 */
Uint8 msxaudio_start(void (*frame)(void));

/**
 * @brief Stops running frames. The SCSP keeps playing until it's reset.
 */
void msxaudio_stop(void);

/**
 * @brief Runs the frame callback, then one driver frame. Call once per V-blank.
 *
 * Does nothing unless started.
 */
void msxaudio_frame(void);

/**
 * @brief Copies a song out of sound RAM into the song buffer.
 *
 * The buffer holds one song, so this replaces the one playing.
 *
 * @param song MSX directory number (MSX_SONG_FIRST onward).
 * @return The song, or NULL if out of range.
 */
const Uint8 *msxaudio_song(Uint8 song);

/**
 * @brief Gets the song the MSX game plays on a standard map.
 *
 * @param map Standard map number.
 * @return MSX directory number.
 */
Uint8 msxaudio_map_song(Uint8 map);

#endif
