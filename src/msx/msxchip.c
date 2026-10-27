#include "types.h"
#include "msxchip.h"

#define MC_RAM8(addr) (*(volatile Uint8 *)(0x25A00000 + (addr)))
#define MC_RAM16(addr) (*(volatile Uint16 *)(0x25A00000 + (addr)))

//Mailbox shared with msx68k.s: a ring of (type << 8) | value words. Type 0x00-0x0F is a PSG
//register, RING_OPLL + 0x00-0x3F an OPLL register, RING_FRAME the end of a frame.
#define RING 0x3800
#define RING_WR 0x3A00
#define RING_RD 0x3A01
#define RING_OPLL 0x40
#define RING_FRAME 0xFF

static Uint8 psgReg[16];

static void ringPut(Uint8 type, Uint8 value) {
    Uint8 wr = MC_RAM8(RING_WR);
    while((Uint8)(wr + 1) == MC_RAM8(RING_RD));
    MC_RAM16(RING + wr * 2) = (type << 8) | value;
    MC_RAM8(RING_WR) = wr + 1;
}

void msxPsgWrite(Uint8 reg, Uint8 value) {
    psgReg[reg & 15] = value;
    ringPut(reg & 15, value);
}

Uint8 msxPsgRead(Uint8 reg) {
    return psgReg[reg & 15];
}

void msxOpllWrite(Uint8 reg, Uint8 value) {
    if(reg >= 0x40) return;
    ringPut(RING_OPLL + reg, value);
}

void msxChipFrame() {
    ringPut(RING_FRAME, 0);
}

void msxChipInit() {
    for(Uint8 i = 0; i < 16; i++) psgReg[i] = 0;
    psgReg[7] = 0xB8;
    MC_RAM8(RING_WR) = 0;
    MC_RAM8(RING_RD) = 0;
}
