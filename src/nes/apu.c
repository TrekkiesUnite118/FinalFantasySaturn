/*
 * apu.c
 *
 * Copyright (C) 2024-2025 celeriyacon - https://github.com/celeriyacon
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
 * Altered for the Final Fantasy Saturn port: builds with sh-coff gcc 3.4 and
 * SBL, NSF/6502 dependencies removed, and apu_frame() paced by the caller
 * through apu_frame_timestamp() instead of busy-waiting on the SCSP.
 */

//
// Note: Reading from the SCSP register space from the SH-2 appears to stall
// the M68K when it tries to access the register space concurrently,
// even when the registers belong to different functional units.
//

#include "cronsf.h"
#include "apu.h"
#include "madrs.h"

uint32 fc_next_timestamp;
uint32 dmc_end_timestamp;
uint32 apu_status;

/*
fifo_a:
 Pulse 0:  0fffffffffffVVVV - Pulse 0 period and volume
 Pulse 1:  0fffffffffffVVVV - Pulse 1 period and volume

fifo_b:
 Triangle: 0fffffffffffVVVV - Triangle period and Noise volume
 Noise:    M0SSffff0000CCCC - Noise mode, noise short variant, noise frequency, and pulse 0 and 1 duty cycle

cfifo:
	   PPPPPPPP00000RRI

f: Frequency: 11 bits (Noise: mode 1 bit, freq 4 bits)
S: Noise short mode variant selector: 2 bits
M: Noise mode: 1 bit
V: Volume: 4 bits
C: Duty cycle: 2 bits

R: Pulse phase reset: 1 bit
P: 7-bit PCM delta(no saturation!)
I: Increment FIFO read position: 1 bit
*/

static const uint8 length_table[0x20] =
{
 0x0A, 0xFE,
 0x14, 0x02,
 0x28, 0x04,
 0x50, 0x06,
 0xA0, 0x08,
 0x3C, 0x0A,
 0x0E, 0x0C,
 0x1A, 0x0E,
 0x0C, 0x10,
 0x18, 0x12,
 0x30, 0x14,
 0x60, 0x16,
 0xC0, 0x18,
 0x48, 0x1A,
 0x10, 0x1C,
 0x20, 0x1E
};

static const int16 dmc_period_table[2][0x10] =
{
 {
  0x1AC, 0x17C, 0x154, 0x140, 0x11E, 0x0FE, 0x0E2, 0x0D6,
  0x0BE, 0x0A0, 0x08E, 0x080, 0x06A, 0x054, 0x048, 0x036
 },

 {
  0x18E, 0x162, 0x13C, 0x12A, 0x114, 0x0EC, 0x0D2, 0x0C6,
  0x0B0, 0x094, 0x084, 0x076, 0x062, 0x04E, 0x042, 0x032
 }
};

#if (APU_DEBUG) & 0x1
static struct apu_debug_t
{
 uint32 sync_apu_prev_timestamp;
 uint32 sync_pcm_prev_timestamp;
 uint32 update_triangle_prev_timestamp;
 uint32 update_fc_body_prev_timestamp;
 uint32 dmc_update_prev_timestamp;
 uint32 dmc_initialized;
} debug;
#endif

static uint64 snoise_lcg;

static struct envelope_t
{
 int8 period;
 int8 divider;
 int8 level;

 int8 mode;

 int8 reload;
} env[3];

static struct sweep_t
{
 int8 period;
 int8 divider;
 int8 enable;

 int8 reload;
} sweep[2];

/*static*/ struct dmc_t
{
 int32 shifter;		// 0
 int32 bit_counter;	// 1
 int32 control;		// 2
 uint32 next_timestamp;	// 3

 int32 prev_pcm;	// 4
 int32 prev_pcm_delta;	// 5

 int32 period;		// 6
 uint32 addr_counter;	// 7
 int32 byte_counter;	// 8
 uint32 start_addr;	// 9
 uint32 start_length;	// A

 uint32 prev_pcm_sample_time; // B

 uintptr_t* banks;	// C
 //
 // Not DMC:
 //
 uint32 timestamp_scale;	// D
 uint32 sample_offs;		// E
 volatile uint16* cfifo;	// F
 //
 // DMC again:
 //
 int16 period_table[0x10];
} dmc;

static int16 length_counter[4];

static struct triangle_t
{
 int16 period;
 int8 control;
 int8 ll_counter;
 int8 ll_counter_reload;
 int8 mode;

 uint32 period_update_prev_timestamp;
 int16 period_update_delay;
} triangle;

#define CFIFO_INC_FIFO_RP 	 0x1
#define CFIFO_PULSE0_PHASE_RESET 0x2
#define CFIFO_PULSE1_PHASE_RESET 0x4

struct stuff_t
{
 int8 cfifo_pending;
 uint32 fifo_cache[4];			// 1, 2, 3, 4
 uint32 fifo_pending[4];		// 5, 6, 7, 8
 volatile uint16* fifo_a;		// 9
 volatile uint16* fifo_b;		// A

 uint32 fifo_offset;			// B
 uint32 fifo_pending_sample_time;	// C

 uint32 lc_enable;			// D
 int32 sweep_mult[2];			// E, F
} stuff;

struct frame_counter_t
{
 int8 sequencer;
 int8 mode;

 int32 period;			// 1
 uint32 divider;		// 2
 //uint32 next_timestamp;	// 3
} frame_counter;


static void update_fifocache_volume(unsigned i)
{
 stuff.fifo_cache[i] &= ~0xF;

 if(env[i].mode & 0x1)
  stuff.fifo_cache[i] |= env[i].period;
 else
  stuff.fifo_cache[i] |= env[i].level;
}

static void sync_apu(uint32 timestamp)
{
#if (APU_DEBUG) & 0x1
 assert(timestamp >= debug.sync_apu_prev_timestamp);
 debug.sync_apu_prev_timestamp = timestamp;
#endif

 const uint32 sample_time = (timestamp * dmc.timestamp_scale + dmc.sample_offs) >> 21;
 uint32 w, x, y, z;
 struct stuff_t* const s = &stuff;

 if(sample_time != s->fifo_pending_sample_time)
 {
  s->fifo_offset = (s->fifo_offset + 4) & 0x1FFC;
  //
  volatile uint16* const efa = (volatile uint16*)((volatile uint8*)s->fifo_a + s->fifo_offset);

  efa[0] = s->fifo_pending[0];
  efa[1] = s->fifo_pending[1];
  //
  volatile uint16* const efb = (volatile uint16*)((volatile uint8*)s->fifo_b + s->fifo_offset);
  efb[0] = s->fifo_pending[2];
  efb[1] = s->fifo_pending[3];
  //
  *((volatile uint8*)&dmc.cfifo[s->fifo_pending_sample_time] + 1) = s->cfifo_pending;
  s->cfifo_pending = 0;
  //
  s->fifo_pending_sample_time = sample_time;
 }

 w = s->fifo_cache[0];
 x = s->fifo_cache[1];
 y = s->fifo_cache[2];
 z = s->fifo_cache[3];

 if(!(apu_status & 0x01))
  w = 0;

 s->fifo_pending[3] = z;

 if(!(apu_status & 0x02))
  x = 0;

 uint32 mask, comp;
 asm("mov #0xFFFFFFF0, %0\n\t" :"=r"(mask));
 asm("mov #0x7F, %0\n\t" :"=r"(comp));

 if(!(apu_status & 0x08))
  y &= mask;

 s->fifo_pending[2] = y;
 //
 //
 if(w <= comp)
  w = 0;

 if(x <= comp)
  x = 0;
 //
 //
 int32 res;

 res = w + ((((w & mask) * s->sweep_mult[0]) >> 8) & mask);
 s->cfifo_pending |= CFIFO_INC_FIFO_RP;
 asm("exts.w %0,%0\n\t": "+&r"(res));

 if(res < 0)
  w = 0;
 //
 res = x + ((((x & mask) * s->sweep_mult[1]) >> 8) & mask);
 asm("exts.w %0,%0\n\t": "+&r"(res));

 s->fifo_pending[0] = w;

 if(res < 0)
  x = 0;
 //
 //
 s->fifo_pending[1] = x; 
}

static void clock_fc_sequencer(uint32 timestamp)
{
 //
 // Pulse and noise enveloping
 //
 for(unsigned i = 0; i < 3; i++)
 {
  if(env[i].reload)
  {
   env[i].level = 0xF;
   env[i].divider = env[i].period;

   env[i].reload = false;
  }
  else
  {
   env[i].divider--;
   if(env[i].divider < 0)
   {
    env[i].divider = env[i].period;

    if(env[i].level || (env[i].mode & 0x2))
     env[i].level = (env[i].level - 1) & 0xF;
   }
  }
  //
  update_fifocache_volume(i);
 }

 //
 // Triangle linear counter
 //
 if(triangle.ll_counter_reload)
 {
  const unsigned nv = triangle.control & 0x7F;

  if((bool)triangle.ll_counter ^ (bool)nv)
   triangle.period_update_delay = 100;

  triangle.ll_counter = nv;
 }
 else if(triangle.ll_counter)
 {
  triangle.ll_counter--;

  if(!triangle.ll_counter)
   triangle.period_update_delay = 100;
 }

 if(!(triangle.control & 0x80))
  triangle.ll_counter_reload = false;


 //
 //
 //
 if(!(frame_counter.sequencer & 0x1))
 {
  //
  // Pulse length counters
  //
  for(unsigned i = 0; i < 2; i++)
  {
   if(!(env[i].mode & 0x2) && length_counter[i])
   {
    length_counter[i]--;

    if(!length_counter[i])
     apu_status &= ~(0x01 << i);
   }
  }

  //
  // Triangle length counter
  //
  if(!(triangle.control & 0x80) && length_counter[2])
  {
   length_counter[2]--;

   if(!length_counter[2])
   {
    apu_status &= ~0x04;
    triangle.period_update_delay = 100;
   }
  }

  //
  // Noise length counter
  //
  if(!(env[2].mode & 0x2) && length_counter[3])
  {
   length_counter[3]--;

   if(!length_counter[3])
    apu_status &= ~0x08;
  }

  //
  // Pulse sweep
  //
  for(unsigned i = 0; i < 2; i++)
  {
   sweep[i].divider--;
   if(sweep[i].divider < 0)
   {
    const int32 mult = stuff.sweep_mult[i];

    sweep[i].divider = sweep[i].period;

    if(sweep[i].enable && stuff.fifo_cache[i] >= 0x80)
    {
     if(mult < 0)
     {
      const uint32 delta = (((stuff.fifo_cache[i] & 0xFFFFFFF0) * -mult) >> 8) & 0xFFFFFFF0;

      stuff.fifo_cache[i] -= delta + (!i << 4);
     }
     else
     {
      const uint32 delta = (((stuff.fifo_cache[i] & 0xFFFFFFF0) * mult) >> 8) & 0xFFFFFFF0;
      const int16 result = stuff.fifo_cache[i] + delta;

      if(result >= 0)
       stuff.fifo_cache[i] = result;
     }
    }
   }

   if(sweep[i].reload)
   {
    sweep[i].divider = sweep[i].period;
    sweep[i].reload = false;
   }
  }
 }
 //
 //
 //
 if(frame_counter.sequencer == 0x3)
 {
  if(!(frame_counter.mode & 0x3))
   apu_status |= 0x40;

  if(frame_counter.mode & 0x2)
  {
   frame_counter.divider += frame_counter.period;
   fc_next_timestamp += frame_counter.divider >> 1;
   frame_counter.divider &= 1;
  }
 }

 frame_counter.sequencer = (frame_counter.sequencer + 1) & 0x3;
 //
 //
 //
 sync_apu(timestamp);
}

static INLINE void update_triangle_period(uint32 timestamp)
{
#if (APU_DEBUG) & 0x1
 assert(timestamp >= debug.update_triangle_prev_timestamp);
 debug.update_triangle_prev_timestamp = timestamp;
#endif

 if(triangle.period_update_delay > 0)
 {
  const uint32 cycles = timestamp - triangle.period_update_prev_timestamp;
  //
  triangle.period_update_delay -= cycles;
  if(triangle.period_update_delay <= 0)
  {
   const uint32 eff_timestamp = timestamp + triangle.period_update_delay;

   stuff.fifo_cache[2] = (stuff.fifo_cache[2] & 0xF) | (triangle.period << 4);

   if(!(apu_status & 0x04) || !triangle.ll_counter)
    stuff.fifo_cache[2] &= 0x000F;
   //
   sync_apu(eff_timestamp);
  }
 }
 triangle.period_update_prev_timestamp = timestamp;
}

