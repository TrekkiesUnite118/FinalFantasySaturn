#include <string.h>
#include "types.h"

#include "graphicrefs.h"
#include "nesutils.h"
#include "player.h"
#include "menu.h"
#include "sprdata.h"
#include "battle.h"
#include "vblank.h"
#include "cd.h"
#include "memmap.h"
#include "magic.h"
#include "mainmenu.h"
#include "stdmap.h"
#include "save.h"
#include "music.h"
#include "bugfix.h"

#define ORB_CHR_SIZE (0x200)
#define ORB_TILE_START (0x60)
#define ORB_UNLIT_TILE (0x76)

#define FRAME_BATTLE_START (96)

//Port palettes. NES palette 3 (blue border) is port palette 0, NES palette 0 (lit orbs) is port palette 1.
#define MM_PAL_BORDER (0)
#define MM_PAL_ORB (1)
//The equip menu's NES palettes 1 (title) and 2 (selected mode). The US ROM gives them the border's colors.
#define MM_PAL_EQ_TITLE (1)
#define MM_PAL_EQ_MODE (2)
//Colored equip menu words. The US build uses the Japanese ROM's colors, over each whole word,
//since the English words don't fit the NES's 4 tile attribute areas.
#ifdef FF1_JP
#define EQ_TITLE_W (4)
#define EQ_MODE_X(i) (0x0C + (i) * 6)
#define EQ_MODE_W(i) (4)
#else
#define EQ_TITLE_W (5)
#define EQ_MODE_X(i) (0x0B + (i) * 7)
#define EQ_MODE_W(i) ((i) == 2 ? 4 : 5)
static const Uint8 eqMenuPal[12] = {0x0F, 0x30, 0x01, 0x22, 0x0F, 0x00, 0x01, 0x37, 0x0F, 0x00, 0x01, 0x16};
#endif

//The NES layout shifted 2 tiles left, with the character boxes 2 tiles wider for 6 character names.
#define MM_CHAR_BOX_W (0x0C)
#define MM_CHAR_BOX_H (0x0E)
#define MM_CURSOR_X (0x00 + 32)

//NES menu bits for the d-pad.
#define NES_DIR_R (0x01)
#define NES_DIR_L (0x02)
#define NES_DIR_D (0x04)
#define NES_DIR_U (0x08)

enum __mainmenumode__ {
    MM_MODE_MAIN,
    MM_MODE_SUB_MAGIC,
    MM_MODE_SUB_STATUS
};

enum __mainmenuoption__ {
    MM_OPT_ITEM,
    MM_OPT_MAGIC,
    MM_OPT_WEAPON,
    MM_OPT_ARMOR,
    MM_OPT_STATUS,
    MM_OPT_SAVE,
    MM_OPT_COUNT
};

static Uint8 menuPalLitOrb[4] = {0x0F, 0x30, 0x01, 0x22};

//Character box X/Y, in party order.
static Uint8 charBoxPos[4][2] = {
    {0x09, 0x01}, {0x15, 0x01}, {0x09, 0x0F}, {0x15, 0x0F}
};

//NES DrawMainMenuCharSprites positions, in party order.
static Uint16 charSpritePos[4][2] = {
    {0x88 + 32, 0x18}, {0xE8 + 32, 0x18}, {0x88 + 32, 0x88}, {0xE8 + 32, 0x88}
};

//NES lut_MainMenuSubCursor.
static Uint16 subCursorPos[4][2] = {
    {0x38 + 32, 0x10}, {0x98 + 32, 0x10}, {0x38 + 32, 0x80}, {0x98 + 32, 0x80}
};

static Uint8 mainCursorY[MM_OPT_COUNT] = {0x88, 0x98, 0xA8, 0xB8, 0xC8, 0xD8};

//Orb X/Y and lit tile: Fire, Water, Air, Earth.
static Uint8 orbPos[4][3] = {
    {0x02, 0x04, 0x64}, {0x04, 0x04, 0x68}, {0x02, 0x06, 0x6C}, {0x04, 0x06, 0x70}
};

static SPRITE_INFO mmSprites[4];
static Uint8 mmMode;
static Uint8 mmCursor;

#ifdef FF1_JP
static const Uint8 mmSaveText[] = {0xFF, 0xD7, 0xC2, 0x6D, 0x00}; //" セーブ"
#else
static const Uint8 mmSaveText[] = {0xFF, 0x9C, 0x8A, 0x9F, 0x8E, 0x00};
#endif
static Uint8 mmPrevDir;
static PadBits mmPadPrev;
static Uint8 mmTarget;

//NES @lutStatusBoxes. The name box is 2 tiles wider for 6 character names, so the class box
//starts 1 tile later and is 1 tile narrower, which still fits the 7 character class names.
static Uint8 statusBoxes[6][4] = {
    {0x00, 0x03, 0x0A, 0x05},
    {0x0A, 0x03, 0x0D, 0x05},
    {0x17, 0x03, 0x08, 0x05},
    {0x04, 0x08, 0x17, 0x07},
    {0x01, 0x0F, 0x0E, 0x0D},
    {0x0F, 0x0F, 0x10, 0x0D}
};

#define STATUS_TEXT_START (0x23)
#define STATUS_SPRITE_X (0x60 + 32)
#define STATUS_SPRITE_Y (0x20)

//NES DrawCharMenuString copies $80 bytes, the magic list copies $C1.
#define CHAR_MENU_STRING_LEN (0x80)
#define CHAR_MENU_STRING_MAX (0xC1)

#define MAGIC_LIST_TEXT (0x2A)
#define MAGIC_TITLE_TEXT (0x29)

//NES description text IDs.
#define DESC_CURE (0x2B)
#define DESC_HEAL (0x2C)
#define DESC_PURE (0x2D)
#define DESC_LIFE (0x2E)
#define DESC_WARP (0x2F)
#define DESC_SOFT (0x30)
#define DESC_EXIT (0x31)
#define DESC_NO_MP (0x32)
#define DESC_CANT_CAST (0x33)

#define MG_START (0xB0)
#define MG_CURE (MG_START + 0x00)
#define MG_CUR2 (MG_START + 0x10)
#define MG_HEAL (MG_START + 0x13)
#define MG_PURE (MG_START + 0x18)
#define MG_CUR3 (MG_START + 0x20)
#define MG_LIFE (MG_START + 0x21)
#define MG_HEL2 (MG_START + 0x23)
#define MG_WARP (MG_START + 0x26)
#define MG_SOFT (MG_START + 0x28)
#define MG_EXIT (MG_START + 0x29)
#define MG_CUR4 (MG_START + 0x30)
#define MG_HEL3 (MG_START + 0x33)
#define MG_LIF2 (MG_START + 0x38)

//NES out of battle ailment IDs used by CureOBAilment.
#define OB_AIL_DEAD (0x01)
#define OB_AIL_STONE (0x02)
#define OB_AIL_POISON (0x03)

enum __magicmenumode__ {
    MG_MODE_LIST,
    MG_MODE_NO_SPELLS,
    MG_MODE_TARGET,
    MG_MODE_HEAL_CONFIRM,
    MG_MODE_DONE,
    MG_MODE_EXIT
};

static Uint8 mgMode;
static Uint8 mgCursor;
static Uint8 mgTargetCursor;
static Uint8 mgSpell;
static Uint16 mgHpRecovery;
static Uint8 mgIntHeal; //Intelligence fix: each target also gets rand[0, Luck / 2].
static Uint8 mgDescOpen;

