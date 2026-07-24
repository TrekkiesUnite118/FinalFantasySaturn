#ifndef NESUTILS_H
#define NESUTILS_H

#include    <sega_def.h>

typedef struct {
    Uint8 class;
    Uint8 padding;
    Uint8 name[4]; //While we'll support 6 this is for reading data from the NES ROM.
    Uint8 name_xPos;
    Uint8 name_yPos;
    Uint8 class_xPos;
    Uint8 class_yPos;
    Uint8 sprite_xPos;
    Uint8 sprite_yPos;
    Uint8 box_xPos;
    Uint8 box_yPos;
    Uint8 cursor_xPos;
    Uint8 cursor_yPos;
} PARTY_GEN;


typedef struct {
    Uint8 header;
    Uint8 xPos;
    Uint8 yPos;
    Uint8 ptr_low;
    Uint8 ptr_high;
} COMMAND_BOX_TEXT;

typedef struct {
    Uint8 header;
    Uint8 xPos;
    Uint8 yPos;
    Uint8 width;
    Uint8 height;
    COMMAND_BOX_TEXT text[5];
} COMMAND_BOX;

typedef struct {
    Uint8 header;
    Uint8 xPos;
    Uint8 yPos;
    Uint8 width;
    Uint8 height;
    Uint8 text_header;
    Uint8 text_xPos;
    Uint8 text_yPos;
} BATTLE_INFO_BOX;

typedef struct {
    Uint8 header;
    Uint8 xPos;
    Uint8 yPos;
    Uint8 width;
    Uint8 height;
} BATTLE_BOX;

#define  CENTER_ADJUSTMENT (4)
#define  NAME_BOX_EXTENSION (2)
#define  BOX_TOP_LEFT_CORNER_CHAR (0xF7)
#define  BOX_TOP_EDGE_CHAR (0xF8)
#define  BOX_TOP_RIGHT_CORNER_CHAR (0xF9)
#define  BOX_LEFT_EDGE_CHAR (0xFA)
#define  BOX_RIGHT_EDGE_CHAR (0xFB)
#define  BOX_BOTTOM_LEFT_CORNER_CHAR (0xFC)
#define  BOX_BOTTOM_EDGE_CHAR (0xFD)
#define  BOX_BOTTOM_RIGHT_CORNER_CHAR (0xFE)
#define  BOX_BLANK_CHAR (0xFF)

#define  MENU_TILE_OFFSET (0x00224810)
#define  BATTLE_TILE_OFFSET (0x0021C010)
#define  BATTLE_BACKDROP_PALETTE_OFFSET (0x00203210)
#define  PURPLE_MENU_PAL_OFFSET (0x0023EB84)
#define  GREEN_MENU_PAL_OFFSET (0x0023EB88)
#define  PARTY_GEN_OFFSET (0x0023A0BE)
#define  CLASS_NAME_OFFSET (0x0022BF53)
#define  CONTINUE_OFFSET (0x0023A263)
#define  NEW_GAME_OFFSET (0x0023A26C)
#define  RESPOND_RATE_OFFSET (0x0023A275)
#define  COPYRIGHT_1_OFFSET (0x002384B8)
#define  COPYRIGHT_2_OFFSET (0x002384CA)
#define  NAME_INPUT_LUT_OFFSET (0x0023A021)
#define  PRELUDE_TEXT_OFFEST (0x00237F30)
#define  TILESET_2BPP_SIZE (0x800)
#define  TILESET_4BPP_SIZE (0x1000)

#define  BATTLE_MAIN_BOX_XY (0x0023F2F5)
#define  BATTLE_MAIN_BOX_WIDTH (0x0023F2FA)
#define  BATTLE_MAIN_BOX_HEIGHT (0x0023F2FC)

#define  BATTLE_MAIN_BACKGROUND_WIDTH_1 (0x0023F39D)
#define  BATTLE_MAIN_BACKGROUND_WIDTH_2 (0x0023F3AA)

#define  BATTLE_CHAR_BOX_X (0x0023F301)
#define  BATTLE_CHAR_BOX_Y (0x0023F306)
#define  BATTLE_CHAR_BOX_WIDTH (0x0023F308)
#define  BATTLE_CHAR_BOX_HEIGHT (0x0023F30A)

#define  BATTLE_STAT_WIDTH (0x0023F3E0)
#define  BATTLE_STAT_XPOS (0x0023F30F)
#define  BATTLE_STAT_YPOS (0x0023F311)