static void update_fc_body(uint32 timestamp)
{
#if (APU_DEBUG) & 0x1
 assert(timestamp >= debug.update_fc_body_prev_timestamp);
 debug.update_fc_body_prev_timestamp = timestamp;
#endif

 do
 {
  const uint32 eff_timestamp = fc_next_timestamp;

  update_triangle_period(eff_timestamp);

  clock_fc_sequencer(eff_timestamp);

  frame_counter.divider += frame_counter.period;
  fc_next_timestamp += frame_counter.divider >> 1;
  frame_counter.divider &= 1;
 } while(timestamp >= fc_next_timestamp);

 update_triangle_period(timestamp);
}

static INLINE void sync_pcm(struct dmc_t* __restrict__ d, const uint32 timestamp, const uint8 new_pcm)
{
#if (APU_DEBUG) & 0x1
 assert(timestamp >= debug.sync_pcm_prev_timestamp);
 debug.sync_pcm_prev_timestamp = timestamp;
#endif

 const uint32 sample_time = (timestamp * d->timestamp_scale + d->sample_offs) >> 21;
 uint8 new_pcm_delta = new_pcm - d->prev_pcm;

 if(sample_time == d->prev_pcm_sample_time)
  new_pcm_delta += d->prev_pcm_delta;

 *((volatile uint8*)&d->cfifo[sample_time] + 0) = -new_pcm_delta;
 d->prev_pcm_delta = new_pcm_delta;
 d->prev_pcm_sample_time = sample_time;
 d->prev_pcm = new_pcm;
}

static INLINE void recalc_dmc_end_timestamp(struct dmc_t* __restrict__ d, uint32 dnts)
{
 if(dnts != (uint32)-1)
 {
  if(d->byte_counter)
   dmc_end_timestamp = dnts + ((d->byte_counter - 1) * 8 + d->bit_counter) * d->period;
 }
}

static INLINE void dmc_start(struct dmc_t* __restrict__ d, uint32 timestamp, uint32* dnts)
{
 d->addr_counter = 0xC000 + (d->start_addr << 6);
 d->byte_counter = (d->start_length << 4) + 1;

 //printf("DMC %04x %04x\n", d->addr_counter, d->addr_counter + d->byte_counter - 1);

 if(*dnts == (uint32)-1)
  *dnts = timestamp + (d->period >> 1);
 //
 //
 recalc_dmc_end_timestamp(d, *dnts);
}

void dmc_update_body(struct dmc_t* __restrict__ d, uint32 timestamp, uint32 dnts)
{
 do
 {
  const uint32 eff_timestamp = dnts;
  //
  //
  dnts += d->period;
  //
  if(UNLIKELY(!d->bit_counter))
  {
   if(UNLIKELY(!d->byte_counter))
   {
    if(d->control & 0x40 & (stuff.lc_enable << 2))
     dmc_start(d, eff_timestamp, &dnts);
    else
    {
     dnts = (uint32)-1;
     dmc_end_timestamp = (uint32)-1;
     break;
    }
   }
   //
   //
   d->shifter = (unsigned)*(int8*)(d->banks[d->addr_counter >> 12] + d->addr_counter) << 2;
   d->addr_counter = (uint16)((d->addr_counter + 1) | 0x8000);
   d->byte_counter--;
   d->bit_counter = 8;

   if(UNLIKELY(!d->byte_counter))
   {
    if(d->control & 0x40)
     dmc_start(d, eff_timestamp, &dnts);
    else
    {
#if (APU_DEBUG) & 0x1
     //printf("DMC End: timestamp=%u, eff_timestamp=%u, dmc_end_timestamp=%u\n", timestamp, eff_timestamp, dmc_end_timestamp);
     assert(dmc_end_timestamp == eff_timestamp);
#endif

     apu_status &= ~0x10;
     apu_status |= d->control;

     dmc_end_timestamp = (uint32)-1;
    }
   }
  }
  const unsigned new_pcm = d->prev_pcm + (d->shifter & 0x4) - 0x2;
   
  if(LIKELY(!(new_pcm & 0x80)))
   sync_pcm(d, eff_timestamp, new_pcm);

  d->shifter >>= 1;
  d->bit_counter--;
 } while(LIKELY(timestamp >= dnts));

 d->next_timestamp = dnts;
}

static INLINE void dmc_update(uint32 timestamp)
{
#if (APU_DEBUG) & 0x1
 assert(timestamp >= debug.dmc_update_prev_timestamp);
 debug.dmc_update_prev_timestamp = timestamp;
#endif

 uint32 dnts;

 dnts = dmc.next_timestamp;

 if(timestamp >= dnts)
 {
  dmc_update_body(&dmc, timestamp, dnts);
 }
}

static const int16 sweep_mult_table[16] =
{
  256,  128,  64,  32,  16,  8,  4,  2,
 -256, -128, -64, -32, -16, -8, -4, -2,
};

void apu_write(uint32 timestamp, uint16 addr, uint8 value)
{
 if(timestamp >= fc_next_timestamp)
  update_fc_body(timestamp);
 //
 update_triangle_period(timestamp);
 //
 switch(addr & 0x1F)
 {
  //
  // Pulse 0:
  //
  case 0x0:
	env[0].period = (value & 0xF);
	env[0].mode = (value >> 4) & 0x3;
	stuff.fifo_cache[3] = (stuff.fifo_cache[3] & 0xFFFFFFFC) | ((value >> 6) & 0x3);	// Duty
	//
	update_fifocache_volume(0);
	//
	sync_apu(timestamp);
	break;

  case 0x1:
	stuff.sweep_mult[0] = sweep_mult_table[value & 0xF];

	sweep[0].period = (value >> 4) & 0x7;
	sweep[0].enable = (value & 0x87) > 0x80;

	sweep[0].reload = true;
	//
	sync_apu(timestamp);
	break;

  case 0x2:
	stuff.fifo_cache[0] = (stuff.fifo_cache[0] & 0x700F) | (value << 4);
	//
	sync_apu(timestamp);
	break;

  case 0x3:
	stuff.fifo_cache[0] = (stuff.fifo_cache[0] & 0x0FFF) | ((value & 0x7) << 12);

	if(stuff.lc_enable & 0x1)
	{
	 length_counter[0] = length_table[(value >> 3) & 0x1F];
	 apu_status |= 0x01;
	}

	env[0].reload = true;
	//
	sync_apu(timestamp);
	//
	stuff.cfifo_pending |= CFIFO_PULSE0_PHASE_RESET;	// AFTER sync_apu()
	break;

  //
  // Pulse 1:
  //
  case 0x4:
	env[1].period = (value & 0xF);
	env[1].mode = (value >> 4) & 0x3;
	stuff.fifo_cache[3] = (stuff.fifo_cache[3] & 0xFFFFFFF3) | ((value >> 4) & 0x0C);	// Duty
	//
	update_fifocache_volume(1);
	//
	sync_apu(timestamp);
	break;

  case 0x5:
	stuff.sweep_mult[1] = sweep_mult_table[value & 0xF];

	sweep[1].period = (value >> 4) & 0x7;
	sweep[1].enable = (value & 0x87) > 0x80;

	sweep[1].reload = true;
	//
	sync_apu(timestamp);
	break;

  case 0x6:
	stuff.fifo_cache[1] = (stuff.fifo_cache[1] & 0x700F) | (value << 4);
	//
	sync_apu(timestamp);
	break;

  case 0x7:
	stuff.fifo_cache[1] = (stuff.fifo_cache[1] & 0x0FFF) | ((value & 0x7) << 12);

	if(stuff.lc_enable & 0x2)
	{
	 length_counter[1] = length_table[(value >> 3) & 0x1F];
	 apu_status |= 0x02;
	}

	env[1].reload = true;
	//
	sync_apu(timestamp);
	//
	stuff.cfifo_pending |= CFIFO_PULSE1_PHASE_RESET;	// AFTER sync_apu()
	break;

  //
  // Triangle:
  //
  case 0x8:
	triangle.control = value;
	break;

  case 0xA:
	triangle.period = (triangle.period & 0x0700) | value;
	triangle.period_update_delay = 100;
	break;

  case 0xB:
	triangle.period = (triangle.period & 0x00FF) | ((value & 0x7) << 8);
	triangle.period_update_delay = 100;

	if(stuff.lc_enable & 0x04)
	{
	 length_counter[2] = length_table[(value >> 3) & 0x1F];
	 apu_status |= 0x04;
	}

	triangle.ll_counter_reload = true;
	break;

  //
  // Noise:
  //
  case 0xC:
	env[2].period = (value & 0xF);
	env[2].mode = (value >> 4) & 0x3;
	//
	update_fifocache_volume(2);
	//
	sync_apu(timestamp);
	break;

  case 0xE:
	{
	 const uint8 ofc = *((uint8*)&stuff.fifo_cache[3] + (sizeof(stuff.fifo_cache[3]) - 2));
	 const uint8 nfc = (value & 0x8F) | ((snoise_lcg >> 32) & 0x30);

	 if((ofc ^ nfc) & nfc & 0x80)
	  snoise_lcg = (snoise_lcg * 6364136223846793005ULL) + 1;

	 *((uint8*)&stuff.fifo_cache[3] + (sizeof(stuff.fifo_cache[3]) - 2)) = nfc;
	}
	//
	sync_apu(timestamp);
	break;

  case 0xF:
	if(stuff.lc_enable & 0x8)
	{
	 length_counter[3] = length_table[(value >> 3) & 0x1F];
	 apu_status |= 0x08;
	}

	env[2].reload = true;
	//
	sync_apu(timestamp);
	break;

  case 0x10:
#if (APU_DEBUG) & 0x1
	debug.dmc_initialized |= 0x1;
#endif

	dmc_update(timestamp);
	//
	dmc.period = dmc.period_table[value & 0xF];
	dmc.control = value & 0xC0;

	if(dmc.control != 0x80)
	 apu_status &= 0x7F;
	//
	recalc_dmc_end_timestamp(&dmc, dmc.next_timestamp);
	break;

  case 0x11:
	dmc_update(timestamp);
	//
	sync_pcm(&dmc, timestamp, value & 0x7F);
	break;

  case 0x12:
#if (APU_DEBUG) & 0x1
	debug.dmc_initialized |= 0x2;
#endif
	dmc_update(timestamp);
	//
	dmc.start_addr = value;
	break;

  case 0x13:
#if (APU_DEBUG) & 0x1
	debug.dmc_initialized |= 0x4;
#endif
	dmc_update(timestamp);
	//
	dmc.start_length = value;
	break;

  case 0x15:
#if (APU_DEBUG) & 0x1
	if(value == 0xFF || ((value & 0x10) && debug.dmc_initialized != 0x7))
	 printf("BAD NSF rip?!\n");
#endif

	dmc_update(timestamp);
	//
	if(!(value & 0x1))
	 length_counter[0] = 0;

	if(!(value & 0x2))
	 length_counter[1] = 0;

	if(!(value & 0x4))
	{
	 if(length_counter[2])
	 {
	  length_counter[2] = 0;
	  triangle.period_update_delay = 100;
	 }
	}

	if(!(value & 0x8))
	 length_counter[3] = 0;

	stuff.lc_enable = value;

	if(value & 0x10)
	{
	 if(!dmc.byte_counter)
	  dmc_start(&dmc, timestamp, &dmc.next_timestamp);

	 apu_status |= 0x10;
	}
	else
	 dmc.byte_counter = 0;

	apu_status &= 0x40 | (value & 0x1F);
	//
	sync_apu(timestamp);
	break;

  case 0x17:
	frame_counter.sequencer = 0;

	if(value & 0x80)
	 clock_fc_sequencer(timestamp);

	frame_counter.divider = frame_counter.period + 1;
	fc_next_timestamp = timestamp + (frame_counter.divider >> 1);
	frame_counter.divider &= 1;

	frame_counter.mode = (value >> 6) & 0x3;
	break;
 }
}

uint8 apu_read_4015(uint32 timestamp, uint16 addr, uint32 raw_data_bus_in)
{
 if(timestamp >= fc_next_timestamp)
  update_fc_body(timestamp);

 if(timestamp >= dmc_end_timestamp)
  dmc_update(timestamp);
 //
 uint8 ret = apu_status;

 apu_status &= ~0x40;

 return ret;
}

void apu_force_update(uint32 timestamp)
{
 if(timestamp >= fc_next_timestamp)
  update_fc_body(timestamp);

 update_triangle_period(timestamp);
 dmc_update(timestamp);
 //
 sync_pcm(&dmc, timestamp, dmc.prev_pcm);
 //
 sync_apu(timestamp);
}

