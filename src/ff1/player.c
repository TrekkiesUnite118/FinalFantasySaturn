#include "types.h"
#include "video.h"
#include "sys.h"
#include <string.h>
#include "cd.h"
#include "memmap.h"
#include "player.h"
#include "print.h"
#include "overworld.h"
#include "rbgscroll.h"
#include "sprdata.h"
#include "vblank.h"
#include "battle.h"
#include "menu.h"
#include "static.h"
#include "magic.h"
#include "stdmap.h"
#include "shop.h"
#include "wipe.h"
#include "nesutils.h"
#include "save.h"
#include "music.h"
#include "mainmenu.h"
#include "story.h"
#include "bugfix.h"


#define DOWN (FIXED(0))
#define UP (FIXED(1))
#define LEFT (FIXED(2))
#define RIGHT (FIXED(3))

#define WARRIOR_START_STAT_OFFSET (0x00203050)
#define THIEF_START_STAT_OFFSET (0x00203060)
#define MONK_START_STAT_OFFSET (0x00203070)
#define REDMAGE_START_STAT_OFFSET (0x00203080)
#define WMAGE_START_STAT_OFFSET (0x00203090)
#define BMAGE_START_STAT_OFFSET (0x002030A0)

#define WARRIOR_VALUE (0)
#define THIEF_VALUE (1)
#define MONK_VALUE (2)
#define REDMAGE_VALUE (3)
#define WMAGE_VALUE (4)
#define BMAGE_VALUE (5)
#define KNIGHT_VALUE (6)
#define NINJA_VALUE (7)
#define MASTER_VALUE (8)
#define REDWIZ_VALUE (9)
#define WHWIZ_VALUE (10)
#define BLWIZ_VALUE (11)

#define WARRIOR_OFFSET (0)
#define THIEF_OFFSET (8)
#define MONK_OFFSET (16)
#define REDMAGE_OFFSET (24)
#define WMAGE_OFFSET (32)
#define BMAGE_OFFSET (40)
#define KNIGHT_OFFSET (48)
#define NINJA_OFFSET (56)
#define MASTER_OFFSET (64)
#define REDWIZ_OFFSET (72)
#define WHWIZ_OFFSET (80)
#define BLWIZ_OFFSET (88)

#define OWTP_DOCKSHIP (0x20)
#define OWTP_FOREST (0x10)
#define OWTP_AIRSHIP_NOLAND (0x08)
#define OWTP_SPEC_MASK (0xC0)
#define OWTP_SPEC_CHIME (0x40)
#define OWTP_SPEC_CARAVAN (0x80)
#define OBJID_FAIRY (0x13)

//NES vehicle values, also the tileprop bits that block each one.
#define VEH_FOOT (0x01)
#define VEH_CANOE (0x02)
#define VEH_SHIP (0x04)
#define VEH_AIRSHIP (0x08)

#define PLAYER_MAXXPOS (FIXED(152))
#define PLAYER_MAXYPOS (FIXED(108))
#define PLAYER_MAXYPOSB (FIXED(116))

//NES sprite coords on the overworld are these offsets from the Saturn screen.
#define OW_SPR_X_ADD (40)
#define OW_SPR_Y_ADD (0)

//NES LoadOWObjectCHR, sprite tiles $10-$6F, as 16x8 halves in the standard map object slots.
#define OW_OBJ_CHR (0x00209C10)
#define OW_OBJ_CHR_SIZE (0x600)
#define OW_OBJ_TILE_START (0x10)
#define OW_OBJ_CHAR_START (384)
#define OW_VEHICLE_CRAM (160) //NES sprite palette 2.
#define OW_OBJECT_CRAM (176) //NES sprite palette 3.
#define OW_SPRITE_PAL (0x002003A0)

//Tile additives for the vehicle and object tables.
#define OW_TILE_SHIP (0x20)
#define OW_TILE_AIRSHIP (0x38)
#define OW_TILE_CANOE (0x50)
#define OW_TILE_SHADOW (0x10)
#define OW_TILE_BRIDGE (0x14)
#define OW_TILE_CANAL (0x18)

//NES lut_VehicleSprY.
#define OW_SPR_Y_FOOT (0x6C)
#define OW_SPR_Y_BOAT (0x6F)
#define OW_SPR_Y_AIRSHIP (0x4F)
#define OW_SPR_X_PLAYER (0x70)

enum {
    OW_AIR_NONE,
    OW_AIR_TAKEOFF,
    OW_AIR_LANDING
};

//Map tilt while flying, in degrees around the X axis.
#define OW_TILT_ANGLE (-55)
//The takeoff and landing move the airship every other frame.
#define OW_AIR_ANIM_FRAMES ((OW_SPR_Y_BOAT - OW_SPR_Y_AIRSHIP) * 2)

#define FRAME_STAND (169)
#define FRAME_WALK1 (FRAME_STAND + 1)
#define FRAME_STAND2 (FRAME_STAND + 2)
#define FRAME_WALK2 (FRAME_STAND + 3)

#define FRAME_STAND3 (FRAME_STAND + 4)
#define FRAME_STAND4 (FRAME_STAND + 5)

#define FRAME_WALK3 (FRAME_STAND + 6)
#define FRAME_WALK4 (FRAME_STAND + 7)

#define FRAME_BATTLE_START (96)

#define CONTINUE_POS (FIXED(88))
#define NEW_GAME_POS (FIXED(128))

#define NAME_LETTER_MIN_X (FIXED(64))
#define NAME_LETTER_MIN_Y (FIXED(80))

#define NAME_LETTER_MAX_X (FIXED(208))
#define NAME_LETTER_MAX_Y (FIXED(176))

#define BAT_MENU_MIN_X (FIXED(120))
#define BAT_MENU_MAX_X (FIXED(168))

#define BAT_MENU_MIN_Y (FIXED(158))
#define BAT_MENU_MAX_Y (FIXED(206))

const Uint16 START_POS_X = 153;
const Uint16 START_POS_Y = 165;

Uint8 current_x_pos;
Uint8 current_y_pos;

Uint8 prev_current_x_pos;
Uint8 prev_current_y_pos;

int prev_bat_menu_y_pos = BAT_MENU_MIN_Y;
int prev_bat_menu_x_pos = BAT_MENU_MIN_X;

Uint8 vehicle = VEH_FOOT;
static Uint8 vehicleNext = VEH_FOOT;
static Uint8 vehChgPause = 0;
static Uint8 owFacing = 0; //NES facing: the direction bits held when the move started.
static Uint8 owMoveSpeed = 0;
static Uint8 owMoveCtr = 0; //NES move_ctr_x/y, only one axis moves at a time.
static Uint8 owTileprop = 0;
static Uint8 owTileprop1 = 0;
static Uint8 owEnteringShop = FALSE;
static Uint8 owBtlFormation = 0;
static Uint8 owInForest = FALSE;
static Uint8 owJoyA = FALSE;
static Uint8 owJoyB = 0; //NES joy_b, counting B presses since the overworld was entered.
static Uint8 owCanPoll = FALSE;
static Uint8 owAirAnim = OW_AIR_NONE;
static Uint8 owAirAnimY = 0;
static Fixed32 owTilt = 0;
static Uint8 owAirFrames = 0;
static Uint8 owAirshipShown = FALSE;

Uint8 currentTurn = 0;
Uint8 currentTurnBufferIndex = 0;
Uint8 isValidTurn = FALSE;
Uint8 targetIndex = 0;
Uint8 ineffective = FALSE;

Uint8 explosionCounter = 0;

SPRITE_INFO player;
SPRITE_INFO playerb;
SPRITE_INFO cursor;


SPRITE_INFO playerBat1;
SPRITE_INFO playerBat2;
SPRITE_INFO playerBat3;
SPRITE_INFO playerBat4;
SPRITE_INFO weaponBat;
SPRITE_INFO explosionBat[3];
Uint8 explosionSlotsDrawn = 0;
Uint8 explosionSlot = 0; //frameCount can't be used here, vblank increments it.
Uint8 battleEndPhase = BATTLE_END_CHEER;
Uint8 battleEndFrame = 0;
Uint8 battleTransitionFrame = 0;

Uint8 victoryStep = VICTORY_PERISHED;
Uint16 eobExpReward;
Uint16 eobGoldReward;
Uint8 eobRewardHighByte; //NES battlereward+2, left over from the GP sum and added to EXP.
Uint8 lvlUpChar;
Uint16 lvlUpPrintHp;
Uint8 statUp[5];
Uint8 statMsgIndex;

Uint8 isMoving = FALSE;
Uint8 battleAnimFrame = 0;
Uint8 weaponVisible = FALSE;
Uint8 totalMovement = 0;
Uint8 changeToBat = 0;
Uint8 responseRate = 0; //0-based like the NES, shown as 1-8.
Uint8 runFailed = FALSE;
Uint8 statusTurn = FALSE; //Turn spent on a sleep/stun/confusion check.
Uint8 turnKilledEnemy = FALSE;
Uint8 statusMsg = 0;
Uint8 enemyRanAway = FALSE;
static Uint8 statusDrawFrame;
static Uint8 statusDrawNext; //turn_exec_state after the status redraw.

//Magic menu.
static Uint8 magicPage = 0;
static Uint8 magicCursorX = 0;
static Uint8 magicCursorY = 0;
static Uint8 selectedSpell = 0; //0-based spell ID.
static Uint8 selectedMpLevel = 0;
//NES btlcmd_magicgfx palette, per party slot. Set when a spell is picked and never cleared,
//so DRINK and ITEM use whatever was left there.
static Uint8 lastMagicPalette[4] = {0, 0, 0, 0};

//DRINK and ITEM menus. The cursor uses magicCursorX/Y.
static Uint8 battlePotions[2]; //NES btl_potion_heal/pure, potions not yet picked this battle.
static Uint8 drinkReserved[4] = {0xFF, 0xFF, 0xFF, 0xFF}; //Potion each character picked this round.
static Uint8 selectedPotion = 0;

//Magic turn.
enum __magicstep__ {
    MAGIC_STEP_NEXT_TARGET,
    MAGIC_STEP_HIT, //Explosion on an enemy, or the flash on a player.
    MAGIC_STEP_CAST,
    MAGIC_STEP_OPS,
    MAGIC_STEP_TARGET_DONE,
    MAGIC_STEP_CLEAR,
    MAGIC_STEP_END_DELAY
};

enum __magictargetmode__ {
    MAGIC_MODE_ALL_ENEMIES,
    MAGIC_MODE_ONE_ENEMY,
    MAGIC_MODE_SELF,
    MAGIC_MODE_ALL_PARTY,
    MAGIC_MODE_ONE_ALLY,
    //Enemy casters.
    MAGIC_MODE_ALL_PLAYERS,
    MAGIC_MODE_RAND_PLAYER,
    MAGIC_MODE_ENEMY_SELF,
    MAGIC_MODE_ALL_ALLIES, //Skips the caster, an NES bug.
    MAGIC_MODE_RAND_ALLY
};

static Uint8 magicTurn = FALSE;
static Uint8 magicCasterIsEnemy = FALSE;
static Uint8 magicConfusedFire = FALSE;
//NES bug: a confused FIRE turn skips the battle end check, so it waits for a later turn's
//defender to be dead or stoned, or the end of the round.
static Uint8 battleEndCheckSkipped = FALSE;
static Uint8 turnDefenderDown = FALSE;
static Uint8 magicNoDraw = FALSE; //No magic sprite or flash, for spells without a battle effect.
static Uint8 magicAbort = FALSE; //The turn ends after its messages, without casting on a target.
static Uint8 magicSpell;
static Uint8 magicGfx;
static Uint8 magicFlashColor;
static Uint8 magicMode;
static Uint8 magicStep;
static Uint8 magicNextTarget;
static Uint8 magicDefender;
static Uint8 magicDefIsPlayer;
static Uint8 magicBoxCount; //NES btl_combatboxcount_alt
static Uint8 magicOpIndex;
static Uint8 magicOpStarted;
static Uint8 magicFlashChar = 0xFF;

static void moveSubMenuCursor(Sint8 dx, Sint8 dy);
static void setItemCursor();
static void startOwPalCycle(Uint8 reverse, Uint8 nextState);
static void owWipeClosedToSM(void);
static void owUpdate(void);
static void owFinishFrame(void);

static Uint8 isSubMenuState() {
    return bat_menu_state == SELECT_MAGIC || bat_menu_state == SELECT_PARTY_TARGET
        || bat_menu_state == SELECT_DRINK || bat_menu_state == SELECT_ITEM;
}

int direction = DOWN;
int currentChar = WARRIOR_OFFSET;
int currentBattleState = 0;

Uint16 MIRROR_ANIM_1 = MIRROR_HORIZ;
Uint16 MIRROR_ANIM_2 = 0;

Uint8 cur_char_create = 0;
Uint8 cur_letter = 0;
Uint8 cur_letter_index = 0;
Uint8 nameBuffer[7] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};
Uint8 selected_class = 0;
Uint8 cur_char_battle_menu = 0;
Uint8 cur_battle_menu_selection = FIGHT;
Uint8 prev_battle_menu_selection = FIGHT;
Uint8 cur_battle_enemy_target = 0;
Uint8 cur_battle_party_target = 0;
Uint8 weapon_char = 0;

Sint8 currentSlot = 0;

Uint8 ignoreA = FALSE;
Uint8 ignoreB = FALSE;
Uint8 ignoreC = FALSE;
Uint8 ignoreX = FALSE;
Uint8 ignoreY = FALSE;
Uint8 ignoreS = FALSE;
Uint8 ignoreRB = FALSE;
Uint8 ignoreLB = FALSE;

//R stands in for Select (lineup) and L for Select+B (minimap).
#define OW_NO_MENU 0xFF
static Uint8 owStartLatched = FALSE;
static Uint8 owSelectNext = OW_NO_MENU;
static Uint8 owPalCycleIn = FALSE;
static Uint8 palCycFrame;
static Uint8 palCycReverse;
static Uint8 palCycNextState;

Uint8 *response_rate_lut = (Uint8 *)0x0022DA32;

Uint8 drawBattleCursor = FALSE;
Uint8 drawTargetNames = FALSE;

Uint8 rotated = FALSE;

Uint8 char_name_screen_lookup_table[70] =  { 0x8A, 0x8B, 0x8C, 0x8D, 0x8E, 0x8F, 0x90, 0x91, 0x92, 0x93,
                                            0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9A, 0x9B, 0x9C, 0x9D,
                                            0x9E, 0x9F, 0xA0, 0xA1, 0xA2, 0xA3, 0xBE, 0xBF, 0xC0, 0xFF,
                                            0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
                                            0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xAB, 0xAC, 0xAD,
                                            0xAE, 0xAF, 0xB0, 0xB1, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 0xB7,
                                            0xB8, 0xB9, 0xBA, 0xBB, 0xBC, 0xBD, 0xC2, 0xC3, 0xC4, 0xC5};

Uint32 pal_3[] = {
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
};
Uint32 pal_4[] = {
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
};
Uint32 pal_5[] = {
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
};
Uint32 pal_6[] = {
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,
};

Uint8 getResponseRate() {
    return responseRate;
}

void player_cursor_init(Fixed32 startX, Fixed32 startY) {

    if(state == TO_CHAR_SELECT_STATE) {

        if(cur_char_create > 3) {
            startX = FIXED(360);
            startY = FIXED(256);
        } else {
            startX = FIXED(getPartyCreateCursorPosX(cur_char_create));
            startY = FIXED(getPartyCreateCursorPosY(cur_char_create));
        }
    }

    sprite_make(cursorSpriteIndex,  startX,  startY, (Sint32) CURSOR_PRIORITY, &cursor);
    cursor.xSize = FIXED(16);
    cursor.ySize = FIXED(16);
    
}

void player_cursor_draw() {
    SPRITE_INFO temp;
    memcpy(&temp, &cursor, sizeof(temp));;

    sprite_draw(&temp);
}

void draw_response_rate() {
    Uint8 responseBuff[2] = {responseRate + 0x81, 0x00};
    drawTextLine(22, 22, responseBuff, 0, 1);
}

void draw_char_name() {
     drawTextLine(13, 4, nameBuffer, 1, 1);
}

void update_player_sprites() {

    Uint16 owsprite_size = 64;
    Uint16 owsprite_width = 16;
    Uint16 owsprite_height = 8;

    Uint8 curr_class = determine_curr_char_class();

    Uint8 *palPtr_a = (Uint8*) MAPMAN_SPRITE_PALETTE_OFFSET;
    Uint8 *palPtr_b = (Uint8*) MAPMAN_SPRITE_PALETTE_OFFSET_2 + (2 * curr_class);


    initMapManPalette(pal_3, pal_4, palPtr_a, palPtr_b);

    vidSetPalette(VID_SPR, mapSpriteCramStart, 16, &pal_3);
    vidSetPalette(VID_SPR, mapSpriteCramStart + 16, 16, &pal_4);

    for (int i = 0; i < 8; i++) {
        sprSetChar(i + FRAME_STAND + currentChar, SPRITE_4BPP, mapSpriteCramStart, owsprite_width, owsprite_height, (Uint8 *)(LWRAM_MAP_SPRITES) + ((i * owsprite_size) + (currentChar * owsprite_size)));
        i++;
        sprSetChar(i + FRAME_STAND + currentChar, SPRITE_4BPP, mapSpriteCramStart + 16, owsprite_width, owsprite_height, (Uint8 *)(LWRAM_MAP_SPRITES) + ((i * owsprite_size) + (currentChar * owsprite_size)));
    }
    player.char_num = FRAME_STAND + currentChar;
    playerb.char_num = FRAME_WALK1 + currentChar;
}

void player_ow_init(Fixed32 startX, Fixed32 startY) {

    sprite_make(FRAME_STAND + currentChar,  startX,  startY, (Sint32) CHARACTER_PRIORITY, &player);
    sprite_make(FRAME_WALK1 + currentChar,  startX,  startY + 8, (Sint32) CHARACTER_PRIORITY, &playerb);
    player.xSize = FIXED(16);
    player.ySize = FIXED(8);
    playerb.xSize = FIXED(16);
    playerb.ySize = FIXED(8);
    MIRROR_ANIM_2 &= ~MIRROR_HORIZ;

    current_x_pos = START_POS_X;
    current_y_pos = START_POS_Y;
    
    prev_current_x_pos = current_x_pos;
    prev_current_y_pos = current_y_pos;

}

void resetPlayerActions() {
    for(Uint8 i = 0; i < 4; i++) {
        playerActions[i].action = NO_ACTION;
        drinkReserved[i] = 0xFF;
    }
}

void player_bat_init() {

    currentTurnBufferIndex = 0;
    battleEndCheckSkipped = FALSE;
    battlePotions[0] = items[ITEM_HEAL];
    battlePotions[1] = items[ITEM_PURE];
    resetPlayerActions();

    Uint8 *startXPos = (Uint8 *)BATTLE_CHAR_DEFAULT_XPOS;
    Uint8 *startYPos = (Uint8 *)BATTLE_CHAR_DEFAULT_YPOS_1;

    battleAnimFrame = 0;
    weaponVisible = FALSE;
    sprite_make(FRAME_BATTLE_START + (party[0].char_class * 6), FIXED(startXPos[0] + 24), FIXED(startYPos[0]), (Sint32) CHARACTER_PRIORITY, &playerBat1);
    sprite_make(FRAME_BATTLE_START + (party[1].char_class * 6), FIXED(startXPos[0] + 24), FIXED(startYPos[5]), (Sint32) CHARACTER_PRIORITY, &playerBat2);
    sprite_make(FRAME_BATTLE_START + (party[2].char_class * 6), FIXED(startXPos[0] + 24), FIXED(startYPos[10]), (Sint32) CHARACTER_PRIORITY, &playerBat3);
    sprite_make(FRAME_BATTLE_START + (party[3].char_class * 6), FIXED(startXPos[0] + 24), FIXED(startYPos[15]), (Sint32) CHARACTER_PRIORITY, &playerBat4);

    playerBat1.xSize = FIXED(16);
    playerBat1.ySize = FIXED(24);
    playerBat2.xSize = FIXED(16);
    playerBat2.ySize = FIXED(24);
    playerBat3.xSize = FIXED(16);
    playerBat3.ySize = FIXED(24);
    playerBat4.xSize = FIXED(16);
    playerBat4.ySize = FIXED(24);

    
    sprite_make(weaponSpriteIndex, FIXED(startXPos[0] + 16), FIXED(startYPos[0]) - 8, (Sint32) WEAPON_PRIORITY, &weaponBat);
    
    for(Uint8 i = 0; i < 3; i++) {
        sprite_make(explosionSpriteIndex, FIXED(startXPos[0] + 16), FIXED(startYPos[0]) - 8, (Sint32) WEAPON_PRIORITY, &explosionBat[i]);
    }
    explosionSlotsDrawn = 0;
}

void player_bat_stand() {

    for(Uint8 i = 0; i < 4; i++) {
        if(battle_char_stats[i].hp > 0) {
            player_stand(i);
        }
    }
}

void party_flip() {

    for(Uint8 i = 0; i < 4; i++) {
        if(battle_char_stats[i].hp > 0) {
            player_flip(i);
        }
    }
}