#define  ITEM_AND_MAGIC_BOX_LUT (0x0023FA21)
#define  DRINK_BOX_LUT (0x0023FA26)

#define  ENEMY_ROSTER_BOX_LUT (0x0023F9F4)

#define  BATTLE_INFO_BOX_LUT (0x0023F9F9)

#define  COMMAND_BOX_LUT (0x0023FA2B)
#define  COMMAND_BOX_TEXT_LUT (0x0023FA30)

#define  BATTLE_MSG_TABLE (0x0022CC50)
#define  BATTLE_MSG_PTR_TABLE (0x0022CF60)

#define  BASIC_TEXT_PTR_TABLE (0x0022B712)
#define  BASIC_TEXT_BANK_ADD (0x00220010)

#define  BASIC_TEXT_ITEM_CNT (27)
#define  BASIC_TEXT_WEAPON_CNT (40)
#define  BASIC_TEXT_ARMOR_CNT (40)
#define  BASIC_TEXT_GOLDITEM_CNT (68)


static PARTY_GEN *partyInitBuff;

extern Uint16 menuPlane[64][64];

extern Uint8 canAnimatePrelude;

static Uint8 boarderPalette_black[4] = {0x0F, 0x00, 0x0F, 0x30};
static Uint8 boarderPalette_blue[4] = {0x0F, 0x00, 0x01, 0x30};
static Uint8 boarderPalette_red[4] = {0x0F, 0x00, 0x16, 0x30};


static Uint8 prelude_pallete_1[4] = {0x0F, 0x00, 0x01, 0x01};
static Uint8 prelude_pallete_2[4] = {0x0F, 0x00, 0x01, 0x00};
static Uint8 prelude_pallete_3[4] = {0x0F, 0x00, 0x01, 0x10};
static Uint8 prelude_pallete_4[4] = {0x0F, 0x00, 0x01, 0x20};
static Uint8 prelude_pallete_5[4] = {0x0F, 0x00, 0x01, 0x30};

extern Uint32 menu_pal_0[];
extern Uint32 menu_pal_1[];
extern Uint32 menu_pal_2[];
extern Uint32 menu_pal_3[];
extern Uint32 menu_pal_4[];

extern Uint8 BATTLE_MENU_PALETTE;

void drawBox(Uint8 xPos, Uint8 yPos, Uint8 width, Uint8 height, Uint8 palNum);

void drawBottomBoxLine(Uint8 xPos, Uint8 yPos, Uint8 width, Uint8 palNum);

void drawTextLine(Uint8 xPos, Uint8 yPos, Uint8 *stringPtr, Uint8 palNum, Uint8 lineSpacing);

void drawPreludeTextLine(Uint8 xPos, Uint8 yPos, Uint8 *stringPtr, Uint8 palNum, Uint8 lineSpacing);

void setupBattleScreen(Uint8 battleValue);

void initEnemyPatternData(Uint8 *patternData, Uint8 pal_id_0, Uint8 pal_id_1);

void drawEnemy(Uint8 enemy_num, Uint8 palette_assignment, Uint8 xPos, Uint8 yPos);

void mainBattleBoxBottoms();

void drawMainBattleBoxes();

void setupTitleScreen();

void setupPartyCreateScreen();

void setupNameEntryScreen();

void setupPreludeScreen();

void clearMenuPlane();

void player_select_init();

void player_select_draw();

void incrementPartyInitBuff(Uint8 cur_char_create);

void decrementPartyInitBuff(Uint8 cur_char_create);

Uint8 getPartyCreateCursorPosX(Uint8 cur_char_create);

Uint8 getPartyCreateCursorPosY(Uint8 cur_char_create);

void drawBattleBackdropRow(Uint8 row);

Uint8 determineAilmentBit(Uint8 currChar);

void drawHpNum(Uint32 num, Uint8 xPos, Uint8 yPos);

void updateAilmentandHP();

void drawBattleMsg(Uint8 xPos, Uint8 yPos, Uint8 msg_id, Uint8 pal);

void drawMonstersPerished(Uint8 xPos, Uint8 yPos, Uint8 pal);

void drawPartyPerished(Uint8 xPos, Uint8 yPos, Uint8 *name_ptr, Uint8 pal);

void clearArea(Uint8 xPos, Uint8 yPos, Uint8 width, Uint8 height);

#endif