//Timestamp at which the frame must end to be lead samples ahead of the DSP.
//The DSP reads the control FIFO 1024 entries past its reported position.
uint32 apu_frame_timestamp(uint32 lead)
{
 const uint32 pos = SCSP(cfifo_position_addr) >> 5;
 const uint32 units = ((pos + 1024 + lead) << 21) - dmc.sample_offs;

 //Already at or past the target (the ring can't tell the difference past 1792 samples).
 if(units >= 0xE0000000)
  return 0;

 return units / dmc.timestamp_scale;
}

void apu_frame(uint32 timestamp)
{
 apu_force_update(timestamp);
 //
 //
 //
 const uint32 adj = timestamp;

 dmc.sample_offs += adj * dmc.timestamp_scale;

 assert(fc_next_timestamp >= timestamp);
 fc_next_timestamp -= adj;

 assert(triangle.period_update_prev_timestamp >= timestamp);
 triangle.period_update_prev_timestamp -= adj;

 if(dmc.next_timestamp != (uint32)-1)
 {
  assert(dmc.next_timestamp >= timestamp);
  dmc.next_timestamp -= adj;
 }

 if(dmc_end_timestamp != (uint32)-1)
 {
  assert(dmc.next_timestamp != (uint32)-1);
  assert(dmc_end_timestamp >= timestamp);
  dmc_end_timestamp -= adj;
 }

#if (APU_DEBUG) & 0x1
 assert(debug.sync_apu_prev_timestamp == timestamp);
 assert(debug.sync_pcm_prev_timestamp == timestamp);
 assert(debug.update_triangle_prev_timestamp == timestamp);
 assert(debug.dmc_update_prev_timestamp == timestamp);

 debug.sync_apu_prev_timestamp -= adj;
 debug.sync_pcm_prev_timestamp -= adj;
 debug.update_triangle_prev_timestamp -= adj;
 debug.update_fc_body_prev_timestamp = timestamp - adj; // (conditionally updated) -= adj;
 debug.dmc_update_prev_timestamp -= adj;
#endif
}
//
//
//
//
//

#define CRA_NEG1	  0x0
#define CRA_1_DIV_256	  0x1
#define CRA_1_DIV_4096	  0x2
#define CRA_1_DIV_2	  0x3
#define CRA_NEG1_DIV_4096 0x4
#define CRA_1_DIV_8	  0x5
#define CRA_1_DIV_2048	  0x6
#define CRA_ZERO	  0x7
#define CRA_1_DIV_16	  0x8
#define CRA_NEG1_DIV_2	  0x9
#define CRA_1_DIV_128	  0xA
#define CRA_1_DIV_32	  0xB

#define CRA_NEG4095_DIV_4096 0x10

#define CRA_TRI_VOLUME	  0x20
#define CRA_PULSE_DCBIAS_SCALE 0x21
#define CRA_PCM_VOLUME	  0x22

#define CRA_APU_VOLUME	  0x30
#define CRA_EXCHIP_VOLUME 0x31

#define DSP_CRA_NEG1		DSP_CRA(CRA_NEG1)
#define DSP_CRA_1_DIV_256	DSP_CRA(CRA_1_DIV_256)
#define DSP_CRA_1_DIV_4096	DSP_CRA(CRA_1_DIV_4096)
#define DSP_CRA_1_DIV_2 	DSP_CRA(CRA_1_DIV_2)
#define DSP_CRA_NEG1_DIV_4096	DSP_CRA(CRA_NEG1_DIV_4096)
#define DSP_CRA_1_DIV_8		DSP_CRA(CRA_1_DIV_8)
#define DSP_CRA_1_DIV_2048	DSP_CRA(CRA_1_DIV_2048)
#define DSP_CRA_ZERO		DSP_CRA(CRA_ZERO)
#define DSP_CRA_1_DIV_16	DSP_CRA(CRA_1_DIV_16)
#define DSP_CRA_NEG1_DIV_2	DSP_CRA(CRA_NEG1_DIV_2)
#define DSP_CRA_1_DIV_128	DSP_CRA(CRA_1_DIV_128)
#define DSP_CRA_1_DIV_32	DSP_CRA(CRA_1_DIV_32)

#define DSP_CRA_NEG4095_DIV_4096 DSP_CRA(CRA_NEG4095_DIV_4096)

#define DSP_CRA_TRI_VOLUME	DSP_CRA(CRA_TRI_VOLUME)
#define DSP_CRA_PCM_VOLUME	DSP_CRA(CRA_PCM_VOLUME)

#define DSP_CRA_APU_VOLUME	DSP_CRA(CRA_APU_VOLUME)
#define DSP_CRA_EXCHIP_VOLUME 	DSP_CRA(CRA_EXCHIP_VOLUME)


#define DSP_CRA_PULSE_DCBIAS_SCALE DSP_CRA(CRA_PULSE_DCBIAS_SCALE)

enum
{
 MEMS_PULSE0_FREQLO 	= 0x00,
 MEMS_PULSE0_FREQHI	= 0x01,
 MEMS_PULSE0_PHOFFSA	= 0x02,
 MEMS_PULSE0_PHOFFSB	= 0x03,
 MEMS_PULSE0_BASE	= 0x04,
 MEMS_PULSE0_PHRST	= 0x05,

 MEMS_PULSE1_FREQLO	= 0x06,
 MEMS_PULSE1_FREQHI	= 0x07,
 MEMS_PULSE1_PHOFFSA	= 0x08,
 MEMS_PULSE1_PHOFFSB	= 0x09,
 MEMS_PULSE1_BASE	= 0x0A,
 MEMS_PULSE1_PHRST	= 0x0B,

 MEMS_TRI_FREQLO	= 0x0C,
 MEMS_TRI_FREQHI	= 0x0D,
 MEMS_TRI_BASE		= 0x0E,

 MEMS_NOISE_FREQLO	= 0x0F,
 MEMS_NOISE_FREQHI	= 0x10,
 MEMS_NOISE_BASELO	= 0x11,
 MEMS_NOISE_BASEHI	= 0x12,
 MEMS_NOISE_MODEMUL	= 0x13,
 MEMS_NOISE_VOLUME	= 0x14,

 MEMS_PCM_QNLSCALE	= 0x15,

 MEMS_TEMP0 		= 0x16,
 MEMS_TEMP1 		= 0x17,
 MEMS_TEMP2 		= 0x18,
 MEMS_TEMP3 		= 0x19,
 MEMS_TEMP4 		= 0x1A,

 MEMS_EXCHIP		= 0x1B,

 MEMS_4096_SL3		= 0x1C,
 MEMS_4096_SL5		= 0x1D,
 MEMS_4096_SL7		= 0x1E,
 MEMS_4096_SL8		= 0x1F
};

enum
{
 MEMS_PULSE0_VOLUME = MEMS_TEMP0,
 MEMS_PULSE1_VOLUME = MEMS_TEMP1,
 // Don't alias/overwrite MEMS_TEMP4
};

enum
{
 MIXS_PULSE0	= 0x20,
 MIXS_PULSE1	= 0x21,
 MIXS_TRIANGLE	= 0x22,
 MIXS_NOISE	= 0x23,
 //
 //
 MIXS_NEG4096_SL9	= 0x2D,
 MIXS_NEG4096_SL10 	= 0x2E,
 MIXS_NEG4096_SL11	= 0x2F,
};

enum
{
#if 0
 TEMP_PULSE0_PHACCLO0 = 0x00,
 TEMP_PULSE0_PHACCLO1 = 0x01,

 TEMP_PULSE0_PHACCHI0 = 0x04,
 TEMP_PULSE0_PHACCHI1 = 0x05,
#endif

 TEMP_PULSE0_VOLDLY0 = 0x30,
 TEMP_PULSE0_VOLDLY1 = 0x31,
 TEMP_PULSE0_VOLDLY2 = 0x32,
 TEMP_PULSE0_VOLDLY3 = 0x33,

 TEMP_PULSE1_VOLDLY0 = 0x34,
 TEMP_PULSE1_VOLDLY1 = 0x35,
 TEMP_PULSE1_VOLDLY2 = 0x36,
 TEMP_PULSE1_VOLDLY3 = 0x37,

 TEMP_NOISE_VOLDLY0 = 0x38,
 TEMP_NOISE_VOLDLY1 = 0x39,
 TEMP_NOISE_VOLDLY2 = 0x3A,
 TEMP_NOISE_VOLDLY3 = 0x3B,

 TEMP_PULSES_SAMPLE  = 0x3D,
 TEMP___P_SAMPLE     = 0x3E,
 TEMP_TNP_SAMPLE     = 0x3F,

 TEMP_PULSE0_DCBIASDLY0 = 0x40,
 TEMP_PULSE0_DCBIASDLY1 = 0x41,
 TEMP_PULSE0_DCBIASDLY2 = 0x42,
 TEMP_PULSE0_DCBIASDLY3 = 0x43,

 TEMP_PULSE1_DCBIASDLY0 = 0x44,
 TEMP_PULSE1_DCBIASDLY1 = 0x45,
 TEMP_PULSE1_DCBIASDLY2 = 0x46,
 TEMP_PULSE1_DCBIASDLY3 = 0x47,

 TEMP_PULSE0_PHRSTDLY0 = 0x48,
 TEMP_PULSE0_PHRSTDLY1 = 0x49,

 TEMP_PULSE1_PHRSTDLY0 = 0x4C,
 TEMP_PULSE1_PHRSTDLY1 = 0x4D,

 TEMP_PULSE_DCBIASTMP = 0x50,

 TEMP_PULSE0A_FMOFFS = 0x54,
 TEMP_PULSE0B_FMOFFS = 0x55,
 TEMP_PULSE1A_FMOFFS = 0x56,
 TEMP_PULSE1B_FMOFFS = 0x57,
 TEMP_TRI_FMOFFS = 0x58,

 TEMP_CFIFO_POS      = 0x60,
 TEMP_CFIFO_POS_PREV = 0x61,

 TEMP_EXCHIP_DLY0 = 0x62,
 TEMP_EXCHIP_DLY1 = 0x63,
 TEMP_EXCHIP_DLY2 = 0x64,
 TEMP_EXCHIP_DLY3 = 0x65,
 TEMP_EXCHIP_DLY4 = 0x66,
 TEMP_EXCHIP_DLY5 = 0x67,
 TEMP_EXCHIP_DLY6 = 0x68,

 TEMP_APU_OUTPUT	   = 0x69,
 TEMP_COMBINED_OUTPUT_DLY0 = 0x6A,
 TEMP_COMBINED_OUTPUT_DLY1 = 0x6B,
 //
 //
 //
 TEMP_GUARD0	     = 0x6D,
 TEMP_PCM_VALUE      = 0x6E,
 TEMP_PCM_VALUE_PREV = 0x6F,
 TEMP_PCM_VALUE_DLY2 = 0x70,
 TEMP_PCM_VALUE_DLY3 = 0x71,
 TEMP_PCM_VALUE_DLY4 = 0x72,


 TEMP_GUARD1	    	= 0x7D,
 TEMP_PTNFIFO_POS	= 0x7E,
 TEMP_PTNFIFO_POS_PREV	= 0x7F,
};


static const uint8 Init68KProg[] =
{
 #include "init68k.bin.h"
};

// 0x0...
#define EFREG_PULSE0_ADDR 0x1
#define EFREG_PULSE1_ADDR 0x5
#define EFREG_TRI_ADDR	   0x9
#define EFREG_NOISE_ADDRHI 0xB
#define EFREG_NOISE_ADDRLO 0xC

#define EFREG_APU_OUTPUT 0xF

static uint64 mprog[256];

