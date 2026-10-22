/*
 * cronsf_scsp.c
 *
 * Copyright (C) 2021-2024 celeriyacon - https://github.com/celeriyacon
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
 * Altered for the Final Fantasy Saturn port: CroNSF's gs/scsp.c, with the
 * SMPC sound on/off commands issued through SBL's sega_per.h macros.
 */

#include "cronsf.h"
#include <sega_per.h>

void smpc_sound_on(void)
{
 PER_SMPC_SND_ON();
}

void smpc_sound_off(void)
{
 PER_SMPC_SND_OFF();
}

void scsp_init(void)
{
 smpc_sound_off();

 SCSP(0x100400) = (1 << 9);

 scsp_wait_full_samples(4);

 for(unsigned slot = 0; slot < 32; slot++)
 {
  SCSP_SREG(slot, 0x00) = 0x0800;
  SCSP_SREG(slot, 0x08) = 0x0000;
  SCSP_SREG(slot, 0x0A) = 0x001F;
  SCSP_SREG(slot, 0x10) = 0x0400;
 }
 SCSP_SREG(0, 0x00) |= 1 << 12;

 scsp_wait_full_samples(4);

 for(unsigned slot = 0; slot < 32; slot++)
 {
  SCSP_SREG(slot, 0x00) = 0x0000;
 }
 SCSP_SREG(0, 0x00) |= 1 << 12;

 scsp_wait_full_samples(256);

 for(unsigned t = 0; t < 2; t++)
 {
  for(uint32 i = 0x100000; i < 0x100400; i += 2)
   SCSP(i) = 0;

  for(uint32 i = 0x100402; i < 0x101000; i += 2)
  {
   if(i != 0x100406)
   {
    SCSP(i) = 0;
   }
  }

  scsp_wait_full_samples(256);
 }

 SCSP_SCIRE = 0xFFFF;
 SCSP_MCIRE = 0xFFFF;

 for(unsigned i = 0x25A00000; i < 0x25A80000; i += 2)
  *(volatile uint16*)i = 0;
}
