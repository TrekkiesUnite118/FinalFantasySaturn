/*******************************************************************
 *
 *                      CD Audio And PCM Sound
 *
 * CD audio tracks through SCSP slots 16 and 17, and PCM sounds
 * through pcmsys. The game only uses the CD audio, for the SAD
 * screen's music. The PCM sounds are unused.
 *
*******************************************************************/
#ifndef SOUND_H
#define SOUND_H

//PCM sound numbers for sound_play. Unused.
#define SOUND_EXPLOSION (0)
#define SOUND_JUMP (1)
#define SOUND_DASH (2)

/**
 * @brief Turns on CD audio output at volume 5.
 *
 * Call after cd_init and nesaudio_init. The NES audio driver's SCSP
 * reset clears these slot settings.
 */
void sound_init(void);

/**
 * @brief Plays a CD audio track, repeating forever.
 *
 * @param track Track number.
 */
void sound_cdda(int track);

/**
 * @brief Plays a PCM sound. Does nothing while the pcm_play call is commented out.
 *
 * @param num Sound number.
 */
void sound_play(short num);
#endif