static Uint8 targetCursorX[4] = {0x10, 0x48, 0x80, 0xB8};

#define EQUIP_WEAPON_TITLE_TEXT (0x34)
#define EQUIP_ARMOR_TITLE_TEXT (0x3E)
#define EQUIP_MODE_TEXT (0x35)
#define EQUIP_NAME_TEXT (0x36)
//Item name IDs minus 1, since equipment IDs are 1-based.
#define EQUIP_WEAPON_NAME_START (0x1C - 1)
#define EQUIP_ARMOR_NAME_START (0x44 - 1)
#define EQUIP_NAME_LEN (7)

#define NES_CLS_BB (0x02)
#define NES_CLS_MA (0x08)

enum __equipmenumode__ {
    EQ_MODE_SELECT,
    EQ_MODE_EQUIP,
    EQ_MODE_TRADE,
    EQ_MODE_TRADE_SECOND,
    EQ_MODE_DROP,
    EQ_MODE_DROP_CONFIRM
};

static Uint8 eqArmor;
static Uint8 eqMode;
static Uint8 eqModeCursor;
static Uint8 eqCursor;
static Uint8 eqCursor2;
static Uint8 eqSfxPrevDir;
//NES item_box: 4 slots for each character.
static Uint8 eqItemBox[16];

//NES lut_EquipStringPositions.
static Uint8 equipStringPos[16][2] = {
    {0x0A, 0x07}, {0x14, 0x07}, {0x0A, 0x09}, {0x14, 0x09},
    {0x0A, 0x0D}, {0x14, 0x0D}, {0x0A, 0x0F}, {0x14, 0x0F},
    {0x0A, 0x13}, {0x14, 0x13}, {0x0A, 0x15}, {0x14, 0x15},
    {0x0A, 0x19}, {0x14, 0x19}, {0x0A, 0x1B}, {0x14, 0x1B}
};

//NES lut_EquipMenuCurs Y. X is $40 for the left column and $90 for the right.
static Uint8 equipCursorY[8] = {0x38, 0x48, 0x68, 0x78, 0x98, 0xA8, 0xC8, 0xD8};

static Uint8 equipModeCursorX[3] = {0x48, 0x80, 0xB8};

#define ITEM_TITLE_TEXT (0x03)
#define ITEM_NAME_LEN (7)
#define ITEM_BOX_MAX (24)
#define OBJID_FAIRY (0x13)
#define GMFLG_OBJVISIBLE (0x01)
#define OWTP_SPEC_FLOATER (0xC0)
#define OBJID_RODPLATE (0x16)
#define OBJID_LUTEPLATE (0x17)

//NES item description text IDs.
#define DESC_NO_ITEMS (0x04)
#define DESC_LUTE (0x05)
#define DESC_LUTE_CANT (0x06)
#define DESC_ROD (0x0F)
#define DESC_ROD_CANT (0x10)
#define DESC_FLOATER (0x11)
#define DESC_FLOATER_CANT (0x12)
#define DESC_BOTTLE (0x16)
#define DESC_BOTTLE_EMPTY (0x17)
#define DESC_TENT_CANT (0x1B)
#define DESC_CABIN_CANT (0x1D)
#define DESC_HOUSE_CANT (0x1F)
#define DESC_HEAL_POTION (0x20)
#define DESC_PURE_POTION (0x21)
#define DESC_SOFT_POTION (0x22)

enum __itemmenumode__ {
    IT_MODE_LIST,
    IT_MODE_EMPTY,
    IT_MODE_TARGET,
    IT_MODE_DONE
};

static Uint8 itMode;
static Uint8 itCursor;
static Uint8 itCount;
static Uint8 itItem;
//NES item_box: the IDs of the items in the list.
static Uint8 itItemBox[ITEM_BOX_MAX];

//Description text for the key items that only show a description, by item ID. 0 for the others.
static Uint8 itemDescOnly[ITEM_ORB_START] = {
    0x00, 0x00, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
    0x00, 0x00, 0x13, 0x14, 0x15, 0x00, 0x18, 0x19
};

//NES lutItemBoxStrPos and lut_ItemMenuCursor columns. Rows are 2 tiles (16 px) apart.
static Uint8 itemStrX[3] = {0x04, 0x0D, 0x16};
static Uint8 itemCursorX[3] = {0x10, 0x58, 0xA0};

static void drawOrbs() {
    for(Uint8 i = 0; i < 4; i++) {
        Uint8 tile = items[ITEM_ORB_START + i] ? orbPos[i][2] : ORB_UNLIT_TILE;
        Uint16 pal = (items[ITEM_ORB_START + i] ? MM_PAL_ORB : MM_PAL_BORDER) << 12;
        Uint8 x = orbPos[i][0] + CENTER_ADJUSTMENT;
        Uint8 y = orbPos[i][1];

        drawPlane[y][x] = pal | tile;
        drawPlane[y][x + 1] = pal | (tile + 1);
        drawPlane[y + 1][x] = pal | (tile + 2);
        drawPlane[y + 1][x + 1] = pal | (tile + 3);
    }
}

//NES DrawMainMenuCharBoxBody. x/y is the text origin.
static void drawCharBoxBody(Uint8 member, Uint8 x, Uint8 y) {
    PLAYER_CHARACTER *c = &party[member];
    Uint8 code = 0x10 | member;
    Uint8 hasMP = 0;
    Uint8 buffer[16];

    for(Uint8 i = 0; i < 8; i++) {
        hasMP |= c->max_mp[i];
    }

    if(hasMP) {
        for(Uint8 i = 0; i < 8; i++) {
            buffer[i * 2] = 0x80 | c->current_mp[i];
            buffer[(i * 2) + 1] = 0x7A;
        }
        buffer[0x0F] = 0x00;
        drawComplexString(x, y + 10, &buffer[8], MM_PAL_BORDER);
        buffer[8] = 0x00;
        drawComplexString(x, y + 9, buffer, MM_PAL_BORDER);

#ifdef FF1_JP
        Uint8 magic[4] = {0xA8, 0xA7, 0x8C, 0x00};
#else
        Uint8 magic[4] = {0x52, 0x4A, 0x8C, 0x00};
#endif
        drawComplexString(x, y + 8, magic, MM_PAL_BORDER);
    }

    Uint8 hp[6] = {code, 0x05, 0x7A, code, 0x06, 0x00};
    drawComplexString(x, y + 5, hp, MM_PAL_BORDER);

    Uint8 info[10] = {code, 0x00, 0x01, 0xC6, code, 0x03, 0x01, code, 0x02, 0x00};
    drawComplexString(x, y, info, MM_PAL_BORDER);
}

