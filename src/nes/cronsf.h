/*
 * cronsf.h
 *
 * Copyright (C) 2021-2025 celeriyacon - https://github.com/celeriyacon
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
 * Altered for the Final Fantasy Saturn port: CroNSF's gs/types.h, gs/scsp.h,
 * gs/dsp-macros.h and gs/smpc.h merged into one header, built on SBL types
 * for sh-coff gcc 3.4 (no stdint.h). Documentation comments added.
 */

#ifndef CRONSF_H
#define CRONSF_H

#include <stdbool.h>
#include <sega_def.h>

#define INLINE inline __attribute__((always_inline))
#define NO_INLINE __attribute__((noinline))

#define UNLIKELY(v) __builtin_expect((v) != 0, 0)
#define LIKELY(v) __builtin_expect((v) != 0, 1)

#define COLD_SECTION

#define assert(v) ((void)0)

typedef signed char int8;
typedef signed short int16;
typedef signed long int32;
typedef signed long long int64;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned long uint32;
typedef unsigned long long uint64;

typedef unsigned long uintptr_t;

/**
 * SCSP access by offset from the start of sound RAM (0x25A00000).
 * Slot registers start at 0x100000, common registers at 0x100400.
 */
#define SCSPVP(offset) ((volatile void*)(0x25A00000 + (offset)))

#define SCSP(offset) (*(volatile uint16*)SCSPVP(offset))
#define SCSP16(offset) SCSP(offset)
#define SCSP8(offset) (*(volatile uint8*)SCSPVP(offset))

#define SCSP_SREG(slot, offset) SCSP16(0x100000 + (slot) * 0x20 + (offset))

#define SCSP_CREG(offset) SCSP16(0x100400 + (offset))
#define SCSP_CREG_LO(offset) SCSP8(0x100401 + (offset))

#define SCSP_SCIPD SCSP_CREG(0x10 << 1)
#define SCSP_SCIRE SCSP_CREG(0x11 << 1)
#define SCSP_MCIRE SCSP_CREG(0x17 << 1)
#define SCSP_SCIPD_HI SCSP8(0x100400 + (0x10 << 1))
#define SCSP_SCIRE_HI SCSP8(0x100400 + (0x11 << 1))

/**
 * @brief Waits for the SCSP's next sample interrupt flag.
 */
static INLINE void scsp_wait_sample_irq(void)
{
 SCSP_SCIRE_HI = 0x4;
 while(!(SCSP_SCIPD_HI & 0x4));
}

// Waits for at least the number of specified full sample periods
// (may be up to 1 beyond the amount requested)
static INLINE void scsp_wait_full_samples(unsigned count)
{
 while(count-- != (unsigned)-1)
  scsp_wait_sample_irq();
}

/**
 * @brief Resets the SCSP.
 *
 * Stops the 68K, keys off and clears every slot, clears the common
 * registers and DSP, then clears sound RAM. The 68K is left stopped.
 */
void scsp_init(void);

/**
 * @brief Starts the 68K (SMPC SNDON).
 */
void smpc_sound_on(void);

/**
 * @brief Stops the 68K (SMPC SNDOFF).
 */
void smpc_sound_off(void);

/**
 * SCSP DSP instruction fields. OR these together to build one 64-bit
 * DSP program step for MPROG.
 */
#define DSP_NXADDR	((uint64)(       1) <<  0)
#define DSP_ADRGB	((uint64)(       1) <<  1)
#define DSP_MASA(v)	((uint64)((v)&0x1F) <<  2)
#define DSP_NOFL	((uint64)(       1) <<  8)
#define DSP_CRA(v)	((uint64)((v)&0x3F) <<  9)
#define DSP_BSEL(v)	((uint64)((v)&0x01) << 16)
#define DSP_ZERO	((uint64)(       1) << 17)
#define DSP_NEGB	((uint64)(       1) << 18)
#define DSP_YRL		((uint64)(       1) << 19)
#define DSP_SHFT0	((uint64)(       1) << 20)
#define DSP_SHFT1	((uint64)(       1) << 21)
#define DSP_FRCL	((uint64)(	 1) << 22)
#define DSP_ADRL	((uint64)(	 1) << 23)
#define DSP_EWA(v)	((uint64)((v)&0x0F) << 24)
#define DSP_EWT		((uint64)(       1) << 28)
#define DSP_MRD		((uint64)(       1) << 29)
#define DSP_MWT		((uint64)(	 1) << 30)
#define DSP_TABLE	((uint64)(       1) << 31)
#define DSP_IWA(v)	((uint64)((v)&0x1F) << 32)
#define DSP_IWT		((uint64)(       1) << 37)
#define DSP_IRA(v)	((uint64)((v)&0x3F) << 38)
#define DSP_YSEL(v)	((uint64)((v)&0x03) << 45)
#define DSP_XSEL(v)	((uint64)((v)&0x01) << 47)
#define DSP_TWA(v)	((uint64)((v)&0x7F) << 48)
#define DSP_TWT		((uint64)(       1) << 55)
#define DSP_TRA(v)	((uint64)((v)&0x7F) << 56)

#define DSP_YSEL_FRCREG 	DSP_YSEL(0)
#define DSP_YSEL_COEF		DSP_YSEL(1)
#define DSP_YSEL_YREG11_23	DSP_YSEL(2)
#define DSP_YSEL_YREG4_15	DSP_YSEL(3)

#define DSP_XSEL_TEMP		DSP_XSEL(0)
#define DSP_XSEL_INPUTS		DSP_XSEL(1)

#define DSP_BSEL_TEMP		DSP_BSEL(0)
#define DSP_BSEL_SFTREG		DSP_BSEL(1)

/**
 * SCSP DSP memory. The trailing comments are the number of entries.
 */
static volatile uint64* const MPROG = (volatile uint64*)SCSPVP(0x100800);	// 128
static volatile uint16* const MADRS = (volatile uint16*)SCSPVP(0x100780);	//  32
static volatile uint16* const COEF = (volatile uint16*)SCSPVP(0x100700);	//  64
static volatile uint32* const TEMP = (volatile uint32*)SCSPVP(0x100C00);	// 128
static volatile uint32* const MEMS = (volatile uint32*)SCSPVP(0x100E00);	//  32
static volatile uint32* const MIXS = (volatile uint32*)SCSPVP(0x100E80);	//  16
static volatile uint16* const EFREG = (volatile uint16*)SCSPVP(0x100EC0);	//  16

/**
 * Convert a value into the register layout of MEMS, TEMP, MIXS and COEF.
 */
#define DSP_MAKE_MEMS(v) (((uint32)((v) & 0xFF) << 16) | (((v) >> 8) & 0xFFFF))
#define DSP_MAKE_TEMP(v) (((uint32)((v) & 0xFF) << 16) | (((v) >> 8) & 0xFFFF))
#define DSP_MAKE_MIXS(v) (((uint32)((v) & 0x0F) << 16) | (((v) >> 4) & 0xFFFF))
#define DSP_MAKE_COEF(v) ((v) << 3)

#endif