void char_init(int classValue, int partyNum, char newName[7]) {

    int startingStatOffset;
    PLAYER_CHARACTER character;
    character.char_class = classValue;

    switch(classValue) {
        case WARRIOR_VALUE:
            startingStatOffset = WARRIOR_START_STAT_OFFSET;
            break;
        case THIEF_VALUE:
            startingStatOffset = THIEF_START_STAT_OFFSET;
            break;
        case MONK_VALUE:
            startingStatOffset = MONK_START_STAT_OFFSET;
            break;
        case REDMAGE_VALUE:
            startingStatOffset = REDMAGE_START_STAT_OFFSET;
            break;
        case WMAGE_VALUE:
            startingStatOffset = WMAGE_START_STAT_OFFSET;
            break;
        case BMAGE_VALUE:
            startingStatOffset = BMAGE_START_STAT_OFFSET;
            break;
        default:
            startingStatOffset = WARRIOR_START_STAT_OFFSET;
            break;
    }

    Uint8 *statPtr = (Uint8*) startingStatOffset;
    Uint8 startHP = statPtr[1];
    Uint8 startStr = statPtr[2];
    Uint8 startAgi = statPtr[3];
    Uint8 startInt = statPtr[4];
    Uint8 startVit = statPtr[5];
    Uint8 startLck = statPtr[6];
    Uint8 startDmg = statPtr[7];
    Uint8 startHit = statPtr[8];
    Uint8 startEva = statPtr[9];
    Uint8 startMDef = statPtr[10];

    character.current_hp = startHP;
    character.max_hp = startHP;
    character.str = startStr;
    character.agi = startAgi;
    character.intel = startInt;
    character.vit = startVit;
    character.luck = startLck;
    character.damage = startDmg;
    character.hit_percent = startHit;
    character.evade_percent = startEva;
    character.ailment = 0;
    character.absorb = 0;
    character.resist = 0;
    character.mag_def = startMDef;
    character.name[0] = newName[0];
    character.name[1] = newName[1];
    character.name[2] = newName[2];
    character.name[3] = newName[3];
    character.name[4] = newName[4];
    character.name[5] = newName[5];
    character.name[6] = newName[6];
    character.level = 1;
    character.exp = 0;
    character.exp_to_next_lvl = 40;

    for(int mp_iter = 0; mp_iter < 8; mp_iter++) {
        character.current_mp[mp_iter] = 0;
        character.max_mp[mp_iter] = 0;
    }

    if(classValue != WARRIOR_VALUE && classValue != THIEF_VALUE && classValue != MONK_VALUE){
        character.current_mp[0] = 2;
        character.max_mp[0] = 2;
    }

    //NES GameStart zeroes the stats page, so nothing is equipped or owned.
    for(int i = 0; i < 4; i++) {
        character.weapons[i] = 0;
        character.armors[i] = 0;
    }

    for(int maglevel = 0; maglevel < 8; maglevel++) {
        for(int magslot = 0; magslot < 3; magslot++) {
            character.magic[maglevel][magslot] = 0;
        }
    }

    party[partyNum] = character;
}

//NES EnterTitleScreen and PtyGen_Joy: new direction presses, and A or B in party creation.
static void titlePtyGenJoy() {
    static PadBits prevPad;
    static Uint8 prevDir;
    PadBits pressed = PadData1 & ~prevPad;
    Uint8 dir = 0;
    Uint8 newDir;

    prevPad = PadData1;
    if(PadData1 & PAD_R) dir |= 0x01;
    if(PadData1 & PAD_L) dir |= 0x02;
    if(PadData1 & PAD_D) dir |= 0x04;
    if(PadData1 & PAD_U) dir |= 0x08;
    newDir = dir != 0 && dir != prevDir;
    prevDir = dir;

    if(state == TITLE_STATE) {
        if(newDir) {
            //Up and down play the select sound.
            if(dir >= 0x04) {
                sfxMenuSel();
            } else {
                //Anything but Right alone (Left, or both) lowers the rate. It wraps 0<->7.
                responseRate = (responseRate + ((dir == 0x01) ? 1 : -1)) & 7;
                sfxMenuMove();
            }
        }
    } else if(state == CHAR_SELECT_STATE || state == CHAR_NAME_STATE) {
        if(pressed & (PAD_A | PAD_B | PAD_C)) {
            sfxMenuSel();
        } else if(newDir) {
            sfxMenuMove();
        }
    }
}

void player_input() {

    static Uint8 resetHeld = FALSE;

    titlePtyGenJoy();
    PadData1E = 0;
    //A+B+C+Start exits to the CD player from the prelude and title, and returns to the prelude anywhere else.
    if(PadData1 == (PAD_A | PAD_B | PAD_C | PAD_S)) {
        if(!resetHeld && !saveBusy) {
            resetHeld = TRUE;
            if(state == TO_PRELUDE || state == PRELUDE || state == TO_TITLE_STATE || state == TITLE_STATE
               || state == TO_BOOT_MSG || state == BOOT_MSG || state == TO_SAD || state == SAD) {
                sysReset();
            }
            softResetPending = TRUE;
        }
    } else {
        resetHeld = FALSE;
    }

    if(!(PadData1 & PAD_A)) {
        ignoreA = 0;
    }
    if(!(PadData1 & PAD_B)) {
        ignoreB = 0;
    }
    if(!(PadData1 & PAD_C)) {
        ignoreC = 0;
    }
    
    if(!(PadData1 & PAD_X)) {
        ignoreX = 0;
    }

    if(!(PadData1 & PAD_S)) {
        ignoreS = 0;
    }

    if(!(PadData1 & PAD_Y)) {
        ignoreY = 0;
    }

    if(!(PadData1 & PAD_RB)) {
        ignoreRB = 0;
    }

    if(!(PadData1 & PAD_LB)) {
        ignoreLB = 0;
    }

    //The NES doesn't read input while battle boxes are drawn.
    if(msgBusy()) {
        return;
    }

    if(state == OW_STATE) {
        owUpdate();
        if(state != OW_STATE) {
            return;
        }
    }

    //The NES doesn't poll the pad mid-step, so a button still held when the step ends is caught then.
    Uint8 owPolling = !(state == OW_STATE && !owCanPoll);

    if(PadData1 & PAD_S && !ignoreS && owPolling) {
        if(state == PRELUDE) {
            state = TO_TITLE_STATE;
        } else if(state == OW_STATE) {
            owStartLatched = TRUE;
        }
        ignoreS = 1;
    }

    if(PadData1 & PAD_RB && !ignoreRB && owPolling) {
        if(state == OW_STATE) {
            owSelectNext = TO_LINEUP_MENU;
        }
        ignoreRB = 1;
    }

    if(PadData1 & PAD_LB && !ignoreLB && owPolling) {
        if(state == OW_STATE) {
            owSelectNext = TO_MINIMAP;
        }
        ignoreLB = 1;
    }

    if (PadData1 & PAD_A && !ignoreA && owPolling) {

        performActionButtonInput();
        ignoreA = 1;
    }
    if (PadData1 & PAD_B && !ignoreB && owPolling) {
        if(state == OW_STATE) {
            owJoyB++;
        }
        performCancelButtonInput();
        ignoreB = 1;
    }
    if (PadData1 & PAD_C && !ignoreC && owPolling) {
        performActionButtonInput();
        ignoreC = 1;
    }

    if (PadData1 & PAD_L) {    
    
        if(state == CHAR_SELECT_STATE) {
            if(!canUpdateValue){
                decrementPartyInitBuff(cur_char_create);
                if(party[cur_char_create].char_class == 0) {
                    party[cur_char_create].char_class = 5;
                    selected_class = 5;
                } else {
                    party[cur_char_create].char_class--;
                    selected_class--;
                }
                
                frame_count_a = 0;
                canUpdateValue = 1;
                
            }
        } else if(state == CHAR_NAME_STATE) {
            if(!canMoveCursor){
                if(cursor.xPos > NAME_LETTER_MIN_X) {
                    cur_letter--;
                    cursor.xPos -= FIXED(16);
                }
                frame_count_b = 0;
                canMoveCursor = 1;
            }
        } else if (state == BAT_MENU_STATE && bat_menu_state == SELECT) {
            if(!canMoveCursor) {
                if(cursor.xPos == BAT_MENU_MIN_X) {
                    //Do Nothing, can't move.
                } else {
                    cursor.xPos = BAT_MENU_MIN_X;
                    cursor.yPos = prev_bat_menu_y_pos;
                    cur_battle_menu_selection = prev_battle_menu_selection;
                }
                frame_count_b = 0;
                canMoveCursor = 1;
            }
        } else if (state == BAT_MENU_STATE && isSubMenuState()) {
            if(!canMoveCursor) {
                moveSubMenuCursor(-1, 0);
                frame_count_b = 0;
                canMoveCursor = 1;
            }
        }

    }
    else if (PadData1 & PAD_R) {

        if(state == CHAR_SELECT_STATE) {
            if(!canUpdateValue){
                incrementPartyInitBuff(cur_char_create);
                if(party[cur_char_create].char_class == 5) {
                    party[cur_char_create].char_class = 0;
                    selected_class = 0;
                } else {
                    party[cur_char_create].char_class++;
                    selected_class++;
                }
                frame_count_a = 0;
                canUpdateValue = 1;
                
            }
        } else if(state == CHAR_NAME_STATE) {
            if(!canMoveCursor){
                if(cursor.xPos < NAME_LETTER_MAX_X) {
                    cur_letter++;
                    cursor.xPos += FIXED(16);
                }
                frame_count_b = 0;
                canMoveCursor = 1;
            }
        } else if (state == BAT_MENU_STATE && bat_menu_state == SELECT) {
            if(!canMoveCursor) {
                if(cursor.xPos == BAT_MENU_MIN_X) {
                   prev_battle_menu_selection = cur_battle_menu_selection;  
                   cur_battle_menu_selection = RUN;
                   prev_bat_menu_y_pos = cursor.yPos;
                   cursor.xPos = BAT_MENU_MAX_X;
                   cursor.yPos = BAT_MENU_MIN_Y;
                } else {
                   //Do nothing, can't move.
                }
                frame_count_b = 0;
                canMoveCursor = 1;
            }
        } else if (state == BAT_MENU_STATE && isSubMenuState()) {
            if(!canMoveCursor) {
                moveSubMenuCursor(1, 0);
                frame_count_b = 0;
                canMoveCursor = 1;
            }
        }

    }
    else if (PadData1 & PAD_D) {
    
        if(state == TITLE_STATE) {
            if(!canMoveCursor) {
                frame_count_b = 0;
                cursor.yPos = NEW_GAME_POS;
                canMoveCursor = 1;
            }
        } else if(state == CHAR_NAME_STATE) {
            if(!canMoveCursor){
                if(cursor.yPos < NAME_LETTER_MAX_Y) {
                    cur_letter+= 10;
                    cursor.yPos += FIXED(16);
                }
                frame_count_b = 0;
                canMoveCursor = 1;
            }
        } else if (state == BAT_MENU_STATE && bat_menu_state == SELECT) {
            if(!canMoveCursor) {
                if(cursor.yPos >= BAT_MENU_MAX_Y || cursor.xPos == BAT_MENU_MAX_X) {
                    //Do Nothing, can't move.
                } else {
                    cursor.yPos += FIXED(16);
                    cur_battle_menu_selection++;
                }
                frame_count_b = 0;
                canMoveCursor = 1;
            }
        } else if (state == BAT_MENU_STATE && bat_menu_state == SELECT_ENEMY_TARGET) {
            if(!canMoveCursor) {
                currentSlot++;
                currentSlot = getNextOccupiedSlot(currentSlot);
                cursor.xPos = FIXED(getXSlotCoord(currentSlot));
                cursor.yPos = FIXED(getYSlotCoord(currentSlot));
                frame_count_b = 0;
                canMoveCursor = 1;
            }
        } else if (state == BAT_MENU_STATE && isSubMenuState()) {
            if(!canMoveCursor) {
                moveSubMenuCursor(0, 1);
                frame_count_b = 0;
                canMoveCursor = 1;
            }
        }

    }
    else if (PadData1 & PAD_U) {
        
        if(state == TITLE_STATE) {
            if(!canMoveCursor) {
                frame_count_b = 0;
                canMoveCursor = 1; 
                cursor.yPos = CONTINUE_POS;
            }
        } else if(state == CHAR_NAME_STATE) {
            if(!canMoveCursor){
                if(cursor.yPos > NAME_LETTER_MIN_Y) {
                    cur_letter -= 10;
                    cursor.yPos -= FIXED(16);
                }
                    frame_count_b = 0;
                    canMoveCursor = 1;
            }
        }  else if (state == BAT_MENU_STATE && bat_menu_state == SELECT) {
            if(!canMoveCursor) {
                if(cursor.yPos - FIXED(16) < BAT_MENU_MIN_Y) {
                    //Do Nothing, can't move.
                } else {
                    cursor.yPos -=  FIXED(16);
                    cur_battle_menu_selection--;
                }
                frame_count_b = 0;
                canMoveCursor = 1;
            }
        } else if (state == BAT_MENU_STATE && bat_menu_state == SELECT_ENEMY_TARGET) {
            if(!canMoveCursor) {
                currentSlot--;
                currentSlot = getPrevOccupiedSlot(currentSlot);
                cursor.xPos = FIXED(getXSlotCoord(currentSlot));
                cursor.yPos = FIXED(getYSlotCoord(currentSlot));
                frame_count_b = 0;
                canMoveCursor = 1;
            }
        } else if (state == BAT_MENU_STATE && isSubMenuState()) {
            if(!canMoveCursor) {
                moveSubMenuCursor(0, -1);
                frame_count_b = 0;
                canMoveCursor = 1;
            }
        }
    }

    if(state == OW_STATE) {
        owFinishFrame();
    }
}

//The direction a NES facing moves and draws: the lowest bit, with no bits facing right.
static Uint8 owDirBit(Uint8 facing) {
    for(Uint8 bit = 0x01; bit <= 0x08; bit <<= 1) {
        if(facing & bit) {
            return bit;
        }
    }
    return 0x01;
}

static Uint8 owJoyDirections() {
    Uint8 joy = 0;

    if(PadData1 & PAD_R) {
        joy |= 0x01;
    }
    if(PadData1 & PAD_L) {
        joy |= 0x02;
    }
    if(PadData1 & PAD_D) {
        joy |= 0x04;
    }
    if(PadData1 & PAD_U) {
        joy |= 0x08;
    }
    return joy;
}

//NES LoadOWObjectCHR and the overworld's sprite palettes 2 and 3.
static void owLoadObjectSprites() {
    Uint8 *halves = (Uint8 *)LWRAM_BUFFER + 0x1000;
    Uint8 *pal = (Uint8 *)OW_SPRITE_PAL;
    Uint32 vehiclePal[16] = {0};
    Uint32 objectPal[16] = {0};

    convertNesTilesToSatTiles((Uint8 *)OW_OBJ_CHR, (Uint8 *)LWRAM_BUFFER, OW_OBJ_CHR_SIZE);
    buildMapSprites((Uint8 *)LWRAM_BUFFER, halves, OW_OBJ_CHR_SIZE * 2);
    for(Uint8 h = 0; h < OW_OBJ_CHR_SIZE / 32; h++) {
        //Only the airship shadow, bridge and canal (tiles below the ship's) use palette 3.
        Uint16 cram = (h < (OW_TILE_SHIP - OW_OBJ_TILE_START) / 2) ? OW_OBJECT_CRAM : OW_VEHICLE_CRAM;
        sprSetChar(OW_OBJ_CHAR_START + h, SPRITE_4BPP, cram, 16, 8, halves + h * 64);
    }
    for(Uint8 i = 0; i < 4; i++) {
        vehiclePal[i] = nes_pal[pal[8 + i]];
        objectPal[i] = nes_pal[pal[12 + i]];
    }
    vidSetPalette(VID_SPR, OW_VEHICLE_CRAM, 16, vehiclePal);
    vidSetPalette(VID_SPR, OW_OBJECT_CRAM, 16, objectPal);
}

//Bottom middle of the party's tile on the flat map screen.
#define OW_GROUND_X (OW_SPR_X_PLAYER + OW_SPR_X_ADD + 8)
#define OW_GROUND_Y (OW_SPR_Y_BOAT + OW_SPR_Y_ADD + 16)

//A sprite standing upright on the rotated map, scaled around the map point it stands on.
typedef struct {
    Sint16 gx, gy; //Ground point on the flat map screen.
    Fixed32 sx, sy; //Ground point on screen.
    Fixed32 scale;
} OW_UPRIGHT;

static Uint8 owUprightInit(OW_UPRIGHT *u, Sint16 gx, Sint16 gy) {
    u->gx = gx;
    u->gy = gy;
    if(!scroll_project(intToFixed(gx), intToFixed(gy), &u->sx, &u->sy, &u->scale)) {
        return FALSE;
    }
    return u->sx > FIXED(-64) && u->sx < FIXED(384) && u->sy < FIXED(304);
}

//Draws the 16x8 char whose top left is at x,y on the flat map screen. A scale of 1 draws it unscaled.
static void owUprightChar(OW_UPRIGHT *u, Uint16 ch, Sint16 x, Sint16 y, Uint16 mirror, Sint32 priority) {
    SprPoint xy[2];

    xy[0].x = (Sint16)fixedToInt(u->sx + (x - u->gx) * u->scale);
    xy[0].y = (Sint16)fixedToInt(u->sy + (y - u->gy) * u->scale);
    if(u->scale == FIXED(1)) {
        sprNormal(priority, mirror, SPRITE_4BPP, 0xffff, ch, xy);
        return;
    }
    //VDP1 includes the bottom right corner.
    xy[1].x = (Sint16)fixedToInt(u->sx + (x + 16 - u->gx) * u->scale) - 1;
    xy[1].y = (Sint16)fixedToInt(u->sy + (y + 8 - u->gy) * u->scale) - 1;
    sprScaled(priority, mirror, SPRITE_4BPP, 0xffff, ch, xy);
}

//Draws the 16x8 char whose top left is at x,y on the flat map screen lying on the rotated map.
static void owFlatChar(Uint16 ch, Sint16 x, Sint16 y, Uint16 mirror) {
    static const Sint8 cornerX[4] = {0, 16, 16, 0};
    static const Sint8 cornerY[4] = {0, 0, 8, 8};
    SprPoint xy[4];
    Fixed32 sx, sy, scale;

    for(Uint8 i = 0; i < 4; i++) {
        if(!scroll_project(intToFixed(x + cornerX[i]), intToFixed(y + cornerY[i]), &sx, &sy, &scale)) {
            return;
        }
        if(sx < FIXED(-64) || sx > FIXED(384) || sy > FIXED(304)) {
            return;
        }
        xy[i].x = (Sint16)fixedToInt(sx);
        xy[i].y = (Sint16)fixedToInt(sy);
    }
    sprDistorted(CHARACTER_PRIORITY, mirror, SPRITE_4BPP, 0xffff, ch, xy);
}

//How a sprite draws on the rotated map.
#define OW_DRAW_UPRIGHT (0)
#define OW_DRAW_FLAT (1)
#define OW_DRAW_PARTY (2) //Upright and unscaled.

//Draws tiles tile/tile+1 over tile+2/tile+3 at screen x,y. On the rotated map it stands on the ground point gx,gy,
//or lies on the map when flat.
static void owDrawSprite(Uint8 tile, Sint16 x, Sint16 y, Uint8 mirror, Sint16 gx, Sint16 gy, Uint8 mode) {
    SPRITE_INFO spr;
    OW_UPRIGHT u;
    Uint16 ch = OW_OBJ_CHAR_START + (tile - OW_OBJ_TILE_START) / 2;
    Uint16 flip = mirror ? MIRROR_HORIZ : 0;

    if(rotated) {
        if(mode == OW_DRAW_FLAT) {
            owFlatChar(ch, x, y, flip);
            owFlatChar(ch + 1, x, y + 8, flip);
        } else if(owUprightInit(&u, gx, gy)) {
            if(mode == OW_DRAW_PARTY) {
                u.scale = FIXED(1);
            }
            owUprightChar(&u, ch, x, y, flip, CHARACTER_PRIORITY);
            owUprightChar(&u, ch + 1, x, y + 8, flip, CHARACTER_PRIORITY);
        }
        return;
    }
    sprite_make(ch, FIXED(x), FIXED(y), (Sint32) CHARACTER_PRIORITY, &spr);
    spr.mirror = flip;
    sprite_draw(&spr);
    spr.char_num = ch + 1;
    spr.yPos += FIXED(8);
    sprite_draw(&spr);
}

//NES lut_VehicleSprTbl: up, down, then left frames, with right mirroring left.
static void owDrawVehicle(Uint8 base, Uint8 facing, Uint8 anim, Sint16 nesY) {
    Uint8 dir = owDirBit(facing);
    Uint8 tile = (dir == 0x08) ? 0x00 : (dir == 0x04) ? 0x08 : 0x10;

    owDrawSprite(base + tile + (anim ? 4 : 0), OW_SPR_X_PLAYER + OW_SPR_X_ADD, nesY + OW_SPR_Y_ADD, dir == 0x01,
        OW_GROUND_X, OW_GROUND_Y, OW_DRAW_PARTY);
}

//NES ConvertOWToSprite. Returns FALSE when off screen.
static Uint8 owObjScreenPos(Uint8 tx, Uint8 ty, Sint16 *x, Sint16 *y) {
    Sint32 dx = ((Sint32)tx * 16 - fixedToInt(player.xPos)) & 0xFFF;
    Sint32 dy = ((Sint32)ty * 16 - fixedToInt(player.yPos)) & 0xFFF;
    Sint32 limit = rotated ? 512 : 0;

    if(dx >= 0x800) {
        dx -= 0x1000;
    }
    if(dy >= 0x800) {
        dy -= 0x1000;
    }
    if(dx <= -16 - limit || dx >= 320 + limit || dy <= -16 - limit || dy >= 240 + limit) {
        return FALSE;
    }
    *x = dx;
    *y = dy;
    return TRUE;
}