void apu_preinit(void)
{
 scsp_init();
 //
 SCSP_CREG(0x01 << 1) = (0x20 << 0) | (3 << 7); // RBP, RBL
 scsp_wait_full_samples(2);
 //
 //
 //
 //
 //
 smpc_sound_off();

 /*
  Slots:

    0...3: (unused alignment padding)

    4: Triangle position reader
    5: Pulse 0-A position reader
    6: Pulse 0-B position reader
    7: Pulse 1-A position reader
    8: Pulse 1-B position reader

    9: Triangle map 

   10: Pulse 0-A waveform
   11: Pulse 0-B waveform
   12: Pulse 1-A waveform
   13: Pulse 1-B waveform

   14: Triangle waveform
   15: Noise waveform
 

 DSP memory accesses:
  29 read + 7 write = 35
  (36 + 1) / 2 = 18 slots equivalent
  13 active slots + 18 DSP slot-equivalents + 1 DRAM refresh slot equivalent = 32 slots
 */
 const unsigned base_slot = 4;

 for(unsigned slot = 0; slot < 32; slot++)
  SCSP_SREG(slot, 0x0C) = (1U << 9);

 // Triangle, pulse0a, pulse0b, pulse1a, pulse1b position reader slots.
 for(unsigned i = 0; i < 5; i++)
 {
  const unsigned slot = base_slot + 0 + i;
  uint32 pa = position_addr + (i << 1);

  SCSP_SREG(slot, 0x00) = (1U << 11) | (0x0 << 9) | (0x1 << 5) | (0U << 4) | ((pa >> 16) << 0);
  SCSP_SREG(slot, 0x02) = pa & 0xFFFF;

  SCSP_SREG(slot, 0x0C) = (1U << 8);
  SCSP_SREG(slot, 0x10) = (0x400 << 0);
 }

 // Triangle map slot
 {
  const unsigned slot = base_slot + 5;
  const unsigned mdxy = (0x40 - 5) & 0x3F;
  const unsigned mdl = 0xF - 5;

  SCSP_SREG(slot, 0x00) = (1U << 11) | (0x0 << 9) | (0x1 << 5) | (0U << 4) | (tri_map_addr >> 16);
  SCSP_SREG(slot, 0x02) = tri_map_addr & 0xFFFF;
  SCSP_SREG(slot, 0x04) = 0;
  SCSP_SREG(slot, 0x0C) = (1U << 8);
  SCSP_SREG(slot, 0x0E) = (mdl << 12) | (mdxy << 6) | (mdxy << 0);
  SCSP_SREG(slot, 0x10) = (0x400 << 0);
 }
 // Pulse wave slots
 for(unsigned i = 0; i < 4; i++)
 {
  const unsigned slot = base_slot + 6 + i;
  const unsigned sb = (i >= 4 || !(i & 1)) ? 0x0 : 0x3;
  const unsigned isel = (i >> 1);
  const unsigned mdxy = (0x40 - 5) & 0x3F;
  const unsigned mdl = 0xA - 1;

  SCSP_SREG(slot, 0x00) = (1U << 11) | (sb << 9) | (0x1 << 5) | (0U << 4) | (saw_bank64k << 0);
  SCSP_SREG(slot, 0x06) = 1024;
  SCSP_SREG(slot, 0x0C) = (1U << 9) | (1U << 8);
  SCSP_SREG(slot, 0x0E) = (mdl << 12) | (mdxy << 6) | (mdxy << 0);
  SCSP_SREG(slot, 0x10) = (0x400 << 0) | (1U << 15);
  SCSP_SREG(slot, 0x14) = (isel << 3) | (0x6 << 0);
 }

 // Triangle wave slot 
 {
  const unsigned slot = base_slot + 10;
  const unsigned isel = 2;
  const unsigned mdxy = (0x40 - 5) & 0x3F;
  const unsigned mdl = 0xF - 5;

  SCSP_SREG(slot, 0x00) = (1U << 11) | (0x0 << 9) | (0x1 << 5) | (1U << 4) | (tri_bank64k << 0);
  SCSP_SREG(slot, 0x0C) = (1U << 9) | (1U << 8);
  SCSP_SREG(slot, 0x0E) = (mdl << 12) | (mdxy << 6) | (mdxy << 0);
  SCSP_SREG(slot, 0x10) = (0x400 << 0);
  SCSP_SREG(slot, 0x14) = (isel << 3) | (0x6 << 0);
 }

 // Noise slot
 {
  const unsigned slot = base_slot + 11;
  const unsigned isel = 3;

  SCSP_SREG(slot, 0x00) = (1U << 11) | (0x0 << 9) | (0x1 << 5) | (1U << 4);

  //
  // Set LSA and LEA to 0xFFFF to work around a race condition with the M68K program writing
  // garbage to LPCTL, causing the effective fractional playback position to be inverted,
  // triggering a hardware bug that screws up FM linear interpolation on the previous
  // slot(triangle wave), resulting in audio glitches.
  //
  SCSP_SREG(slot, 0x04) = 0xFFFF;
  SCSP_SREG(slot, 0x06) = 0xFFFF;

  SCSP_SREG(slot, 0x0C) = (1U << 9) | (1U << 8);
  SCSP_SREG(slot, 0x10) = (0x400 << 0);
  SCSP_SREG(slot, 0x14) = (isel << 3) | (0x6 << 0);
 }

 // DSP PCM output slot
 {
  const unsigned slot = EFREG_APU_OUTPUT;

  SCSP_SREG(slot, 0x16) = (0x07 << 5) | (0x00 << 0);
 }

 // Value slots
 for(unsigned i = 0; i < 3; i++)
 {
  static const int8 isel[3] = { MIXS_NEG4096_SL9, MIXS_NEG4096_SL10, MIXS_NEG4096_SL11 };
  static const int8 imxl[3] = { 0x5, 0x6, 0x7 };
  const unsigned slot = 24 + i; //base_slot + 5;
  SCSP_SREG(slot, 0x02) = 0;
  SCSP_SREG(slot, 0x04) = 0;
  SCSP_SREG(slot, 0x06) = 1;

  SCSP_SREG(slot, 0x0C) = (1 << 8);
  SCSP_SREG(slot, 0x10) = 0x400;
  SCSP_SREG(slot, 0x14) = (isel[i] << 3) | imxl[i];
  SCSP_SREG(slot, 0x16) = (0 << 13);
  SCSP_SREG(slot, 0x00) = (0 << 12) |
	 		  (0 << 11) |
			  (2 << 9) |
 			  (2 << 7) |
			  ((0x00 & 0x3) << 5) |
			  ((0x00000 >> 16) & 0xF);
 }

 SCSP_SREG(0x1F, 0x00) |= 1U << 12;

 scsp_wait_full_samples(8);

 for(unsigned i = 0; i < sizeof(Init68KProg); i++)
  SCSP8(i) = Init68KProg[i];

 smpc_sound_on();

 // Before continuing, wait to ensure M68K execution has moved
 // to EFREG space.
 scsp_wait_full_samples(16);
 //
 //
 //
 SCSP_CREG(0x01 << 1) = (rbp_init << 0);

 for(unsigned i = 0; i < 0x20; i++)
  MADRS[i] = madrs_init[i];

 COEF[CRA_NEG1]       = DSP_MAKE_COEF(-4096);
 COEF[CRA_1_DIV_256]  = DSP_MAKE_COEF(   16);
 COEF[CRA_1_DIV_4096] = DSP_MAKE_COEF(    1);
 COEF[CRA_1_DIV_2]    = DSP_MAKE_COEF( 2048);
 COEF[CRA_NEG1_DIV_4096] = DSP_MAKE_COEF(   -1);
 COEF[CRA_1_DIV_8]    = DSP_MAKE_COEF(4096 / 8);
 COEF[CRA_1_DIV_2048] = DSP_MAKE_COEF(    2);
 COEF[CRA_ZERO]       = DSP_MAKE_COEF(    0);
 COEF[CRA_1_DIV_16]   = DSP_MAKE_COEF(4096 / 16);
 COEF[CRA_1_DIV_128]  = DSP_MAKE_COEF(4096 / 128);
 COEF[CRA_1_DIV_32]  = DSP_MAKE_COEF(4096 / 32);
 COEF[CRA_NEG1_DIV_2] = DSP_MAKE_COEF(-2048);
 COEF[CRA_NEG4095_DIV_4096] = DSP_MAKE_COEF(-4095);

 COEF[CRA_TRI_VOLUME] = DSP_MAKE_COEF(tri_coef_volume);
 COEF[CRA_PCM_VOLUME] = DSP_MAKE_COEF(pcm_coef_volume);

 COEF[CRA_APU_VOLUME] = DSP_MAKE_COEF(-4096);
 COEF[CRA_EXCHIP_VOLUME] = DSP_MAKE_COEF(-4096);

 COEF[CRA_PULSE_DCBIAS_SCALE] = DSP_MAKE_COEF(-106 * 4096 / 128 / 2 / 2);

 MEMS[MEMS_4096_SL3] = DSP_MAKE_MEMS(4096 << 3);
 MEMS[MEMS_4096_SL5] = DSP_MAKE_MEMS(4096 << 5);
 MEMS[MEMS_4096_SL7] = DSP_MAKE_MEMS(4096 << 7);
 MEMS[MEMS_4096_SL8] = DSP_MAKE_MEMS(4096 << 8);
 //
 //
 //
 stuff.fifo_a = &SCSP16((((rbp_init << 12) + madrs_init[MADR_PTNFIFOA]) << 1) - (0x2000 / 2));
 stuff.fifo_b = &SCSP16((((rbp_init << 12) + madrs_init[MADR_PTNFIFOB]) << 1) - (0x2000 / 2));
 dmc.cfifo = &SCSP16((((rbp_init << 12) + madrs_init[MADR_CFIFO]) << 1) - (0x1000 / 2));
 //
 //
 //
 //
 //
 //
 //
 uint64* m = mprog;

 *(m++) = DSP_TWT | DSP_TWA(TEMP_APU_OUTPUT) |
	  DSP_ZERO |
	  DSP_XSEL_TEMP | DSP_TRA(TEMP_COMBINED_OUTPUT_DLY1) |
	  DSP_YSEL_COEF | DSP_CRA(CRA_NEG1);
/* DSP_BSEL_SFTREG | DSP_NEGB |		      //
	  DSP_XSEL_TEMP | DSP_TRA(TEMP_EXCHIP_DLY5) | //
	  DSP_YSEL_COEF | DSP_CRA(CRA_NEG1);	      //
*/
 *(m++) = DSP_EWT | DSP_EWA(EFREG_APU_OUTPUT) | // Pulse + Triangle+noise+PCM samples+expansion chip -> output
	  DSP_BSEL_TEMP | DSP_TRA(TEMP_PCM_VALUE_PREV) |	// PCM channel value+delta->value
	  DSP_XSEL_INPUTS | DSP_IRA(MIXS_NEG4096_SL11) |	// PCM channel value+delta->value
	  DSP_YSEL_YREG11_23;					// PCM channel value+delta->value

 *(m++) = DSP_TWT | DSP_TWA(TEMP_PCM_VALUE) |			// PCM channel value+delta->value
 //
 // Pulse 0 phase accum:
 //
 	  DSP_BSEL_TEMP | DSP_TRA(0x01 + (0 << 3)) |
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_PULSE0_FREQLO) |
	  DSP_YSEL_COEF | DSP_CRA_1_DIV_256;

 *(m++) = DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(0x00 + (0 << 3)) |
	  DSP_ZERO |						// Pulse 0 volume delay.
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_PULSE0_VOLUME) |	// Pulse 0 volume delay.
          DSP_YSEL_COEF | DSP_CRA_NEG1;				// Pulse 0 volume delay.

 *(m++) = DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(TEMP_PULSE0_VOLDLY0) | // Pulse 0 volume delay
	  DSP_ZERO |
	  DSP_XSEL_TEMP | DSP_TRA(0x00 + (0 << 3)) |
	  DSP_YSEL_COEF | DSP_CRA_1_DIV_16;

 *(m++) = DSP_FRCL |
	  DSP_ZERO |						// Pulse 0 phase reset delay
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_PULSE0_PHRST) |	// Pulse 0 phase reset delay
	  DSP_YSEL_COEF | DSP_CRA_1_DIV_2;			// Pulse 0 phase reset delay

 *(m++) = DSP_SHFT1 | DSP_TWT | DSP_TWA(TEMP_PULSE0_PHRSTDLY0) | // Pulse 0 phase reset delay
	  DSP_ZERO |
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_4096_SL5) |
	  DSP_YSEL_FRCREG;

 *(m++) = DSP_SHFT0 | DSP_SHFT1 | DSP_FRCL | 
	  DSP_ZERO |
	  DSP_XSEL_TEMP | DSP_TRA(0x00 + (0 << 3)) |
	  DSP_YSEL_COEF | DSP_CRA_NEG1;

 *(m++) = DSP_BSEL_SFTREG | DSP_NEGB |
	  DSP_XSEL_INPUTS | DSP_IRA(MIXS_NEG4096_SL10) |
	  DSP_YSEL_FRCREG;

 *(m++) = DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(0x00 + (0 << 3)) |
          DSP_BSEL_TEMP | DSP_NEGB | DSP_TRA(0x05 + (0 << 3)) |
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_PULSE0_FREQHI) |
	  DSP_YSEL_COEF | DSP_CRA_NEG1;

 *(m++) = DSP_BSEL_SFTREG | DSP_NEGB |
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_4096_SL3) |
	  DSP_YSEL_FRCREG;

 *(m++) = DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(0x04 + (0 << 3)) |
 //
 // Pulse 1 phase accum:
 //
          DSP_BSEL_TEMP | DSP_TRA(0x01 + (1 << 3)) |
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_PULSE1_FREQLO) |
	  DSP_YSEL_COEF | DSP_CRA_1_DIV_256;

 *(m++) = DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(0x00 + (1 << 3)) |
	  DSP_ZERO |						// Pulse 1 volume delay.
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_PULSE1_VOLUME) |	// Pulse 1 volume delay.
          DSP_YSEL_COEF | DSP_CRA_NEG1;				// Pulse 1 volume delay.

 *(m++) = DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(TEMP_PULSE1_VOLDLY0) | // Pulse 1 volume delay
	  DSP_ZERO |
	  DSP_XSEL_TEMP | DSP_TRA(0x00 + (1 << 3)) |
	  DSP_YSEL_COEF | DSP_CRA_1_DIV_16;

 *(m++) = DSP_FRCL |
	  DSP_ZERO |						// Pulse 1 phase reset delay
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_PULSE1_PHRST) |	// Pulse 1 phase reset delay
	  DSP_YSEL_COEF | DSP_CRA_1_DIV_2;			// Pulse 1 phase reset delay

 *(m++) = DSP_SHFT1 | DSP_TWT | DSP_TWA(TEMP_PULSE1_PHRSTDLY0) | // Pulse 1 phase reset delay
	  DSP_ZERO |
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_4096_SL5) |
	  DSP_YSEL_FRCREG;

 *(m++) = DSP_SHFT0 | DSP_SHFT1 | DSP_FRCL | 
	  DSP_ZERO |
	  DSP_XSEL_TEMP | DSP_TRA(0x00 + (1 << 3)) |
	  DSP_YSEL_COEF | DSP_CRA_NEG1;

 *(m++) = DSP_BSEL_SFTREG | DSP_NEGB |
	  DSP_XSEL_INPUTS | DSP_IRA(MIXS_NEG4096_SL10) |
	  DSP_YSEL_FRCREG;

 *(m++) = DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(0x00 + (1 << 3)) |
          DSP_BSEL_TEMP | DSP_NEGB | DSP_TRA(0x05 + (1 << 3)) |
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_PULSE1_FREQHI) |
	  DSP_YSEL_COEF | DSP_CRA_NEG1;

 *(m++) = DSP_BSEL_SFTREG | DSP_NEGB |
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_4096_SL3) |
	  DSP_YSEL_FRCREG;

 *(m++) = DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(0x04 + (1 << 3)) |
 //
 // Triangle phase accum:
 //
	  DSP_BSEL_TEMP | DSP_TRA(0x01 + (2 << 3)) |
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_TRI_FREQLO) |
	  DSP_YSEL_COEF | DSP_CRA_1_DIV_256;

 *(m++) = DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(0x00 + (2 << 3)) |
	  DSP_ZERO |					   // Pulse 0(H) phase reset
	  DSP_XSEL_TEMP | DSP_TRA(TEMP_PULSE0_PHRSTDLY1) | // Pulse 0(H) phase reset
	  DSP_YSEL_COEF | DSP_CRA_1_DIV_2;		   // Pulse 0(H) phase reset

 *(m++) = DSP_SHFT1 | DSP_FRCL |			   // Pulse 0(H) phase reset
	  DSP_ZERO |					   // Pulse 1(H) phase reset
	  DSP_XSEL_TEMP | DSP_TRA(TEMP_PULSE1_PHRSTDLY1) | // Pulse 1(H) phase reset
	  DSP_YSEL_COEF | DSP_CRA_1_DIV_2;		   // Pulse 1(H) phase reset

 *(m++) = DSP_SHFT1 | DSP_FRCL |			   // Pulse 1(H) phase reset
	  DSP_BSEL_TEMP |				   // Pulse 0(H) phase reset
	  DSP_XSEL_TEMP | DSP_TRA(0x04 + (0 << 3)) |	   // Pulse 0(H) phase reset
	  DSP_YSEL_FRCREG;				   // Pulse 0(H) phase reset
 
 *(m++) = DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(0x04 + (0 << 3)) |	// Pulse 0(H) phase reset
	  DSP_BSEL_TEMP |				   // Pulse 1(H) phase reset
	  DSP_XSEL_TEMP | DSP_TRA(0x04 + (1 << 3)) |	   // Pulse 1(H) phase reset
	  DSP_YSEL_FRCREG;				   // Pulse 1(H) phase reset

 *(m++) = DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(0x04 + (1 << 3)) |	// Pulse 1(H) phase reset
	  DSP_ZERO |
	  DSP_XSEL_TEMP | DSP_TRA(0x00 + (2 << 3)) |
	  DSP_YSEL_COEF | DSP_CRA_1_DIV_16 |
	  DSP_YRL | DSP_IRA(MEMS_PULSE0_PHRST);			// PTN FIFO position increment

 *(m++) = DSP_FRCL | 
	  DSP_BSEL_TEMP | DSP_TRA(TEMP_PTNFIFO_POS_PREV) |	// PTN FIFO position increment
	  DSP_XSEL_INPUTS | DSP_IRA(MIXS_NEG4096_SL9) |		// PTN FIFO position increment
	  DSP_YSEL_YREG4_15;					// PTN FIFO position increment

 *(m++) = DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(TEMP_PTNFIFO_POS) |			// PTN FIFO position increment
	  DSP_ZERO |								// Pulse0 base addr->EFREG
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_PULSE0_BASE) |				// Pulse0 base addr->EFREG
	  DSP_YSEL_COEF | DSP_CRA_NEG1;						// Pulse0 base addr->EFREG

 *(m++) = DSP_SHFT0 | DSP_SHFT1 | DSP_EWT | DSP_EWA(EFREG_PULSE0_ADDR) |	// Pulse0 base addr->EFREG
	  DSP_ZERO |
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_4096_SL5) |
	  DSP_YSEL_FRCREG;

 *(m++) = DSP_SHFT0 | DSP_SHFT1 | DSP_FRCL | 
	  DSP_ZERO |
	  DSP_XSEL_TEMP | DSP_TRA(0x00 + (2 << 3)) |
	  DSP_YSEL_COEF | DSP_CRA_NEG1;

 *(m++) = DSP_BSEL_SFTREG | DSP_NEGB |
	  DSP_XSEL_INPUTS | DSP_IRA(MIXS_NEG4096_SL10) |
	  DSP_YSEL_FRCREG;

 *(m++) = DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(0x00 + (2 << 3)) |
          DSP_BSEL_TEMP | DSP_NEGB | DSP_TRA(0x05 + (2 << 3)) |
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_TRI_FREQHI) |
	  DSP_YSEL_COEF | DSP_CRA_NEG1;

 *(m++) = DSP_BSEL_SFTREG | DSP_NEGB |
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_4096_SL3) |
	  DSP_YSEL_FRCREG;

 *(m++) = DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(0x04 + (2 << 3)) |
 //
 // Noise phase accum:
 //
          DSP_BSEL_TEMP | DSP_TRA(0x01 + (3 << 3)) |
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_NOISE_FREQLO) |
	  DSP_YSEL_COEF | DSP_CRA_1_DIV_256;

 *(m++) = DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(0x00 + (3 << 3)) |
	  DSP_ZERO |					// PCM quasi-nonlinear scale LUT
	  DSP_XSEL_TEMP | DSP_TRA(TEMP_PCM_VALUE_DLY3) |// PCM quasi-nonlinear scale LUT
	  DSP_YSEL_COEF | DSP_CRA_1_DIV_16;		// PCM quasi-nonlinear scale LUT

 *(m++) = DSP_ADRL | DSP_SHFT0 | DSP_SHFT1 |		// PCM quasi-nonlinear scale LUT
	  DSP_ZERO |					// Pulse1 base addr->EFREG
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_PULSE1_BASE) | // Pulse1 base addr->EFREG
	  DSP_YSEL_COEF | DSP_CRA_NEG1;			// Pulse1 base addr->EFREG

 *(m++) = DSP_SHFT0 | DSP_SHFT1 | DSP_EWT | DSP_EWA(EFREG_PULSE1_ADDR) | 	// Pulse1 base addr->EFREG
	  DSP_ZERO |
	  DSP_XSEL_TEMP | DSP_TRA(0x00 + (3 << 3)) |
	  DSP_YSEL_COEF | DSP_CRA_1_DIV_16;

 *(m++) = DSP_FRCL |
	  DSP_ZERO |						// Noise volume delay.
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_NOISE_VOLUME) |	// Noise volume delay.
          DSP_YSEL_COEF | DSP_CRA_NEG1;				// Noise volume delay.

 *(m++) = DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(TEMP_NOISE_VOLDLY0) | // Noise volume delay
	  DSP_ZERO |
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_4096_SL5) |
	  DSP_YSEL_FRCREG;

 *(m++) = DSP_SHFT0 | DSP_SHFT1 | DSP_FRCL | 
	  DSP_ZERO |
	  DSP_XSEL_TEMP | DSP_TRA(0x00 + (3 << 3)) |
	  DSP_YSEL_COEF | DSP_CRA_NEG1 |
	  DSP_YRL | DSP_IRA(MEMS_NOISE_MODEMUL);				// Noise

 *(m++) = DSP_BSEL_SFTREG | DSP_NEGB |
	  DSP_XSEL_INPUTS | DSP_IRA(MIXS_NEG4096_SL10) |
	  DSP_YSEL_FRCREG;

 *(m++) = DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(0x00 + (3 << 3)) |
          DSP_BSEL_TEMP | DSP_NEGB | DSP_TRA(0x05 + (3 << 3)) |
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_NOISE_FREQHI) |
	  DSP_YSEL_COEF | DSP_CRA_NEG1;

 *(m++) = DSP_BSEL_SFTREG | DSP_NEGB |
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_4096_SL3) |
	  DSP_YSEL_FRCREG;

 *(m++) = DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(0x04 + (3 << 3)) |
	  DSP_ZERO |							// Triangle base addr->EFREG
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_TRI_BASE) |			// Triangle base addr->EFREG
	  DSP_YSEL_COEF | DSP_CRA_NEG1;					// Triangle base addr->EFREG

 *(m++) = DSP_SHFT0 | DSP_SHFT1 | DSP_EWT | DSP_EWA(EFREG_TRI_ADDR);	// Triangle base addr->EFREG
 //
 //
 //
 //
 //
 //
 *(m++) = 0;

 *(m++) = DSP_ZERO |								// Noise
	  DSP_XSEL_TEMP | DSP_TRA(0x05 + (3 << 3)) |				// Noise
	  DSP_YSEL_YREG11_23;							// Noise

 *(m++) = DSP_BSEL_SFTREG | DSP_NEGB |						// Noise
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_NOISE_BASELO) | 			// Noise
	  DSP_YSEL_COEF | DSP_CRA_NEG1;						// Noise

 *(m++) = DSP_SHFT0 | DSP_SHFT1 | DSP_EWT | DSP_EWA(EFREG_NOISE_ADDRLO) |	// Noise
	  DSP_ZERO |								// Noise
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_NOISE_BASEHI) |			// Noise
	  DSP_YSEL_COEF | DSP_CRA_NEG1;						// Noise

 *(m++) = DSP_SHFT0 | DSP_SHFT1 | DSP_EWT | DSP_EWA(EFREG_NOISE_ADDRHI);	// Noise

 //
 //
 //
 *(m++) = DSP_ZERO |
	  DSP_XSEL_TEMP | DSP_TRA(TEMP_APU_OUTPUT) |
	  DSP_YSEL_COEF | DSP_CRA(CRA_APU_VOLUME);

 *(m++) = DSP_BSEL_SFTREG |
	  DSP_XSEL_TEMP | DSP_TRA(TEMP_EXCHIP_DLY5) |
	  DSP_YSEL_COEF | DSP_CRA(CRA_EXCHIP_VOLUME);

 *(m++) = DSP_TWT | DSP_TWA(TEMP_COMBINED_OUTPUT_DLY0);

 //
 // Parameter FIFO reading:
 //
 *(m++) = DSP_ZERO |
	  DSP_XSEL_TEMP | DSP_TRA(TEMP_PTNFIFO_POS) |
  	  DSP_YSEL_COEF | DSP_CRA_NEG1 |
	  DSP_MRD | DSP_ADRGB | DSP_MASA(MADR_PCM_QNLSCALE) | DSP_NOFL | DSP_TABLE; // PCM quasi-nonlinear scale LUT
 *(m++) = DSP_ADRL | DSP_SHFT0 | DSP_SHFT1;

 *(m++) = DSP_IWT | DSP_IWA(MEMS_PCM_QNLSCALE) |	// PCM quasi-nonlinear scale LUT
	  DSP_MRD | DSP_ADRGB | DSP_MASA(MADR_PTNFIFOA) | DSP_NOFL | DSP_TABLE;
 *(m++) = 0;

 *(m++) = DSP_IWT | DSP_IWA(MEMS_TEMP0) |
	  DSP_MRD | DSP_ADRGB | DSP_MASA(MADR_PTNFIFOA) | DSP_NXADDR | DSP_NOFL | DSP_TABLE;
 *(m++) = 0;

 *(m++) = DSP_IWT | DSP_IWA(MEMS_TEMP1) |
	  DSP_MRD | DSP_ADRGB | DSP_MASA(MADR_PTNFIFOB) | DSP_NOFL | DSP_TABLE;
 *(m++) = 0;

 *(m++) = DSP_IWT | DSP_IWA(MEMS_TEMP2) |
	  DSP_MRD | DSP_ADRGB | DSP_MASA(MADR_PTNFIFOB) | DSP_NXADDR | DSP_NOFL | DSP_TABLE |
	  DSP_ZERO |
	  DSP_XSEL_TEMP | DSP_TRA(TEMP_CFIFO_POS_PREV) |// Get control FIFO
  	  DSP_YSEL_COEF | DSP_CRA_1_DIV_2;		// position, and latch
 *(m++) = DSP_ADRL | DSP_SHFT0 | DSP_SHFT1; 		// it as an address.

 *(m++) = DSP_IWT | DSP_IWA(MEMS_TEMP3) |
	  DSP_MRD | DSP_ADRGB | DSP_MASA(MADR_EXCHIP) | DSP_NOFL | DSP_TABLE;		// EXCHIP

 *(m++) = DSP_ZERO |
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_TEMP0) |
	  DSP_YSEL_COEF | DSP_CRA_NEG1;

 *(m++) = DSP_IWT | DSP_IWA(MEMS_EXCHIP) |						// EXCHIP
	  DSP_MRD | DSP_ADRGB | DSP_MASA(MADR_CFIFO) | DSP_NOFL | DSP_TABLE |
	  DSP_BSEL_SFTREG | DSP_NEGB |
	  DSP_YSEL_COEF | DSP_CRA_ZERO;
 *(m++) = DSP_ADRL | DSP_SHFT0 | DSP_SHFT1 |
	  DSP_BSEL_TEMP | DSP_TRA(TEMP_PULSE0_DCBIASDLY2) | DSP_NEGB |	// Pulse 0 sample
	  DSP_XSEL_INPUTS | DSP_IRA(MIXS_PULSE0) |		// Pulse 0 sample
	  DSP_YSEL_COEF | DSP_CRA_1_DIV_2;			// Pulse 0 sample

 *(m++) = DSP_IWT | DSP_IWA(MEMS_TEMP4) |
	  DSP_MRD | DSP_ADRGB | DSP_MASA(MADR_PULSE_FREQLO) | DSP_NOFL | DSP_TABLE |
	  DSP_FRCL | DSP_SHFT1 |				// Pulse 0 sample
	  DSP_BSEL_TEMP | DSP_TRA(TEMP_PULSE1_DCBIASDLY2) | DSP_NEGB |	// Pulse 1 sample
	  DSP_XSEL_INPUTS | DSP_IRA(MIXS_PULSE1) |		// Pulse 1 sample
	  DSP_YSEL_COEF | DSP_CRA_1_DIV_2;			// Pulse 1 sample
 *(m++) = DSP_FRCL | DSP_SHFT1 |				// Pulse 1 sample
	  DSP_ZERO |						// Pulse 0 sample
	  DSP_XSEL_TEMP | DSP_TRA(TEMP_PULSE0_VOLDLY2) |	// Pulse 0 sample
	  DSP_YSEL_FRCREG;					// Pulse 0 sample

 *(m++) = DSP_IWT | DSP_IWA(MEMS_PULSE0_FREQLO) |
	  DSP_MRD | DSP_ADRGB | DSP_MASA(MADR_PULSE_FREQHI) | DSP_NOFL | DSP_TABLE |
	  DSP_BSEL_SFTREG |					// Pulse 0 sample
	  DSP_XSEL_TEMP | DSP_TRA(TEMP_PULSE1_VOLDLY2) |	// Pulse 1 sample
	  DSP_YSEL_FRCREG;					// Pulse 1 sample

 *(m++) = DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(TEMP_PULSES_SAMPLE) | // Pulse 0+1 samples -> TEMP[TEMP_PULSES_SAMPLE]
	  DSP_ZERO |
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_TEMP1) |
	  DSP_YSEL_COEF | DSP_CRA_NEG1;

 *(m++) = DSP_IWT | DSP_IWA(MEMS_PULSE0_FREQHI) |
	  DSP_MRD | DSP_ADRGB | DSP_MASA(MADR_PULSE_BASE) | DSP_NOFL | DSP_TABLE | 
	  DSP_BSEL_SFTREG | DSP_NEGB |
	  DSP_YSEL_COEF | DSP_CRA_ZERO;
 *(m++) = DSP_ADRL | DSP_SHFT0 | DSP_SHFT1 |
	  DSP_ZERO |				// Guard 0
	  DSP_YSEL_COEF | DSP_CRA_ZERO;		// Guard 0

 *(m++) = DSP_IWT | DSP_IWA(MEMS_PULSE0_BASE) |
	  DSP_MRD | DSP_ADRGB | DSP_MASA(MADR_PULSE_FREQLO) | DSP_NOFL | DSP_TABLE |
	  DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(TEMP_GUARD0) |	// Guard 0
	  DSP_ZERO |				// Guard 1
	  DSP_YSEL_COEF | DSP_CRA_ZERO;		// Guard 1
 *(m++) = DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(TEMP_GUARD1);	// Guard 1

 *(m++) = DSP_IWT | DSP_IWA(MEMS_PULSE1_FREQLO) |
	  DSP_MRD | DSP_ADRGB | DSP_MASA(MADR_PULSE_FREQHI) | DSP_NOFL | DSP_TABLE;
 *(m++) = DSP_ZERO |
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_TEMP2) |
	  DSP_YSEL_COEF | DSP_CRA_NEG1;

 *(m++) = DSP_IWT | DSP_IWA(MEMS_PULSE1_FREQHI) |
	  DSP_MRD | DSP_ADRGB | DSP_MASA(MADR_PULSE_BASE) | DSP_NOFL | DSP_TABLE |
	  DSP_BSEL_SFTREG | DSP_NEGB |
	  DSP_YSEL_COEF | DSP_CRA_ZERO;
 *(m++) = DSP_ADRL | DSP_SHFT0 | DSP_SHFT1 |
	  DSP_BSEL_TEMP | DSP_TRA(TEMP_CFIFO_POS_PREV) |			// CFIFO position increment
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_4096_SL8) |				// CFIFO position increment
	  DSP_YSEL_COEF | DSP_CRA_1_DIV_128;					// CFIFO position increment

 *(m++) = DSP_IWT | DSP_IWA(MEMS_PULSE1_BASE) |
	  DSP_MWT | DSP_SHFT0 | DSP_SHFT1 | DSP_MASA(MADR_CFIFO_POSITION) | DSP_NOFL | DSP_TABLE |	// CFIFO position increment
	  DSP_TWT | DSP_TWA(TEMP_CFIFO_POS);								// CFIFO position increment
 *(m++) = 0;

 *(m++) = DSP_MRD | DSP_ADRGB | DSP_MASA(MADR_TRI_FREQLO) | DSP_NOFL | DSP_TABLE;
 *(m++) = 0;

 *(m++) = DSP_IWT | DSP_IWA(MEMS_TRI_FREQLO) |
	  DSP_MRD | DSP_ADRGB | DSP_MASA(MADR_TRI_FREQHI) | DSP_NOFL | DSP_TABLE;
 *(m++) = 0;

 *(m++) = DSP_IWT | DSP_IWA(MEMS_TRI_FREQHI) |
	  DSP_MRD | DSP_ADRGB | DSP_MASA(MADR_TRI_BASE) | DSP_NOFL | DSP_TABLE |
	  DSP_ADRL | DSP_IRA(MEMS_TEMP3) |
 	  DSP_ZERO |						// Pulse 0-A FM offs
	  DSP_XSEL_TEMP | DSP_TRA(0x04 + (0 << 3)) |		// Pulse 0-A FM offs
	  DSP_YSEL_COEF | DSP_CRA_NEG1;				// Pulse 0-A FM offs
 *(m++) = DSP_BSEL_SFTREG |					// Pulse 0-A FM offs
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_PULSE0_PHOFFSA) |	// Pulse 0-A FM offs
	  DSP_YSEL_COEF | DSP_CRA_NEG1;				// Pulse 0-A FM offs

 *(m++) = DSP_IWT | DSP_IWA(MEMS_TRI_BASE) |
	  DSP_MRD | DSP_ADRGB | DSP_MASA(MADR_NOISE_MODEMUL) | DSP_NOFL | DSP_TABLE |
	  DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(TEMP_PULSE0A_FMOFFS) | // Pulse 0-A FM offs
	  DSP_ZERO |						// Pulse 0-B FM offs
	  DSP_XSEL_TEMP | DSP_TRA(0x04 + (0 << 3)) |		// Pulse 0-B FM offs
	  DSP_YSEL_COEF | DSP_CRA_NEG1;				// Pulse 0-B FM offs
 *(m++) = DSP_BSEL_SFTREG |					// Pulse 0-B FM offs
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_PULSE0_PHOFFSB) |	// Pulse 0-B FM offs
	  DSP_YSEL_COEF | DSP_CRA_NEG1;				// Pulse 0-B FM offs

 *(m++) = DSP_IWT | DSP_IWA(MEMS_NOISE_MODEMUL) |
	  DSP_MRD | DSP_ADRGB | DSP_MASA(MADR_NOISE_FREQLO) | DSP_NOFL | DSP_TABLE |
	  DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(TEMP_PULSE0B_FMOFFS) |	// Pulse 0-B FM offs
	  DSP_ZERO |						// Pulse 1-A FM offs
	  DSP_XSEL_TEMP | DSP_TRA(0x04 + (1 << 3)) |		// Pulse 1-A FM offs
	  DSP_YSEL_COEF | DSP_CRA_NEG1;				// Pulse 1-A FM offs
 *(m++) = DSP_BSEL_SFTREG |					// Pulse 1-A FM offs
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_PULSE1_PHOFFSA) |	// Pulse 1-A FM offs
	  DSP_YSEL_COEF | DSP_CRA_NEG1;				// Pulse 1-A FM offs

 *(m++) = DSP_IWT | DSP_IWA(MEMS_NOISE_FREQLO) |
	  DSP_MRD | DSP_ADRGB | DSP_MASA(MADR_NOISE_FREQHI) | DSP_NOFL | DSP_TABLE |
	  DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(TEMP_PULSE1A_FMOFFS) | // Pulse 1-A FM offs
	  DSP_ZERO |						// Pulse 1-B FM offs
	  DSP_XSEL_TEMP | DSP_TRA(0x04 + (1 << 3)) |		// Pulse 1-B FM offs
	  DSP_YSEL_COEF | DSP_CRA_NEG1;				// Pulse 1-B FM offs
 *(m++) = DSP_BSEL_SFTREG |					// Pulse 1-B FM offs
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_PULSE1_PHOFFSB) |	// Pulse 1-B FM offs
	  DSP_YSEL_COEF | DSP_CRA_NEG1;				// Pulse 1-B FM offs

 *(m++) = DSP_IWT | DSP_IWA(MEMS_NOISE_FREQHI) |
	  DSP_MRD | DSP_ADRGB | DSP_MASA(MADR_NOISE_BASELO) | DSP_NOFL | DSP_TABLE |
	  DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(TEMP_PULSE1B_FMOFFS) | // Pulse 1-B FM offs
	  DSP_ZERO |					// -MEMS_EXCHIP->TEMP_EXCHIP_DLY0
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_EXCHIP) |	// -MEMS_EXCHIP->TEMP_EXCHIP_DLY0
	  DSP_YSEL_COEF | DSP_CRA_NEG1;			// -MEMS_EXCHIP->TEMP_EXCHIP_DLY0

 *(m++) = DSP_TWT | DSP_TWA(TEMP_EXCHIP_DLY0) |		// -MEMS_EXCHIP->TEMP_EXCHIP_DLY0
	  DSP_YRL | DSP_IRA(MEMS_TEMP3) |
	  DSP_ZERO |
	  DSP_XSEL_TEMP | DSP_TRA(0x05 + (2 << 3)) |
	  DSP_YSEL_COEF | DSP_CRA_NEG1;

 *(m++) = DSP_IWT | DSP_IWA(MEMS_NOISE_BASELO) |
	  DSP_MRD | DSP_ADRGB | DSP_MASA(MADR_NOISE_BASEHI) | DSP_NOFL | DSP_TABLE |
	  DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(TEMP_TRI_FMOFFS) |
	  DSP_ZERO |
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_4096_SL8) |
	  DSP_YSEL_YREG4_15;
 *(m++) = DSP_ADRL | DSP_SHFT0 | DSP_SHFT1 |
	  DSP_ZERO |								// Pulse DC bias delay(constant)
	  DSP_XSEL_INPUTS | DSP_IRA(MIXS_NEG4096_SL11) |			// Pulse DC bias delay(constant)
	  DSP_YSEL_COEF | DSP_CRA_NEG1;						// Pulse DC bias delay(constant)

 *(m++) = DSP_IWT | DSP_IWA(MEMS_NOISE_BASEHI) |
	  DSP_MRD | DSP_ADRGB | DSP_MASA(MADR_PULSE0_PHOFFSA) | DSP_NOFL | DSP_TABLE |
	  DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(TEMP_PULSE_DCBIASTMP) |	// Pulse DC bias delay(constant)
	  DSP_BSEL_SFTREG |							// Pulse 0 DC bias delay
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_PULSE0_PHOFFSA) |			// Pulse 0 DC bias delay
          DSP_YSEL_COEF | DSP_CRA_NEG1;						// Pulse 0 DC bias delay
 *(m++) = DSP_BSEL_SFTREG | DSP_NEGB |						// Pulse 0 DC bias delay
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_PULSE0_PHOFFSB) |			// Pulse 0 DC bias delay
	  DSP_YSEL_COEF | DSP_CRA_NEG1;						// Pulse 0 DC bias delay

 *(m++) = DSP_IWT | DSP_IWA(MEMS_PULSE0_PHOFFSA) |
	  DSP_MRD | DSP_ADRGB | DSP_MASA(MADR_PULSE0_PHOFFSB) | DSP_NOFL | DSP_TABLE |
	  DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(TEMP_PULSE0_DCBIASDLY0) |	// Pulse 0 DC bias delay
	  DSP_BSEL_TEMP | DSP_TRA(TEMP_PULSE_DCBIASTMP) |			// Pulse 1 DC bias delay
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_PULSE1_PHOFFSA) |			// Pulse 1 DC bias delay
          DSP_YSEL_COEF | DSP_CRA_NEG1;						// Pulse 1 DC bias delay
 *(m++) = DSP_BSEL_SFTREG | DSP_NEGB |						// Pulse 1 DC bias delay
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_PULSE1_PHOFFSB) |			// Pulse 1 DC bias delay
	  DSP_YSEL_COEF | DSP_CRA_NEG1;						// Pulse 1 DC bias delay


 *(m++) = DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(TEMP_PULSE1_DCBIASDLY0) | 	// Pulse 1 DC bias delay
	  DSP_IWT | DSP_IWA(MEMS_PULSE0_PHOFFSB) |
	  DSP_MRD | DSP_ADRGB | DSP_MASA(MADR_PULSE1_PHOFFSA) | DSP_NOFL | DSP_TABLE |
	  DSP_ZERO |
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_TEMP4) |	// Phase reset LUT
	  DSP_YSEL_COEF | DSP_CRA_1_DIV_2;		// Phase reset LUT
 *(m++) = DSP_FRCL | DSP_SHFT0 | DSP_SHFT1 |		// Phase reset LUT
	  DSP_ZERO |					// Zero control FIFO word
	  DSP_XSEL_TEMP | DSP_TRA(TEMP_CFIFO_POS_PREV) |// Zero control FIFO word
  	  DSP_YSEL_COEF | DSP_CRA_1_DIV_2;		// Zero control FIFO word

 *(m++) = DSP_IWT | DSP_IWA(MEMS_PULSE1_PHOFFSA) |
	  DSP_MRD | DSP_ADRGB | DSP_MASA(MADR_PULSE1_PHOFFSB) | DSP_NOFL | DSP_TABLE |
	  DSP_ADRL | DSP_SHFT0 | DSP_SHFT1;
 *(m++) = DSP_YRL | DSP_IRA(MEMS_TEMP0) |		// Pulse0 volume LUT
	  DSP_ZERO |					// Zero control FIFO word
	  DSP_YSEL_COEF | DSP_CRA_ZERO;			// Zero control FIFO word

 *(m++) = DSP_IWT | DSP_IWA(MEMS_PULSE1_PHOFFSB) |
	  DSP_MWT | DSP_SHFT0 | DSP_SHFT1 | DSP_ADRGB | DSP_MASA(MADR_CFIFO) | DSP_NOFL | DSP_TABLE |	// Zero control FIFO word
	  DSP_ZERO |					// Phase reset LUT
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_4096_SL5) |	// Phase reset LUT
	  DSP_YSEL_FRCREG;				// Phase reset LUT
 *(m++) = DSP_ADRL | DSP_SHFT0 | DSP_SHFT1 |		// Phase reset LUT
	  DSP_ZERO |								// Pulse 0 DC bias delay, scaling
	  DSP_XSEL_TEMP | DSP_TRA(TEMP_PULSE0_DCBIASDLY0) |			// Pulse 0 DC bias delay, scaling
          DSP_YSEL_COEF | DSP_CRA_PULSE_DCBIAS_SCALE;				// Pulse 0 DC bias delay, scaling

 *(m++) = DSP_MRD | DSP_ADRGB | DSP_MASA(MADR_PULSE0_PHRST) | DSP_NOFL | DSP_TABLE | // Phase reset LUT
	  DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(TEMP_PULSE0_DCBIASDLY0) |	// Pulse 0 DC bias delay, scaling
	  DSP_ZERO |								// Pulse 1 DC bias delay, scaling
	  DSP_XSEL_TEMP | DSP_TRA(TEMP_PULSE1_DCBIASDLY0) |			// Pulse 1 DC bias delay, scaling
          DSP_YSEL_COEF | DSP_CRA_PULSE_DCBIAS_SCALE;				// Pulse 1 DC bias delay, scaling
 *(m++) = DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(TEMP_PULSE1_DCBIASDLY0) |	// Pulse 1 DC bias delay, scaling
	  DSP_ZERO |					// Pulse0 volume LUT
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_4096_SL8) |	// Pulse0 volume LUT
	  DSP_YSEL_YREG4_15;				// Pulse0 volume LUT

 *(m++) = DSP_IWT | DSP_IWA(MEMS_PULSE0_PHRST) |	// Phase reset LUT
	  DSP_MRD | DSP_ADRGB | DSP_MASA(MADR_PULSE1_PHRST) | DSP_NOFL | DSP_TABLE |	// Phase reset LUT
	  DSP_ADRL | DSP_SHFT0 | DSP_SHFT1 |		// Pulse0 volume LUT
	  DSP_YRL | DSP_IRA(MEMS_TEMP1);		// Pulse1 volume LUT
 *(m++) = DSP_ZERO |					// Pulse1 volume LUT
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_4096_SL8) |	// Pulse1 volume LUT
	  DSP_YSEL_YREG4_15;				// Pulse1 volume LUT

 *(m++) = DSP_IWT | DSP_IWA(MEMS_PULSE1_PHRST) |	// Phase reset LUT
	  DSP_MRD | DSP_ADRGB | DSP_MASA(MADR_PULSE_VOLUME) | DSP_NOFL | DSP_TABLE |	// Pulse0 volume LUT
	  DSP_ADRL | DSP_SHFT0 | DSP_SHFT1 |		// Pulse1 volume LUT
	  DSP_YRL | DSP_IRA(MEMS_TEMP2);	  	// Noise volume LUT
 *(m++) = DSP_ZERO |					// Noise volume LUT
	  DSP_XSEL_INPUTS | DSP_IRA(MEMS_4096_SL8) |	// Noise volume LUT
	  DSP_YSEL_YREG4_15;				// Noise volume LUT

 *(m++) = DSP_IWT | DSP_IWA(MEMS_PULSE0_VOLUME) |	// Pulse0 volume LUT
	  DSP_MRD | DSP_ADRGB | DSP_MASA(MADR_PULSE_VOLUME) | DSP_NOFL | DSP_TABLE |	// Pulse1 volume LUT
	  DSP_ADRL | DSP_SHFT0 | DSP_SHFT1 |		// Noise volume LUT
	  DSP_YRL | DSP_IRA(MIXS_NOISE);		// Noise sample
 *(m++) = DSP_ZERO |					// Triangle sample
	  DSP_XSEL_INPUTS | DSP_IRA(MIXS_TRIANGLE) |	// Triangle sample
	  DSP_YSEL_COEF | DSP_CRA_TRI_VOLUME;		// Triangle sample

 *(m++) = DSP_IWT | DSP_IWA(MEMS_PULSE1_VOLUME) |	// Pulse1 volume LUT
	  DSP_MRD | DSP_ADRGB | DSP_MASA(MADR_NOISE_VOLUME) | DSP_NOFL | DSP_TABLE |	// Noise volume LUT
	  DSP_BSEL_SFTREG |				// Triangle sample
	  DSP_XSEL_TEMP | DSP_TRA(TEMP_NOISE_VOLDLY3) |	// Noise sample
	  DSP_YSEL_YREG11_23;				// Noise sample
 *(m++) = DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(TEMP_TNP_SAMPLE) | // Triangle+noise samples -> TEMP[TEMP_TNP_SAMPLE]
	  DSP_ZERO |						// Triangle position(for FM)
	  DSP_XSEL_TEMP | DSP_TRA(TEMP_TRI_FMOFFS) |		// Triangle position(for FM)
	  DSP_YSEL_COEF | DSP_CRA_NEG1 |			// Triangle position(for FM)
	  DSP_YRL | DSP_IRA(MEMS_PCM_QNLSCALE);		// Triangle, noise, PCM; qnlscale

 *(m++) = DSP_IWT | DSP_IWA(MEMS_NOISE_VOLUME) |
	  DSP_MWT | DSP_SHFT0 | DSP_SHFT1 | DSP_MASA(MADR_TRI_POSITION) | DSP_NOFL | DSP_TABLE | // Triangle position(for FM)
	  DSP_ZERO |					 // (Triangle+noise) * qnlscale
	  DSP_XSEL_TEMP | DSP_TRA(TEMP_TNP_SAMPLE) |	 // (Triangle+noise) * qnlscale
	  DSP_YSEL_YREG11_23;			         // (Triangle+noise) * qnlscale
 *(m++) = DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(TEMP_TNP_SAMPLE) | // (Triangle+noise) * qnlscale -> TEMP[TEMP_TNP_SAMPLE]
	  DSP_ZERO |						// Pulse 0-A position(for FM)
	  DSP_XSEL_TEMP | DSP_TRA(TEMP_PULSE0A_FMOFFS) |	// Pulse 0-A position(for FM)
	  DSP_YSEL_COEF | DSP_CRA_NEG1;				// Pulse 0-A position(for FM)

 *(m++) = DSP_MWT | DSP_SHFT0 | DSP_SHFT1 | DSP_MASA(MADR_PULSE0A_POSITION) | DSP_NOFL | DSP_TABLE |	// Pulse 0-A position(for FM)
	  DSP_ZERO |					// PCM * PCM volume
	  DSP_XSEL_TEMP | DSP_TRA(TEMP_PCM_VALUE_DLY3) |// PCM * PCM volume
	  DSP_YSEL_COEF | DSP_CRA(CRA_PCM_VOLUME);	// PCM * PCM volume
 *(m++) = DSP_SHFT0 | DSP_SHFT1 | DSP_TWT | DSP_TWA(TEMP___P_SAMPLE) | // PCM * PCM volume -> TEMP[TEMP___P_SAMPLE]
	  DSP_ZERO |						// Pulse 0-B position(for FM)
	  DSP_XSEL_TEMP | DSP_TRA(TEMP_PULSE0B_FMOFFS) |	// Pulse 0-B position(for FM)
	  DSP_YSEL_COEF | DSP_CRA_NEG1;				// Pulse 0-B position(for FM)

 *(m++) = DSP_MWT | DSP_SHFT0 | DSP_SHFT1 | DSP_MASA(MADR_PULSE0B_POSITION) | DSP_NOFL | DSP_TABLE;	// Pulse 0-B position(for FM)
 *(m++) = DSP_ZERO |						// Pulse 1-A position(for FM)
	  DSP_XSEL_TEMP | DSP_TRA(TEMP_PULSE1A_FMOFFS) |	// Pulse 1-A position(for FM)
	  DSP_YSEL_COEF | DSP_CRA_NEG1;				// Pulse 1-A position(for FM)

 *(m++) = DSP_MWT | DSP_SHFT0 | DSP_SHFT1 | DSP_MASA(MADR_PULSE1A_POSITION) | DSP_NOFL | DSP_TABLE;	// Pulse 1-A position(for FM)
 *(m++) = DSP_ZERO |						// Pulse 1-B position(for FM)
	  DSP_XSEL_TEMP | DSP_TRA(TEMP_PULSE1B_FMOFFS) |	// Pulse 1-B position(for FM)
	  DSP_YSEL_COEF | DSP_CRA_NEG1;				// Pulse 1-B position(for FM)

 *(m++) = DSP_MWT | DSP_SHFT0 | DSP_SHFT1 | DSP_MASA(MADR_PULSE1B_POSITION) | DSP_NOFL | DSP_TABLE |	// Pulse 1-B position(for FM)
	  DSP_ZERO |					// ((Triangle+noise) * qnlscale) + PCM) * qnlscale
	  DSP_XSEL_TEMP | DSP_TRA(TEMP___P_SAMPLE) |	// ((Triangle+noise) * qnlscale) + PCM) * qnlscale
	  DSP_YSEL_YREG11_23;				// ((Triangle+noise) * qnlscale) + PCM) * qnlscale
 *(m++) = DSP_BSEL_SFTREG |				// ((Triangle+noise) * qnlscale) + PCM) * qnlscale
	  DSP_XSEL_TEMP | DSP_TRA(TEMP_TNP_SAMPLE) |	// ((Triangle+noise) * qnlscale) + PCM) * qnlscale
	  DSP_YSEL_YREG11_23;				// ((Triangle+noise) * qnlscale) + PCM) * qnlscale

 *(m++) = DSP_BSEL_SFTREG | DSP_NEGB |			//
	  DSP_XSEL_TEMP | DSP_TRA(TEMP_PULSES_SAMPLE) |	//
	  DSP_YSEL_COEF | DSP_CRA(CRA_NEG1) |		//
	  DSP_YRL | DSP_IRA(MEMS_TEMP4);		// PCM channel value+delta->value
 //
 //
 //

