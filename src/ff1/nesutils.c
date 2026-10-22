#include "nesutils.h"
#include "graphicrefs.h"

void buildMapTiles(Uint8 *src, Uint8 *dest, Uint8* assignmentOffset) {

    Uint8 *assignmentval_a = 0;
    Uint8 *assignmentval_b = 0;
    Uint8 *assignmentval_c = 0;
    Uint8 *assignmentval_d = 0;

    for(Uint8 imagecount = 0; imagecount < 128; imagecount ++)
    {
        assignmentval_a = (assignmentOffset[imagecount] << 5) + src;
        assignmentval_b = (assignmentOffset[imagecount + 128] << 5) + src;
        assignmentval_c = (assignmentOffset[imagecount + 256] << 5) + src;
        assignmentval_d = (assignmentOffset[imagecount + 384] << 5) + src;

        for(Uint8 i = 0; i < 8; i++) {
            dest[0] = assignmentval_a[0];
            dest[1] = assignmentval_a[1];
            dest[2] = assignmentval_a[2];
            dest[3] = assignmentval_a[3];

            dest += 4;
            assignmentval_a += 4;
        }

        for(Uint8 i = 0; i < 8; i++) {
            dest[0] = assignmentval_b[0];
            dest[1] = assignmentval_b[1];
            dest[2] = assignmentval_b[2];
            dest[3] = assignmentval_b[3];

            dest += 4;
            assignmentval_b += 4;
        }

        for(Uint8 i = 0; i < 8; i++) {
            dest[0] = assignmentval_c[0];
            dest[1] = assignmentval_c[1];
            dest[2] = assignmentval_c[2];
            dest[3] = assignmentval_c[3];

            dest += 4;
            assignmentval_c += 4;
        }

        for(Uint8 i = 0; i < 8; i++) {
            dest[0] = assignmentval_d[0];
            dest[1] = assignmentval_d[1];
            dest[2] = assignmentval_d[2];
            dest[3] = assignmentval_d[3];

            dest += 4;
            assignmentval_d += 4;
        }
    }

}

void buildMapSprites(Uint8 *src, Uint8 *dest, Uint16 size) {

    Uint8 *tile_a = 0;
    Uint8 *tile_b = 0;
    
    Uint8 rowIter = 0;
    Uint8 sourceIter = 0;

     for(Uint16 i = 0; i < size; i += 4) {
        tile_a = src + i;
        tile_b = (src + i) + 0x20;

        for(Uint8 iter = 0; iter < 8; iter++) {
            if(iter < 4) {
                dest[iter] = tile_a[sourceIter];
            } else{
                dest[iter] = tile_b[sourceIter];
            } 
            sourceIter++;
            if(sourceIter >= 4) {
                sourceIter = 0;
            }
        }

        dest +=8;
        rowIter++;
        if(rowIter == 8) {
            i+= 0x20;
            rowIter = 0;
        }
     }
}

void buildWeaponSprites(Uint8 *src, Uint8 *dest) {

    Uint8 *tile_a = 0;
    Uint8 *tile_b = 0;
    Uint8 *tile_c = 0;
    Uint8 *tile_d = 0;

    Uint8 sourceIter = 0;

    for(Uint16 i = 0; i < 32; i ++) {
        tile_a = src;
        tile_b = src + 0x20;
        tile_c = src + 0x40;
        tile_d = src + 0x60;

        for(Uint8 rowIter = 0; rowIter < 8; rowIter++) {
            for(Uint8 iter = 0; iter < 8; iter++) {
                if(iter < 4) {
                    dest[iter] = tile_a[sourceIter + (4 * rowIter)];
                } else{
                    dest[iter] = tile_b[sourceIter + (4 * rowIter)];
                } 
                sourceIter++;
                if(sourceIter >= 4) {
                    sourceIter = 0;
                }
            }
            dest += 8;
        }

        for(Uint8 rowIter = 0; rowIter < 8; rowIter++) {
            for(Uint8 iter = 0; iter < 8; iter++) {
                if(iter < 4) {
                    dest[iter] = tile_c[sourceIter + (4 * rowIter)];
                } else{
                    dest[iter] = tile_d[sourceIter + (4 * rowIter)];
                } 
                sourceIter++;
                if(sourceIter >= 4) {
                    sourceIter = 0;
                }
            }
            dest += 8;
        }
        
        src += 0x80;
    }
}