static void owDrawObject(Uint8 visible, Uint8 tx, Uint8 ty, Uint8 tile) {
    Sint16 x, y;

    if(visible && owObjScreenPos(tx, ty, &x, &y)) {
        Uint8 mode = OW_DRAW_UPRIGHT;

        if(tile == OW_TILE_BRIDGE || tile == OW_TILE_CANAL) {
            mode = OW_DRAW_FLAT;
        } else if(x + 8 == OW_GROUND_X && y + 16 == OW_GROUND_Y) {
            //On the party's tile, so it matches the vehicle when boarded.
            mode = OW_DRAW_PARTY;
        }
        owDrawSprite(tile, x, y, FALSE, x + 8, y + 16, mode);
    }
}

//NES DrawOWObj_BridgeCanal, canal first so the bridge draws over it.
static void owDrawBridgeCanal() {
    owDrawObject(canalVisible, canalX, canalY, OW_TILE_CANAL);
    owDrawObject(bridgeVisible, bridgeX, bridgeY, OW_TILE_BRIDGE);
}

//NES DrawAirshipShadow, drawn every other frame.
static void owDrawShadow() {
    if(nesFrameCounter & 0x01) {
        owDrawSprite(OW_TILE_SHADOW, OW_SPR_X_PLAYER + OW_SPR_X_ADD, OW_SPR_Y_BOAT + OW_SPR_Y_ADD, FALSE, 0, 0, OW_DRAW_FLAT);
    }
}

//NES DrawPlayerMapmanSprite, minus the airship shadow.
static void owDrawMapman() {
    SPRITE_INFO temp;
    Uint8 anim = (owMoveCtr & 0x08) != 0;

    switch(vehicle) {
        case VEH_AIRSHIP:
            owDrawVehicle(OW_TILE_AIRSHIP, owFacing, nesFrameCounter & 0x04, OW_SPR_Y_AIRSHIP);
            return;
        case VEH_SHIP:
            owDrawVehicle(OW_TILE_SHIP, owFacing, anim, OW_SPR_Y_BOAT);
            return;
        case VEH_CANOE:
            owDrawVehicle(OW_TILE_CANOE, owFacing, anim, OW_SPR_Y_BOAT);
            return;
    }
    setMapmanSprite(owDirBit(owFacing), anim);
    if(rotated) {
        OW_UPRIGHT u;
        Sint16 x = fixedToInt(PLAYER_MAXXPOS);

        if(owUprightInit(&u, OW_GROUND_X, OW_GROUND_Y)) {
            u.scale = FIXED(1);
            owUprightChar(&u, player.char_num, x, fixedToInt(PLAYER_MAXYPOS), player.mirror, player.priority);
            if(!owInForest) {
                owUprightChar(&u, playerb.char_num, x, fixedToInt(PLAYER_MAXYPOSB), playerb.mirror, playerb.priority);
            }
        }
        return;
    }
    memcpy(&temp, &player, sizeof(temp));
    temp.xPos = PLAYER_MAXXPOS;
    temp.yPos = PLAYER_MAXYPOS;
    sprite_draw(&temp);
    //The forest hides the bottom half.
    if(!owInForest) {
        memcpy(&temp, &playerb, sizeof(temp));
        temp.xPos = PLAYER_MAXXPOS;
        temp.yPos = PLAYER_MAXYPOSB;
        sprite_draw(&temp);
    }
}

//NES DrawOWSprites and AirshipTransitionFrame. Later sprites draw over earlier ones, so the NES order is reversed.
void player_ow_draw() {
    scroll_set(SCROLL_BACKGROUND1, player.xPos, player.yPos);

    if(owAirAnim != OW_AIR_NONE) {
        owDrawBridgeCanal();
        owDrawObject(shipVisible, shipX, shipY, OW_TILE_SHIP);
        owDrawShadow();
        owDrawVehicle(OW_TILE_AIRSHIP, 0x01, nesFrameCounter & 0x08, owAirAnimY);
        return;
    }
    switch(vehicle) {
        case VEH_AIRSHIP:
            owDrawBridgeCanal();
            owDrawObject(shipVisible, shipX, shipY, OW_TILE_SHIP);
            owDrawShadow();
            owDrawMapman();
            break;
        case VEH_SHIP:
            //The bridge draws over the ship, so the ship sails under it.
            owDrawObject(airshipVisible, airshipX, airshipY, OW_TILE_AIRSHIP);
            owDrawMapman();
            owDrawBridgeCanal();
            break;
        case VEH_CANOE:
            owDrawBridgeCanal();
            owDrawObject(airshipVisible, airshipX, airshipY, OW_TILE_AIRSHIP);
            owDrawObject(shipVisible, shipX, shipY, OW_TILE_SHIP);
            owDrawMapman();
            break;
        default:
            owDrawBridgeCanal();
            owDrawObject(shipVisible, shipX, shipY, OW_TILE_SHIP);
            owDrawObject(owAirshipShown, airshipX, airshipY, OW_TILE_AIRSHIP);
            owDrawMapman();
            break;
    }
}

//NES DockShip: the ship stays on the tile being left.
static void owDockShip() {
    shipX = current_x_pos;
    shipY = current_y_pos;
    //Silences the noise used for the waves.
    musicApuWrite(0x400C, 0x30);
    music_track = MUSIC_OVERWORLD;
}

//NES UnboardBoat_Abs.
static Uint8 owUnboardBoatAbs() {
    Uint8 old = vehicle;

    vehicle = VEH_FOOT;
    vehicleNext = VEH_FOOT;
    owTileprop1 = 0;
    if(old == VEH_SHIP) {
        owDockShip();
    }
    return TRUE;
}

//NES UnboardBoat.
static Uint8 owUnboardBoat() {
    if(owTileprop & VEH_FOOT) {
        return FALSE;
    }
    return owUnboardBoatAbs();
}

//NES BoardCanoe. The party walks the step on foot, then pauses.
static Uint8 owBoardCanoe() {
    Uint8 old = vehicle;

    if((owTileprop & VEH_CANOE) || !hasCanoe) {
        return FALSE;
    }
    vehChgPause = 4;
    vehicle = VEH_FOOT;
    vehicleNext = VEH_CANOE;
    owTileprop1 = 0;
    if(old == VEH_SHIP) {
        owDockShip();
    }
    return TRUE;
}

//NES BoardShip.
static Uint8 owBoardShip(Uint8 x, Uint8 y) {
    if(!shipVisible || shipX != x || shipY != y) {
        return FALSE;
    }
    vehicle = VEH_FOOT;
    vehicleNext = VEH_SHIP;
    vehChgPause = 4;
    owTileprop1 = 0;
    music_track = MUSIC_SHIP;
    return TRUE;
}

//NES IsOnCanal.
static Uint8 owIsOnCanal(Uint8 x, Uint8 y) {
    return canalVisible && canalX == x && canalY == y;
}

//NES IsOnBridge, which passes the canal too.
static Uint8 owIsOnBridge(Uint8 x, Uint8 y) {
    if(bridgeVisible && bridgeX == x && bridgeY == y) {
        return TRUE;
    }
    return owIsOnCanal(x, y);
}

//NES OWCanMove for the tile at x,y. Rolls the encounter and catches the caravan.
static Uint8 owCanMove(Uint8 x, Uint8 y) {
    owTileprop = getTileProperty1(y, x);
    owTileprop1 = getTileProperty2(y, x);

    if(owTileprop & vehicle) {
        return FALSE;
    }
    if(owTileprop1 & 0x40) {
        if(battleStepRNG() >= ((vehicle == VEH_SHIP) ? 3 : 10)) {
            owTileprop1 = 0;
            return TRUE;
        }
        //The domain is picked from the tile being left, like the NES.
        Uint8 fromNew = BUGFIX_ON(BUGFIX_ENCOUNTER_ZONE);
        owBtlFormation = getBattleNum(determineBattleDomain(fromNew ? x : current_x_pos, fromNew ? y : current_y_pos,
                                                            FIXED(0x40 | (owTileprop1 & 0x03))));
        return TRUE;
    }
    if(vehicle != VEH_FOOT) {
        return TRUE;
    }
    switch(owTileprop & OWTP_SPEC_MASK) {
        case OWTP_SPEC_CHIME:
            return items[ITEM_CHIME] != 0;
        case OWTP_SPEC_CARAVAN:
            //Closed once the fairy is out of the bottle.
            if(!(gameFlags[OBJID_FAIRY] & 0x01)) {
                owEnteringShop = TRUE;
                shop_id = SHOP_ID_CARAVAN;
            }
            return TRUE;
    }
    return TRUE;
}

//NES ProcessOWInput @Movement and StartMapMove.
static void owStartMove(Uint8 joy) {
    Uint8 dir = owDirBit(joy);
    Uint8 x = current_x_pos + (dir == 0x01) - (dir == 0x02);
    Uint8 y = current_y_pos + (dir == 0x04) - (dir == 0x08);
    Uint8 ok;

    switch(vehicle) {
        case VEH_AIRSHIP:
            ok = TRUE;
            break;
        case VEH_SHIP:
            if(owCanMove(x, y)) {
                ok = !owIsOnCanal(x, y);
            } else if(owBoardCanoe()) {
                ok = TRUE;
            } else {
                ok = (owTileprop & (OWTP_DOCKSHIP | VEH_FOOT)) == OWTP_DOCKSHIP && owUnboardBoatAbs();
            }
            break;
        case VEH_CANOE:
            ok = owCanMove(x, y) || owUnboardBoat() || owBoardShip(x, y);
            break;
        default:
            if(owCanMove(x, y)) {
                ok = TRUE;
            } else {
                owTileprop1 = 0;
                if(owIsOnBridge(x, y)) {
                    if(!bridgeScene) {
                        bridgeScene++;
                    }
                    ok = TRUE;
                } else {
                    ok = owBoardCanoe() || owBoardShip(x, y);
                }
            }
            break;
    }
    if(!ok) {
        owTileprop = 0;
        owTileprop1 = 0;
        return;
    }
    owFacing = joy;
    //Canoe 1, ship 2, airship 4, from the vehicle after any boarding.
    owMoveSpeed = (vehicle >= VEH_CANOE) ? vehicle >> 1 : 1;
    current_x_pos = x;
    current_y_pos = y;
    isMoving = TRUE;
}

//NES OW_MovePlayer. Right and down count the move counter up, left and up count it down.
static void owMoveStep() {
    Uint8 dir = owDirBit(owFacing);
    Fixed32 step = FIXED(owMoveSpeed);

    if(dir & 0x05) {
        owMoveCtr = (owMoveCtr + owMoveSpeed) & 0x0F;
    } else {
        owMoveCtr = (owMoveCtr - owMoveSpeed) & 0x0F;
    }
    switch(dir) {
        case 0x01:
            player.xPos += step;
            break;
        case 0x02:
            player.xPos -= step;
            break;
        case 0x04:
            player.yPos += step;
            break;
        default:
            player.yPos -= step;
            break;
    }
    playerb.xPos = player.xPos;
    playerb.yPos = player.yPos + FIXED(8);
    if(vehicle == VEH_FOOT) {
        smPoisonSfx();
    }
    if(owMoveCtr == 0) {
        isMoving = FALSE;
        owMoveSpeed = 0;
        //NES MapPoisonDamage, checked before the vehicle changes, so boarding steps count.
        if(vehicle == VEH_FOOT) {
            smStepDamage(TRUE);
        }
        //Rebases the scroll on the tile, since the map repeats and the airship can wrap around it.
        setOWPosition(current_x_pos, current_y_pos);
    }
}

//NES FlyAirship.
static void owFlyAirship() {
    if(!airshipVisible || airshipX != current_x_pos || airshipY != current_y_pos) {
        return;
    }
    vehicle = VEH_AIRSHIP;
    vehicleNext = VEH_AIRSHIP;
    music_track = MUSIC_AIRSHIP;
    owAirAnim = OW_AIR_TAKEOFF;
    owAirAnimY = OW_SPR_Y_BOAT;
    owAirFrames = 0;
}

static void owSetTilt(Fixed32 angle) {
    owTilt = angle;
    scroll_rotate_x(angle);
    rotated = angle != 0;
}

//The map tilts as the airship climbs, fully tilted two thirds of the way up, and flattens as it lands.
static void owTiltAirAnim() {
    Sint32 t = owAirFrames++;

    if(owAirAnim == OW_AIR_LANDING) {
        t = OW_AIR_ANIM_FRAMES - t;
    }
    t = t * 3 / 2;
    if(t < 0) {
        t = 0;
    } else if(t > OW_AIR_ANIM_FRAMES) {
        t = OW_AIR_ANIM_FRAMES;
    }
    owSetTilt(FIXED(OW_TILT_ANGLE) / OW_AIR_ANIM_FRAMES * t);
}

//NES LandAirship, after the landing animation. A tile the airship can't land on takes off again.
static void owLandAirship() {
    if(getTileProperty1(current_y_pos, current_x_pos) & OWTP_AIRSHIP_NOLAND) {
        owAirAnim = OW_AIR_TAKEOFF;
        owAirAnimY = OW_SPR_Y_BOAT;
        owAirFrames = 0;
        return;
    }
    airshipX = current_x_pos;
    airshipY = current_y_pos;
    vehicle = VEH_FOOT;
    vehicleNext = VEH_FOOT;
    music_track = MUSIC_OVERWORLD;
}

//NES AnimateAirshipTakeoff/AnimateAirshipLanding: the airship moves every other frame, then faces right.
static void owAirAnimStep() {
    //NES AirshipTransitionFrame noise.
    musicApuWrite(0x400C, 0x38);
    musicApuWrite(0x400E, (Uint8)nesFrameCounter);
    musicApuWrite(0x400F, 0x00);
    owTiltAirAnim();
    if(nesFrameCounter & 0x01) {
        return;
    }
    if(owAirAnim == OW_AIR_TAKEOFF) {
        if(--owAirAnimY >= OW_SPR_Y_AIRSHIP) {
            return;
        }
        owAirAnim = OW_AIR_NONE;
        owFacing = 0x01;
    } else {
        if(++owAirAnimY < OW_SPR_Y_BOAT + 1) {
            return;
        }
        owAirAnim = OW_AIR_NONE;
        owFacing = 0x01;
        musicApuWrite(0x400C, 0x00);
        owLandAirship();
    }
}

//NES DoOWTransitions. Returns TRUE when it leaves the overworld.
static Uint8 owDoTransitions() {
    if(bridgeScene && !(bridgeScene & 0x80)) {
        storySelect(FALSE);
        startOwPalCycle(FALSE, TO_STORY);
        return TRUE;
    }
    if(owEnteringShop) {
        startOwPalCycle(FALSE, TO_SHOP);
        return TRUE;
    }
    if(owTileprop1 & 0x80) {
        currentBackDrop = getTileBattleBackdrop(current_y_pos, current_x_pos);
        smTeleportIn(owTileprop1 & 0x3F);
        wipeStart(FALSE, VID_RBG0, owWipeClosedToSM);
        return TRUE;
    }
    if(owTileprop1 & 0x40) {
        currentBackDrop = getTileBattleBackdrop(current_y_pos, current_x_pos);
        battle_init(owBtlFormation);
        startBattleTransition();
        return TRUE;
    }
    //R stands in for Select and L for Select+B, latched by player_input.
    if(owStartLatched || owSelectNext != OW_NO_MENU) {
        musicApuWrite(0x400C, 0x00);
        mmFromMap = owStartLatched;
        startOwPalCycle(FALSE, owStartLatched ? TO_MAIN_MENU : owSelectNext);
        return TRUE;
    }
    return FALSE;
}

//NES EnterOverworldLoop up to ProcessOWInput reading the pad, which is when owCanPoll is set.
static void owUpdate() {
    owCanPoll = FALSE;
    if(owAirAnim != OW_AIR_NONE) {
        owAirAnimStep();
        return;
    }
    if(isMoving) {
        owMoveStep();
        if(isMoving) {
            return;
        }
    }
    vehicle = vehicleNext;
    if(owDoTransitions()) {
        return;
    }
    owInForest = (owTileprop & OWTP_FOREST) != 0;
    if(vehChgPause) {
        vehChgPause--;
        return;
    }
    owCanPoll = TRUE;
}

//NES ProcessOWInput after the pad is read, then the Floater's flashing airship from DrawOWSprites.
static void owFinishFrame() {
    if(owCanPoll) {
        Uint8 joy = owJoyDirections();
        if(joy) {
            owStartMove(joy);
        } else {
            Uint8 checkB = TRUE;
            if(owJoyA) {
                owJoyA = FALSE;
                if(vehicle == VEH_AIRSHIP) {
                    owAirAnim = OW_AIR_LANDING;
                    owAirAnimY = OW_SPR_Y_AIRSHIP;
                    owAirFrames = 0;
                    checkB = FALSE;
                } else if(vehicle == VEH_FOOT) {
                    owFlyAirship();
                    checkB = FALSE;
                }
            }
            //NES ProcessOWInput @ANotPressed: the 55th B press on the ship, with A and B held.
            if(checkB && owJoyB == 55) {
                owJoyB++;
                if(vehicle == VEH_SHIP && (PadData1 & (PAD_A | PAD_C)) && (PadData1 & PAD_B)) {
                    startOwPalCycle(FALSE, TO_MINIGAME);
                    return;
                }
            }
        }
    }
    if(vehicle == VEH_FOOT && owAirAnim == OW_AIR_NONE) {
        owAirshipShown = airshipVisible != 0;
        if(airshipVisible && airshipVisible < 0x1F) {
            owAirshipShown = !(airshipVisible & 0x02);
            airshipVisible++;
        }
    }
    //NES EnterOverworldLoop @AirshipSFX and @ShipSFX noise.
    if(owAirAnim == OW_AIR_NONE) {
        Uint8 fc = nesFrameCounter;
        if(vehicle == VEH_AIRSHIP) {
            musicApuWrite(0x400C, 0x38);
            musicApuWrite(0x400E, fc << 1);
            musicApuWrite(0x400F, 0x00);
        } else if(vehicle == VEH_SHIP) {
            if(fc & 0x80) {
                fc ^= 0xFF;
            }
            musicApuWrite(0x400C, (fc >> 4) | 0x30);
            musicApuWrite(0x400E, 0x0A);
            musicApuWrite(0x400F, 0x00);
        }
    }
}

//Stoned characters are drawn crouched at their home position in the stone palette.
static void drawBattleCharacter(SPRITE_INFO *sprite, Uint8 index) {
    //A flashing character is hidden while bit 1 of the counter is set.
    Uint8 flashing = (turn_exec_state == FLASH_CHARACTER && index == curr_enemy_target) || index == magicFlashChar;
    if(flashing && (flashCounter & 0x02)) {
        return;
    }
    Uint8 isDead = isPlayerDead(index) || (battle_char_stats[index].ailment & AIL_DEAD);
    if(!isDead && (battle_char_stats[index].ailment & AIL_STONE)) {
        Uint8 *startXPos = (Uint8 *)BATTLE_CHAR_DEFAULT_XPOS;
        sprite->char_num = FRAME_BATTLE_START + (party[index].char_class * 6) + 4;
        sprite->xPos = FIXED(startXPos[0] + 24);
        sprite_draw_pal(sprite, STONE_PALETTE_CRAM);
    } else if(sprite->char_num == FRAME_BATTLE_START + (party[index].char_class * 6) + 5) {
        //NES DrawCharacter draws the 24x16 dead graphic at X $A8 and 8 px below the 16x24 poses,
        //without changing the character's position.
        Uint8 *startXPos = (Uint8 *)BATTLE_CHAR_DEFAULT_XPOS;
        SPRITE_INFO dead = *sprite;
        dead.xPos = FIXED(startXPos[0] + 24 - 8);
        dead.yPos += FIXED(8);
        sprite_draw(&dead);
    } else {
        sprite_draw(sprite);
    }
}

void player_bat_draw() {
    drawBattleCharacter(&playerBat1, 0);
    drawBattleCharacter(&playerBat2, 1);
    drawBattleCharacter(&playerBat3, 2);
    drawBattleCharacter(&playerBat4, 3);

    if(weaponVisible){
        sprite_draw(&weaponBat);
    }
    for(Uint8 i = 0; i < explosionSlotsDrawn; i++) {
        sprite_draw(&explosionBat[i]);
    }
}

void drawSmallFormationExplosion(Uint8 slotNum, Uint8 spriteSlot) {
    Uint8 *lut_ptr = (Uint8 *) LUT_SMALL_FORMATION_EXPLOSION_LUT;
    Uint8 xIter = slotNum * 2;
    Uint8 minX = lut_ptr[xIter];
    Uint8 minY = lut_ptr[xIter + 1];
    Uint8 maxX = minX + 0x18;
    Uint8 maxY = minY + 0x18;

    drawExplosions(spriteSlot, minX, maxX, minY, maxY);

}

void drawSmallMixFormationExplosion(Uint8 slotNum, Uint8 spriteSlot) {
    Uint8 *lut_ptr = (Uint8 *) LUT_MIX_SMALL_FORMATION_EXPLOSION_LUT;
    Uint8 xIter = slotNum * 2;
    Uint8 minX = lut_ptr[xIter];
    Uint8 minY = lut_ptr[xIter + 1];
    Uint8 maxX = minX + 0x18;
    Uint8 maxY = minY + 0x18;

    drawExplosions(spriteSlot, minX, maxX, minY, maxY);

}

void drawLargeFormationExplosion(Uint8 slotNum, Uint8 spriteSlot) {
    Uint8 *lut_ptr = (Uint8 *) LUT_LARGE_FORMATION_EXPLOSION_LUT;
    Uint8 xIter = slotNum * 2;
    Uint8 minX = lut_ptr[xIter];
    Uint8 minY = lut_ptr[xIter + 1];
    Uint8 maxX = minX + 0x28;
    Uint8 maxY = minY + 0x28;

    drawExplosions(spriteSlot, minX, maxX, minY, maxY);

}