#if (APU_DEBUG)
 {
  int pulse0_addr_i = -1;
  int pulse1_addr_i = -1;
  int tri_addr_i = -1;
  int noise_addrhi_i = -1;
  int noise_addrlo_i = -1;

  printf("%u\n", (unsigned)(m - mprog));

  for(unsigned i = 0; i < 128; i++)
  {
   assert((i & 1) || !(mprog[i] & (DSP_MRD | DSP_MWT)));
   //
   if(mprog[i] & DSP_MRD)
   {
    printf("Read: %u\n", i);
   }

   if(mprog[i] & DSP_MWT)
   {
    printf("Write: %u\n", i);
   }

   if(mprog[i] & DSP_EWT)
   {
    const unsigned ewa = (mprog[i] >> 24) & 0xF;

    if(ewa == EFREG_PULSE0_ADDR)
    {
     assert(pulse0_addr_i == -1);
     pulse0_addr_i = i;
    }

    if(ewa == EFREG_PULSE1_ADDR)
    {
     assert(pulse1_addr_i == -1);
     pulse1_addr_i = i;
    }

    if(ewa == EFREG_TRI_ADDR)
    {
     assert(tri_addr_i == -1);
     tri_addr_i = i;
    }

    if(ewa == EFREG_NOISE_ADDRHI)
    {
     assert(noise_addrhi_i == -1);
     noise_addrhi_i = i;
    }

    if(ewa == EFREG_NOISE_ADDRLO)
    {
     assert(noise_addrlo_i == -1);
     noise_addrlo_i = i;
    }
   }
  }

  assert(pulse0_addr_i != -1);
  assert(pulse1_addr_i != -1);
  assert(tri_addr_i != -1);
  assert(noise_addrhi_i != -1);
  assert(noise_addrlo_i != -1);

  printf("pulse0: %3u\n", pulse0_addr_i);
  printf("pulse1: %3u\n", pulse1_addr_i);
  printf("tri: %3u\n", tri_addr_i);
  printf("noise hi: %3u\n", noise_addrhi_i);
  printf("noise lo: %3u\n", noise_addrlo_i);

  assert((m - mprog) == 128);

/*
  FIXME

  assert(pulse0_addr_i == (13 - 6) * 4);
  assert(pulse1_addr_i == (15 - 6) * 4);
  assert(tri_addr_i == (18 - 6) * 4);
*/
 }
