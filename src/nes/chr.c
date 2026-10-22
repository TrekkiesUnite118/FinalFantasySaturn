#include "chr.h"

void convertNesTilesToSatTiles(Uint8 *src, Uint8 *dest, Uint16 length)
{
    Uint16 destPos = 0;
    Uint8 skip_iter = 0;
    Uint8 first_plane = 0;
    Uint8 second_plane = 0;
    Uint8 byteArray[4] = {0, 0, 0, 0};
    Uint8 bitIter = 0;
    Uint8 byteArrayPos = 0;
    Uint8 bit_a = 0;
    Uint8 bit_b = 0;
    Uint8 bit_c = 0;
    Uint8 bit_d = 0;
    Uint8 byteVal = 0;

    for(Uint16 i = 0; i < length; i++) {
        first_plane = src[i];
        second_plane = src[i + 8];

        bitIter = 0;
        byteArrayPos = 0;
        while(bitIter < 8) {
            bit_a = (first_plane >> bitIter) & 1;
            bit_b = (second_plane >> bitIter) & 1;
            bit_c = (first_plane >> (bitIter + 1)) & 1;
            bit_d = (second_plane >> (bitIter + 1)) & 1;

            byteVal = 0;

            byteVal |= bit_c << 4;
            byteVal |= bit_d << 5;

            byteVal |= bit_a << 0;
            byteVal |= bit_b << 1;
            byteArray[byteArrayPos] = byteVal;
            byteArrayPos++;
            bitIter += 2;
        }

        dest[destPos] = byteArray[3];
        dest[destPos + 1] = byteArray[2];
        dest[destPos + 2] = byteArray[1];
        dest[destPos + 3] = byteArray[0];
        destPos += 4;

        //This is to skip to the next row so we don't process the 2nd plane's bytes again.
        skip_iter++;
        if(skip_iter == 8) {
            i+=8;
            skip_iter = 0;
        }

    }
}