//Number of 3-sprite explosion rounds drawn on the target.
Uint8 getExplosionCount(Uint8 enemyIndex) {
    switch(battle.battle_type) {
        case 0:
            return 6;
        case 1:
            return 8;
        case 2:
            return (enemyIndex < 2) ? 8 : 15;
        case 3:
        case 4:
            return 20;
        default:
            return 0;
    }
}

void drawExplosions(Uint8 spriteSlot, Uint8 minX, Uint8 maxX, Uint8 minY, Uint8 maxY) {

    Uint8 frame = rand_min_max(0, 2);
    Uint8 xPos = rand_min_max(minX, maxX);
    Uint8 yPos = rand_min_max(minY, maxY);
    explosionBat[spriteSlot].xPos = FIXED(xPos + 24);
    explosionBat[spriteSlot].yPos = FIXED(yPos);
    explosionBat[spriteSlot].char_num = frame + explosionSpriteIndex;
    if(explosionSlotsDrawn <= spriteSlot) {
        explosionSlotsDrawn = spriteSlot + 1;
    }
}

//NES DrawEnemyEffect: one explosion sprite per frame, in rounds of 3. Returns TRUE once done.
static Uint8 explosionFrame(Uint8 enemyIndex) {
    if(explosionCounter >= getExplosionCount(enemyIndex)) {
        explosionSlotsDrawn = 0;
        return TRUE;
    }

    if(battle.battle_type >= 3) {
        drawExplosions(explosionSlot, 0x20, 0x60, 0x40, 0x78);
    } else if(battle.battle_type == 0) {
        drawSmallFormationExplosion(enemyIndex, explosionSlot);
    } else if(battle.battle_type == 1 || enemyIndex < 2) {
        drawLargeFormationExplosion(enemyIndex, explosionSlot);
    } else {
        drawSmallMixFormationExplosion(enemyIndex - 2, explosionSlot);
    }
    explosionSlot++;
    if(explosionSlot == 3) {
        explosionSlot = 0;
        explosionCounter++;
    }
    return FALSE;
}

//NES EraseEnemyGraphic, one row per call. Returns TRUE once done.
static Uint8 eraseEnemyFrame(Uint8 enemyIndex) {
    Uint8 slotNum = getEnemySlotNum(enemyIndex);
    slotOccupied[slotNum] = FALSE;

    //The NES doesn't erase Chaos.
    if(battle.battle_type == 4) {
        return TRUE;
    }

    Uint8 xy = 4;
    if(battle.battle_type == 3) {
        xy = 12;
    } else if(enemies[enemyIndex].pic_num >= 2) {
        xy = 6;
    }
    return deleteEnemy(slotXCoords[slotNum], slotYCoords[slotNum], xy);
}


static SPRITE_INFO *battleCharSprite(int partyNum) {
    SPRITE_INFO *bats[4] = {&playerBat1, &playerBat2, &playerBat3, &playerBat4};
    return bats[partyNum];
}

static void setNaturalPose(int partyNum) {
    if(isPlayerCrouching(partyNum, party[partyNum].max_hp)) {
        player_crouch(partyNum);
    } else {
        player_stand(partyNum);
    }
}

//NES CharacterWalkAnimation: 8 steps of 2 px, 2 frames each. With settle, the natural
//pose is then held for 2 frames. Returns TRUE once done.
static Uint8 battleWalk(int partyNum, int dx, Uint8 settle) {
    SPRITE_INFO *sprite = battleCharSprite(partyNum);

    if(battleAnimFrame < 16) {
        if((battleAnimFrame & 1) == 0) {
            Uint8 loop = 8 - (battleAnimFrame >> 1);
            sprite->char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6) + ((loop & 2) ? 1 : 0);
            sprite->xPos += FIXED(dx);
        }
    } else if(battleAnimFrame == 16) {
        isMoving = FALSE;
        setNaturalPose(partyNum);
    }

    battleAnimFrame++;
    if(battleAnimFrame == (settle ? 18 : 16)) {
        battleAnimFrame = 0;
        isMoving = FALSE;
        return TRUE;
    }
    return FALSE;
}

//NES WalkForwardAndStrike: 8 swings of 2 frames starting on the B frame, 1 frame with the
//weapon cleared, then standing for 2 frames before the walk back.
void swingWeapon(int partyNum) {
    SPRITE_INFO *sprite = battleCharSprite(partyNum);
    Uint16 baseFrame = FRAME_BATTLE_START + (party[partyNum].char_class * 6);

    if(battleAnimFrame < 16) {
        if((battleAnimFrame & 1) == 0) {
            Uint8 loop = 8 - (battleAnimFrame >> 1);
            if(loop & 2) {
                sprite->char_num = baseFrame + 2;
                weaponBat.xPos = sprite->xPos - FIXED(16);
                weaponBat.yPos = sprite->yPos;
                weaponBat.mirror = 0;
            } else {
                sprite->char_num = baseFrame + 1;
                weaponBat.xPos = sprite->xPos + FIXED(8);
                weaponBat.yPos = sprite->yPos - FIXED(8);
                weaponBat.mirror = MIRROR_HORIZ;
            }
        }
    } else if(battleAnimFrame == 16) {
        weaponVisible = FALSE;
    } else if(battleAnimFrame == 17) {
        player_stand(partyNum);
    }

    battleAnimFrame++;
    if(battleAnimFrame == 19) {
        battleAnimFrame = 0;
        isMoving = TRUE;
        turn_exec_state = WALK_BACK;
        explosionCounter = 0;
        explosionSlot = 0;
    }
}

//NES WalkForwardAndCastMagic: 8 frames of the cheer pose, alternating the magic sprite's A/B
//frames. Each one first runs DoMagicFlash, 4 frames with the 2nd and 3rd in the spell color,
//then shows the new pose and sprite for 2 frames. Without the sprite each is just 2 frames.
static void castMagic(int partyNum) {
    SPRITE_INFO *sprite = battleCharSprite(partyNum);
    Uint8 step = magicNoDraw ? 2 : 6;
    Uint8 total = step * 8;

    if(battleAnimFrame < total) {
        Uint8 sub = battleAnimFrame % step;
        if(!magicNoDraw && sub == 1) {
            setBattleBackColor(magicFlashColor);
        } else if(!magicNoDraw && sub == 3) {
            setBattleBackColor(0x0F);
        }
        if(sub == (magicNoDraw ? 0 : 4)) {
            Uint8 loop = 8 - (battleAnimFrame / step);
            sprite->char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6) + 3;
            if(!magicNoDraw) {
                weaponBat.char_num = weaponSpriteIndex + ((magicGfx - 0x80) / 4) + ((loop & 2) ? 0 : 1);
                weaponBat.xPos = sprite->xPos - FIXED(16);
                weaponBat.yPos = sprite->yPos;
                weaponBat.mirror = 0;
                weaponVisible = TRUE;
            }
        }
    } else if(battleAnimFrame == total) {
        weaponVisible = FALSE;
    } else if(battleAnimFrame == total + 1) {
        player_stand(partyNum);
    }

    battleAnimFrame++;
    if(battleAnimFrame == total + 3) {
        battleAnimFrame = 0;
        isMoving = TRUE;
        turn_exec_state = WALK_BACK;
    }
}

void moveBattlePlayerLeft(int partyNum) {
    //The attack walk goes straight into the swing.
    if(!battleWalk(partyNum, -2, state == BAT_MENU_STATE)) {
        return;
    }

    if(state == BAT_MENU_STATE) {
        bat_menu_state = SELECT;
    } else if (state == BAT_EXECUTE_STATE && turn_exec_state == ANIMATE1) {
        turn_exec_state = CALCULATE;
    }
}

void moveBattlePlayerRight(int partyNum) {
    if(!battleWalk(partyNum, 2, TRUE)) {
        return;
    }

    Uint8 *startXPos = (Uint8 *)BATTLE_CHAR_DEFAULT_XPOS;
    battleCharSprite(partyNum)->xPos = FIXED(startXPos[0] + 24);

    if(state == BAT_MENU_STATE) {
        if(bat_menu_state == CANCEL_ACTION) {
            //NES CancelBattleAction: walk back left and pick a command again, cursor on FIGHT.
            cursor.xPos = FIXED(120);
            cursor.yPos = FIXED(160);
            bat_menu_state = INIT;
            return;
        }
        if(bat_menu_state == SELECT_CANCEL) {
            //Back up past characters who can't input a command.
            if(cur_char_battle_menu > 0) {
                do {
                    cur_char_battle_menu--;
                } while(cur_char_battle_menu > 0 && !canInputCommand(cur_char_battle_menu));
            }
            //NES UndoCharacterBattleCommand gives back a potion that character picked.
            if(drinkReserved[cur_char_battle_menu] != 0xFF) {
                battlePotions[drinkReserved[cur_char_battle_menu]]++;
                drinkReserved[cur_char_battle_menu] = 0xFF;
            }
        } else {
            cur_char_battle_menu += 1;
            while(cur_char_battle_menu < 4 && !canInputCommand(cur_char_battle_menu)) {
                cur_char_battle_menu++;
            }
        }

        if(cur_char_battle_menu == 4) {
            cur_char_battle_menu = 0;
            bat_menu_state = END;
        } else {
            state = TO_BAT_MENU_STATE;
        }

    } else if (state == BAT_EXECUTE_STATE && turn_exec_state == WALK_BACK) {
        //No explosion or damage roll on a target that is already dead.
        if(magicTurn) {
            magicStep = magicAbort ? MAGIC_STEP_OPS : MAGIC_STEP_NEXT_TARGET;
            turn_exec_state = MAGIC_EFFECT;
        } else if(ineffective) {
            turn_exec_state = RESULT;
        } else {
            //NES DoExplosionEffect.
            battleSfx(BTL_SFX_PLAYER_ATTACK);
            turn_exec_state = PREP_EXPLOSION;
        }
    }
}

void player_crouch(int partyNum) {
    
    switch(partyNum) {
        case 0:
            playerBat1.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6) + 4;
            playerBat1.angle = 0;
            playerBat1.xSize = FIXED(16);
            playerBat1.ySize = FIXED(24);
            break;
        case 1:
            playerBat2.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6) + 4;
            playerBat2.angle = 0;
            playerBat2.xSize = FIXED(16);
            playerBat2.ySize = FIXED(24);
            break;
        case 2:
            playerBat3.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6) + 4;
            playerBat3.angle = 0;
            playerBat3.xSize = FIXED(16);
            playerBat3.ySize = FIXED(24);
            break;
        case 3:
            playerBat4.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6) + 4;
            playerBat4.angle = 0;
            playerBat4.xSize = FIXED(16);
            playerBat4.ySize = FIXED(24);
            break;
        default:
            break;
    }
    
}

void player_stand(int partyNum) {
    
    if(!isMoving) {
        switch(partyNum) {
            case 0:
                playerBat1.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6);
                playerBat1.angle = 0;
                playerBat1.xSize = FIXED(16);
                playerBat1.ySize = FIXED(24);
                break;
            case 1:
                playerBat2.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6);
                playerBat2.angle = 0;
                playerBat2.xSize = FIXED(16);
                playerBat2.ySize = FIXED(24);
                break;
            case 2:
                playerBat3.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6);
                playerBat3.angle = 0;
                playerBat3.xSize = FIXED(16);
                playerBat3.ySize = FIXED(24);
                break;
            case 3:
                playerBat4.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6);
                playerBat4.angle = 0;
                playerBat4.xSize = FIXED(16);
                playerBat4.ySize = FIXED(24);
                break;
            default:
                break;
        }
    }
    
}

void player_flip(int partyNum) {
    switch(partyNum) {
            case 0:
                playerBat1.mirror = MIRROR_HORIZ;
                break;
            case 1:
                playerBat2.mirror = MIRROR_HORIZ;
                break;
            case 2:
                playerBat3.mirror = MIRROR_HORIZ;
                break;
            case 3:
                playerBat4.mirror = MIRROR_HORIZ;
                break;
            default:
                break;
        }
}

void player_dead(int partyNum) {
    
    switch(partyNum) {
        case 0:
            playerBat1.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6) + 5;
            playerBat1.angle = 0;
            playerBat1.xSize = FIXED(24);
            playerBat1.ySize = FIXED(16);
            break;
        case 1:
            playerBat2.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6) + 5;
            playerBat2.angle = 0;
            playerBat2.xSize = FIXED(24);
            playerBat2.ySize = FIXED(16);
            break;
        case 2:
            playerBat3.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6) + 5;
            playerBat3.angle = 0;
            playerBat3.xSize = FIXED(24);
            playerBat3.ySize = FIXED(16);
            break;
        case 3:
            playerBat4.char_num = FRAME_BATTLE_START + (party[partyNum].char_class * 6) + 5;
            playerBat4.angle = 0;
            playerBat4.xSize = FIXED(24);
            playerBat4.ySize = FIXED(16);
            break;
        default:
            break;
    }
    
    
}

void party_crouch() {
    for(Uint8 i = 0; i < 4; i++) {
        if(isPlayerCrouching(i, party[i].max_hp)){
            player_crouch(i);
        }
    }
}

void party_dead() {
    for(Uint8 i = 0; i < 4; i++) {
        if(battle_char_stats[i].hp == 0) {
            player_dead(i);
        }
    }
}


//The cheer frame is 3 past the stand frame.
void party_cheer_pose(Uint8 cheer) {
    SPRITE_INFO *bats[4] = {&playerBat1, &playerBat2, &playerBat3, &playerBat4};

    for(Uint8 i = 0; i < 4; i++) {
        if(battle_char_stats[i].hp > 0) {
            bats[i]->char_num = FRAME_BATTLE_START + (party[i].char_class * 6) + (cheer ? 3 : 0);
        }
    }
}

void initPartyGold() {
    Uint8 *gold = (Uint8*) NEW_GAME_GOLD;
    partyGold = gold[0] | (gold[1] << 8) | (gold[2] << 16);
}

void initPartyItems() {
    memcpy(items, (Uint8 *)NEW_GAME_ITEMS, sizeof(items));
    memcpy(gameFlags, (Uint8 *)NEW_GAME_FLAGS, sizeof(gameFlags));
    shipVisible = *(Uint8 *)NEW_GAME_SHIP_VIS;
    shipX = ((Uint8 *)NEW_GAME_SHIP_VIS)[1];
    shipY = ((Uint8 *)NEW_GAME_SHIP_VIS)[2];
    airshipVisible = *(Uint8 *)NEW_GAME_AIRSHIP_VIS;
    airshipX = ((Uint8 *)NEW_GAME_AIRSHIP_VIS)[1];
    airshipY = ((Uint8 *)NEW_GAME_AIRSHIP_VIS)[2];
    bridgeVisible = *(Uint8 *)NEW_GAME_BRIDGE_VIS;
    bridgeX = ((Uint8 *)NEW_GAME_BRIDGE_VIS)[1];
    bridgeY = ((Uint8 *)NEW_GAME_BRIDGE_VIS)[2];
    canalVisible = *(Uint8 *)NEW_GAME_CANAL_VIS;
    canalX = ((Uint8 *)NEW_GAME_CANAL_VIS)[1];
    canalY = ((Uint8 *)NEW_GAME_CANAL_VIS)[2];
    bridgeScene = *(Uint8 *)NEW_GAME_BRIDGE_SCENE;
    hasCanoe = *(Uint8 *)NEW_GAME_HAS_CANOE;
    memset(bugFixes, 0, sizeof(bugFixes));
}

void initBattleEnd(Uint8 phase) {
    isMoving = FALSE;
    battleEndPhase = phase;
    battleEndFrame = 0;
    respondRateCounter = 0;
    victoryStep = VICTORY_PERISHED;
}

static void setBattleEndPhase(Uint8 phase) {
    battleEndPhase = phase;
    respondRateCounter = 0;
}

static Uint32 getExpToAdvance(Uint8 level0) {
    Uint8 *ptr = (Uint8*) EXP_TO_ADVANCE_LUT + level0 * 3;
    return ptr[0] | (ptr[1] << 8) | (ptr[2] << 16);
}

static void computeBattleRewards() {
    Uint32 expSum = 0;
    Uint32 goldSum = 0;
    Uint8 survivors = 0;

    for(Uint8 i = 0; i < 9; i++) {
        expSum += enemies[i].experience;
        goldSum += enemies[i].money_reward;
    }

    for(Uint8 i = 0; i < 4; i++) {
        if(!(party[i].ailment & (AIL_DEAD | AIL_STONE))) {
            survivors++;
        }
    }

    //Only the low 16 bits are divided.
    eobExpReward = (Uint16) expSum / survivors;
    if(eobExpReward == 0) {
        eobExpReward = 1;
    }

    partyGold += goldSum;
    if(partyGold > 999999) {
        partyGold = 999999;
    }
    eobGoldReward = (Uint16) goldSum;
    eobRewardHighByte = goldSum >> 16;
}

//"EXP up | ##P" / "GOLD | ##G"
static void drawRewardBoxes() {
    Uint8 buffer[12];
    Uint8 len;

    len = appendBattleMsg(buffer, 0, 0x49);
    buffer[len] = 0x00;
    drawCombatBoxText(0, buffer);

    len = appendNumber(buffer, 0, eobExpReward);
    buffer[len++] = 0x99;
    buffer[len] = 0x00;
    drawCombatBoxText(1, buffer);

    buffer[0] = 0x90;
    buffer[1] = 0x98;
    buffer[2] = 0x95;
    buffer[3] = 0x8D;
    buffer[4] = 0x00;
    drawCombatBoxText(2, buffer);

    len = appendNumber(buffer, 0, eobGoldReward);
    buffer[len++] = 0x90;
    buffer[len] = 0x00;
    drawCombatBoxText(3, buffer);
}

//"Lev. up! | <Name> L##" / "HP max | ###pts."
static void drawLevelUpBoxes() {
    Uint8 buffer[16];
    Uint8 len;

    len = appendBattleMsg(buffer, 0, 0x30);
    buffer[len] = 0x00;
    drawCombatBoxText(0, buffer);

    len = appendTrimmedName(buffer, 0, party[lvlUpChar].name);
    buffer[len++] = 0xFF;
    buffer[len++] = 0x95;
    len = appendNumber(buffer, len, party[lvlUpChar].level);
    buffer[len] = 0x00;
    drawCombatBoxText(1, buffer);

    len = appendBattleMsg(buffer, 0, 0x31);
    buffer[len] = 0x00;
    drawCombatBoxText(2, buffer);

    len = appendNumber(buffer, 0, lvlUpPrintHp);
    len = appendBattleMsg(buffer, len, 0x32);
    buffer[len] = 0x00;
    drawCombatBoxText(3, buffer);
}

//NES LvlUp_LevelUp. Returns FALSE at level 50.
static Uint8 levelUpCharacter(PLAYER_CHARACTER *ch) {
    if(ch->level == 50) {
        return FALSE;
    }

    Uint8 cls = ch->char_class;
    Uint8 *data = (Uint8*) LEVEL_UP_DATA + (cls % 6) * 98 + (ch->level - 1) * 2;
    Uint8 *hitBonus = (Uint8*) LVLUP_HIT_RATE_BONUS_LUT;
    Uint8 *magDefBonus = (Uint8*) LVLUP_MAG_DEF_BONUS_LUT;
    Uint8 *stats[5] = {&ch->str, &ch->agi, &ch->intel, &ch->vit, &ch->luck};

    ch->level++;
    eobRewardHighByte = 0;

    Uint16 value = ch->hit_percent + hitBonus[cls];
    ch->hit_percent = value > 200 ? 200 : value;
    value = ch->mag_def + magDefBonus[cls];
    ch->mag_def = value > 200 ? 200 : value;

    //Knights and Ninjas share Fighter and Thief data, so those two are skipped.
    if(cls != WARRIOR_VALUE && cls != THIEF_VALUE) {
        Uint8 mpCap = (cls == KNIGHT_VALUE || cls == NINJA_VALUE) ? 4 : 9;
        for(Uint8 i = 0; i < 8; i++) {
            if(data[1] & (1 << i)) {
                ch->max_mp[i]++;
            }
            if(ch->max_mp[i] == mpCap + 1) {
                ch->max_mp[i]--;
            }
        }
    }

    Uint8 hpGain = (ch->vit >> 2) + 1;
    if(data[0] & 0x20) {
        hpGain += rand_min_max(20, 25);
    }
    ch->max_hp += hpGain;
    if(ch->max_hp > 999) {
        ch->max_hp = 999;
    }
    lvlUpPrintHp = ch->max_hp;

    for(Uint8 k = 0; k < 5; k++) {
        if(data[0] & (0x10 >> k)) {
            statUp[k] = 1;
        } else {
            statUp[k] = (battleRNG() & 0x03) == 0;
        }
        //A stat at 99 isn't raised but is still reported as up.
        if(*stats[k] + statUp[k] != 100) {
            *stats[k] += statUp[k];
        } else if(BUGFIX_ON(BUGFIX_STAT_UP_REPORT)) {
            statUp[k] = 0;
        }
    }

    if(statUp[0] && !(ch->str & 1) && ch->damage != 200) {
        ch->damage++;
    }
    if(statUp[1] && ch->evade_percent != 200) {
        ch->evade_percent++;
    }

    //The NES checks for no weapon instead of no armor before setting absorb.
    if(cls == MONK_VALUE || cls == MASTER_VALUE) {
        Uint8 armed = FALSE;
        Uint8 armored = FALSE;
        for(Uint8 i = 0; i < 4; i++) {
            if(ch->weapons[i] & 0x80) {
                armed = TRUE;
            }
            if(ch->armors[i] & 0x80) {
                armored = TRUE;
            }
        }
        if(!armed) {
            ch->damage = ch->level * 2;
        }
        if(BUGFIX_ON(BUGFIX_MONK_ABSORB) ? !armored : !armed) {
            ch->absorb = ch->level;
        }
    }

    return TRUE;
}