//NES EnterMainMenu and DrawMainMenu.
void setupMainMenuScreen() {
    for(Uint8 i = 0; i < 4; i++) {
        menu_pal_0[i] = nes_pal[boarderPalette_blue[i]];
        menu_pal_1[i] = nes_pal[menuPalLitOrb[i]];
    }
    convertNesTilesToSatTiles((Uint8 *)ORB_CHR, (Uint8 *)(LWRAM_MENU_TILES + (ORB_TILE_START * 32)), ORB_CHR_SIZE);

    drawBox(0x00, 0x02, 0x08, 0x08, MM_PAL_BORDER);
    drawOrbs();

    drawBox(0xFF, 0x0A, 0x0A, 0x05, MM_PAL_BORDER);
    drawComplexString(0x00, 0x0C, getMenuString(0x01), MM_PAL_BORDER);

    drawBox(0x00, 0x0F, 0x08, 0x0E, MM_PAL_BORDER);
    drawComplexString(0x01, 0x11, getMenuString(0x02), MM_PAL_BORDER);
    if(saveAllowed()) {
        drawComplexString(0x01, 0x1B, (Uint8 *)mmSaveText, MM_PAL_BORDER);
    }

    for(Uint8 i = 0; i < 4; i++) {
        drawBox(charBoxPos[i][0], charBoxPos[i][1], MM_CHAR_BOX_W, MM_CHAR_BOX_H, MM_PAL_BORDER);
        drawCharBoxBody(i, charBoxPos[i][0] + 1, charBoxPos[i][1] + 2);
    }
}

//NES ResumeMainMenu.
void initMainMenu() {
    for(Uint8 i = 0; i < 4; i++) {
        sprite_make(FRAME_BATTLE_START, 0, 0, (Sint32) CHARACTER_PRIORITY, &mmSprites[i]);
    }
    player_cursor_init(FIXED(MM_CURSOR_X), FIXED(mainCursorY[0]));

    mmMode = MM_MODE_MAIN;
    mmCursor = 0;
    mmPrevDir = 0;
    mmPadPrev = PadData1;
}

static Uint8 getNesDir() {
    Uint8 dir = 0;

    if(PadData1 & PAD_R) dir |= NES_DIR_R;
    if(PadData1 & PAD_L) dir |= NES_DIR_L;
    if(PadData1 & PAD_D) dir |= NES_DIR_D;
    if(PadData1 & PAD_U) dir |= NES_DIR_U;
    return dir;
}

//NES MoveCursorUpDown. Up and Down held together counts as Up.
static void moveCursorUpDown() {
    Uint8 dir = getNesDir() & (NES_DIR_U | NES_DIR_D);
    Uint8 count = saveAllowed() ? MM_OPT_COUNT : MM_OPT_SAVE;

    if(dir == mmPrevDir) {
        return;
    }
    mmPrevDir = dir;
    if(dir == NES_DIR_D) {
        mmCursor = (mmCursor + 1 < count) ? mmCursor + 1 : 0;
    } else if(dir != 0) {
        mmCursor = (mmCursor > 0) ? mmCursor - 1 : count - 1;
    }
    if(dir != 0) {
        sfxMenuMove();
    }
}

//NES MoveMainMenuSubCursor. Left/Right flips the column, anything with Up/Down flips the row.
static void moveSubCursor() {
    Uint8 dir = getNesDir();

    if(dir == mmPrevDir) {
        return;
    }
    mmPrevDir = dir;
    if(dir == 0) {
        return;
    }
    mmCursor ^= (dir < NES_DIR_D) ? 0x01 : 0x02;
    sfxMenuMove();
}

void doMainMenu() {
    PadBits pressed = PadData1 & ~mmPadPrev;
    mmPadPrev = PadData1;

    if(mmMode == MM_MODE_MAIN) {
        if(pressed & (PAD_A | PAD_C)) {
            sfxMenuSel();
            if(mmCursor == MM_OPT_MAGIC) {
                mmMode = MM_MODE_SUB_MAGIC;
                mmCursor = 0;
            } else if(mmCursor == MM_OPT_STATUS) {
                mmMode = MM_MODE_SUB_STATUS;
                mmCursor = 0;
            } else if(mmCursor == MM_OPT_WEAPON || mmCursor == MM_OPT_ARMOR) {
                eqArmor = (mmCursor == MM_OPT_ARMOR);
                state = TO_EQUIP_MENU;
            } else if(mmCursor == MM_OPT_SAVE) {
                saveOpen(SV_MODE_SAVE);
                state = TO_SAVE_MENU;
            } else {
                state = TO_ITEM_MENU;
            }
        } else if(pressed & PAD_B) {
            exitOwMenu();
        } else if(pressed & PAD_RB) {
            sfxMenuSel();
            state = TO_BUGFIX_MENU;
        } else {
            moveCursorUpDown();
        }
        return;
    }

    //NES MainMenuSubTarget.
    if(pressed & (PAD_A | PAD_C)) {
        Uint8 ailment = party[mmCursor].ailment;
        mmTarget = mmCursor;
        if(mmMode == MM_MODE_SUB_STATUS) {
            state = TO_STATUS_MENU;
        } else if(ailment == AIL_DEAD || ailment == AIL_STONE) {
            sfxError();
            mmCursor = 0;
        } else {
            state = TO_MAGIC_MENU;
        }
    } else if(pressed & PAD_B) {
        mmMode = MM_MODE_MAIN;
        mmCursor = 0;
    } else {
        moveSubCursor();
    }
}

//NES DrawCharMenuString_Len. Copies len bytes like the NES, setting the character on every $10 byte.
static void drawCharMenuString(Uint8 x, Uint8 y, Uint8 id, Uint8 member, Uint16 len) {
    Uint8 buffer[CHAR_MENU_STRING_MAX];
    Uint8 *str = getMenuString(id);

    for(Uint16 i = 0; i < len; i++) {
        buffer[i] = (str[i] == 0x10) ? (0x10 | member) : str[i];
    }
    drawComplexString(x, y, buffer, MM_PAL_BORDER);
}

//NES EnterStatusMenu.
void setupStatusScreen() {
    for(Uint8 i = 0; i < 4; i++) {
        menu_pal_0[i] = nes_pal[boarderPalette_blue[i]];
    }
    for(Uint8 i = 0; i < 6; i++) {
        drawBox(statusBoxes[i][0], statusBoxes[i][1], statusBoxes[i][2], statusBoxes[i][3], MM_PAL_BORDER);
        drawCharMenuString(statusBoxes[i][0] + 1, statusBoxes[i][1] + 2, STATUS_TEXT_START + i, mmTarget, CHAR_MENU_STRING_LEN);
    }
}

void initStatusMenu() {
    sprite_make(FRAME_BATTLE_START, 0, 0, (Sint32) CHARACTER_PRIORITY, &mmSprites[0]);
    mmPadPrev = PadData1;
}

//NES MenuWaitForBtn_SFX, then ResumeMainMenu.
void doStatusMenu() {
    PadBits pressed = PadData1 & ~mmPadPrev;
    mmPadPrev = PadData1;

    if(pressed & (PAD_A | PAD_B | PAD_C)) {
        sfxMenuSel();
        state = TO_MAIN_MENU;
    }
}

void status_draw() {
    drawOBSprite(&mmSprites[0], mmTarget, STATUS_SPRITE_X, STATUS_SPRITE_Y);
}

//Spell at a NES magic cursor (level * 4 + slot). Slot 3 is padding and always empty.
static Uint8 getSpellAt(Uint8 cur) {
    if((cur & 0x03) == 0x03) {
        return 0;
    }
    return party[mmTarget].magic[cur >> 2][cur & 0x03];
}

//NES DrawItemDescBox.
static void openDescBox(Uint8 id) {
    drawBox(0x01, 0x16, 0x1E, 0x07, MM_PAL_BORDER);
    drawComplexString(0x02, 0x18, getMenuString(id), MM_PAL_BORDER);
    mgDescOpen = TRUE;
}

//NES DrawItemDescBox_Fanfare.
static void openDescBoxFanfare(Uint8 id) {
    music_track = MUSIC_FANFARE;
    openDescBox(id);
}

