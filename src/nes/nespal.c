#include "nespal.h"

Uint32 nes_pal[64] = {
	0x727272,0x8B1720,0XAC0000,0x9B0041,0x72008B,0x0F00AC,0x0000A4,0x00077A,0X002841,0x004100,0x005100,0x0f3800,0x593817,0x0000,0x0000,0x0000,
	0xBCBCBC,0xEE7200,0XEE3820,0xF60083,0xBC00BC,0x5900E6,0x0028DD,0x0749CD,0X00728B,0x009300,0x00AC00,0x389300,0x8B8300,0x0F0F0F,0x0000,0x0000,
    0xFEFEFE,0xFEBC38,0XFE9359,0xFE8BA4,0xFE7AF6,0xB472FE,0x6272FE,0x389BFE,0X37BCF6,0x0FD583,0x49DD49,0x9BFE59,0xDDEE00,0x383838,0x0000,0x0000,
    0xFEFEFE,0xFEE6AC,0XFED5C5,0xFECDD5,0xFEC5FE,0xDDC5FE,0xB4BCFE,0xACDDFE,0XA4E6FE,0xA4FEE6,0xBCF6AC,0xCDFEB4,0xF6FE9B,0x8B8B8B,0x0000,0x0000
};

//Approximates NES color emphasis by dimming the non-emphasized channels to ~0.816 (all three bits dim every channel).
//Emphasis bits: 0 = red, 1 = green, 2 = blue. Colors $xE/$xF aren't affected.
Uint32 emphasizeColor(Uint8 nesColor, Uint8 emphasis) {
    Uint32 color = nes_pal[nesColor];
    Uint32 result = 0;

    if(emphasis == 0 || (nesColor & 0x0F) >= 0x0E) {
        return color;
    }

    for(Uint8 ch = 0; ch < 3; ch++) {
        Uint32 value = (color >> (ch * 8)) & 0xFF;
        if(emphasis == 0x07 || !(emphasis & (1 << ch))) {
            value = (value * 209) >> 8;
        }
        result |= value << (ch * 8);
    }
    return result;
}