static Uint8 checkLevelUp(PLAYER_CHARACTER *ch) {
    Uint8 leveled = FALSE;

    if(ch->level < 50 && ch->exp >= getExpToAdvance(ch->level - 1)) {
        leveled = levelUpCharacter(ch);
    }
    ch->exp_to_next_lvl = ch->level == 50 ? 0 : (Uint16)(getExpToAdvance(ch->level - 1) - ch->exp);
    return leveled;
}

//NES LvlUp_AwardAndUpdateExp. Only one level up per battle, like the NES.
static Uint8 awardExp(Uint8 index) {
    PLAYER_CHARACTER *ch = &party[index];

    if(!(ch->ailment & (AIL_DEAD | AIL_STONE))) {
        ch->exp += eobExpReward + (eobRewardHighByte << 16);
        if(ch->exp > 999999) {
            ch->exp = 999999;
        }
        return checkLevelUp(ch);
    }
    ch->exp_to_next_lvl = ch->level == 50 ? 0 : (Uint16)(getExpToAdvance(ch->level - 1) - ch->exp);
    return FALSE;
}

//"Str up!" etc. for each raised stat, then the final delay.
static void showNextStatUp() {
    while(statMsgIndex < 5 && !statUp[statMsgIndex]) {
        statMsgIndex++;
    }

    if(statMsgIndex < 5) {
        Uint8 buffer[16];
        Uint8 len = appendBattleMsg(buffer, 0, BTLMSG_STR + statMsgIndex);
        len = appendBattleMsg(buffer, len, BTLMSG_UP);
        buffer[len] = 0x00;
        drawCombatBoxText(4, buffer);
        victoryStep = VICTORY_STAT_UP;
        setBattleEndPhase(BATTLE_END_DELAY_INPUT);
    } else {
        victoryStep = VICTORY_LEVEL_DONE;
        setBattleEndPhase(BATTLE_END_DELAY);
    }
}

static void awardNextCharacter() {
    while(lvlUpChar < 4) {
        if(awardExp(lvlUpChar)) {
            drawLevelUpBoxes();
            statMsgIndex = 0;
            showNextStatUp();
            return;
        }
        lvlUpChar++;
    }
    startBattleFadeOut(TO_OW_STATE);
}

static void battleEndStepDone() {
    //The NES only undraws the stat box, the level up boxes stay.
    if(state == BAT_VICTORY_STATE && victoryStep == VICTORY_STAT_UP) {
        msgUndraw(1);
    } else {
        clearMessageArea();
    }

    if(state == GAME_OVER) {
        startBattleFadeOut(GAME_RESTART);
        return;
    }
    if(state == BAT_RUN_AWAY) {
        //The party flips once the boxes are undrawn.
        battleEndPhase = BATTLE_END_RUN_FLIP;
        return;
    }

    switch(victoryStep) {
        case VICTORY_PERISHED:
            computeBattleRewards();
            drawRewardBoxes();
            victoryStep = VICTORY_REWARDS;
            setBattleEndPhase(BATTLE_END_WAIT_INPUT);
            break;
        case VICTORY_REWARDS:
            lvlUpChar = 0;
            awardNextCharacter();
            break;
        case VICTORY_STAT_UP:
            statMsgIndex++;
            showNextStatUp();
            break;
        case VICTORY_LEVEL_DONE:
            if(BUGFIX_ON(BUGFIX_MULTI_LEVEL_UP) && checkLevelUp(&party[lvlUpChar])) {
                drawLevelUpBoxes();
                statMsgIndex = 0;
                showNextStatUp();
                break;
            }
            lvlUpChar++;
            awardNextCharacter();
            break;
    }
}

#define CHAOS_FORMATION (0x7B)
#define CHAOS_DISSOLVE_FRAMES (8 * 256)

volatile Uint32 *chaosEraseAddr = 0;
static Uint8 chaosTileRows[256];
static Uint16 chaosDissolveStep;
static Uint8 chaosPrevTile;
static Uint8 chaosNoiseVol; //NES $9C.

//NES ChaosDeath_FadeNoise: the low rumble's volume drops by 1 each call. The 17th call's $EF loops its decay.
static void chaosFadeNoise() {
    chaosNoiseVol--;
    musicApuWrite(0x400C, chaosNoiseVol);
    musicApuWrite(0x400D, 0xFF);
    musicApuWrite(0x400F, 0xFF);
    musicApuWrite(0x400E, 0x0F);
}

static void startChaosDissolve() {
    chaosNoiseVol = 0;
    chaosFadeNoise();
    for(Uint16 i = 0; i < 256; i++) {
        chaosTileRows[i] = battleRNG();
    }
    chaosDissolveStep = 0;
    chaosPrevTile = 0;
}

//NES ChaosDeath: each frame erases one pixel row of a random BG tile $00-$79 and shakes the BG.
static void chaosDissolveFrame() {
    //Halfway through each 256 erases.
    if((chaosDissolveStep & 0xFF) == 0x80) {
        chaosFadeNoise();
    }
    Uint8 tile = rand_min_max(0, 0x79);
    Uint8 row = chaosTileRows[tile] & 0x07;

    chaosEraseAddr = (volatile Uint32 *)(VID_VRAM_A1 + (tile * 32) + (row * 4));
    static_move_to(FIXED(chaosPrevTile & 0x03), FIXED(tile & 0x03));
    chaosPrevTile = tile;

    chaosDissolveStep++;
    //After every 256 erases, each tile's erase row moves down one.
    if((chaosDissolveStep & 0xFF) == 0) {
        for(Uint16 i = 0; i < 256; i++) {
            chaosTileRows[i]++;
        }
        chaosFadeNoise();
    }
}

//Victory, run away and game over sequences, one call per frame.
void doBattleEnd() {
    Uint8 respondRate = response_rate_lut[getResponseRate()];

    if(msgBusy()) {
        return;
    }

    switch(battleEndPhase) {
        case BATTLE_END_CHEER:
            //NES PlayFanfareAndCheer, after the combat boxes are undrawn.
            if(battleEndFrame == 0) {
                btlFollowupMusic = MUSIC_VICTORY;
                music_track = MUSIC_VICTORY;
            }
            //64 iterations of 2 frames, cheering while bit 3 of the NES down counter is set.
            if(battleEndFrame < 128) {
                party_cheer_pose((0x40 - (battleEndFrame >> 1)) & 0x08);
            } else if(battleEndFrame == 128) {
                player_bat_stand();
                party_crouch();
            } else if(battleEndFrame == 130) {
                //Chaos gives no rewards, the NES does its dissolve effect instead.
                if(battleFormation == CHAOS_FORMATION) {
                    //NES ChaosDeath's wait calls MusicPlay directly, so the fanfare isn't restarted when it ends.
                    btlFollowupMusic = MUSIC_ENDED;
                    battleEndPhase = BATTLE_END_CHAOS_WAIT;
                    //This frame is the first of ChaosDeath's wait.
                    battleEndFrame = 1;
                    break;
                }
                drawMonstersPerishedMessage();
                battleEndPhase = BATTLE_END_WAIT_INPUT;
            }
            battleEndFrame++;
            break;
        case BATTLE_END_CHAOS_WAIT:
            //110 frames for the fanfare's main jingle.
            battleEndFrame++;
            if(battleEndFrame == 110) {
                //NES ChaosDeath stops the music, leaving only the noise.
                music_track = MUSIC_ENDED;
                btlFollowupMusic = MUSIC_ENDED;
                musicApuWrite(0x4015, 0x08);
                startChaosDissolve();
                battleEndPhase = BATTLE_END_CHAOS_DISSOLVE;
            }
            break;
        case BATTLE_END_CHAOS_DISSOLVE:
            chaosDissolveFrame();
            if(chaosDissolveStep == CHAOS_DISSOLVE_FRAMES) {
                battleEndFrame = 0;
                battleEndPhase = BATTLE_END_CHAOS_PAUSE;
            }
            break;
        case BATTLE_END_CHAOS_PAUSE:
            battleEndFrame++;
            if(battleEndFrame == 120) {
                musicApuWrite(0x4015, 0);
                static_move_to(FIXED(0), FIXED(0));
                battleEndFrame = 0;
                battleEndPhase = BATTLE_END_CHAOS_EXIT;
                startBattleFadeOut(BAT_CHAOS_EXIT);
            }
            break;
        case BATTLE_END_CHAOS_EXIT:
            //NES ExitBattle waits 120 more frames after the fade out when Chaos is defeated.
            battleEndFrame++;
            if(battleEndFrame == 120) {
                storySelect(TRUE);
                state = TO_STORY;
            }
            break;
        case BATTLE_END_WAIT_INPUT:
        case BATTLE_END_INPUT:
            //The NES continues on any held button.
            if(PadData1 != 0) {
                ignoreA = TRUE;
                ignoreB = TRUE;
                ignoreC = TRUE;
                if(battleEndPhase == BATTLE_END_INPUT) {
                    battleEndStepDone();
                } else {
                    setBattleEndPhase(BATTLE_END_DELAY);
                }
            }
            break;
        case BATTLE_END_DELAY:
            if(respondRateCounter >= respondRate) {
                battleEndStepDone();
            }
            break;
        case BATTLE_END_DELAY_INPUT:
            if(respondRateCounter >= respondRate) {
                setBattleEndPhase(BATTLE_END_INPUT);
            }
            break;
        case BATTLE_END_RUN_FLIP:
            party_flip();
            startBattleFadeOut(TO_OW_STATE);
            break;
    }
}

//NES lut_MagicCursorPos and lut_PlayerTargetCursorPos. The port's menu cursors are 24 px right
//of the NES, and the command menu cursor is also 2 px lower.
static const Uint8 magicCursorXPos[3] = {0x20, 0x48, 0x70};
static const Uint8 magicCursorYPos[4] = {0xA6, 0xB6, 0xC6, 0xD6};
static const Uint8 partyTargetCursorYPos[4] = {0x34, 0x4C, 0x64, 0x7C};

static Uint8 nothingText[8] = {0x97, 0xB2, 0xB7, 0xAB, 0xAC, 0xB1, 0xAA, 0x00};

static void setMagicCursor() {
    cursor.xPos = FIXED(magicCursorXPos[magicCursorX] + 24);
    cursor.yPos = FIXED(magicCursorYPos[magicCursorY] + 2);
}

//Dead characters' cursor is 8 px left, the caster's is 16 px left since they stepped forward.
static void setPartyTargetCursor() {
    Uint8 x = 0xA0 + 24;
    if(isPlayerDead(magicCursorY) || (battle_char_stats[magicCursorY].ailment & AIL_DEAD)) {
        x -= 8;
    }
    if(magicCursorY == cur_char_battle_menu) {
        x -= 16;
    }
    cursor.xPos = FIXED(x);
    cursor.yPos = FIXED(partyTargetCursorYPos[magicCursorY]);
}

//NES MenuSelection_Magic always opens on page 0 with the cursor on the first spell.
static void openMagicMenu() {
    magicPage = 0;
    magicCursorX = 0;
    magicCursorY = 0;
    drawMagicMenu(cur_char_battle_menu, 0);
    setMagicCursor();
    drawBattleCursor = FALSE;
    bat_menu_state = SELECT_MAGIC;
}

//Magic menu columns wrap. Moving past the bottom of page 0 or the top of page 1 switches pages.
static void moveSubMenuCursor(Sint8 dx, Sint8 dy) {
    if(bat_menu_state == SELECT_PARTY_TARGET) {
        magicCursorY = (magicCursorY + dy) & 0x03;
        setPartyTargetCursor();
        return;
    }
    //NES MenuSelection_2x4 wraps both ways.
    if(bat_menu_state == SELECT_DRINK || bat_menu_state == SELECT_ITEM) {
        magicCursorX = (magicCursorX + dx) & 0x01;
        magicCursorY = (magicCursorY + dy) & 0x03;
        setItemCursor();
        return;
    }

    if(dx < 0) {
        magicCursorX = magicCursorX == 0 ? 2 : magicCursorX - 1;
    } else if(dx > 0) {
        magicCursorX = magicCursorX == 2 ? 0 : magicCursorX + 1;
    }

    if(dy > 0) {
        if(magicCursorY < 3) {
            magicCursorY++;
        } else if(magicPage == 0) {
            magicPage = 1;
            magicCursorY = 0;
            msgUndraw(1);
            drawMagicMenu(cur_char_battle_menu, 1);
        }
    } else if(dy < 0) {
        if(magicCursorY > 0) {
            magicCursorY--;
        } else if(magicPage == 1) {
            magicPage = 0;
            magicCursorY = 3;
            msgUndraw(1);
            drawMagicMenu(cur_char_battle_menu, 0);
        }
    }
    setMagicCursor();
}

static void setMagicAction(Uint8 targetNum, Uint8 isPlayerTarget) {
    updatePlayerBattleAction(cur_char_battle_menu, targetNum, MAGIC, selectedSpell, isPlayerTarget);
    playerActions[cur_char_battle_menu].mpLevel = selectedMpLevel;
    drawBattleCursor = FALSE;
    bat_menu_state = END_SELECT;
}

static void setEnemyTargetAction(Uint8 slot) {
    if(cur_battle_menu_selection == MAGIC) {
        setMagicAction(slot, FALSE);
        return;
    }
    updatePlayerBattleAction(cur_char_battle_menu, slot, cur_battle_menu_selection, 0, FALSE);
    bat_menu_state = END_SELECT;
}

//NES BattleSubMenu_Magic. An empty slot or no MP for the level shows "Nothing".
static void selectMagicSpell() {
    Uint8 level = (magicPage * 4) + magicCursorY;
    Uint8 spell = battle_char_stats[cur_char_battle_menu].magic[level][magicCursorX];

    msgUndraw(1);
    drawBattleCursor = FALSE;
    if(spell == 0 || battle_char_stats[cur_char_battle_menu].current_mp[level] == 0) {
        drawCombatBoxText(4, nothingText);
        bat_menu_state = MAGIC_NOTHING;
        return;
    }

    selectedSpell = spell - 1;
    selectedMpLevel = level;
    lastMagicPalette[cur_char_battle_menu] = getMagicData(selectedSpell)[MAGDATA_PALETTE];
    Uint8 target = getMagicData(selectedSpell)[MAGDATA_TARGET];
    if(target & MAGIC_TARGET_ALL_ENEMIES) {
        setMagicAction(0, FALSE);
    } else if(target & MAGIC_TARGET_ONE_ENEMY) {
        bat_menu_state = TO_SELECT_ENEMY_TARGET;
    } else if(target & MAGIC_TARGET_SELF) {
        setMagicAction(cur_char_battle_menu, TRUE);
    } else if(target & MAGIC_TARGET_ALL_PARTY) {
        setMagicAction(0, TRUE);
    } else {
        magicCursorY = 0;
        setPartyTargetCursor();
        bat_menu_state = SELECT_PARTY_TARGET;
    }
}

//NES lut_DrinkCursorPos and lut_ItemCursorPos. The drink rows repeat Heal and Pure.
static const Uint8 itemCursorXPos[2] = {0x10, 0x58};

static void setItemCursor() {
    if(bat_menu_state == SELECT_DRINK) {
        cursor.xPos = FIXED(0x18 + 24);
        cursor.yPos = FIXED(magicCursorYPos[magicCursorY & 0x01] + 2);
    } else {
        cursor.xPos = FIXED(itemCursorXPos[magicCursorX] + 24);
        cursor.yPos = FIXED(magicCursorYPos[magicCursorY] + 2);
    }
}

//NES DoNothingMessageBox. DRINK and ITEM cancel the action after it.
static void showNothingBox() {
    drawBattleCursor = FALSE;
    drawCombatBoxText(4, nothingText);
    bat_menu_state = MAGIC_NOTHING;
}

//NES BattleSubMenu_Drink: "Nothing" without potions, otherwise the cursor starts on Heal.
static void openDrinkMenu() {
    drawBattleCursor = FALSE;
    if(battlePotions[0] == 0 && battlePotions[1] == 0) {
        showNothingBox();
        return;
    }
    drawDrinkMenu(battlePotions[0], battlePotions[1]);
    magicCursorX = 0;
    magicCursorY = 0;
    bat_menu_state = SELECT_DRINK;
    setItemCursor();
}

static void selectDrink() {
    msgUndraw(1);
    drawBattleCursor = FALSE;
    selectedPotion = magicCursorY & 0x01;
    if(battlePotions[selectedPotion] == 0) {
        showNothingBox();
        return;
    }
    magicCursorY = 0;
    setPartyTargetCursor();
    bat_menu_state = SELECT_PARTY_TARGET;
}

//The potion is taken from the battle count now, and from the inventory when it's drunk.
static void setDrinkAction(Uint8 targetNum) {
    updatePlayerBattleAction(cur_char_battle_menu, targetNum, DRINK, DRINK_HEAL + selectedPotion, TRUE);
    battlePotions[selectedPotion]--;
    drinkReserved[cur_char_battle_menu] = selectedPotion;
    drawBattleCursor = FALSE;
    bat_menu_state = END_SELECT;
}

//NES BattleSubMenu_Item: "Nothing" if the character holds no equipment.
static void openItemMenu() {
    PLAYER_CHARACTER *ch = &party[cur_char_battle_menu];
    Uint8 held = 0;

    drawBattleCursor = FALSE;
    for(Uint8 i = 0; i < 4; i++) {
        held |= ch->weapons[i] | ch->armors[i];
    }
    if(held == 0) {
        showNothingBox();
        return;
    }
    drawItemMenu(cur_char_battle_menu);
    magicCursorX = 0;
    magicCursorY = 0;
    bat_menu_state = SELECT_ITEM;
    setItemCursor();
}

//The item casts the spell in byte 3 of its weapon or armor data, with no target selection. The
//target is the user's own index, so a one-enemy spell hits the enemy in that slot (NES bug).
static void selectItem() {
    PLAYER_CHARACTER *ch = &party[cur_char_battle_menu];
    Uint8 isArmor = magicCursorX & 0x01;
    Uint8 slot = magicCursorY & 0x03;
    Uint8 equip = (isArmor ? ch->armors[slot] : ch->weapons[slot]) & 0x7F;
    Uint8 *data;
    Uint8 itemId;

    msgUndraw(1);
    drawBattleCursor = FALSE;
    if(equip == 0) {
        showNothingBox();
        return;
    }
    if(isArmor) {
        itemId = ITEM_ARMOR_START - 1 + equip;
        data = (Uint8*) ARMOR_LUT_LOCATION + ((equip - 1) * 4);
    } else {
        itemId = ITEM_WEAPON_START - 1 + equip;
        data = (Uint8*) WEAPON_LUT_LOCATION + ((equip - 1) * 8);
    }
    //No spell (0) becomes 0xFF.
    updatePlayerBattleAction(cur_char_battle_menu, cur_char_battle_menu, ITEM, data[3] - 1, TRUE);
    playerActions[cur_char_battle_menu].itemId = itemId;
    bat_menu_state = END_SELECT;
}

void doBattleMenu() {
    if(msgBusy()) {
        return;
    }

    if(bat_menu_state == SELECT || bat_menu_state == SELECT_ENEMY_TARGET || isSubMenuState()) {
        drawBattleCursor = TRUE;
    } else {
        drawBattleCursor = FALSE;
    }

    if(bat_menu_state == SELECT_MAGIC || bat_menu_state == TO_SELECT_MAGIC) {
        drawTargetNames = FALSE;
    } else {
        drawTargetNames = TRUE;
    }

    switch(bat_menu_state) {
        case INIT:
            cur_battle_menu_selection = FIGHT;
            prev_battle_menu_selection = FIGHT;
            if (!canInputCommand(cur_char_battle_menu)) {
                cur_char_battle_menu++;
                if(cur_char_battle_menu == 4) {
                    cur_char_battle_menu = 0;
                    bat_menu_state = END;
                }
            } else {
                if(isPlayerCrouching(cur_char_battle_menu, party[cur_char_battle_menu].max_hp)) {
                    player_stand(cur_char_battle_menu);
                }
                isMoving = TRUE;
                moveBattlePlayerLeft(cur_char_battle_menu);
            }
            break;
        case SELECT:
            break;
        case SELECT_CANCEL:
        case CANCEL_ACTION:
            if(isPlayerCrouching(cur_char_battle_menu, party[cur_char_battle_menu].max_hp)) {
                player_stand(cur_char_battle_menu);
            }
            isMoving = TRUE;
            moveBattlePlayerRight(cur_char_battle_menu);
            break;
        case TO_SELECT_ENEMY_TARGET:
            //NES EnemyTargetMenu_FiendChaos: no cursor, the only target is picked.
            if(battle.battle_type >= 3) {
                setEnemyTargetAction(0);
                break;
            }
            currentSlot = getNextOccupiedSlot(0);

            if(!canMoveCursor) {
                cursor.xPos = FIXED(getXSlotCoord(currentSlot));
                cursor.yPos = FIXED(getYSlotCoord(currentSlot));
                frame_count_b = 0;
                canMoveCursor = 1;
                bat_menu_state = SELECT_ENEMY_TARGET;
            }
            break;
        case MAGIC_NOTHING:
            //NES DoNothingMessageBox: any button, then the magic menu again, or the action is cancelled.
            if(PadData1 != 0) {
                ignoreA = TRUE;
                ignoreB = TRUE;
                ignoreC = TRUE;
                msgUndraw(1);
                if(cur_battle_menu_selection == MAGIC) {
                    openMagicMenu();
                } else {
                    bat_menu_state = CANCEL_ACTION;
                }
            }
            break;
        case SELECT_ENEMY_TARGET:
            break;
        case END_SELECT:
            if(isPlayerCrouching(cur_char_battle_menu, party[cur_char_battle_menu].max_hp)) {
                player_stand(cur_char_battle_menu);
            }
            isMoving = TRUE;
            moveBattlePlayerRight(cur_char_battle_menu);           
            break;
        case END:
            currentTurnBufferIndex = 0;
            state = TO_BAT_EXECUTE_STATE;
        default:
            break;
    }
}