static void closeDescBox() {
    clearArea(0x01, 0x16, 0x1E, 0x07);
    mgDescOpen = FALSE;
}

//NES DrawItemTargetMenu. The box is 1 tile wider on each side so the last 6 character name fits.
static void drawTargetMenu() {
    clearMenuPlane();
    drawBox(0x00, 0x0B, 0x20, 0x08, MM_PAL_BORDER);

    for(Uint8 i = 0; i < 4; i++) {
        Uint8 code = 0x10 | i;
        Uint8 x = 0x04 + (i * 7);
        Uint8 name[3] = {code, 0x00, 0x00};
        Uint8 ailment[3] = {code, 0x02, 0x00};
        Uint8 curHp[4] = {0xFF, code, 0x05, 0x00};
        Uint8 maxHp[4] = {0x7A, code, 0x06, 0x00};

        drawComplexString(x, 0x0D, name, MM_PAL_BORDER);
        drawComplexString(x, 0x0F, ailment, MM_PAL_BORDER);
        drawComplexString(x, 0x10, curHp, MM_PAL_BORDER);
        drawComplexString(x, 0x11, maxHp, MM_PAL_BORDER);
    }
}

//NES EnterMagicMenu. The title box is 2 tiles wider for 6 character names.
static void enterMagicMenu() {
    clearMenuPlane();
    drawBox(0x01, 0x03, 0x1E, 0x13, MM_PAL_BORDER);
    drawCharMenuString(0x02, 0x05, MAGIC_LIST_TEXT, mmTarget, CHAR_MENU_STRING_MAX);
    drawBox(0x01, 0x01, 0x0A, 0x04, MM_PAL_BORDER);
    drawCharMenuString(0x02, 0x03, MAGIC_TITLE_TEXT, mmTarget, CHAR_MENU_STRING_LEN);

    mgDescOpen = FALSE;
    mmPrevDir = 0;
    mgMode = MG_MODE_NO_SPELLS;
    for(Uint8 cur = 0; cur < 0x20; cur++) {
        if(getSpellAt(cur) != 0) {
            mgCursor = cur;
            mgMode = MG_MODE_LIST;
            break;
        }
    }
}

void setupMagicScreen() {
    for(Uint8 i = 0; i < 4; i++) {
        menu_pal_0[i] = nes_pal[boarderPalette_blue[i]];
    }
    enterMagicMenu();
}

void initMagicMenu() {
    player_cursor_init(FIXED(0), FIXED(0));
    mmPadPrev = PadData1;
}

//NES MoveMagicMenuCursor. Skips empty slots. Down alone moves down, any other Up/Down combination moves up.
static void moveMagicCursor() {
    Uint8 dir = getNesDir();

    if(dir == mmPrevDir) {
        return;
    }
    mmPrevDir = dir;
    if(dir == 0) {
        return;
    }

    do {
        if(dir >= NES_DIR_D) {
            mgCursor = (dir == NES_DIR_D) ? (mgCursor + 4) & 0x1F : (mgCursor - 4) & 0x1F;
        } else if(dir == NES_DIR_R) {
            mgCursor = ((mgCursor & 0x03) == 0x02) ? (mgCursor & 0x1C) : mgCursor + 1;
        } else {
            mgCursor = ((mgCursor & 0x03) == 0x00) ? ((mgCursor & 0x1C) | 0x02) : mgCursor - 1;
        }
    } while(getSpellAt(mgCursor) == 0);

    sfxMenuMove();
    if(mgDescOpen) {
        closeDescBox();
    }
}

//NES MenuRecoverHP_Abs.
static void recoverHP(Uint8 member, Uint16 hp) {
    PLAYER_CHARACTER *c = &party[member];

    c->current_hp += hp;
    if(c->current_hp > c->max_hp) {
        c->current_hp = c->max_hp;
    }
    //The jingle starts with square 2 silenced.
    music_track = MUSIC_HEAL;
    musicApuWrite(0x4004, 0x30);
    musicApuWrite(0x4005, 0x7F);
    musicApuWrite(0x4006, 0);
    musicApuWrite(0x4007, 0);
}

static void useMP() {
    party[mmTarget].current_mp[mgCursor >> 2]--;
}

//NES ItemTargetMenuLoop cursor movement.
static void moveTargetCursor() {
    Uint8 dir = getNesDir() & (NES_DIR_R | NES_DIR_L);

    if(dir == mmPrevDir) {
        return;
    }
    mmPrevDir = dir;
    if(dir == NES_DIR_R) {
        mgTargetCursor = (mgTargetCursor + 1) & 0x03;
    } else if(dir != 0) {
        mgTargetCursor = (mgTargetCursor - 1) & 0x03;
    }
    if(dir != 0) {
        sfxMenuMove();
    }
}

static void startTarget(Uint8 descId) {
    drawTargetMenu();
    openDescBox(descId);
    mgTargetCursor = 0;
    mgMode = MG_MODE_TARGET;
}

//NES MagicMenu_Loop @A_Pressed.
static void castSpell() {
    if(party[mmTarget].current_mp[mgCursor >> 2] == 0) {
        openDescBox(DESC_NO_MP);
        return;
    }

    //Matches the NES ORA, which gives the 8th spell of odd levels the ID of the level below's 8th spell.
    mgSpell = (((mgCursor << 1) & 0x38) | getSpellAt(mgCursor)) + (MG_START - 1);
    switch(mgSpell) {
        case MG_CURE:
            mgHpRecovery = (nesFrameCounter & 0x0F) | 0x10;
            startTarget(DESC_CURE);
            break;
        case MG_CUR2:
            mgHpRecovery = (nesFrameCounter & 0x1F) | 0x20;
            startTarget(DESC_CURE);
            break;
        case MG_CUR3:
            mgHpRecovery = (nesFrameCounter & 0x3F) | 0x40;
            startTarget(DESC_CURE);
            break;
        case MG_CUR4:
            startTarget(DESC_CURE);
            break;
        case MG_HEAL:
            mgHpRecovery = (nesFrameCounter & 0x07) + 0x10;
            openDescBox(DESC_HEAL);
            mgMode = MG_MODE_HEAL_CONFIRM;
            break;
        case MG_HEL2:
            mgHpRecovery = (nesFrameCounter & 0x0F) + 0x20;
            openDescBox(DESC_HEAL);
            mgMode = MG_MODE_HEAL_CONFIRM;
            break;
        case MG_HEL3:
            mgHpRecovery = (nesFrameCounter & 0x1F) + 0x40;
            openDescBox(DESC_HEAL);
            mgMode = MG_MODE_HEAL_CONFIRM;
            break;
        case MG_PURE:
            startTarget(DESC_PURE);
            break;
        case MG_LIFE:
        case MG_LIF2:
            startTarget(DESC_LIFE);
            break;
        case MG_SOFT:
            startTarget(DESC_SOFT);
            break;
        case MG_WARP:
        case MG_EXIT:
            openDescBox(mgSpell == MG_WARP ? DESC_WARP : DESC_EXIT);
            useMP();
            mgMode = MG_MODE_EXIT;
            break;
        default:
            openDescBox(DESC_CANT_CAST);
            break;
    }

    mgIntHeal = FALSE;
    if(BUGFIX_ON(BUGFIX_INT_MAGIC)) {
        Uint16 hp = intHealAmount(mgSpell - MG_START, party[mmTarget].intel);
        if(hp) {
            mgHpRecovery = hp;
            mgIntHeal = TRUE;
        }
    }
}