#endif
}

void apu_kill(void)
{
 SCSP_CREG_LO(0x00 << 1) = (0x00 << 0);	// MVOL to min

 for(unsigned i = 0; i < 128; i++)
  MPROG[i] = mprog[i] & ~(DSP_TWT | DSP_MWT | DSP_EWT);

 scsp_wait_full_samples(4);
 
 for(unsigned i = 0; i < 128; i++)
  MPROG[i] = 0;

 scsp_wait_full_samples(4);

 for(unsigned i = 0; i < 0x800; i++)
  dmc.cfifo[i] = 0;

 for(unsigned i = 0; i < 0x1000; i++)
 {
  stuff.fifo_a[i] = 0;
  stuff.fifo_b[i] = 0;
 }

 {
  volatile int16* exb = apu_get_exchip_buffer();

  for(unsigned i = 0; i < 0x800; i++)
   exb[i] = 0;
 }

 for(unsigned i = 0; i < 128; i++)
  TEMP[i] = 0;
 //
 //
 MEMS[MEMS_PULSE0_FREQLO] = 0;
 MEMS[MEMS_PULSE0_FREQHI] = 0;
 MEMS[MEMS_PULSE0_PHOFFSA] = 0;
 MEMS[MEMS_PULSE0_PHOFFSB] = 0;
 MEMS[MEMS_PULSE0_BASE] = 0;
 MEMS[MEMS_PULSE0_PHRST] = 0;

 MEMS[MEMS_PULSE1_FREQLO] = 0;
 MEMS[MEMS_PULSE1_FREQHI] = 0;
 MEMS[MEMS_PULSE1_PHOFFSA] = 0;
 MEMS[MEMS_PULSE1_PHOFFSB] = 0;
 MEMS[MEMS_PULSE1_BASE] = 0;
 MEMS[MEMS_PULSE1_PHRST] = 0;

 MEMS[MEMS_TRI_FREQLO] = 0;
 MEMS[MEMS_TRI_FREQHI] = 0;
 MEMS[MEMS_TRI_BASE] = 0;

 MEMS[MEMS_NOISE_FREQLO] = 0;
 MEMS[MEMS_NOISE_FREQHI] = 0;
 MEMS[MEMS_NOISE_BASELO] = 0;
 MEMS[MEMS_NOISE_BASEHI] = 0;
 MEMS[MEMS_NOISE_MODEMUL] = 0;

 MEMS[MEMS_PCM_QNLSCALE] = 0;

 MEMS[MEMS_TEMP0] = 0;
 MEMS[MEMS_TEMP1] = 0;
 MEMS[MEMS_TEMP2] = 0;
 MEMS[MEMS_TEMP3] = 0;
 MEMS[MEMS_TEMP4] = 0;
 //
 //
 for(unsigned i = 0; i < 5; i++)
  SCSP16(position_addr + (i << 1)) = 0;

 SCSP16(cfifo_position_addr) = 0;
 //
 //
 scsp_wait_full_samples(2);
}