void startBattleTransition() {
    battleTransitionFrame = 0;
    //Only the noise plays during the transition.
    musicApuWrite(0x4015, 0x08);
    state = BAT_TRANSITION;
}

static void startOwPalCycle(Uint8 reverse, Uint8 nextState) {
    scroll_pal_cycle_init(reverse);
    palCycReverse = reverse;
    palCycNextState = nextState;
    //Cycling out starts from input, which runs before this tick's doOwPalCycle call.
    palCycFrame = reverse ? 0 : 0xFF;
    state = OW_PAL_CYCLE;
}

//NES CyclePalettes: a frame to hide the sprites, then a step every 4th frame until no colors change.
void doOwPalCycle() {
    if(palCycFrame != 0 && (palCycFrame & 0x03) == 0) {
        if(scroll_pal_cycle_step(palCycReverse) == 0) {
            state = palCycNextState;
            return;
        }
        scroll_pal_cycle_draw();
    }
    palCycFrame++;
}

//Menus and battles return through the NES EnterOW_PalCyc reverse cycle.
void exitOwMenu() {
    if(menuReturnSM) {
        menuReturnSM = FALSE;
        smReenterMap();
        return;
    }
    owPalCycleIn = TRUE;
    state = TO_OW_STATE;
}

static void owWipeOpened() {
    state = OW_STATE;
}

static void owWipeClosedToSM() {
    state = TO_SM_STATE;
}

//Called once TO_OW_STATE has set up the map.
void player_ow_enter() {
    battleReturnSM = FALSE;
    owStartLatched = FALSE;
    owSelectNext = OW_NO_MENU;
    //NES PrepOverworld.
    musicApuWrite(0x4015, 0);
    music_track = musicVehicleTrack(vehicle);
    owTileprop = 0;
    owTileprop1 = 0;
    owEnteringShop = FALSE;
    owFacing = 0;
    owJoyA = FALSE;
    owJoyB = 0;
    isMoving = FALSE;
    owMoveSpeed = 0;
    owMoveCtr = 0;
    owAirAnim = OW_AIR_NONE;
    owAirshipShown = airshipVisible >= 0x1F;
    owLoadObjectSprites();
    owSetTilt(vehicle == VEH_AIRSHIP ? FIXED(OW_TILT_ANGLE) : 0);
    //Sets the scroll player_ow_draw would, since the wipe and cycle in run before it.
    scroll_set(SCROLL_BACKGROUND1, player.xPos, player.yPos);
    if(owWipeIn) {
        owWipeIn = FALSE;
        owPalCycleIn = FALSE;
        wipeStart(TRUE, VID_RBG0, owWipeOpened);
    } else if(owPalCycleIn) {
        owPalCycleIn = FALSE;
        startOwPalCycle(TRUE, OW_STATE);
    } else {
        state = OW_STATE;
    }
}

//NES lut_PlayerMapmanSprTbl. facing holds the NES direction bits (1 right, 2 left, 4 down, 8 up), lowest bit first. anim picks the second walk frame.
void setMapmanSprite(Uint8 facing, Uint8 anim) {
    if(facing & 0x03) {
        player.char_num = (anim ? FRAME_WALK3 : FRAME_STAND3) + currentChar;
        playerb.char_num = (anim ? FRAME_WALK4 : FRAME_STAND4) + currentChar;
        player.mirror = (facing & 0x01) ? MIRROR_HORIZ : 0;
        playerb.mirror = player.mirror;
    } else {
        Uint8 up = !(facing & 0x04);
        player.char_num = (up ? FRAME_STAND2 : FRAME_STAND) + currentChar;
        playerb.char_num = (up ? FRAME_WALK2 : FRAME_WALK1) + currentChar;
        player.mirror = 0;
        playerb.mirror = anim ? MIRROR_HORIZ : 0;
    }
}

//Places the party on overworld tile x,y, with the scroll the overworld uses for it.
void setOWPosition(Uint8 x, Uint8 y) {
    current_x_pos = x;
    current_y_pos = y;
    prev_current_x_pos = x;
    prev_current_y_pos = y;
    player.xPos = FIXED(x * 16 - 152);
    player.yPos = FIXED(y * 16 - 111);
    playerb.xPos = player.xPos;
    playerb.yPos = player.yPos + FIXED(8);
}

Uint8 getOWVehicle() {
    return vehicle;
}

void setOWVehicle(Uint8 v) {
    vehicle = v;
    vehicleNext = v;
}

//The box is 2 tiles wider than the NES for 6 character names, so names and cursor draw 8 px left and sprites 8 px right.
#define LINEUP_NAME_X (0x0C)
#define LINEUP_CURSOR_X (0x58 + 32 - 8)
#define LINEUP_SPRITE_SHIFT (32 + 8)

//NES lineup slot table (str_buf), in NES screen coordinates.
typedef struct {
    Uint8 dead; //Dead or stone.
    Uint8 member; //Party index of the character in this slot.
    Uint8 x;
    Uint8 y;
    Uint8 destX;
    Uint8 destY;
} LINEUP_SLOT;

static LINEUP_SLOT lineupSlots[4];
static SPRITE_INFO lineupSprites[4];
static Uint8 luMode;
static Uint8 luCursor;
static Uint8 luCursor2;
static PadBits luJoyPrev;
static PadBits luPadPrev;
//NES joy_a/joy_b catchers. They're only cleared in modes 0 and 2, so presses during an animation carry over.
static Uint8 luLatchA;
static Uint8 luLatchB;

static void drawLineupNames() {
    Uint8 name[7];

    for(Uint8 i = 0; i < 4; i++) {
        memcpy(name, party[lineupSlots[i].member].name, 6);
        name[6] = 0x00;
        drawTextLine(LINEUP_NAME_X, 0x08 + (i * 4), name, 0, 1);
    }
}

void initLineupMenu() {
    for(Uint8 i = 0; i < 4; i++) {
        lineupSlots[i].dead = (party[i].ailment == AIL_DEAD || party[i].ailment == AIL_STONE);
        lineupSlots[i].member = i;
        lineupSlots[i].x = 0x98;
        lineupSlots[i].y = 0x38 + (i * 0x20);
        lineupSlots[i].destX = 0x98;
        lineupSlots[i].destY = lineupSlots[i].y;
        sprite_make(FRAME_BATTLE_START, 0, 0, (Sint32) CHARACTER_PRIORITY, &lineupSprites[i]);
    }
    drawLineupNames();
    player_cursor_init(FIXED(LINEUP_CURSOR_X), FIXED(0x38 + 8));

    luMode = 0;
    luCursor = 0;
    luCursor2 = 0;
    luLatchA = FALSE;
    luLatchB = FALSE;
    luPadPrev = PadData1;
    luJoyPrev = PadData1 & (PAD_U | PAD_D);
}

//NES LineupMenu_AnimStep: moves a slot's sprite 1 px toward its destination, Y first. Returns TRUE if it moved.
static Uint8 lineupAnimStep(Uint8 slot) {
    LINEUP_SLOT *s = &lineupSlots[slot];

    if(s->y != s->destY) {
        s->y += (s->y > s->destY) ? -1 : 1;
        return TRUE;
    }
    if(s->x != s->destX) {
        s->x += (s->x > s->destX) ? -1 : 1;
        return TRUE;
    }
    return FALSE;
}

//NES LineupMenu_ProcessMode. Modes: 0 pick, 1 first slides left, 2 pick second, 3 second slides right, 4 both move home.
static void lineupProcessMode() {
    switch(luMode) {
        case 1:
            if(lineupSlots[luCursor2].x < 0x89) {
                luMode++;
            } else {
                lineupSlots[luCursor2].x--;
            }
            break;
        case 3:
            if(lineupSlots[luCursor].x >= 0xA8) {
                luMode++;
            } else {
                lineupSlots[luCursor].x++;
            }
            break;
        case 4:
            //When both cursors are the same slot, it moves twice per frame.
            if(lineupAnimStep(luCursor)) {
                lineupAnimStep(luCursor2);
                break;
            }
            if(lineupAnimStep(luCursor2)) {
                break;
            }
            LINEUP_SLOT tmp = lineupSlots[luCursor];
            lineupSlots[luCursor] = lineupSlots[luCursor2];
            lineupSlots[luCursor2] = tmp;
            luMode = 0;
            luCursor = luCursor2;
            drawLineupNames();
            break;
        default:
            break;
    }
}

//NES LineupMenu_Finalize.
static void lineupFinalize() {
    PLAYER_CHARACTER oldParty[4];

    memcpy(oldParty, party, sizeof(oldParty));
    for(Uint8 i = 0; i < 4; i++) {
        party[i] = oldParty[lineupSlots[i].member];
    }
}

//NES LineupMenu_ProcessJoy.
static void lineupProcessJoy() {
    if(luMode != 0 && luMode != 2) {
        return;
    }

    if(luLatchB) {
        if(luMode == 0) {
            lineupFinalize();
            exitOwMenu();
            return;
        }
        luMode = 4;
        luCursor = luCursor2;
        luLatchB = FALSE;
        return;
    }

    if(luLatchA) {
        luLatchA = FALSE;
        if(luMode == 0) {
            luMode = 1;
            luCursor2 = luCursor;
            return;
        }
        //Picking the same slot, or a swap that would put a dead or stone character in the lead, is cancelled.
        if(luCursor == luCursor2 || (luCursor == 0 && lineupSlots[luCursor2].dead) || (luCursor2 == 0 && lineupSlots[luCursor].dead)) {
            luCursor = luCursor2;
            luMode = 3;
            return;
        }
        Uint8 tmpY = lineupSlots[luCursor].destY;
        lineupSlots[luCursor].destY = lineupSlots[luCursor2].destY;
        lineupSlots[luCursor2].destY = tmpY;
        luMode = 3;
        return;
    }

    //Moves only when the Up/Down state changes, with no repeat. Both held counts as Down.
    PadBits dir = PadData1 & (PAD_U | PAD_D);
    if(dir == luJoyPrev) {
        return;
    }
    luJoyPrev = dir;
    if(dir == PAD_U) {
        luCursor = (luCursor - 1) & 0x03;
    } else if(dir != 0) {
        luCursor = (luCursor + 1) & 0x03;
    }
}

//NES lineup main loop: catch presses, animate, then process input.
void doLineupMenu() {
    PadBits pressed = PadData1 & ~luPadPrev;
    PadBits dir = PadData1 & (PAD_U | PAD_D);
    PadBits prevDir = luPadPrev & (PAD_U | PAD_D);
    luPadPrev = PadData1;

    if(pressed & (PAD_A | PAD_C)) {
        luLatchA = TRUE;
    }
    if(pressed & PAD_B) {
        luLatchB = TRUE;
    }
    //NES LineupMenu_UpdateJoy. The latches aren't cleared during the animations, so the sound restarts every frame.
    if(luLatchA || luLatchB) {
        sfxMenuSel();
    } else if(dir && dir != prevDir) {
        sfxMenuMove();
    }
    lineupProcessMode();
    lineupProcessJoy();
}

//NES DrawOBSprite: dead characters aren't drawn, poisoned are crouched, stoned are crouched in the stone palette.
void drawOBSprite(SPRITE_INFO *sprite, Uint8 member, Uint16 x, Uint16 y) {
    PLAYER_CHARACTER *c = &party[member];

    if(c->ailment == AIL_DEAD) {
        return;
    }
    sprite->char_num = FRAME_BATTLE_START + (c->char_class * 6);
    sprite->xPos = FIXED(x);
    sprite->yPos = FIXED(y);
    if(c->ailment == AIL_STONE) {
        sprite->char_num += 4;
        sprite_draw_pal(sprite, STONE_PALETTE_CRAM);
    } else {
        if(c->ailment == AIL_POISON) {
            sprite->char_num += 4;
        }
        sprite_draw(sprite);
    }
}

//NES LineupMenu_DrawCharSprites and LineupMenu_DrawCursor.
void lineup_draw() {
    for(Uint8 i = 0; i < 4; i++) {
        LINEUP_SLOT *s = &lineupSlots[i];
        drawOBSprite(&lineupSprites[i], s->member, s->x + LINEUP_SPRITE_SHIFT, s->y);
    }
    cursor.xPos = FIXED(LINEUP_CURSOR_X);
    cursor.yPos = FIXED(lineupSlots[luCursor].y + 8);
    player_cursor_draw();
}

//NES BattleTransition: 65 frames with sprites hidden, changing the emphasis mode every 4 frames.
void doBattleTransition() {
    if(battleTransitionFrame < 65) {
        if((battleTransitionFrame & 0x03) == 0) {
            if(battleReturnSM) {
                smSetEmphasis((battleTransitionFrame >> 2) & 0x07);
            } else {
                scroll_set_emphasis((battleTransitionFrame >> 2) & 0x07);
            }
        }
        //Restarting the decay each frame holds the noise at full volume.
        musicApuWrite(0x400C, 0x0F);
        musicApuWrite(0x400E, (battleTransitionFrame >> 1) | 0x03);
        musicApuWrite(0x400F, 0);
        battleTransitionFrame++;
    } else {
        musicApuWrite(0x4015, 0);
        //The NES turns the screen off before setting up the battle.
        if(battleReturnSM) {
            sm_disable();
        } else {
            scroll_disable();
        }
        state = TO_BAT_STATE;
    }
}

//NES ReSortPartyByAilment: dead, stone, then poisoned characters move to the back.
//Matches the NES bug where a swap uses the character IDs in the sort buffer instead of the
//two slots compared, which can scramble the order of the other characters.
void resortPartyByAilment() {
    Uint8 order[4];

    for(Uint8 i = 0; i < 4; i++) {
        order[i] = i;
        if(party[i].ailment == AIL_DEAD) {
            order[i] += 0x40;
        } else if(party[i].ailment == AIL_STONE) {
            order[i] += 0x20;
        } else if(party[i].ailment == AIL_POISON) {
            order[i] += 0x10;
        }
    }

    for(Uint8 pass = 0; pass < 4; pass++) {
        for(Uint8 slot = 0; slot < 3; slot++) {
            if(order[slot] >= order[slot + 1]) {
                Uint8 a = BUGFIX_ON(BUGFIX_PARTY_SORT) ? slot : order[slot] & 0x03;
                Uint8 b = BUGFIX_ON(BUGFIX_PARTY_SORT) ? slot + 1 : order[slot + 1] & 0x03;
                PLAYER_CHARACTER tmpChar = party[a];
                party[a] = party[b];
                party[b] = tmpChar;
                Uint8 tmp = order[a];
                order[a] = order[b];
                order[b] = tmp;
            }
        }
    }
}

void startBattleFadeOut(Uint8 nextState) {
    battleFadeNextState = nextState;
    battleFadeFrame = 0;
    state = BAT_FADE_OUT;
}

//Matches the NES BattleFadeIn/BattleFadeOut timing, one call per frame.
void doBattleFade() {
    Uint8 shades;

    //Boxes finish undrawing before the fade out.
    if(msgBusy()) {
        return;
    }

    if(state == BAT_FADE_IN) {
        //A blank frame, then 4 shades down to 0.
        shades = 4;
        if(battleFadeFrame > 0) {
            shades = 4 - ((battleFadeFrame - 1) / BATTLE_FADE_STEP_FRAMES);
        }
        if(shades != battleFadeShade) {
            setBattleShade(shades);
        }
        if(shades == 0) {
            battleFadeFrame = 0;
            state = BAT_DRAW_STATUS;
            return;
        }
    } else {
        shades = 1 + (battleFadeFrame / BATTLE_FADE_STEP_FRAMES);
        if(shades > 4) {
            restoreBattleSpritePalettes();
            //The NES sorts before its fade out, which doesn't redraw the party, so sorting once black looks the same.
            if(battleFadeNextState == TO_OW_STATE || battleFadeNextState == BAT_CHAOS_EXIT) {
                resortPartyByAilment();
            }
            if(battleFadeNextState == TO_OW_STATE && battleReturnSM) {
                battleReturnSM = FALSE;
                smReenterMap();
                return;
            }
            if(battleFadeNextState == TO_OW_STATE) {
                owPalCycleIn = TRUE;
            }
            state = battleFadeNextState;
            return;
        }
        if(shades != battleFadeShade) {
            setBattleShade(shades);
        }
    }
    battleFadeFrame++;
}

//The NES shows the party for a frame, then draws one character's status per frame.
void doBattleDrawStatus() {
    if(battleFadeFrame > 0) {
        drawCharacterStatus(battleFadeFrame - 1);
    }
    battleFadeFrame++;
    if(battleFadeFrame == 5) {
        state = BAT_STATE;
    }
}

//NES Battle_DoPlayerTurn and Player_DoMagicEffect for MAGIC, DRINK and ITEM: spends the MP or
//potion and draws the attacker and attack boxes. A silenced caster doesn't walk forward. DRINK,
//ITEM and spells without a battle effect are cast without the magic sprite or flash.
static void startMagicTurn(Uint8 playerIndex) {
    TURN_ACTION *act = &playerActions[playerIndex];
    BATTLE_CHARACTER *caster = &battle_char_stats[playerIndex];
    Uint8 *data;
    Uint8 *name;
    Uint8 target;

    magicTurn = TRUE;
    magicCasterIsEnemy = FALSE;
    magicSpell = act->secondaryActionId;
    setSpellCaster(act->action == MAGIC, party[playerIndex].intel, party[playerIndex].luck);
    if(act->action == MAGIC) {
        caster->current_mp[act->mpLevel]--;
        name = getSpellName(magicSpell);
    } else if(act->action == DRINK) {
        if(magicSpell == DRINK_HEAL) {
            items[ITEM_HEAL]--;
        } else {
            items[ITEM_PURE]--;
        }
        name = getItemName(ITEM_HEAL + magicSpell - DRINK_HEAL);
    } else {
        name = getItemName(act->itemId);
    }

    drawAttackerName(caster->name);
    drawAttackMessage(name);
    magicBoxCount = 2;

    magicOpCount = 0;
    magicOpIndex = 0;
    magicOpStarted = FALSE;
    magicAbort = FALSE;
    magicNoDraw = act->action != MAGIC;
    magicNextTarget = 0;

    //The NES also blocks Item and Drink.
    if((caster->ailment & AIL_MUTE) && (act->action == MAGIC || !BUGFIX_ON(BUGFIX_MUTE_ITEMS))) {
        addMagicAltMessage(ALTBTLMSG_SILENCED_1, act->targetIsPlayer);
        magicAbort = TRUE;
        magicStep = MAGIC_STEP_OPS;
        turn_exec_state = MAGIC_EFFECT;
        return;
    }
    //An item without a spell is 0xFF.
    if(magicSpell == 0xFF || getMagicData(magicSpell)[MAGDATA_EFFECT] == 0) {
        addMagicAltMessage(act->action == ITEM ? ALTBTLMSG_NOTHINGHAPPENS : ALTBTLMSG_INEFFECTIVENOW, act->targetIsPlayer);
        magicAbort = TRUE;
        magicNoDraw = TRUE;
        turn_exec_state = ANIMATE1;
        return;
    }

    data = getMagicData(magicSpell);
    magicGfx = data[MAGDATA_GRAPHIC];
    //White is flashed as gray.
    magicFlashColor = data[MAGDATA_PALETTE] == 0x20 ? 0x10 : data[MAGDATA_PALETTE];

    target = data[MAGDATA_TARGET];
    if(target & MAGIC_TARGET_ALL_ENEMIES) {
        magicMode = MAGIC_MODE_ALL_ENEMIES;
    } else if(target & MAGIC_TARGET_ONE_ENEMY) {
        magicMode = MAGIC_MODE_ONE_ENEMY;
    } else if(target & MAGIC_TARGET_SELF) {
        magicMode = MAGIC_MODE_SELF;
    } else if(target & MAGIC_TARGET_ALL_PARTY) {
        magicMode = MAGIC_MODE_ALL_PARTY;
    } else {
        magicMode = MAGIC_MODE_ONE_ALLY;
    }
    //NES PreparePlayerMagAttack plays it before the walk forward, for spells only.
    if(act->action == MAGIC) {
        battleSfx(BTL_SFX_MAGIC);
    }
    turn_exec_state = ANIMATE1;
}

