/*******************************************************************
 *
 *                      FF1 Music And Sound Effects
 *
 * The NES sound driver, run once per audio frame by audio.
 * Start a song by writing its track number to music_track.
 *
 * Game code writes APU registers through musicApuWrite, which queues
 * them for the next audio frame.
 *
 * With the MSX driver, tracks play the MSX game's songs and the sfx
 * functions its effects. APU writes do nothing.
 *
*******************************************************************/
#ifndef MUSIC_H
#define MUSIC_H

#include "types.h"

//Track numbers for music_track.
#define MUSIC_PRELUDE (0x41)
#define MUSIC_BRIDGE (0x42)
#define MUSIC_ENDING (0x43)
#define MUSIC_BATTLE (0x50)
#define MUSIC_MENU (0x51)
#define MUSIC_GAME_OVER (0x52)
#define MUSIC_VICTORY (0x53)
#define MUSIC_FANFARE (0x54)
#define MUSIC_CRYSTAL (0x55)
#define MUSIC_SAVE (0x56)
#define MUSIC_HEAL (0x57)
#define MUSIC_TREASURE (0x58)
#define MUSIC_SHOP (0x4F)
#define MUSIC_OVERWORLD (0x44)
#define MUSIC_SHIP (0x45)
#define MUSIC_AIRSHIP (0x46)
//Bit 7 set: the song ended. $81 once the channels are silenced.
#define MUSIC_ENDED (0x80)
#define MUSIC_SILENCED (0x81)

//NES music_track. Writing a track number with bit 6 set starts that song.
volatile Uint8 music_track;
//NES sq2_sfx: frames left on a sound effect that has square 2.
volatile Uint8 sq2_sfx;
//NES btl_followupmusic.
volatile Uint8 btlFollowupMusic;
//NES dlgmusic_backup.
volatile Uint8 dlgMusicBackup;
//TRUE while the NES isn't calling MusicPlay.
volatile Uint8 musicHold;
//Frames left in PlaySFX_Error, which the game and music wait out.
volatile Uint8 sfxErrorFrames;
//Battle sound effect ID + 1 for musicFrame to start, 0 for none.
volatile Uint8 battleSfxRequest;
//TRUE while the NES isn't calling UpdateBattleSFX, during BattleScreenShake.
volatile Uint8 battleSfxHold;

//NES data_BattleSoundEffects IDs.
#define BTL_SFX_MAGIC (0)
#define BTL_SFX_ENEMY_ATTACK (1)
#define BTL_SFX_PLAYER_ATTACK (2)

//MSX effects with no NES sfx function, for musicMsxSfx. The airship's propeller loops on its own.
#define MSX_SFX_BATTLE_START (7)
#define MSX_SFX_WIPE_CLOSE (11)
#define MSX_SFX_WIPE_OPEN (12)
#define MSX_SFX_AIRSHIP (13)
#define MSX_SFX_ALTAR (15)
#define MSX_SFX_TAKEOFF (16)
#define MSX_SFX_LANDING (17)

//The sound option: TRUE for the MSX driver. Only TRUE when musicMsxAvailable.
extern Uint8 sndMsx;

/**
 * @brief Resets the sound driver. audio_init's init callback.
 */
void musicInit(void);

/**
 * @brief Queues an APU register write for the next audio frame.
 *
 * Safe to call from the main loop or a V-blank routine.
 *
 * @param addr APU register address ($4000-$4017).
 * @param value Value to write.
 */
void musicApuWrite(Uint16 addr, Uint8 value);

/**
 * @brief Gets the song for a vehicle from the ROM.
 *
 * @param vehicle Vehicle (VEH_).
 * @return Track number.
 */
Uint8 musicVehicleTrack(Uint8 vehicle);

/**
 * @brief Gets the song for a standard map tileset from the ROM.
 *
 * @param tileset Tileset number.
 * @return Track number.
 */
Uint8 musicTilesetTrack(Uint8 tileset);

/**
 * @brief Plays one frame of the current song (NES MusicPlay).
 *
 * musicFrame calls this. Starts a new song when music_track has one.
 */
void musicPlay(void);

/**
 * @brief Runs the queued APU writes and sound effects, then musicPlay
 * when the NES would call it. audio_init's frame callback.
 */
void musicFrame(void);

/**
 * @brief Gives whether the MSX driver (MSXSND.BIN) is on the CD.
 */
Uint8 musicMsxAvailable(void);

/**
 * @brief Switches to the MSX or NES driver and sets sndMsx. Call from the main loop.
 *
 * Silent while the driver loads from the CD, then restarts the song
 * that was playing. Falls back to NES if the MSX driver won't load.
 *
 * @param msx TRUE for the MSX driver.
 */
void musicSetMsx(Uint8 msx);

/**
 * @brief Plays an MSX effect (MSX_SFX_) with the MSX driver. Does nothing with the NES driver.
 *
 * @param id MSX effect.
 */
void musicMsxSfx(Uint8 id);

/**
 * @brief Stops the MSX song on the next audio frame, before that frame's effect. Does nothing with the NES driver.
 */
void musicMsxStopSong(void);

/**
 * @brief Plays the menu select sound.
 */
void sfxMenuSel(void);

/**
 * @brief Plays the menu cursor move sound.
 */
void sfxMenuMove(void);

/**
 * @brief Plays the error sound.
 *
 * The NES waits for its 16 frames, so the game waits on sfxErrorFrames.
 */
void sfxError(void);

/**
 * @brief Plays the dialogue box open or close sound.
 *
 * @param sweep Square 2 sweep: $8E sweeps up for opening, $95 down for closing.
 * @param frames Frames the sound keeps square 2.
 */
void sfxDialogueBox(Uint8 sweep, Uint8 frames);

/**
 * @brief Plays the door sound.
 */
void sfxDoor(void);

/**
 * @brief Plays the minigame's sound for a piece that can't slide.
 */
void sfxMinigameError(void);

/**
 * @brief Plays the poison step damage sound.
 */
void sfxPoison(void);

/**
 * @brief Plays the damage floor sound.
 */
void sfxTileDamage(void);

/**
 * @brief Starts a battle sound effect on the next audio frame.
 *
 * @param id Battle sound effect (BTL_SFX_).
 */
void battleSfx(Uint8 id);

#endif