COLD_SECTION static void init_scsp_part(void)
{
 apu_kill();
 //
 //
 //
 //
 //
 //
 {
  scsp_wait_full_samples(4);

  for(unsigned i = 0; i < 128; i++)
   MPROG[i] = mprog[i] & ~(DSP_TWT | DSP_MWT | DSP_EWT);

  for(unsigned i = 0; i < 128; i++)
  {
   const bool twt = (bool)(mprog[i] & DSP_TWT);
   const unsigned twa = (mprog[i] >> 48) & 0x7F;

   if(twt && (twa == TEMP_GUARD0 || twa == TEMP_GUARD1))
    MPROG[i] = mprog[i] & ~(DSP_MWT | DSP_EWT);
  }

  scsp_wait_full_samples(8);

  for(unsigned i = 0; i < 128; i++)
   MPROG[i] = mprog[i] & ~(DSP_MWT | DSP_EWT);

  scsp_wait_full_samples(8);

  for(unsigned i = 0; i < 128; i++)
   MPROG[i] = mprog[i];
 }

 scsp_wait_full_samples(8);
 //
 //
 //
 //
 //
 SCSP_CREG_LO(0x00 << 1) = (0x0F << 0);	// MVOL to max
}

void apu_init(bool pal, uint32 apu_volume)
{
 init_scsp_part();

 assert(apu_volume <= 65536);

#if EXCHIP_SINESWEEP
 apu_volume = 0;
#endif

 COEF[CRA_APU_VOLUME] = DSP_MAKE_COEF(-((((4096 * apu_volume) >> 15) + 1) >> 1));

 stuff.fifo_offset = 0x2000 / 2;
 for(unsigned i = 0; i < 4; i++)
  stuff.fifo_cache[i] = 0;

 stuff.fifo_pending_sample_time = 0;

 for(unsigned i = 0; i < 4; i++)
  stuff.fifo_pending[i] = 0;

 stuff.cfifo_pending = 0;
 //
 //
 //
 fc_next_timestamp = 0;
 triangle.period_update_prev_timestamp = 0;
 frame_counter.period = pal ? 16627 : 14915;
 dmc.timestamp_scale = pal ? time_scale_pal : time_scale_ntsc;
 dmc.sample_offs = 0;
 dmc.prev_pcm_sample_time = 0;
 dmc.next_timestamp = (uint32)-1;
 dmc_end_timestamp = (uint32)-1;

 for(unsigned i = 0; i < 16; i++)
  dmc.period_table[i] = dmc_period_table[pal][i];

#if (APU_DEBUG) & 0x1
 debug.sync_apu_prev_timestamp = 0;
 debug.sync_pcm_prev_timestamp = 0;
 debug.update_triangle_prev_timestamp = 0;
 debug.update_fc_body_prev_timestamp = 0;
 debug.dmc_update_prev_timestamp = 0;
#endif
}