//NES Enemy_DoMagicEffect, for spells and special attacks. Enemies cast without an animation.
//A confused enemy casts FIRE on a random enemy, without the attack box or the silence check.
static void startEnemyMagicTurn(Uint8 enemyIndex, Uint8 attackId, Uint8 confused) {
    ENEMY_DATA *caster = &enemies[enemyIndex];
    Uint8 target;

    magicTurn = TRUE;
    magicCasterIsEnemy = TRUE;
    setSpellCaster(FALSE, 0, 0);
    magicConfusedFire = confused;
    magicSpell = attackId;

    drawAttackerName(caster->name);
    magicBoxCount = 1;

    magicOpCount = 0;
    magicOpIndex = 0;
    magicOpStarted = FALSE;
    magicAbort = FALSE;
    magicNoDraw = FALSE;
    magicNextTarget = 0;
    magicStep = MAGIC_STEP_NEXT_TARGET;
    turn_exec_state = MAGIC_EFFECT;

    if(confused) {
        magicMode = MAGIC_MODE_RAND_ALLY;
        return;
    }
    drawAttackMessage(getAttackName(attackId));
    magicBoxCount = 2;

    //The NES draws the attack box a second time for a silenced enemy.
    if(caster->ailment & AIL_MUTE) {
        drawAttackMessage(getAttackName(attackId));
        magicBoxCount = 3;
        addMagicAltMessage(ALTBTLMSG_INEFFECTIVE, FALSE);
        magicAbort = TRUE;
        magicStep = MAGIC_STEP_OPS;
        return;
    }

    target = getMagicData(attackId)[MAGDATA_TARGET];
    if(target & MAGIC_TARGET_ALL_ENEMIES) {
        magicMode = MAGIC_MODE_ALL_PLAYERS;
    } else if(target & MAGIC_TARGET_ONE_ENEMY) {
        magicMode = MAGIC_MODE_RAND_PLAYER;
    } else if(target & MAGIC_TARGET_SELF) {
        magicMode = MAGIC_MODE_ENEMY_SELF;
    } else if(target & MAGIC_TARGET_ALL_PARTY) {
        magicMode = MAGIC_MODE_ALL_ALLIES;
    } else {
        magicMode = MAGIC_MODE_RAND_ALLY;
    }
}

//Draws the next target's defender box. Returns FALSE when there are no targets left.
static Uint8 startNextMagicTarget() {
    Uint8 caster = currentTurn - 0x80;

    switch(magicMode) {
        case MAGIC_MODE_ALL_ENEMIES:
            while(magicNextTarget < 9 && enemies[magicNextTarget].hp == 0) {
                magicNextTarget++;
            }
            if(magicNextTarget >= 9) {
                return FALSE;
            }
            magicDefender = magicNextTarget++;
            magicDefIsPlayer = FALSE;
            break;
        case MAGIC_MODE_ALL_PARTY:
            if(magicNextTarget >= 4) {
                return FALSE;
            }
            magicDefender = magicNextTarget++;
            magicDefIsPlayer = TRUE;
            break;
        case MAGIC_MODE_ALL_PLAYERS:
            while(magicNextTarget < 4 && (battle_char_stats[magicNextTarget].ailment & (AIL_DEAD | AIL_STONE))) {
                magicNextTarget++;
            }
            if(magicNextTarget >= 4) {
                return FALSE;
            }
            magicDefender = magicNextTarget++;
            magicDefIsPlayer = TRUE;
            break;
        case MAGIC_MODE_ALL_ALLIES:
            while(magicNextTarget < 9 && ((magicNextTarget == currentTurn && !BUGFIX_ON(BUGFIX_ALLY_SPELLS))
                                          || enemies[magicNextTarget].hp == 0)) {
                magicNextTarget++;
            }
            if(magicNextTarget >= 9) {
                return FALSE;
            }
            magicDefender = magicNextTarget++;
            magicDefIsPlayer = FALSE;
            break;
        case MAGIC_MODE_RAND_PLAYER:
        case MAGIC_MODE_ENEMY_SELF:
        case MAGIC_MODE_RAND_ALLY:
            if(magicNextTarget > 0) {
                return FALSE;
            }
            magicNextTarget = 1;
            magicDefIsPlayer = magicMode == MAGIC_MODE_RAND_PLAYER;
            if(magicMode == MAGIC_MODE_RAND_PLAYER) {
                magicDefender = determinePlayerTarget();
            } else if(magicMode == MAGIC_MODE_ENEMY_SELF) {
                magicDefender = currentTurn;
            } else {
                do {
                    magicDefender = rand_min_max(0, 8);
                } while(enemies[magicDefender].hp == 0);
            }
            break;
        default:
            if(magicNextTarget > 0) {
                return FALSE;
            }
            magicNextTarget = 1;
            magicDefIsPlayer = magicMode != MAGIC_MODE_ONE_ENEMY;
            magicDefender = magicMode == MAGIC_MODE_SELF ? caster : playerActions[caster].targetIndex;
            break;
    }

    //Casting on all players flashes each one before their box is drawn.
    if(magicMode != MAGIC_MODE_ALL_PLAYERS) {
        drawCombatBoxText(2, magicDefIsPlayer ? battle_char_stats[magicDefender].name : enemies[magicDefender].name);
        magicBoxCount++;
    }
    magicOpCount = 0;
    magicOpIndex = 0;
    magicOpStarted = FALSE;

    if(!magicDefIsPlayer && enemies[magicDefender].hp == 0) {
        //The single enemy target is already gone.
        addMagicAltMessage(ALTBTLMSG_INEFFECTIVE, FALSE);
        magicStep = MAGIC_STEP_OPS;
    } else {
        explosionCounter = 0;
        explosionSlot = 0;
        //NES btlmag_playerhitsfx: the heal sound for a player's spell on a player.
        battleSfx((magicDefIsPlayer && !magicCasterIsEnemy) ? BTL_SFX_MAGIC : BTL_SFX_PLAYER_ATTACK);
        if(magicDefIsPlayer) {
            //NES FlashCharacterSprite, 16 frames.
            flashCounter = 0x10;
            magicFlashChar = magicDefender;
        }
        magicStep = MAGIC_STEP_HIT;
    }
    return TRUE;
}

//Runs magicOps, one step per call. Returns TRUE once they're all done.
static Uint8 runMagicOps(Uint8 respondRate) {
    if(magicOpIndex >= magicOpCount) {
        return TRUE;
    }

    MAGIC_OP *op = &magicOps[magicOpIndex];
    if(!magicOpStarted) {
        magicOpStarted = TRUE;
        respondRateCounter = 0;
        switch(op->type) {
            case MAGIC_OP_MSG:
                drawLutBottomdMessage(op->value);
                return FALSE;
            case MAGIC_OP_DMG:
                drawDmgString(op->value);
                magicBoxCount++;
                break;
            case MAGIC_OP_ERASE:
                setLastDeletedEnemyRow(0);
                return FALSE;
            default:
                return FALSE;
        }
    } else {
        switch(op->type) {
            case MAGIC_OP_MSG:
                if(respondRateCounter < respondRate) {
                    return FALSE;
                }
                clearBottomMessage();
                break;
            case MAGIC_OP_DELAY:
                if(respondRateCounter < respondRate) {
                    return FALSE;
                }
                break;
            case MAGIC_OP_ERASE:
                if(!eraseEnemyFrame(op->value)) {
                    return FALSE;
                }
                break;
        }
    }

    magicOpStarted = FALSE;
    magicOpIndex++;
    return FALSE;
}

//NES DrawCharacterStatus: natural poses on the first frame, then one character's status per frame.
static void startStatusDraw(Uint8 nextState) {
    statusDrawFrame = 0;
    statusDrawNext = nextState;
    turn_exec_state = DRAW_STATUS;
}

static void doStatusDraw() {
    if(statusDrawFrame == 0) {
        for(Uint8 i = 0; i < 4; i++) {
            if(battle_char_stats[i].ailment & AIL_DEAD) {
                battle_char_stats[i].hp = 0;
            }
            if(isPlayerDead(i)) {
                player_dead(i);
            } else {
                setNaturalPose(i);
            }
        }
    } else {
        drawCharacterStatus(statusDrawFrame - 1);
    }
    statusDrawFrame++;
    if(statusDrawFrame == 5) {
        turn_exec_state = statusDrawNext;
    }
}

//NES Battle_PlayerMagic_CastOnTarget and Battle_EndMagicTurn, after the caster walks back.
static void doMagicTurn(Uint8 respondRate) {
    switch(magicStep) {
        case MAGIC_STEP_NEXT_TARGET:
            if(!startNextMagicTarget()) {
                magicStep = MAGIC_STEP_CLEAR;
            }
            break;
        case MAGIC_STEP_HIT:
            if(magicDefIsPlayer) {
                flashCounter--;
                if(flashCounter != 0) {
                    break;
                }
                magicFlashChar = 0xFF;
                //Dead or stoned allies are only checked when a player targets others.
                if(!magicCasterIsEnemy && magicMode != MAGIC_MODE_SELF && (battle_char_stats[magicDefender].ailment & (AIL_DEAD | AIL_STONE))) {
                    addMagicAltMessage(ALTBTLMSG_INEFFECTIVE, TRUE);
                    magicStep = MAGIC_STEP_OPS;
                    break;
                }
                if(magicMode == MAGIC_MODE_ALL_PLAYERS) {
                    drawCombatBoxText(2, battle_char_stats[magicDefender].name);
                    magicBoxCount++;
                    magicStep = MAGIC_STEP_CAST;
                    break;
                }
            } else if(!explosionFrame(magicDefender)) {
                break;
            }
            magicStep = MAGIC_STEP_CAST;
            //fall through
        case MAGIC_STEP_CAST:
            if(magicDefIsPlayer) {
                castSpellOnPlayer(magicSpell, magicDefender);
                //The NES updates the dead sprite every frame.
                if(battle_char_stats[magicDefender].ailment & AIL_DEAD) {
                    player_dead(magicDefender);
                }
                if(battle_char_stats[magicDefender].ailment & (AIL_DEAD | AIL_STONE)) {
                    turnDefenderDown = TRUE;
                }
            } else {
                castSpellOnEnemy(magicSpell, magicDefender);
                if(enemies[magicDefender].ailment & (AIL_DEAD | AIL_STONE)) {
                    turnDefenderDown = TRUE;
                }
            }
            magicStep = MAGIC_STEP_OPS;
            break;
        case MAGIC_STEP_OPS:
            if(!runMagicOps(respondRate)) {
                break;
            }
            if(magicAbort) {
                magicStep = MAGIC_STEP_CLEAR;
            } else if(magicMode == MAGIC_MODE_ALL_ENEMIES || magicMode == MAGIC_MODE_ALL_PARTY
                    || magicMode == MAGIC_MODE_ALL_PLAYERS || magicMode == MAGIC_MODE_ALL_ALLIES) {
                respondRateCounter = 0;
                magicStep = MAGIC_STEP_TARGET_DONE;
            } else {
                magicStep = MAGIC_STEP_NEXT_TARGET;
            }
            break;
        case MAGIC_STEP_TARGET_DONE:
            //NES RespondDelay_UndrawAllBut2Boxes, the attacker and spell boxes stay.
            if(respondRateCounter < respondRate) {
                break;
            }
            if(magicBoxCount > 2) {
                msgUndraw(magicBoxCount - 2);
                magicBoxCount = 2;
            }
            magicStep = MAGIC_STEP_NEXT_TARGET;
            if(magicDefIsPlayer) {
                startStatusDraw(MAGIC_EFFECT);
            }
            break;
        case MAGIC_STEP_CLEAR:
            //NES ClearAllCombatBoxes: undraw, then a delay.
            msgUndraw(magicBoxCount);
            magicBoxCount = 0;
            respondRateCounter = 0;
            magicStep = MAGIC_STEP_END_DELAY;
            break;
        case MAGIC_STEP_END_DELAY:
            if(respondRateCounter >= respondRate) {
                turn_exec_state = FINISH_TURN;
            }
            break;
    }
}

//"Terminated" or "Slain" when the attack's damage kills the target.
static void drawKillMessage() {
    if(currentTurn > 0x7F) {
        targetIndex = playerActions[currentTurn - 0x80].targetIndex;
        if(curr_attack_dmg >= enemies[targetIndex].hp) {
            drawLutBottomdMessage(BTLMSG_TERMINATED);
            turnKilledEnemy = TRUE;
        }
    } else if(curr_attack_dmg >= battle_char_stats[curr_enemy_target].hp) {
        drawLutBottomdMessage(BTLMSG_SLAIN);
    }
}

void doBattleExec() {
    if(msgBusy()) {
        return;
    }

     Uint8 respondRate = response_rate_lut[getResponseRate()];
    switch(turn_exec_state) {
        case BETWEEN_TURN:
            //The last turn's boxes are already undrawn, so the NES goes straight to the next turn.
            ineffective = FALSE;
            while(turn_exec_state == BETWEEN_TURN) {
                if(isPartyDead()) {
                    state = TO_GAME_OVER;
                    turn_exec_state = EXEC_END;
                    break;
                }
                if(allEnemiesDead() && !battleEndCheckSkipped) {
                    state = TO_BAT_VICTORY_STATE;
                    turn_exec_state = EXEC_END;
                    break;
                }
                if(currentTurnBufferIndex > 12) {
                    turn_exec_state = EXEC_END;
                    break;
                }

                currentTurn = battleOrderBuffer[currentTurnBufferIndex];
                if(currentTurn > 0x7F) {
                    Uint8 playerIndex = currentTurn - 0x80;
                    //Players without a command only act to try to wake up or unstun.
                    //A surprised party has no commands. The NES still redraws the status after their turn.
                    //The NES still lets a sleeping or stunned character recover when surprised.
                    if((isSurprised && BUGFIX_ON(BUGFIX_SURPRISE_TURN))
                       || ((isSurprised || playerActions[playerIndex].action == NO_ACTION) && !(battle_char_stats[playerIndex].ailment & (AIL_SLEEP | AIL_STUN)))) {
                        currentTurnBufferIndex++;
                        startStatusDraw(BETWEEN_TURN);
                    } else {
                        turn_exec_state = EXEC_INIT;
                    }
                } else if(isPreemptive) {
                    currentTurnBufferIndex++;
                } else if(enemies[currentTurn].hp > 0) {
                    turn_exec_state = EXEC_INIT;
                } else {
                    currentTurnBufferIndex++;
                }
            }
            break;
        case EXEC_INIT:
            {
                respondRateCounter = 0;
                statusTurn = FALSE;
                statusMsg = 0;
                ailmentMsgCount = 0;
                ailmentMsgIndex = 0;
                ailmentMsgShown = FALSE;
                enemyRanAway = FALSE;
                curr_attack_ailment_death = FALSE;
                curr_attack_hits = 0;
                curr_attack_crits = 0;
                magicTurn = FALSE;
                magicConfusedFire = FALSE;
                turnDefenderDown = FALSE;
                if(currentTurn > 0x7F) {
                    Uint8 playerIndex = currentTurn - 0x80;
                    BATTLE_CHARACTER *player = &battle_char_stats[playerIndex];
                    switch(playerActions[playerIndex].action) {
                        case NO_ACTION:
                            drawAttackerName(player->name);
                            if(player->ailment & AIL_SLEEP) {
                                //Wakes if max HP is above rand[0, 0x50].
                                if(party[playerIndex].max_hp > rand_min_max(0, 0x50)) {
                                    player->ailment &= ~AIL_SLEEP;
                                    player->ib_ailment &= ~AIL_SLEEP;
                                    statusMsg = BTLMSG_WOKEUP;
                                } else {
                                    statusMsg = BTLMSG_SLEEPING;
                                }
                            } else if((battleRNG() & 0x03) == 0) {
                                player->ailment &= ~AIL_STUN;
                                player->ib_ailment &= ~AIL_STUN;
                                statusMsg = BTLMSG_CURED;
                            } else {
                                statusMsg = BTLMSG_PARALYZED_B;
                            }
                            statusTurn = TRUE;
                            turn_exec_state = RESULT;
                            break;
                        case FIGHT:
                            targetIndex = playerActions[currentTurn - 0x80].targetIndex;
                            drawAttackerDefenderMessage(battle_char_stats[currentTurn - 0x80].name, enemies[targetIndex].name);
                            if(enemies[targetIndex].hp == 0) {
                                ineffective = TRUE;
                            }
                            turn_exec_state = ANIMATE1;
                            break;
                        case MAGIC:
                        case DRINK:
                        case ITEM:
                            startMagicTurn(playerIndex);
                            break;
                        case RUN:
                            drawAttackerName(battle_char_stats[currentTurn - 0x80].name);
                            if(canRun(currentTurn - 0x80)) {
                                runFailed = FALSE;
                                turn_exec_state = EXEC_END;
                                state = TO_BAT_RUN_AWAY;
                            } else {
                                runFailed = TRUE;
                                turn_exec_state = RESULT;
                            }
                            break;
                        default:
                            turn_exec_state = ANIMATE1;
                            break;
                    }
                } else {
                    ENEMY_DATA *enemy = &enemies[currentTurn];
                    if(enemy->ailment & (AIL_SLEEP | AIL_STUN)) {
                        drawAttackerName(enemy->name);
                        if(enemy->ailment & AIL_SLEEP) {
                            //NES bug: the wake-up check is broken, so the roll is unused and enemies always wake.
                            Uint8 roll = rand_min_max(0, 0x50);
                            if(!BUGFIX_ON(BUGFIX_ENEMY_SLEEP) || enemy->hp_max > roll) {
                                enemy->ailment &= ~AIL_SLEEP;
                                statusMsg = BTLMSG_WOKEUP;
                            } else {
                                statusMsg = BTLMSG_SLEEPING;
                            }
                        } else if(battleRNG() < 25) {
                            enemy->ailment &= ~AIL_STUN;
                            statusMsg = BTLMSG_CURED;
                        } else {
                            statusMsg = BTLMSG_PARALYZED_B;
                        }
                        statusTurn = TRUE;
                        turn_exec_state = RESULT;
                    } else if(enemy->ailment & AIL_CONF) {
                        if(battleRNG() < 0x40) {
                            drawAttackerName(enemy->name);
                            enemy->ailment &= ~AIL_CONF;
                            statusMsg = BTLMSG_CURED;
                            statusTurn = TRUE;
                            turn_exec_state = RESULT;
                        } else {
                            startEnemyMagicTurn(currentTurn, SPELL_FIRE, TRUE);
                        }
                    } else if(enemyRunsAway(enemy)) {
                        drawAttackerName(enemy->name);
                        enemy->hp = 0;
                        enemy->ailment = AIL_DEAD;
                        enemy->experience = 0;
                        enemy->money_reward = 0;
                        statusMsg = BTLMSG_RUNAWAY;
                        enemyRanAway = TRUE;
                        statusTurn = TRUE;
                        turn_exec_state = RESULT;
                    } else {
                        Uint8 action = enemyChooseAiAction(enemy);
                        if(action != ENEMY_ACTION_PHYSICAL) {
                            startEnemyMagicTurn(currentTurn, action, FALSE);
                        } else {
                            curr_enemy_target = determinePlayerTarget();
                            drawAttackerDefenderMessage(enemy->name, battle_char_stats[curr_enemy_target].name);
                            turn_exec_state = CALCULATE;
                        }
                    }
                }
            }
            break;
        case ANIMATE1:
            if(isPlayerCrouching(currentTurn - 0x80, party[currentTurn - 0x80].max_hp)) {
                player_stand(currentTurn - 0x80);
            }
            isMoving = TRUE;
            moveBattlePlayerLeft(currentTurn - 0x80);
            break;
        case CALCULATE:
            if(currentTurn > 0x7F && magicTurn) {
                //NES UpdateVariablePalette, also used by the explosions. DRINK and ITEM use the stale palette.
                updateWeaponPalettes(lastMagicPalette[currentTurn - 0x80], FALSE);
                battleAnimFrame = 0;
                weaponVisible = FALSE;
                turn_exec_state = ANIMATE2;
                break;
            } else if(currentTurn > 0x7F) {
              weapon_char = (battle_char_stats[currentTurn - 0x80].wep_gfx -0x80) / 4;
                weaponBat.char_num = weaponSpriteIndex + weapon_char;
                switch(currentTurn  - 0x80) {
                    case 0:
                        weaponBat.yPos = playerBat1.yPos;
                        break;
                    case 1:
                        weaponBat.yPos = playerBat2.yPos;
                        break;
                    case 2:
                        weaponBat.yPos = playerBat3.yPos;
                        break;
                    case 3:
                        weaponBat.yPos = playerBat4.yPos;
                        break;
                }
                updateWeaponPalettes(battle_char_stats[currentTurn - 0x80].wep_pal, TRUE);
                battleAnimFrame = 0;
                weaponVisible = TRUE;
                turn_exec_state = ANIMATE2;
                break;
            } else {
                takeEnemyTurn(currentTurn);
                frameCount = 0;
                //NES BattleScreenShake doesn't update the sound effect, so the boom lasts longer.
                battleSfx(BTL_SFX_ENEMY_ATTACK);
                battleSfxHold = TRUE;
                turn_exec_state = SCREEN_SHAKE;
                break;
            }
        case SCREEN_SHAKE:
            shakeScreen();
            break;
        case FLASH_CHARACTER:
            flashCounter--;
            if(flashCounter == 0) {
                turn_exec_state = RESULT;
            }
            break;
        case ANIMATE2:
            if(magicTurn) {
                castMagic(currentTurn - 0x80);
            } else {
                swingWeapon(currentTurn - 0x80);
            }
            break;
        case MAGIC_EFFECT:
            doMagicTurn(respondRate);
            break;
        case WALK_BACK:
            moveBattlePlayerRight(currentTurn - 0x80);
            break;
        case PREP_EXPLOSION:
        case EXPLOSION:
            if(explosionFrame(targetIndex)) {
                //The NES rolls hits and damage after the explosion.
                takePlayersTurn(currentTurn - 0x80);
                turn_exec_state = RESULT;
            }
            break;
        case DELETING_ENEMY:
            //An enemy turn deletes the enemy itself after it runs away.
            if(currentTurn > 0x7F) {
                targetIndex = playerActions[currentTurn - 0x80].targetIndex;
            } else {
                targetIndex = currentTurn;
            }
            if(eraseEnemyFrame(targetIndex)) {
                respondRateCounter = 0;
                //A fleeing enemy's attacker box is undrawn after its graphic is erased.
                turn_exec_state = enemyRanAway ? ENEMY_ATTACKER_CLEAR : FINISH_TURN;
            }
            break;
        case RESULT:
            //NES DrawBtlMsg_ClearIt: each ailment message is drawn, shown for one delay, then undrawn.
            if(ailmentMsgShown) {
                if(respondRateCounter < respondRate) {
                    break;
                }
                clearBottomMessage();
                ailmentMsgShown = FALSE;
                ailmentMsgIndex++;
            }
            if(ailmentMsgIndex < ailmentMsgCount) {
                drawLutBottomdMessage(ailmentMsgs[ailmentMsgIndex]);
                ailmentMsgShown = TRUE;
                respondRateCounter = 0;
                break;
            }

            respondRateCounter = 0;
            turnKilledEnemy = FALSE;
            if(statusTurn) {
                if(statusMsg != 0) {
                    drawLutBottomdMessage(statusMsg);
                }
                if(currentTurn > 0x7F) {
                    turn_exec_state = RESULT_CLEAR;
                } else if(statusMsg != 0) {
                    turn_exec_state = ENEMY_MSG_CLEAR;
                } else {
                    turn_exec_state = ENEMY_ATTACKER_CLEAR;
                }
            } else if(ineffective) {
                drawLutBottomdMessage(BTLMSG_INEFFECTIVE);
                turn_exec_state = RESULT_CLEAR;
            } else if(runFailed) {
                drawLutBottomdMessage(BTLMSG_CANTRUN);
                turn_exec_state = RESULT_CLEAR;
            } else if(curr_attack_ailment_death) {
                //Death from an ailment shows no damage.
                turn_exec_state = RESULT_CLEAR;
            } else if(curr_attack_miss) {
                drawLutResultMessage(BTLMSG_MISSED);
                turn_exec_state = RESULT_CLEAR;
            } else {
                if(curr_attack_hits >= 2) {
                    drawHitsString(curr_attack_hits);
                }
                drawDmgString(curr_attack_dmg);
                if(curr_attack_crits) {
                    //The NES waits a delay after the critical hit message.
                    drawLutBottomdMessage(BTLMSG_CRITICALHIT);
                    turn_exec_state = RESULT_2;
                    break;
                }
                drawKillMessage();
                turn_exec_state = RESULT_CLEAR;
            }
            break;
        case RESULT_2:
            if(respondRateCounter < respondRate) {
                break;
            }
            respondRateCounter = 0;
            drawKillMessage();
            turn_exec_state = RESULT_CLEAR;
            break;
        case RESULT_CLEAR:
            //NES RespondDelay_ClearCombatBoxes. A killed enemy is erased after its boxes are undrawn.
            if(respondRateCounter < respondRate) {
                break;
            }
            clearMessageArea();
            if(turnKilledEnemy) {
                setLastDeletedEnemyRow(0);
                turn_exec_state = DELETING_ENEMY;
            } else {
                turn_exec_state = FINISH_TURN;
            }
            break;
        case ENEMY_MSG_CLEAR:
            //NES ShowAltBattleMessage: delay, then only the message is undrawn.
            if(respondRateCounter < respondRate) {
                break;
            }
            clearBottomMessage();
            if(enemyRanAway) {
                setLastDeletedEnemyRow(0);
                turn_exec_state = DELETING_ENEMY;
            } else {
                turn_exec_state = ENEMY_ATTACKER_CLEAR;
            }
            break;
        case ENEMY_ATTACKER_CLEAR:
            //NES ClearAllCombatBoxes: undraw, then a delay.
            clearMessageArea();
            respondRateCounter = 0;
            turn_exec_state = ENEMY_END_DELAY;
            break;
        case ENEMY_END_DELAY:
            if(respondRateCounter >= respondRate) {
                turn_exec_state = FINISH_TURN;
            }
            break;

        case WAIT:
            break;
        case FINISH_TURN:
            respondRateCounter = 0;
            if(statusTurn) {
                statusTurn = FALSE;
                if(enemyRanAway) {
                    turnDefenderDown = TRUE;
                }
                currentTurnBufferIndex++;
                startStatusDraw(BETWEEN_TURN);
            } else if(magicTurn) {
                //Magic applies its results as each target is hit.
                currentTurnBufferIndex++;
                startStatusDraw(BETWEEN_TURN);
            } else if(currentTurn > 0x7F) {
                if(ineffective) {
                    turnDefenderDown = TRUE;
                } else if(!runFailed) {
                    targetIndex = playerActions[currentTurn - 0x80].targetIndex;
                    if(!curr_attack_miss) {
                        if(curr_attack_dmg >= enemies[targetIndex].hp) {
                            enemies[targetIndex].hp = 0;
                            enemies[targetIndex].ailment = AIL_DEAD;
                            turnDefenderDown = TRUE;
                        } else {
                            enemies[targetIndex].hp -= curr_attack_dmg;
                        }
                    }
                }
                curr_attack_dmg = 0;
                curr_attack_miss = FALSE;
                runFailed = FALSE;
                currentTurnBufferIndex++;
                startStatusDraw(BETWEEN_TURN);
            } else {
                BATTLE_CHARACTER *target = &battle_char_stats[curr_enemy_target];
                target->ailment = curr_attack_ailments;
                if(curr_attack_ailment_death) {
                    target->hp = 0;
                } else if(!curr_attack_miss) {
                    if(curr_attack_dmg >= target->hp) {
                        target->hp = 0;
                        target->ailment = AIL_DEAD;
                    } else {
                        target->hp -= curr_attack_dmg;
                        if(isPlayerCrouching(curr_enemy_target, party[curr_enemy_target].max_hp)) {
                            player_crouch(curr_enemy_target);
                        }
                    }
                }
                if(target->hp == 0) {
                    player_dead(curr_enemy_target);
                }
                target->ib_ailment = target->ailment;
                if(target->hp == 0 || (target->ailment & (AIL_DEAD | AIL_STONE))) {
                    turnDefenderDown = TRUE;
                }
                //Players who are killed, stoned, slept or stunned lose their command.
                if(target->ailment & (AIL_DEAD | AIL_STONE | AIL_SLEEP | AIL_STUN)) {
                    playerActions[curr_enemy_target].action = NO_ACTION;
                }
                curr_attack_dmg = 0;
                curr_attack_miss = FALSE;
                currentTurnBufferIndex++;
                startStatusDraw(BETWEEN_TURN);
            }
            if(magicConfusedFire && !BUGFIX_ON(BUGFIX_CONFUSED_HANG)) {
                battleEndCheckSkipped = TRUE;
            } else if(turnDefenderDown) {
                battleEndCheckSkipped = FALSE;
            }
            break;
        case EXEC_END:
            isSurprised = FALSE;
            isPreemptive = FALSE;
            applyEndOfRoundEffects();
            startStatusDraw(ROUND_END);
            break;
        case DRAW_STATUS:
            doStatusDraw();
            break;
        case ROUND_END:
            resetPlayerActions();
            battleEndCheckSkipped = FALSE;
            if(isPartyDead()) {
                state = TO_GAME_OVER;
            } else if(allEnemiesDead()) {
                state = TO_BAT_VICTORY_STATE;
            } else {
                state = TO_BAT_MENU_STATE;
            }
            break;

    }
}