static Uint16 intHealLuck(Uint8 member) {
    return mgIntHeal ? rand_min_max(0, party[member].luck >> 1) : 0;
}

//NES CureOBAilment.
static Uint8 cureOBAilment(Uint8 member, Uint8 obAilment) {
    Uint8 ailment = party[member].ailment;
    Uint8 nesAilment = (ailment == AIL_POISON) ? OB_AIL_POISON : ailment;

    if(nesAilment != obAilment) {
        return FALSE;
    }
    party[member].ailment = 0;
    return TRUE;
}

//Applies the spell to the target. Returns FALSE if it can't be used on them.
static Uint8 castOnTarget(Uint8 member) {
    PLAYER_CHARACTER *c = &party[member];

    switch(mgSpell) {
        case MG_CURE:
        case MG_CUR2:
        case MG_CUR3:
            if(c->ailment == AIL_DEAD || c->ailment == AIL_STONE) {
                return FALSE;
            }
            recoverHP(member, mgHpRecovery + intHealLuck(member));
            break;
        case MG_CUR4:
            //NES bug: no ailment check, so it refills a dead or stoned character's HP.
            if(BUGFIX_ON(BUGFIX_CUR4_FIELD) && (c->ailment == AIL_DEAD || c->ailment == AIL_STONE)) {
                return FALSE;
            }
            c->current_hp = c->max_hp;
            break;
        case MG_LIFE:
            if(!cureOBAilment(member, OB_AIL_DEAD)) {
                return FALSE;
            }
            c->current_hp = 1;
            break;
        case MG_LIF2:
            if(!cureOBAilment(member, OB_AIL_DEAD)) {
                return FALSE;
            }
            c->current_hp = c->max_hp;
            break;
        case MG_PURE:
            if(!cureOBAilment(member, OB_AIL_POISON)) {
                return FALSE;
            }
            break;
        case MG_SOFT:
            if(!cureOBAilment(member, OB_AIL_STONE)) {
                return FALSE;
            }
            break;
        default:
            break;
    }
    return TRUE;
}

void doMagicMenu() {
    PadBits pressed = PadData1 & ~mmPadPrev;
    mmPadPrev = PadData1;

    Uint8 a = (pressed & (PAD_A | PAD_C)) != 0;
    Uint8 b = (pressed & PAD_B) != 0;

    switch(mgMode) {
        case MG_MODE_LIST:
            if(a) {
                sfxMenuSel();
                castSpell();
            } else if(b) {
                state = TO_MAIN_MENU;
            } else {
                moveMagicCursor();
            }
            break;
        case MG_MODE_NO_SPELLS:
            if(a || b) {
                sfxMenuSel();
                state = TO_MAIN_MENU;
            }
            break;
        //NES ItemTargetMenuLoop. A target that can't be used restarts the loop, resetting the cursor.
        case MG_MODE_TARGET:
            if(a) {
                sfxMenuSel();
                if(castOnTarget(mgTargetCursor)) {
                    useMP();
                    drawTargetMenu();
                    mgMode = MG_MODE_DONE;
                } else {
                    sfxError();
                    mgTargetCursor = 0;
                }
            } else if(b) {
                sfxMenuSel();
                enterMagicMenu();
            } else {
                moveTargetCursor();
            }
            break;
        //NES MenuRecoverPartyHP. Dead and stoned characters are skipped.
        case MG_MODE_HEAL_CONFIRM:
            if(a) {
                for(Uint8 i = 0; i < 4; i++) {
                    if(party[i].ailment != AIL_DEAD && party[i].ailment != AIL_STONE) {
                        recoverHP(i, mgHpRecovery + intHealLuck(i));
                    }
                }
                drawTargetMenu();
                useMP();
                mgMode = MG_MODE_DONE;
            } else if(b) {
                enterMagicMenu();
            }
            break;
        case MG_MODE_DONE:
            if(a || b) {
                sfxMenuSel();
                enterMagicMenu();
            }
            break;
        //On the overworld WARP does the same as EXIT, returning to the overworld.
        case MG_MODE_EXIT:
            if(a || b) {
                sfxMenuSel();
                if(menuReturnSM) {
                    //NES UseMagic_WARP turns the APU off before going back a floor.
                    if(mgSpell == MG_WARP) {
                        musicApuWrite(0x4015, 0);
                    }
                    smMenuExit(mgSpell == MG_WARP);
                } else {
                    exitOwMenu();
                }
            }
            break;
        default:
            break;
    }
}

static Uint8 *getEquipSlots(Uint8 member) {
    return eqArmor ? party[member].armors : party[member].weapons;
}

//NES SortEquipmentList: moves empty slots to the end.
static void sortEquipmentList() {
    for(Uint8 i = 0; i < 4; i++) {
        Uint8 *slots = getEquipSlots(i);
        Uint8 sorted[4] = {0, 0, 0, 0};
        Uint8 count = 0;

        for(Uint8 j = 0; j < 4; j++) {
            if(slots[j] != 0) {
                sorted[count++] = slots[j];
            }
        }
        memcpy(slots, sorted, 4);
    }
}

//NES UnadjustEquipStats and UnadjustBBEquipStats.
static void unadjustEquipStats() {
    for(Uint8 i = 0; i < 4; i++) {
        PLAYER_CHARACTER *c = &party[i];
        Uint8 *slots = getEquipSlots(i);

        for(Uint8 j = 0; j < 4; j++) {
            if(!(slots[j] & 0x80)) {
                continue;
            }
            Uint8 index = (slots[j] & 0x7F) - 1;
            if(eqArmor) {
                Uint8 *arm = (Uint8 *)ARMOR_LUT_LOCATION + (index * 4);
                c->evade_percent += arm[0];
            } else {
                Uint8 *wep = (Uint8 *)WEAPON_LUT_LOCATION + (index * 8);
                c->hit_percent -= wep[0];
                c->damage -= wep[1];
            }
        }

        if(eqArmor) {
            c->absorb = 0;
            c->resist = 0;
        } else if(c->char_class == NES_CLS_BB || c->char_class == NES_CLS_MA) {
            c->damage = 0;
        }
    }
}

//NES ReadjustEquipStats and ReadjustBBEquipStats.
static void readjustEquipStats() {
    for(Uint8 i = 0; i < 4; i++) {
        PLAYER_CHARACTER *c = &party[i];
        Uint8 *slots = getEquipSlots(i);

        for(Uint8 j = 0; j < 4; j++) {
            if(!(slots[j] & 0x80)) {
                continue;
            }
            Uint8 index = (slots[j] & 0x7F) - 1;
            if(eqArmor) {
                Uint8 *arm = (Uint8 *)ARMOR_LUT_LOCATION + (index * 4);
                c->evade_percent -= arm[0];
                c->absorb += arm[1];
                c->resist |= arm[2];
            } else {
                Uint8 *wep = (Uint8 *)WEAPON_LUT_LOCATION + (index * 8);
                c->hit_percent += wep[0];
                c->damage += wep[1];
            }
        }

        //A BB/MA with no weapon has 0 damage here, and with no armor has 0 absorb.
        if(c->char_class == NES_CLS_BB || c->char_class == NES_CLS_MA) {
            if(eqArmor) {
                if(c->absorb == 0) {
                    c->absorb = c->level;
                }
            } else if(c->damage == 0) {
                c->damage = c->level * 2;
            } else {
                //The NES LSR leaves bit 0 of strength in the carry for the ADC, so odd strength rounds up.
                c->damage += (c->str >> 1) + (c->str & 0x01);
            }
        }
    }
}