uint32 apu_get_timestamp_scale(void)
{
 return dmc.timestamp_scale;
}

volatile int16* apu_get_exchip_buffer(void)
{
 return (volatile int16*)&SCSP16((((rbp_init << 12) + madrs_init[MADR_EXCHIP]) << 1) - (0x1000 / 2));
}


void apu_power(void)
{
 snoise_lcg = 0xDEADBEEFCAFEF00DULL;
 //

 for(unsigned i = 0; i < 3; i++)
 {
  env[i].period = 0;
  env[i].divider = 0;
  env[i].level = 0;

  env[i].mode = 0;
  env[i].reload = false;
 }

 for(unsigned i = 0; i < 2; i++)
 {
  sweep[i].period = 0;
  sweep[i].divider = 0;
  sweep[i].enable = 0;

  stuff.sweep_mult[i] = sweep_mult_table[0];

  sweep[i].reload = false;
 }

 for(unsigned i = 0; i < 4; i++)
  length_counter[i] = 0;

 stuff.lc_enable = 0;
 apu_status = 0;

 triangle.period = 0;
 triangle.control = 0;
 triangle.ll_counter = 0;
 triangle.ll_counter_reload = false;
 triangle.mode = 0;

 triangle.period_update_delay = 0;

 dmc.period = dmc.period_table[0];
 dmc.next_timestamp = (uint32)-1;
 dmc_end_timestamp = (uint32)-1;
 dmc.addr_counter = 0;
 dmc.byte_counter = 0;
 dmc.shifter = 0;
 dmc.bit_counter = 0;
 dmc.control = 0;
 dmc.start_addr = 0;
 dmc.start_length = 0;

 dmc.prev_pcm = 0;
 dmc.prev_pcm_delta = 0;

 fc_next_timestamp = 0;
 frame_counter.divider = 1;
 frame_counter.sequencer = 0;
 frame_counter.mode = 0;

#if (APU_DEBUG) & 0x1
 debug.dmc_initialized = 0;
#endif
}

void apu_start_sync(void)
{
 for(;;)
 {
  const uint16 raw_sspos = SCSP(cfifo_position_addr);
  unsigned d = (dmc.sample_offs - (raw_sspos >> 5)) & 0x7FF;

  if(d < 4)
   break;
 }
}