//NES tileprop_now: the special bits of the overworld tile the party is on.
Uint8 getOWTileSpecial() {
    return getTileProperty1(current_y_pos, current_x_pos) & 0xC0;
}

void determineSurprise() {

    isSurprised = FALSE;
    isPreemptive = FALSE;

    if(battle.no_run) {
        return;
    }

    Uint8 seed = (Uint8)(party[0].agi + party[0].luck) >> 3;
    Sint16 value = rand_min_max((seed + seed), (seed + 100)) - battle.surprise_rate;
    if(value < 0) {
        value = 0;
    }

    if(value < 11) {
        isSurprised = TRUE;
    } else if(value >= 90) {
        isPreemptive = TRUE;
    }
}

void copyPlayerStatsToBattleStats() {
    for(Uint8 i = 0; i < 4; i++) {
        battle_char_stats[i].slot_index = i;
        battle_char_stats[i].char_class = party[i].char_class;
        battle_char_stats[i].level = party[i].level;
        battle_char_stats[i].luck = party[i].luck;
        battle_char_stats[i].ailment = party[i].ailment;
        battle_char_stats[i].ib_ailment = party[i].ailment;
        battle_char_stats[i].hp = party[i].current_hp;
        battle_char_stats[i].hit_percent = party[i].hit_percent;
        battle_char_stats[i].mag_def = party[i].mag_def;
        battle_char_stats[i].evade_percent = party[i].evade_percent;
        battle_char_stats[i].absorb = party[i].absorb;
        battle_char_stats[i].damage = party[i].damage;
        battle_char_stats[i].elem_resist = party[i].resist;
        battle_char_stats[i].numhitsmult = 1;
        Uint8 numHits = party[i].hit_percent;
        battle_char_stats[i].numhits = (numHits >> 5) + 1;

        //The NES attacks with these instead of the weapon's element and category.
        battle_char_stats[i].category = 0;
        battle_char_stats[i].elem_weak = 0;
        battle_char_stats[i].wep_element = 0;
        battle_char_stats[i].wep_category = 0;
        //Get Equiped Weapon data
        Uint8 wepId = 0;

        for(Uint8 weaponIter = 0; weaponIter < 4; weaponIter++) {
            if(party[i].weapons[weaponIter] > 0x7F) {
                wepId = party[i].weapons[weaponIter] - 0x80;
                break;
            }
        }

        if(wepId > 0) {
            //Weapon IDs are 1-based in the LUT.
            int lut_index = (wepId - 1) * 0x8;
            int lut_offset = WEAPON_LUT_LOCATION + lut_index;

            Uint8 *wep_ptr = (Uint8*) lut_offset;

            //The NES sets the crit rate to the weapon index.
            battle_char_stats[i].crit_rate = BUGFIX_ON(BUGFIX_CRIT_RATE) ? wep_ptr[2] : wepId;
            battle_char_stats[i].wep_element = wep_ptr[4];
            battle_char_stats[i].wep_category = wep_ptr[5];
            battle_char_stats[i].wep_gfx = wep_ptr[6];
            battle_char_stats[i].wep_pal = wep_ptr[7];
        } else {
            battle_char_stats[i].crit_rate = 0;
            battle_char_stats[i].wep_gfx = 0;
            battle_char_stats[i].wep_pal = 0;
        }

        //Master and Blackbelt get special perks if they're unarmed.
        if(battle_char_stats[i].char_class == 2 || battle_char_stats[i].char_class == 8) {

            if(battle_char_stats[i].wep_gfx == 0 ) {
                battle_char_stats[i].wep_gfx = 0xAC; //Unarmed blackbelt fist
                battle_char_stats[i].wep_pal = 0x28; //Unarmed blackbelt palette
                //The NES stores levels 0-based.
                battle_char_stats[i].crit_rate = 2 * (battle_char_stats[i].level - 1);
                battle_char_stats[i].numhits = 2 * battle_char_stats[i].numhits;
            }
        }

        for(Uint8 magicLvl = 0; magicLvl < 8; magicLvl++) {
            battle_char_stats[i].current_mp[magicLvl] = party[i].current_mp[magicLvl];
            battle_char_stats[i].max_mp[magicLvl] = party[i].max_mp[magicLvl];
            for(Uint8 spellNum = 0; spellNum < 3; spellNum++){
                battle_char_stats[i].magic[magicLvl][spellNum] = party[i].magic[magicLvl][spellNum];
            }
        }


        battle_char_stats[i].name[0] = party[i].name[0];
        battle_char_stats[i].name[1] = party[i].name[1];
        battle_char_stats[i].name[2] = party[i].name[2];
        battle_char_stats[i].name[3] = party[i].name[3];
        battle_char_stats[i].name[4] = party[i].name[4];
        battle_char_stats[i].name[5] = party[i].name[5];
        battle_char_stats[i].name[6] = 0;
        battle_char_stats[i].name[7] = 0;
    }
}

void copyBattleStatsToPlayerStats() {
    for(Uint8 i = 0; i < 4; i++) {
        //Only dead, stone or poison persist out of battle, in that priority.
        Uint8 ailment = battle_char_stats[i].ailment;
        if(ailment & AIL_DEAD) {
            party[i].ailment = AIL_DEAD;
        } else if(ailment & AIL_STONE) {
            party[i].ailment = AIL_STONE;
        } else if(ailment & AIL_POISON) {
            party[i].ailment = AIL_POISON;
        } else {
            party[i].ailment = 0;
        }
        //Battle changes to hit, evade, absorb, damage and resist are IB only on the NES.
        party[i].current_hp = battle_char_stats[i].hp;
        party[i].mag_def = battle_char_stats[i].mag_def;
        party[i].level = battle_char_stats[i].level;
        party[i].luck = battle_char_stats[i].luck;
        
        for(Uint8 magicLvl = 0; magicLvl < 8; magicLvl++) {
            party[i].current_mp[magicLvl] = battle_char_stats[i].current_mp[magicLvl];
            party[i].max_mp[magicLvl] = battle_char_stats[i].max_mp[magicLvl];
            for(Uint8 spellNum = 0; spellNum < 3; spellNum++){
                party[i].magic[magicLvl][spellNum] = battle_char_stats[i].magic[magicLvl][spellNum];
            }
        }
        
    }
}


void reset() {
    cur_char_create = 0;
}

//NES GameStart's new game data and NewGamePartyGeneration's default party.
void newGameInit() {
    char blankName[7] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};

    for(Uint8 i = 0; i < 4; i++) {
        char_init(i, i, blankName);
    }
    resetPartyInitBuff();
    initPartyGold();
    initPartyItems();
    reset();
    player_ow_init(FIXED(2296), FIXED(2529));
    setOWVehicle(VEH_FOOT);
    menuReturnSM = FALSE;
    battleReturnSM = FALSE;
}

volatile Uint8 softResetPending = FALSE;

//Back to the prelude. Starting or loading a game sets up the rest.
void softReset() {
    sm_reset();
    scroll_disable();
    static_disable();
    vidSetWindow(VID_W1, 0, 0, 0, 0, 0, 0, 0);
    //Sprite palettes are only loaded at startup, so a battle fade has to be undone.
    if(battleFadeShade) {
        restoreBattleSpritePalettes();
    }
    battleBackColorPending = 0;
    msgReset();
    print_init();
    musicHold = FALSE;
    musicApuWrite(0x4015, 0);
    newGameInit();

    //Buttons still held from the reset are ignored until released.
    ignoreA = TRUE;
    ignoreB = TRUE;
    ignoreC = TRUE;
    ignoreS = TRUE;
    state = TO_PRELUDE;
}

Uint8 determine_curr_char_class() {

     for(Uint8 i = 0; i < 4; i++) {
        if(party[i].current_hp > 0) {
            return party[i].char_class;
        }
     }

     return 0;
}


void determine_curr_char() {
    for(Uint8 i = 0; i < 4; i++) {
        if(party[i].current_hp > 0) {

            switch(party[i].char_class ) {
                case WARRIOR_VALUE:
                    currentChar = WARRIOR_OFFSET;
                    break;
                case THIEF_VALUE:
                    currentChar = THIEF_OFFSET;
                    break;
                case MONK_VALUE:
                    currentChar = MONK_OFFSET;
                    break;
                case REDMAGE_VALUE:
                    currentChar = REDMAGE_OFFSET;
                    break;
                case WMAGE_VALUE:
                    currentChar = WMAGE_OFFSET;
                    break;
                case BMAGE_VALUE:
                    currentChar = BMAGE_OFFSET;
                    break;
                default:
                    currentChar = WARRIOR_OFFSET;
                    break;
            }

            i = 4;

        }
    }
    
    current_x_pos = prev_current_x_pos;
    current_y_pos = prev_current_y_pos;
    
    totalMovement = 0;
    
}

void initMapManPalette(Uint32 *palette_a, Uint32 *palette_b, Uint8 *palPtr, Uint8 *palPtr2) {

    palette_a[0] = nes_pal[palPtr[0]];
    palette_a[1] = nes_pal[palPtr[1]];
    palette_a[2] = nes_pal[palPtr2[0]];
    palette_a[3] = nes_pal[palPtr[3]];

    palette_b[0] = nes_pal[palPtr[0]];
    palette_b[1] = nes_pal[palPtr[1]];
    palette_b[2] = nes_pal[palPtr2[1]];
    palette_b[3] = nes_pal[palPtr[3]];
}

void performActionButtonInput() {
    if(state == PRELUDE) {
        state = TO_TITLE_STATE;
    } else if(state == OW_STATE) {
        owJoyA = TRUE;
    } else if(state == BAT_MENU_STATE && bat_menu_state == SELECT) {

        switch(cur_battle_menu_selection) {
            case FIGHT:
                prev_bat_menu_x_pos = cursor.xPos;
                prev_bat_menu_y_pos = cursor.yPos;
                bat_menu_state = TO_SELECT_ENEMY_TARGET;
                break;
            case MAGIC:
                prev_bat_menu_x_pos = cursor.xPos;
                prev_bat_menu_y_pos = cursor.yPos;
                openMagicMenu();
                break;
            case DRINK:
                prev_bat_menu_x_pos = cursor.xPos;
                prev_bat_menu_y_pos = cursor.yPos;
                openDrinkMenu();
                break;
            case ITEM:
                prev_bat_menu_x_pos = cursor.xPos;
                prev_bat_menu_y_pos = cursor.yPos;
                openItemMenu();
                break;
            case RUN: 
                updatePlayerBattleAction(cur_char_battle_menu, 0, cur_battle_menu_selection, 0, FALSE);
                bat_menu_state = END_SELECT;
                break;
        }

    } else if (state == BAT_MENU_STATE && bat_menu_state == SELECT_ENEMY_TARGET) {
        setEnemyTargetAction(currentSlot);
    } else if (state == BAT_MENU_STATE && bat_menu_state == SELECT_MAGIC) {
        selectMagicSpell();
    } else if (state == BAT_MENU_STATE && bat_menu_state == SELECT_PARTY_TARGET) {
        if(cur_battle_menu_selection == DRINK) {
            setDrinkAction(magicCursorY);
        } else {
            setMagicAction(magicCursorY, TRUE);
        }
    } else if (state == BAT_MENU_STATE && bat_menu_state == SELECT_DRINK) {
        selectDrink();
    } else if (state == BAT_MENU_STATE && bat_menu_state == SELECT_ITEM) {
        selectItem();
    } else if (state == BAT_EXECUTE_STATE && turn_exec_state == WAIT) { 
        clearMessageArea();
        turn_exec_state = FINISH_TURN;
    } else if(state == TITLE_STATE) {
        if(cursor.yPos == CONTINUE_POS) {
            saveOpen(SV_MODE_LOAD);
            state = TO_SAVE_MENU;
        } else {
            newGameInit();
            state = TO_CHAR_SELECT_STATE;
        }
    } else if(state == CHAR_SELECT_STATE) {
        if(cur_char_create == 4) {
            //Overworld loading takes many frames, and the title music would hold its last notes.
            musicApuWrite(0x4015, 0);
            owWipeIn = TRUE;
            state = TO_OW_STATE;
        } else {
            state = TO_CHAR_NAME_STATE;
                selected_class = party[cur_char_create].char_class;
        }
    }  else if(state == CHAR_NAME_STATE) {
        nameBuffer[cur_letter_index] = char_name_screen_lookup_table[cur_letter];

        cur_letter_index++;
        if(cur_letter_index == 6) {
            selected_class = party[cur_char_create].char_class;
            char_init(selected_class, cur_char_create, nameBuffer);
                            
            cur_char_create++;
            nameBuffer[0] = 0xFF;
            nameBuffer[1] = 0xFF;
            nameBuffer[2] = 0xFF;
            nameBuffer[3] = 0xFF;
            nameBuffer[4] = 0xFF;
            nameBuffer[5] = 0xFF;
            nameBuffer[6] = 0x00;
            cur_letter_index = 0;
            cur_letter = 0;
            state = TO_CHAR_SELECT_STATE;
        }
    }
}

void performCancelButtonInput() {
    if(state == CHAR_NAME_STATE) {
        if(cur_letter_index == 0) {
            state = TO_CHAR_SELECT_STATE;
            cur_letter_index = 0;
            cur_letter = 0;
        } else {
            cur_letter_index--;
            nameBuffer[cur_letter_index] = 0xFF;
        }
        
    } else if(state == CHAR_SELECT_STATE) {
        
        cur_letter = 0;

        if(cur_char_create > 0) {
            cur_char_create--;

            selected_class = party[cur_char_create].char_class;
            cursor.xPos = FIXED(getPartyCreateCursorPosX(cur_char_create));
            cursor.yPos = FIXED(getPartyCreateCursorPosY(cur_char_create));
        } else {
            state = TO_TITLE_STATE;
        }

    } else if(state == BAT_MENU_STATE  && bat_menu_state == SELECT) {
        bat_menu_state = SELECT_CANCEL;
    } else if (state == BAT_MENU_STATE && bat_menu_state == SELECT_ENEMY_TARGET) {
        //Backing out of a spell's target goes back to the magic menu.
        if(cur_battle_menu_selection == MAGIC) {
            openMagicMenu();
        } else {
            bat_menu_state = CANCEL_ACTION;
        }
    } else if (state == BAT_MENU_STATE && (bat_menu_state == SELECT_MAGIC || bat_menu_state == SELECT_DRINK || bat_menu_state == SELECT_ITEM)) {
        //Only the submenu box is undrawn, the roster and command boxes stay.
        msgUndraw(1);
        drawBattleCursor = FALSE;
        bat_menu_state = CANCEL_ACTION;
    } else if (state == BAT_MENU_STATE && bat_menu_state == SELECT_PARTY_TARGET) {
        if(cur_battle_menu_selection == DRINK) {
            openDrinkMenu();
        } else {
            openMagicMenu();
        }
    }
}