//NES EquipShop sell: the stats are readjusted in case the item was equipped.
void removeEquipment(Uint8 armor, Uint8 member, Uint8 slot) {
    eqArmor = armor;
    unadjustEquipStats();
    getEquipSlots(member)[slot] = 0;
    sortEquipmentList();
    readjustEquipStats();
}

//NES DrawEquipMenuStrings. Each string is "E-" or 2 spaces, then 7 characters of the name.
static void drawEquipStrings() {
    Uint8 nameStart = eqArmor ? EQUIP_ARMOR_NAME_START : EQUIP_WEAPON_NAME_START;

    for(Uint8 i = 0; i < 16; i++) {
        Uint8 buffer[2 + EQUIP_NAME_LEN + 1];
        Uint8 item = eqItemBox[i];

        memset(buffer, 0xFF, sizeof(buffer) - 1);
        buffer[sizeof(buffer) - 1] = 0x00;
        if(item & 0x7F) {
            memcpy(&buffer[2], getItemName(nameStart + (item & 0x7F)), EQUIP_NAME_LEN);
            if(item & 0x80) {
                buffer[0] = 0xC7;
                buffer[1] = 0xC2;
            }
        }
        drawComplexString(equipStringPos[i][0], equipStringPos[i][1], buffer, MM_PAL_BORDER);
    }
}

//NES IsEquipLegal. Also unequips the character's other weapon, or their other armor of the same type.
static Uint8 isEquipLegal(Uint8 item) {
    Uint8 index = item - 1;
    Uint8 member = eqCursor >> 2;
    Uint16 classBit = 0x800 >> party[member].char_class;
    Uint8 *perm = (Uint8 *)(eqArmor ? ARMOR_PERMISSIONS : WEAPON_PERMISSIONS) + (index * 2);

    if((perm[0] | (perm[1] << 8)) & classBit) {
        return FALSE;
    }

    Uint8 *slots = &eqItemBox[member * 4];
    if(!eqArmor) {
        for(Uint8 i = 0; i < 4; i++) {
            slots[i] &= 0x7F;
        }
        return TRUE;
    }

    Uint8 *types = (Uint8 *)ARMOR_TYPES;
    for(Uint8 i = 0; i < 4; i++) {
        if((member * 4) + i == eqCursor || !(slots[i] & 0x80)) {
            continue;
        }
        if(types[(slots[i] & 0x7F) - 1] == types[index]) {
            slots[i] &= 0x7F;
        }
    }
    return TRUE;
}

//Sets the palette of a 2 row high area of the equip menu's top boxes, like the NES attribute writes.
static void setEquipAttrib(Uint8 x, Uint8 width, Uint8 pal) {
    for(Uint8 y = 2; y < 4; y++) {
        for(Uint8 i = 0; i < width; i++) {
            Uint16 *tile = &drawPlane[y][x + i + CENTER_ADJUSTMENT];
            *tile = (*tile & 0x0FFF) | (pal << 12);
        }
    }
}

//NES UpdateEquipMenuModeAttrib.
static void updateEquipModeAttrib() {
    for(Uint8 i = 0; i < 3; i++) {
        setEquipAttrib(EQ_MODE_X(i), EQ_MODE_W(i), (i == eqModeCursor) ? MM_PAL_EQ_MODE : MM_PAL_BORDER);
    }
}

//NES EnterEquipMenu.
void setupEquipScreen() {
#ifdef FF1_JP
    const Uint8 *menuPal = (Uint8 *)LUT_MENU_PALETTES;
#else
    const Uint8 *menuPal = eqMenuPal;
#endif

    for(Uint8 i = 0; i < 4; i++) {
        menu_pal_0[i] = nes_pal[boarderPalette_blue[i]];
        menu_pal_1[i] = nes_pal[menuPal[4 + i]];
        menu_pal_2[i] = nes_pal[menuPal[8 + i]];
    }
    sortEquipmentList();

    drawBox(0x01, 0x01, 0x07, 0x04, MM_PAL_BORDER);
    drawComplexString(0x02, 0x03, getMenuString(eqArmor ? EQUIP_ARMOR_TITLE_TEXT : EQUIP_WEAPON_TITLE_TEXT), MM_PAL_BORDER);
    drawBox(0x08, 0x01, 0x17, 0x04, MM_PAL_BORDER);
    drawComplexString(0x09, 0x03, getMenuString(EQUIP_MODE_TEXT), MM_PAL_BORDER);
    //NES DrawEquipMenu's title attributes.
    setEquipAttrib(0x02, EQ_TITLE_W, MM_PAL_EQ_TITLE);

    //The name boxes are 2 tiles wider to the left for 6 character names.
    for(Uint8 i = 0; i < 4; i++) {
        Uint8 y = 0x05 + (i * 6);
        drawBox(0x08, y, 0x17, 0x06, MM_PAL_BORDER);
        drawBox(0xFF, y, 0x0A, 0x04, MM_PAL_BORDER);
        drawComplexString(0x00, y + 2, getMenuString(EQUIP_NAME_TEXT + (i * 2)), MM_PAL_BORDER);
    }

    for(Uint8 i = 0; i < 4; i++) {
        memcpy(&eqItemBox[i * 4], getEquipSlots(i), 4);
    }
    unadjustEquipStats();
    drawEquipStrings();
}

void initEquipMenu() {
    player_cursor_init(FIXED(0), FIXED(0));
    mmPadPrev = PadData1;
    eqSfxPrevDir = getNesDir();
    eqMode = EQ_MODE_SELECT;
    eqModeCursor = 0;
}

//NES MoveEquipMenuCurs. Left/Right flips the column, Down alone moves down, any other Up/Down combination moves up.
static void moveEquipCursor() {
    Uint8 dir = getNesDir();

    if(dir == mmPrevDir) {
        return;
    }
    mmPrevDir = dir;
    if(dir == 0) {
        return;
    }
    if(dir < NES_DIR_D) {
        eqCursor ^= 0x01;
    } else if(dir == NES_DIR_D) {
        eqCursor = (eqCursor + 2) & 0x0F;
    } else {
        eqCursor = (eqCursor - 2) & 0x0F;
    }
}

//NES MoveEquipMenuModeCurs.
static void moveEquipModeCursor() {
    Uint8 dir = getNesDir() & (NES_DIR_R | NES_DIR_L);

    if(dir == mmPrevDir) {
        return;
    }
    mmPrevDir = dir;
    if(dir == NES_DIR_R) {
        eqModeCursor = (eqModeCursor < 2) ? eqModeCursor + 1 : 0;
    } else if(dir != 0) {
        eqModeCursor = (eqModeCursor > 0) ? eqModeCursor - 1 : 2;
    }
}