void buildBattleSprites(Uint8 *src, Uint8 *dest) {

    Uint8 *tile_a = 0;
    Uint8 *tile_b = 0;
    Uint8 *tile_c = 0;
    Uint8 *tile_d = 0;
    Uint8 *tile_e = 0;
    Uint8 *tile_f = 0;

    Uint8 sourceIter = 0;

    for(int i = 0; i < 12; i++) {
        tile_a = src;
        tile_b = src + 0x20;
        tile_c = src + 0x40;
        tile_d = src + 0x60;
        tile_e = src + 0x80;
        tile_f = src + 0xA0;

        for(int frameIter = 0; frameIter < 6; frameIter++) {
        
            sourceIter = 0;
            if (frameIter == 5) {
                for(Uint8 rowIter = 0; rowIter < 8; rowIter++) {
                    for(Uint8 iter = 0; iter < 12; iter++) {
                        if(iter < 4) {
                            dest[iter] = tile_a[sourceIter + (4 * rowIter)];
                        } else if (iter >= 4 && iter < 8) {
                            dest[iter] = tile_b[sourceIter + (4 * rowIter)];
                        } else {
                            dest[iter] = tile_c[sourceIter + (4 * rowIter)];
                        }
                        sourceIter++;
                        if(sourceIter >= 4) {
                            sourceIter = 0;
                        }
                    }
                    dest += 12;
                }

                for(Uint8 rowIter = 0; rowIter < 8; rowIter++) {
                    for(Uint8 iter = 0; iter < 12; iter++) {
                        if(iter < 4) {
                            dest[iter] = tile_d[sourceIter + (4 * rowIter)];
                        } else if (iter >= 4 && iter < 8) {
                            dest[iter] = tile_e[sourceIter + (4 * rowIter)];
                        } else {
                            dest[iter] = tile_f[sourceIter + (4 * rowIter)];
                        }
                        sourceIter++;
                        if(sourceIter >= 4) {
                            sourceIter = 0;
                        }
                    }
                    dest += 12;
                }
            

            } else {
                
                for(Uint8 rowIter = 0; rowIter < 8; rowIter++) {
                    for(Uint8 iter = 0; iter < 8; iter++) {
                        if(iter < 4) {
                            dest[iter] = tile_a[sourceIter + (4 * rowIter)];
                        } else{
                            dest[iter] = tile_b[sourceIter + (4 * rowIter)];
                        } 
                        sourceIter++;
                        if(sourceIter >= 4) {
                            sourceIter = 0;
                        }
                    }
                    dest += 8;
                }

                for(Uint8 rowIter = 0; rowIter < 8; rowIter++) {
                    for(Uint8 iter = 0; iter < 8; iter++) {
                        if(iter < 4) {
                            dest[iter] = tile_c[sourceIter + (4 * rowIter)];
                        } else{
                            dest[iter] = tile_d[sourceIter + (4 * rowIter)];
                        } 
                        sourceIter++;
                        if(sourceIter >= 4) {
                            sourceIter = 0;
                        }
                    }
                    dest += 8;
                }

                
                for(Uint8 rowIter = 0; rowIter < 8; rowIter++) {
                    for(Uint8 iter = 0; iter < 8; iter++) {
                        if(iter < 4) {
                            dest[iter] = tile_e[sourceIter + (4 * rowIter)];
                        } else{
                            dest[iter] = tile_f[sourceIter + (4 * rowIter)];
                        } 
                        sourceIter++;
                        if(sourceIter >= 4) {
                            sourceIter = 0;
                        }
                    }
                    dest += 8;
                }

                if(frameIter + 1 == 1) {
                    tile_e += 0x40;
                    tile_f += 0x40;
                } else if(frameIter + 1 == 2) {
                    tile_a += 0x100;
                    tile_b += 0x100;
                    tile_c += 0x100;
                    tile_d += 0x100;
                    tile_e += 0xC0;
                    tile_f += 0xC0;
                } else {
                    tile_a += 0xC0;
                    tile_b += 0xC0;
                    tile_c += 0xC0;
                    tile_d += 0xC0;
                    tile_e += 0xC0;
                    tile_f += 0xC0;
                }
            }

        }
        src += 0x400;
    }

}


