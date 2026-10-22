/*
 * apu.h
 *
 * Copyright (C) 2024 celeriyacon - https://github.com/celeriyacon
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 */

/*
 * Altered for the Final Fantasy Saturn port: apu_frame_timestamp() added,
 * expansion chip entry points removed, documentation comments added.
 */

#ifndef CRONSF_APU_H
#define CRONSF_APU_H

#include "cronsf.h"

/**
 * @brief Resets the SCSP and sets up the APU's sound slots and DSP memory.
 *
 * This clears sound RAM and starts the 68K's program from init68k.bin.h.
 * Load the APU tables (SCSPAPU.BIN) into sound RAM after this.
 */
void apu_preinit(void);

/**
 * @brief Starts the APU's DSP program and sets the timing and volume.
 *
 * @param pal TRUE for PAL timing, FALSE for NTSC.
 * @param apu_volume Output volume in 16.16 fixed point, up to 65536 (1.0).
 */
void apu_init(bool pal, uint32 apu_volume);

/**
 * @brief Silences the APU: master volume off, DSP program stopped and its
 * buffers cleared.
 */
void apu_kill(void);

/**
 * @brief Resets the APU's channels to their NES power on state.
 */
void apu_power(void);

/**
 * @brief Gets the APU's clock rate.
 *
 * @return SCSP sample time per NES CPU cycle, in units of 1/2^21 samples.
 */
uint32 apu_get_timestamp_scale(void);

/**
 * @brief Gets the expansion chip sample buffer in sound RAM.
 *
 * Not used by this port: the expansion chip entry points were removed.
 *
 * @return Pointer to the buffer.
 */
volatile int16* apu_get_exchip_buffer(void);

/**
 * @brief Waits until the DSP's read position catches up with the APU.
 *
 * nesaudio_init calls it after apu_power, before the first APU frame.
 */
void apu_start_sync(void);

/**
 * @brief Writes an APU register.
 *
 * @param timestamp NES CPU cycle of the write in the current frame.
 * Must not go backwards within a frame.
 * @param addr APU register address ($4000-$4017).
 * @param value Value to write.
 */
void apu_write(uint32 timestamp, uint16 addr, uint8 value);

/**
 * @brief Reads the APU status register ($4015).
 *
 * Reading it clears the frame IRQ flag (bit 6), as on the NES.
 *
 * @param timestamp NES CPU cycle of the read in the current frame.
 * @param addr Unused.
 * @param raw_data_bus_in Unused.
 * @return The status register value.
 */
uint8 apu_read_4015(uint32 timestamp, uint16 addr, uint32 raw_data_bus_in);

/**
 * @brief Brings the APU's output up to the given timestamp.
 *
 * @param timestamp NES CPU cycle in the current frame.
 */
void apu_force_update(uint32 timestamp);

/**
 * @brief Gets the timestamp the current frame should end at to keep the
 * output a set number of samples ahead of the DSP.
 *
 * @param lead Number of samples to stay ahead of the DSP.
 * @return The timestamp, or 0 if the output is already that far ahead.
 */
uint32 apu_frame_timestamp(uint32 lead);

/**
 * @brief Ends the current APU frame at the given timestamp.
 *
 * The next frame's timestamps start at 0.
 *
 * @param timestamp NES CPU cycle the frame ends at. Must not be before
 * the frame's last write.
 */
void apu_frame(uint32 timestamp);

#endif
