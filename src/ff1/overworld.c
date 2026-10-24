#include <string.h>
#include "types.h"

#include "overworld.h"
#include "battle.h"

Uint16 decompressedMap[256][256];
Uint16 mapPlane[32][32];

Uint8 battleStep = 0xFF;
Uint8 battleStepSign = 0;
int battleCount = -1;
int battleStepSeeded = 0;

void seedEncounterCounter(void) {
    Uint8 seed = readRtcSeed();
    battleCount = seed;
    battleStepSign = seed;
}

void overworld_init() {
    decomp_map();
}

void decomp_map() {

    int offset, ptr;
    int coY, coX, co, run;

    BYTE temp;

    ptr = OVERWORLDMAP_OFFSET;
    for(coY = 0; coY < 256; coY++, ptr += 2){
        Uint8 *mapPtr = (Uint8*) ptr;
        Uint8 ptrVal1 = mapPtr[0];
        Uint8 ptrVal2 = mapPtr[1];
        offset = (ptrVal1 + (ptrVal2 << 8)) + OVERWORLDMAP_BANK_ADD;
        for(coX = 0; coX < 256; offset++){

            Uint8 *tempPtr = (Uint8*) offset;
            temp = tempPtr[0];
            run = tempPtr[1];
            if(temp < 0x80){
                decompressedMap[coY][coX] = temp;
                coX++;}
            else if(temp == 0xFF){
                for(; coX < 256; coX++)
                    decompressedMap[coY][coX] = 0;
            }
            else{
                offset += 1;
                temp -= 0x80;
                if(!run) run = 256;
                for(co = 0; co < run && coX < 256; co++, coX++)
                    decompressedMap[coY][coX] = temp;
            }
        }
    }
}

void update_plane(int map_y, int map_x) {

    int start_map_x = map_x;

    for(int i = 0; i < 32; i++) {
        for(int j = 0; j < 32; j++) {

            if(map_x == 256) {
                map_x = 0;
            }
            if(map_y == 256) {
                map_y = 0;
            }

            if(map_x == -1) {
                map_x = 255;
            }
            if(map_y == -1) {
                map_y = 255;
            }

            Uint16 tileVal = decompressedMap[map_y][map_x];
            Uint8 *palPtr = (Uint8 *)OVERWORLD_PALETTE_ASSIGNMENT;
            Uint8 palette_val = (palPtr[tileVal] & 3);

            Uint16 tile = palette_val << 12;
            tile += tileVal;

            mapPlane[i][j] = tile;
            map_x++;
        }
        map_y++;
        map_x = start_map_x;
    }
} 

BYTE getTileProperty1(int map_y, int map_x){

    Uint16 tile = decompressedMap[map_y][map_x];

    int offset = OVERWORLD_TILEDATA + (tile * 2);
     char *propPtr = (char*) offset;

    BYTE prop = propPtr[0];

    return prop;
}

BYTE getTileProperty2(int map_y, int map_x){
    Uint16 tile = decompressedMap[map_y][map_x];

    int offset = OVERWORLD_TILEDATA + (tile * 2);
     char *propPtr = (char*) offset + 1;

    BYTE prop = propPtr[0];

    return prop;
}

BYTE getTileBattleBackdrop(int map_y, int map_x){
    Uint16 tile = decompressedMap[map_y][map_x];

    int offset = BATTLE_BACKDROP + tile;
     char *propPtr = (char*) offset;

    BYTE prop = propPtr[0];

    return prop;
}

//NES BattleStepRNG.
Uint8 battleStepRNG() {

    if(!battleStepSeeded) {
        battleStep = readRtcSeed();
        battleStepSeeded = 1;
    }

    if(battleStepSign & 0x80) {
        battleStep--;
    } else {
        battleStep++;
    }

    if(battleStep == 0){
        battleStepSign += 0xA0;
    }

    int offset = RNG_TABLE + battleStep;
     char *rngPtr = (char*) offset;

    return rngPtr[0];
}

Uint8 determineBattleDomain(Uint8 pos_x, Uint8 pos_y, int property) {
    Uint8 domain = 0;
    Uint8 x_col = 0;
    Uint8 y_col = 0;

    switch(property) {
        case NORMAL_BATTLE:
            x_col = (pos_x >> 5) & 7;
            y_col = (pos_y >> 2) & 56;
            domain = x_col | y_col;
            break;
        case RIVER_BATTLE_1:
        case RIVER_BATTLE_2:
            if (pos_y >= 0x80) {
                domain = 0x41;
            } else {
                domain = 0x40;
            }
            break;
        case OCEAN_BATTLE:
            domain = 0x42;
            break;
    }

    return domain;
}

BYTE getBattleNum(int domain) {
    if(battleCount == 255) {
        battleCount = -1;
    }
    battleCount++;

    int offset = RNG_TABLE + battleCount;
     char *rngPtr = (char*) offset;

    BYTE rng = rngPtr[0];
    rng = rng & 0x3F;

    offset = BATTLE_FORMAT_WEIGHT_TABLE + rng;
    char *formationPtr = (char*) offset;
    BYTE formation = formationPtr[0];

    offset = BATTLE_LOOKUP_TABLE + (domain * 8) + formation;
    char *battlePtr = (char*) offset;
    BYTE battleNum = battlePtr[0];

    return battleNum;
}