void doEquipMenu() {
    PadBits pressed = PadData1 & ~mmPadPrev;
    mmPadPrev = PadData1;
    //NES EquipMenuFrame increments the framecounter twice.
    nesFrameCounter++;
    updateEquipModeAttrib();

    Uint8 a = (pressed & (PAD_A | PAD_C)) != 0;
    Uint8 b = (pressed & PAD_B) != 0;

    //NES EquipMenuFrame sounds, which ignore the mode.
    Uint8 dir = getNesDir();
    if(a || b) {
        sfxMenuSel();
    } else if(dir != 0 && dir != eqSfxPrevDir) {
        sfxMenuMove();
    }
    eqSfxPrevDir = dir;

    switch(eqMode) {
        case EQ_MODE_SELECT:
            if(a) {
                eqCursor = 0;
                eqCursor2 = 0;
                eqMode = (eqModeCursor == 0) ? EQ_MODE_EQUIP : (eqModeCursor == 1) ? EQ_MODE_TRADE : EQ_MODE_DROP;
            } else if(b) {
                for(Uint8 i = 0; i < 4; i++) {
                    memcpy(getEquipSlots(i), &eqItemBox[i * 4], 4);
                }
                sortEquipmentList();
                readjustEquipStats();
                state = TO_MAIN_MENU;
            } else {
                moveEquipModeCursor();
            }
            return;
        case EQ_MODE_EQUIP:
            if(a) {
                Uint8 item = eqItemBox[eqCursor];
                //Unequipping is always allowed.
                if(item != 0 && ((item & 0x80) || isEquipLegal(item))) {
                    eqItemBox[eqCursor] ^= 0x80;
                    drawEquipStrings();
                } else {
                    sfxError();
                }
            } else if(b) {
                break;
            } else {
                moveEquipCursor();
            }
            return;
        case EQ_MODE_TRADE:
            if(a) {
                eqCursor2 = eqCursor;
                eqMode = EQ_MODE_TRADE_SECOND;
            } else if(b) {
                break;
            } else {
                moveEquipCursor();
            }
            return;
        //Trading unequips both items, even when both cursors are on the same slot.
        case EQ_MODE_TRADE_SECOND:
            if(a) {
                Uint8 tmp = eqItemBox[eqCursor];
                eqItemBox[eqCursor] = eqItemBox[eqCursor2] & 0x7F;
                eqItemBox[eqCursor2] = tmp & 0x7F;
                drawEquipStrings();
                eqMode = EQ_MODE_TRADE;
            } else if(b) {
                eqMode = EQ_MODE_TRADE;
            } else {
                moveEquipCursor();
            }
            return;
        case EQ_MODE_DROP:
            if(a) {
                if(eqItemBox[eqCursor] != 0) {
                    eqMode = EQ_MODE_DROP_CONFIRM;
                } else {
                    sfxError();
                }
            } else if(b) {
                break;
            } else {
                moveEquipCursor();
            }
            return;
        case EQ_MODE_DROP_CONFIRM:
            if(a) {
                eqItemBox[eqCursor] = 0;
                drawEquipStrings();
                eqMode = EQ_MODE_DROP;
            } else if(b) {
                eqMode = EQ_MODE_DROP;
            }
            return;
        default:
            return;
    }

    //B in a sub-mode returns to the mode cursor, which resets to EQUIP.
    eqMode = EQ_MODE_SELECT;
    eqModeCursor = 0;
}

static void drawEquipCursor(Uint8 cur, Uint8 xOr) {
    cursor.xPos = FIXED((((cur & 0x01) ? 0x90 : 0x40) | xOr) + 32);
    cursor.yPos = FIXED(equipCursorY[cur >> 1]);
    player_cursor_draw();
}

//NES DrawEquipMenuCurs and DrawEquipMenuCursSecondary. Bit 1 of the framecounter picks which cursor
//flickers on, and with the double increment it changes every frame.
void equip_draw() {
    Uint8 bit1 = (nesFrameCounter >> 1) & 0x01;

    switch(eqMode) {
        case EQ_MODE_SELECT:
            cursor.xPos = FIXED(equipModeCursorX[eqModeCursor] + 32);
            cursor.yPos = FIXED(0x14);
            player_cursor_draw();
            break;
        case EQ_MODE_TRADE_SECOND:
            if(bit1) {
                drawEquipCursor(eqCursor, 0x00);
            } else {
                drawEquipCursor(eqCursor2, 0x04);
            }
            break;
        case EQ_MODE_DROP_CONFIRM:
            if(!bit1) {
                drawEquipCursor(eqCursor, 0x00);
            }
            break;
        default:
            drawEquipCursor(eqCursor, 0x00);
            break;
    }
}

//NES DrawItemBox. Lists every item held except the orbs, with a 2 digit count for the quantity items.
static void drawItemBox() {
    drawBox(0x01, 0x03, 0x1E, 0x13, MM_PAL_BORDER);

    itCount = 0;
    for(Uint8 id = 1; id < ITEM_STOP; id++) {
        if(id >= ITEM_ORB_START && id < ITEM_TENT) {
            continue;
        }
        if(items[id] != 0) {
            itItemBox[itCount++] = id;
        }
    }

    for(Uint8 i = 0; i < itCount; i++) {
        Uint8 id = itItemBox[i];
        Uint8 buffer[ITEM_NAME_LEN + 1];

        memcpy(buffer, getItemName(id), ITEM_NAME_LEN);
        buffer[ITEM_NAME_LEN] = 0x00;
        if(id >= ITEM_TENT) {
            Uint8 qty = items[id];
            buffer[5] = (qty >= 10) ? 0x80 + ((qty / 10) % 10) : 0xFF;
            buffer[6] = 0x80 + (qty % 10);
        }
        drawComplexString(itemStrX[i % 3], 0x06 + ((i / 3) * 2), buffer, MM_PAL_BORDER);
    }
}

//NES ResumeItemMenu: redraws the list without clearing the screen, so an open description box stays up.
static void resumeItemMenu() {
    drawItemBox();
    drawBox(0x01, 0x01, 0x08, 0x04, MM_PAL_BORDER);
    drawComplexString(0x02, 0x03, getMenuString(ITEM_TITLE_TEXT), MM_PAL_BORDER);

    if(itCount == 0) {
        openDescBox(DESC_NO_ITEMS);
        itMode = IT_MODE_EMPTY;
        return;
    }
    itCursor = 0;
    mmPrevDir = 0;
    itMode = IT_MODE_LIST;
}

//NES EnterItemMenu.
static void enterItemMenu() {
    clearMenuPlane();
    mgDescOpen = FALSE;
    resumeItemMenu();
}

void setupItemScreen() {
    for(Uint8 i = 0; i < 4; i++) {
        menu_pal_0[i] = nes_pal[boarderPalette_blue[i]];
    }
    enterItemMenu();
}

void initItemMenu() {
    player_cursor_init(FIXED(0), FIXED(0));
    mmPadPrev = PadData1;
}

//NES MoveItemMenuCurs. Left/Right wrap through the whole list, Up/Down wrap within the column.
static void moveItemCursor() {
    Uint8 dir = getNesDir();

    if(dir == mmPrevDir) {
        return;
    }
    mmPrevDir = dir;
    if(dir == 0) {
        return;
    }

    Sint16 cur = itCursor;
    if(dir == NES_DIR_R) {
        cur = (cur + 1 < itCount) ? cur + 1 : 0;
    } else if(dir < NES_DIR_D) {
        cur = (cur > 0) ? cur - 1 : itCount - 1;
    } else if(dir == NES_DIR_D) {
        cur += 3;
        if(cur >= itCount) {
            while(cur >= 3) {
                cur -= 3;
            }
        }
    } else if(cur >= 3) {
        cur -= 3;
    } else if(itCount >= 3) {
        while(cur < itCount) {
            cur += 3;
        }
        cur -= 3;
    }
    itCursor = cur;

    sfxMenuMove();
    if(mgDescOpen) {
        closeDescBox();
    }
}

//NES MenuRecoverPartyHP, then the party's MP for the House. The save prompt is skipped.
static void useRestItem(Uint8 hp, Uint8 restoreMP) {
    items[itItem]--;
    for(Uint8 i = 0; i < 4; i++) {
        PLAYER_CHARACTER *c = &party[i];
        if(c->ailment == AIL_DEAD || c->ailment == AIL_STONE) {
            continue;
        }
        recoverHP(i, hp);
        if(restoreMP) {
            memcpy(c->current_mp, c->max_mp, sizeof(c->current_mp));
        }
    }
    drawTargetMenu();
    itMode = IT_MODE_DONE;
}

static void startItemTarget(Uint8 descId) {
    startTarget(descId);
    itMode = IT_MODE_TARGET;
}

//NES ItemMenu_Loop @APressed and the UseItem routines.
static void useItem() {
    itItem = itItemBox[itCursor];

    if(itItem < ITEM_ORB_START && itemDescOnly[itItem] != 0) {
        openDescBox(itemDescOnly[itItem]);
        return;
    }

    switch(itItem) {
        case ITEM_LUTE:
            if(smUsePlateItem(OBJID_LUTEPLATE, TP_SPEC_USELUTE)) {
                openDescBoxFanfare(DESC_LUTE);
            } else {
                openDescBox(DESC_LUTE_CANT);
            }
            break;
        case ITEM_ROD:
            if(smUsePlateItem(OBJID_RODPLATE, TP_SPEC_USEROD)) {
                openDescBoxFanfare(DESC_ROD);
            } else {
                openDescBox(DESC_ROD_CANT);
            }
            break;
        case ITEM_FLOATER:
            if(!menuReturnSM && getOWTileSpecial() == OWTP_SPEC_FLOATER && airshipVisible == 0) {
                airshipVisible++;
                openDescBoxFanfare(DESC_FLOATER);
            } else {
                openDescBox(DESC_FLOATER_CANT);
            }
            break;
        case ITEM_BOTTLE:
            if(gameFlags[OBJID_FAIRY] & GMFLG_OBJVISIBLE) {
                openDescBox(DESC_BOTTLE_EMPTY);
            } else {
                items[ITEM_BOTTLE] = 0;
                gameFlags[OBJID_FAIRY] |= GMFLG_OBJVISIBLE;
                openDescBoxFanfare(DESC_BOTTLE);
                resumeItemMenu();
            }
            break;
        case ITEM_TENT:
            if(menuReturnSM) {
                openDescBox(DESC_TENT_CANT);
            } else {
                useRestItem(30, FALSE);
            }
            break;
        case ITEM_CABIN:
            if(menuReturnSM) {
                openDescBox(DESC_CABIN_CANT);
            } else {
                useRestItem(60, FALSE);
            }
            break;
        case ITEM_HOUSE:
            if(menuReturnSM) {
                openDescBox(DESC_HOUSE_CANT);
            } else {
                useRestItem(120, TRUE);
            }
            break;
        case ITEM_HEAL:
            startItemTarget(DESC_HEAL_POTION);
            break;
        case ITEM_PURE:
            startItemTarget(DESC_PURE_POTION);
            break;
        case ITEM_SOFT:
            startItemTarget(DESC_SOFT_POTION);
            break;
        default:
            break;
    }
}

//Applies the potion to the target. Returns FALSE if it can't be used on them.
static Uint8 useItemOnTarget(Uint8 member) {
    PLAYER_CHARACTER *c = &party[member];

    switch(itItem) {
        case ITEM_HEAL:
            if(c->ailment == AIL_DEAD || c->ailment == AIL_STONE) {
                return FALSE;
            }
            recoverHP(member, 30);
            break;
        case ITEM_PURE:
            if(!cureOBAilment(member, OB_AIL_POISON)) {
                return FALSE;
            }
            break;
        case ITEM_SOFT:
            if(!cureOBAilment(member, OB_AIL_STONE)) {
                return FALSE;
            }
            break;
        default:
            break;
    }
    items[itItem]--;
    return TRUE;
}

void doItemMenu() {
    PadBits pressed = PadData1 & ~mmPadPrev;
    mmPadPrev = PadData1;

    Uint8 a = (pressed & (PAD_A | PAD_C)) != 0;
    Uint8 b = (pressed & PAD_B) != 0;

    switch(itMode) {
        case IT_MODE_LIST:
            if(a) {
                sfxMenuSel();
                useItem();
            } else if(b) {
                state = TO_MAIN_MENU;
            } else {
                moveItemCursor();
            }
            break;
        case IT_MODE_EMPTY:
            if(a || b) {
                sfxMenuSel();
                state = TO_MAIN_MENU;
            }
            break;
        //A target that can't be used restarts the loop, resetting the cursor.
        case IT_MODE_TARGET:
            if(a) {
                sfxMenuSel();
                if(useItemOnTarget(mgTargetCursor)) {
                    drawTargetMenu();
                    itMode = IT_MODE_DONE;
                } else {
                    sfxError();
                    mgTargetCursor = 0;
                }
            } else if(b) {
                sfxMenuSel();
                enterItemMenu();
            } else {
                moveTargetCursor();
            }
            break;
        case IT_MODE_DONE:
            if(a || b) {
                //The rest items' skipped save prompt waits without a sound.
                if(itItem != ITEM_TENT && itItem != ITEM_CABIN && itItem != ITEM_HOUSE) {
                    sfxMenuSel();
                }
                enterItemMenu();
            }
            break;
        default:
            break;
    }
}

void item_draw() {
    if(itMode == IT_MODE_LIST) {
        cursor.xPos = FIXED(itemCursorX[itCursor % 3] + 32);
        cursor.yPos = FIXED(0x30 + ((itCursor / 3) * 0x10));
        player_cursor_draw();
    } else if(itMode == IT_MODE_TARGET) {
        cursor.xPos = FIXED(targetCursorX[mgTargetCursor] + 32);
        cursor.yPos = FIXED(0x68);
        player_cursor_draw();
    }
}

void magic_draw() {
    //The NES leaves the cursor up while WARP or EXIT waits for a button.
    if(mgMode == MG_MODE_LIST || mgMode == MG_MODE_EXIT) {
        cursor.xPos = FIXED(((mgCursor & 0x03) * 48) + 0x50 + 32);
        cursor.yPos = FIXED(((mgCursor << 2) & 0xF0) + 0x28);
        player_cursor_draw();
    } else if(mgMode == MG_MODE_TARGET) {
        cursor.xPos = FIXED(targetCursorX[mgTargetCursor] + 32);
        cursor.yPos = FIXED(0x68);
        player_cursor_draw();
    }
}

void mainmenu_draw() {
    for(Uint8 i = 0; i < 4; i++) {
        drawOBSprite(&mmSprites[i], i, charSpritePos[i][0], charSpritePos[i][1]);
    }
    if(mmMode == MM_MODE_MAIN) {
        cursor.xPos = FIXED(MM_CURSOR_X);
        cursor.yPos = FIXED(mainCursorY[mmCursor]);
    } else {
        cursor.xPos = FIXED(subCursorPos[mmCursor][0]);
        cursor.yPos = FIXED(subCursorPos[mmCursor][1]);
    }
    player_cursor_draw();
}
