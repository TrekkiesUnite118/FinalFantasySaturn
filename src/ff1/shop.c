#include <string.h>
#include "types.h"
#include "video.h"
#include "graphicrefs.h"
#include "nesutils.h"
#include "player.h"
#include "menu.h"
#include "sprdata.h"
#include "vblank.h"
#include "cd.h"
#include "memmap.h"
#include "battle.h"
#include "dialog.h"
#include "mainmenu.h"
#include "stdmap.h"
#include "shop.h"
#include "music.h"
#include "msxgfx.h"

//NES BG palettes 3, 1, 2 and 0.
#define SHOP_PAL_BORDER (0)
#define SHOP_PAL_TITLE (1)
#define SHOP_PAL_GOLD (2)
#define SHOP_PAL_KEEPER (3)

#define SHOP_BOX_DIALOGUE (0)
#define SHOP_BOX_TITLE (1)
#define SHOP_BOX_INV (2)
#define SHOP_BOX_CMD (3)
#define SHOP_BOX_GOLD (4)

//NES shop string IDs.
#define SHOP_STR_GOLD (0x08)
#define SHOP_STR_WELCOME (0x09)
#define SHOP_STR_BUYSELLEXIT (0x0A)
#define SHOP_STR_BUYEXIT (0x0B)
#define SHOP_STR_NOROOM (0x0C)
#define SHOP_STR_WHATWOULD (0x0D)
#define SHOP_STR_GOLDOK (0x0E)
#define SHOP_STR_YESNO (0x0F)
#define SHOP_STR_CANTAFFORD (0x10)
#define SHOP_STR_WHOGETS (0x11)
#define SHOP_STR_THANKYOU (0x13)
#define SHOP_STR_WHOSELLS (0x14)
#define SHOP_STR_WHOLEARNS (0x17)
#define SHOP_STR_CANTLEARN (0x19)
#define SHOP_STR_KNOWN (0x1A)
#define SHOP_STR_NOSELL (0x1E)
#define SHOP_STR_WHOREVIVE (0x20)
#define SHOP_STR_REVIVED (0x21)
#define SHOP_STR_LEVELFULL (0x22)
#define SHOP_STR_NOHELP (0x23)
#define SHOP_STR_TOOBAD (0x25)

#define SHOPKEEP_X (0x0B)
#define SHOPKEEP_Y (0x06)
#define SHOPKEEP_SIZE (10)
#define SHOPKEEP_TILES (14) //Tiles per shopkeeper graphic.

#define SHOP_INV_MAX (5)
#define SHOP_SPELL_START (0xB0)

#define FRAME_BATTLE_START (96)

enum __shopmode__ {
    SHOP_MODE_NONE,
    SHOP_MODE_LOOP,
    SHOP_MODE_WAIT,
    SHOP_MODE_JINGLE,
    SHOP_MODE_FADE_OUT,
    SHOP_MODE_FADE_IN
};

//What to do once the current loop, wait or fade finishes.
enum __shopstep__ {
    ST_EXIT,
    ST_EQ_CMD,
    ST_EQ_BUY_ITEM,
    ST_EQ_BUY_YN,
    ST_EQ_BUY_CHAR,
    ST_EQ_SELL_CHAR,
    ST_EQ_SELL_ITEM,
    ST_EQ_SELL_YN,
    ST_IT_CMD,
    ST_IT_ITEM,
    ST_IT_YN,
    ST_MG_CHAR,
    ST_MG_SPELL,
    ST_MG_YN,
    ST_CL_START,
    ST_CL_SEL,
    ST_CL_YN,
    ST_INN_STAY,
    ST_INN_YN,
    ST_INN_FADED,
    ST_INN_DONE
};

static Uint8 shopBoxPal[5] = {SHOP_PAL_BORDER, SHOP_PAL_TITLE, SHOP_PAL_BORDER, SHOP_PAL_BORDER, SHOP_PAL_GOLD};

//NES DrawShopPartySprites positions, in party order.
static Uint16 shopSpritePos[4][2] = {
    {0x88 + 32, 0x50}, {0x98 + 32, 0x38}, {0x98 + 32, 0x50}, {0x98 + 32, 0x68}
};

//NES lut_ShopCurs_Cmd and lut_ShopCurs_List.
static Uint8 shopCursCmd[4][2] = {{0x28, 0xA0}, {0x28, 0xB0}, {0x28, 0xC0}, {0x28, 0xD0}};
static Uint8 shopCursList[5][2] = {{0xA8, 0x20}, {0xA8, 0x40}, {0xA8, 0x60}, {0xA8, 0x80}, {0xA8, 0xA0}};

//NES ShopLoop_CharNames @NamesString.
static Uint8 shopNamesStr[] = {0x10, 0x00, 0x01, 0x11, 0x00, 0x01, 0x12, 0x00, 0x01, 0x13, 0x00, 0x00};

static Uint8 shopType;
static SPRITE_INFO shopSprites[4];
static PadBits shopPadPrev;
static Uint8 shopJoyA; //NES joy_a/joy_b catchers.
static Uint8 shopJoyB;
static Uint8 shopPrevDir; //NES joy_prevdir, up/down only.
static Uint8 shopSfxPrevDir; //NES joy for _ShopFrame_CheckBtns, all directions.

static Uint8 shopMode;
static Uint8 shopStep;
static Uint8 shopCursor;
static Uint8 shopCursorMax;
static Uint8 (*shopCursLut)[2];

static Uint8 shopItemBox[SHOP_INV_MAX + 1]; //NES item_box, null terminated. Inns and clinics keep their price here.
static Uint8 shopStr[SHOP_INV_MAX * 8 + 1]; //NES str_buf+$10.
static Uint8 shopCurItem;
static Uint16 shopCurPrice;
static Uint8 shopMember;
static Uint8 shopSlot;
static Uint8 shopDead[4]; //Party indexes in the clinic list.

static Uint8 shopSprNesPal[8]; //NES cur_pal+$10, the battle sprite palettes 0 and 1.
static Uint8 shopSprBackup[8]; //NES tmp_pal.

static Uint8 *getShopString(Uint8 id) {
    Uint8 *ptr = (Uint8 *)LUT_SHOP_STRINGS + (id * 2);
    return (Uint8 *)SHOP_BANK_ADD + (ptr[0] | (ptr[1] << 8));
}

static void getShopBoxOrigin(Uint8 box, Uint8 *x, Uint8 *y) {
    Uint8 *lut = (Uint8 *)LUT_SHOP_BOX;

    *x = lut[box] + 1;
    *y = lut[5 + box] + 2;
}

//NES DrawShopBox. Returns the text origin through x and y.
static void drawShopBox(Uint8 box, Uint8 *x, Uint8 *y) {
    Uint8 *lut = (Uint8 *)LUT_SHOP_BOX;

    drawBox(lut[box], lut[5 + box], lut[10 + box], lut[15 + box], shopBoxPal[box]);
    getShopBoxOrigin(box, x, y);
}

//NES LoadShopBoxDims and EraseBox.
static void eraseShopBox(Uint8 box) {
    Uint8 *lut = (Uint8 *)LUT_SHOP_BOX;

    clearArea(lut[box], lut[5 + box], lut[10 + box], lut[15 + box]);
}

//NES DrawShopDialogueBox.
static void shopDialogue(Uint8 id) {
    Uint8 x, y;

    drawShopBox(SHOP_BOX_DIALOGUE, &x, &y);
    drawComplexString(x, y, getShopString(id), SHOP_PAL_BORDER);
}

//Replaces the NES "Stay, to save your data." since the inn doesn't save: "Welcome / .. / Stay / the / night?"
#ifdef FF1_JP
//"いらっしゃい! / ひとばん / とまって / いくかしら?"
static Uint8 innStayStr[] = {
    0x8B, 0xB0, 0x7C, 0x95, 0x7D, 0x8B, 0xC4, 0x01,
    0xA4, 0x9D, 0x57, 0xB6, 0x01,
    0x9D, 0xA8, 0x7C, 0x9C, 0x01,
    0x8B, 0x91, 0x8F, 0x95, 0xB0, 0xC5, 0x00
};
#else
static Uint8 innStayStr[] = {
    0xA0, 0xA8, 0xAF, 0xA6, 0xB2, 0x34, 0x05,
    0xFF, 0xFF, 0x69, 0x05,
    0x9C, 0xB7, 0xA4, 0xBC, 0x05,
    0xB7, 0xAB, 0xA8, 0x05,
    0xB1, 0xAC, 0xAA, 0xAB, 0xB7, 0xC5, 0x00
};
#endif

//NES DrawShopGoldBox.
static void drawShopGoldBox() {
    static Uint8 goldStr[] = {0x04, 0x00};
    Uint8 x, y;

    drawShopBox(SHOP_BOX_GOLD, &x, &y);
    drawComplexString(x, y, goldStr, SHOP_PAL_GOLD);
    drawComplexString(x + 6, y, getShopString(SHOP_STR_GOLD), SHOP_PAL_GOLD);
}

//NES LoadShopInventory.
static void loadShopInventory() {
    Uint8 *ptr = (Uint8 *)LUT_SHOP_DATA + (shop_id * 2);
    Uint8 *inv = (Uint8 *)SHOP_BANK_ADD + (ptr[0] | (ptr[1] << 8));

    memcpy(shopItemBox, inv, SHOP_INV_MAX);
    shopItemBox[SHOP_INV_MAX] = 0;
}

//NES _CommonShopLoop_Main setup.
static void startShopLoop(Uint8 (*lut)[2], Uint8 max, Uint8 step) {
    shopCursLut = lut;
    shopCursorMax = max;
    shopCursor = 0;
    shopPrevDir = 0;
    if(PadData1 & PAD_U) shopPrevDir |= 0x08;
    if(PadData1 & PAD_D) shopPrevDir |= 0x04;
    shopStep = step;
    shopMode = SHOP_MODE_LOOP;
}

//NES ShopLoop_YesNo, ShopLoop_BuyExit and ShopLoop_BuySellExit.
static void shopCmdLoop(Uint8 strId, Uint8 max, Uint8 step) {
    Uint8 x, y;

    drawShopBox(SHOP_BOX_CMD, &x, &y);
    drawComplexString(x, y, getShopString(strId), SHOP_PAL_BORDER);
    startShopLoop(shopCursCmd, max, step);
}

//NES ShopLoop_CharNames.
static void shopCharNames(Uint8 step) {
    Uint8 x, y;

    drawShopBox(SHOP_BOX_CMD, &x, &y);
    drawComplexString(x, y, shopNamesStr, SHOP_PAL_BORDER);
    startShopLoop(shopCursCmd, 4, step);
}

//NES ShopFrameNoCursor loops waiting for A or B.
static void shopWaitAB(Uint8 step) {
    shopJoyA = 0;
    shopJoyB = 0;
    shopStep = step;
    shopMode = SHOP_MODE_WAIT;
}

//NES ShopSelectBuyItem and ShopSelectBuyMagic.
static void shopSelectList(Uint8 magic, Uint8 step) {
    Uint8 x, y;
    Uint8 *s = shopStr;
    Uint8 count = 0;

    while(count < SHOP_INV_MAX && shopItemBox[count] != 0) {
        Uint8 id = shopItemBox[count];
        s[0] = 0x02;
        s[1] = id;
        s[2] = 0x01;
        if(magic) {
            s[3] = 0xC6; //The 'L' tile, then the spell level.
            s[4] = (((id - SHOP_SPELL_START) >> 3) + 0x81) & 0xFF;
        } else {
            s[3] = 0xFF;
            s[4] = 0xFF;
        }
        s[5] = 0x03;
        s[6] = id;
        s[7] = 0x01;
        s += 8;
        count++;
    }
    *s = 0x00;

    drawShopBox(SHOP_BOX_INV, &x, &y);
    drawComplexString(x, y, shopStr, SHOP_PAL_BORDER);
    eraseShopBox(SHOP_BOX_CMD);
    startShopLoop(shopCursList, count, step);
}

static void drawShopNumber(Uint16 num) {
    Uint8 buffer[8];
    Uint8 x, y;

    getShopBoxOrigin(SHOP_BOX_DIALOGUE, &x, &y);
    formatNumber(buffer, num, 5);
    drawComplexString(x, y, buffer, SHOP_PAL_BORDER);
}

//NES DrawShopBuyItemConfirm.
static void shopBuyConfirm() {
    shopDialogue(SHOP_STR_GOLDOK);
    shopCurItem = shopItemBox[shopCursor];
    shopCurPrice = getItemPrice(shopCurItem);
    drawShopNumber(shopCurPrice);
}

//NES DrawShopSellItemConfirm.
static void shopSellConfirm() {
    shopSlot = shopCursor;
    shopDialogue(SHOP_STR_GOLDOK);
    shopCurPrice = getItemPrice(shopItemBox[shopCursor]) >> 1;
    drawShopNumber(shopCurPrice);
}

//NES DrawInnClinicConfirm.
static Uint16 innClinicPrice() {
    return shopItemBox[0] | (shopItemBox[1] << 8);
}

static void innClinicConfirm() {
    shopDialogue(SHOP_STR_GOLDOK);
    drawShopNumber(innClinicPrice());
}

//NES ShopPayPrice.
static void shopPay(Uint16 price) {
    partyGold -= price;
    drawShopGoldBox();
}

//NES InnClinic_CanAfford. Returns FALSE after leaving the shop when they can't afford it.
static Uint8 innClinicCanAfford() {
    if(partyGold < innClinicPrice()) {
        shopDialogue(SHOP_STR_CANTAFFORD);
        shopWaitAB(ST_EXIT);
        return FALSE;
    }
    shopPay(innClinicPrice());
    return TRUE;
}

static void equipLoop() {
    shopCmdLoop(SHOP_STR_BUYSELLEXIT, 3, ST_EQ_CMD);
}

static void equipCancel() {
    shopDialogue(SHOP_STR_TOOBAD);
    equipLoop();
}

//NES EquipShop_GiveItemToChar.
static Uint8 equipGiveItem() {
    Uint8 armor = (shopType == SHOP_ARMOR);
    Uint8 *slots = armor ? party[shopMember].armors : party[shopMember].weapons;

    for(Uint8 i = 0; i < 4; i++) {
        if(slots[i] == 0) {
            slots[i] = shopCurItem - (armor ? TCITYPE_ARMSTART : TCITYPE_WEPSTART) + 1;
            return TRUE;
        }
    }
    return FALSE;
}

//NES EquipMenu_BuildSellBox. Stops at the first empty slot.
static Uint8 equipBuildSellBox() {
    Uint8 armor = (shopType == SHOP_ARMOR);
    Uint8 *slots = armor ? party[shopMember].armors : party[shopMember].weapons;
    Uint8 count = 0;

    while(count < 4 && slots[count] != 0) {
        shopItemBox[count] = (slots[count] & 0x7F) + (armor ? TCITYPE_ARMSTART : TCITYPE_WEPSTART) - 1;
        count++;
    }
    shopItemBox[count] = 0;
    return count;
}

static void itemLoop() {
    shopCmdLoop(SHOP_STR_BUYEXIT, 2, ST_IT_CMD);
}

static void magicStart() {
    shopDialogue(SHOP_STR_WHOLEARNS);
    shopCharNames(ST_MG_CHAR);
}

static void magicLoop() {
    shopCharNames(ST_MG_CHAR);
}

//NES MagicShop_AssertLearn. Returns FALSE after drawing why they can't learn it.
static Uint8 magicAssertLearn() {
    PLAYER_CHARACTER *c = &party[shopMember];
    Uint8 magicId = shopCurItem - SHOP_SPELL_START;
    Uint8 level = magicId >> 3;
    Uint8 spell = (magicId & 0x07) + 1;
    Uint8 *ptr = (Uint8 *)LUT_MAGIC_PERMIS_PTR + (c->char_class * 2);
    Uint8 *permis = (Uint8 *)SHOP_BANK_ADD + (ptr[0] | (ptr[1] << 8));

    if(permis[level] & (0x80 >> (magicId & 0x07))) {
        shopDialogue(SHOP_STR_CANTLEARN);
        return FALSE;
    }
    for(Uint8 i = 0; i < 3; i++) {
        if(c->magic[level][i] == spell) {
            shopDialogue(SHOP_STR_KNOWN);
            return FALSE;
        }
    }
    for(Uint8 i = 0; i < 3; i++) {
        if(c->magic[level][i] == 0) {
            shopSlot = i;
            return TRUE;
        }
    }
    shopDialogue(SHOP_STR_LEVELFULL);
    return FALSE;
}

//NES EnterShop_Clinic and ClinicBuildNameString.
static void clinicStart() {
    Uint8 x, y;
    Uint8 *s = shopStr;
    Uint8 count = 0;

    shopJoyA = 0;
    shopJoyB = 0;
    for(Uint8 i = 0; i < 4; i++) {
        if(party[i].ailment == AIL_DEAD) {
            s[0] = 0x10 + i;
            s[1] = 0x00;
            s[2] = 0x01;
            s += 3;
            shopDead[count++] = i;
        }
    }
    *s = 0x00;

    if(count == 0) {
        shopDialogue(SHOP_STR_NOHELP);
        shopWaitAB(ST_EXIT);
        return;
    }
    shopDialogue(SHOP_STR_WHOREVIVE);
    //NES Clinic_SelectTarget.
    drawShopBox(SHOP_BOX_CMD, &x, &y);
    drawComplexString(x, y, shopStr, SHOP_PAL_BORDER);
    startShopLoop(shopCursCmd, count, ST_CL_SEL);
}

//Done before the inn's recovery and again by SaveGame.
static void innSilenceSq2() {
    musicApuWrite(0x4004, 0x30);
    musicApuWrite(0x4005, 0x7F);
    musicApuWrite(0x4006, 0);
    musicApuWrite(0x4007, 0);
}

//NES MenuFillPartyHP and MenuRecoverPartyMP.
static void innRecover() {
    for(Uint8 i = 0; i < 4; i++) {
        PLAYER_CHARACTER *c = &party[i];
        if(c->ailment == AIL_DEAD || c->ailment == AIL_STONE) {
            continue;
        }
        c->current_hp = c->max_hp;
        memcpy(c->current_mp, c->max_mp, sizeof(c->current_mp));
    }
}

//NES EnterShop_Inn @Exit. The NES "Hold Reset" text is skipped, so the shop closes now.
static void innExit() {
    eraseShopBox(SHOP_BOX_CMD);
    exitOwMenu();
}

static void uploadShopSprPalettes() {
    for(Uint8 i = 0; i < 4; i++) {
        player_pal_0[i] = nes_pal[shopSprNesPal[i]];
        player_pal_1[i] = nes_pal[shopSprNesPal[4 + i]];
    }
    vidSetPalette(VID_SPR, 16, 16, player_pal_0);
    vidSetPalette(VID_SPR, 32, 16, player_pal_1);
}

//NES DimBatSprPalettes. Returns how many colors weren't black yet.
static Uint8 dimShopSprPalettes() {
    Uint8 count = 0;

    for(Uint8 i = 1; i < 8; i++) {
        if(shopSprNesPal[i] == 0x0F) {
            continue;
        }
        shopSprNesPal[i] = (shopSprNesPal[i] < 0x10) ? 0x0F : shopSprNesPal[i] - 0x10;
        count++;
    }
    return count;
}

//NES BrightenBatSprPalettes. Returns how many colors weren't restored yet.
static Uint8 brightenShopSprPalettes() {
    Uint8 count = 0;

    for(Uint8 i = 1; i < 8; i++) {
        if(shopSprNesPal[i] == shopSprBackup[i]) {
            continue;
        }
        if(shopSprNesPal[i] == 0x0F) {
            shopSprNesPal[i] = shopSprBackup[i] & 0x0F;
        } else {
            shopSprNesPal[i] += 0x10;
        }
        count++;
    }
    return count;
}

//Runs the step the finished loop, wait or fade was for. b is the NES C flag from the shop loop.
static void shopContinue(Uint8 b) {
    switch(shopStep) {
        case ST_EQ_CMD:
            if(b || shopCursor == 2) {
                exitOwMenu();
            } else if(shopCursor == 0) {
                loadShopInventory();
                shopDialogue(SHOP_STR_WHATWOULD);
                shopSelectList(FALSE, ST_EQ_BUY_ITEM);
            } else {
                shopDialogue(SHOP_STR_WHOSELLS);
                shopCharNames(ST_EQ_SELL_CHAR);
            }
            break;
        case ST_EQ_BUY_ITEM:
            if(b) {
                equipLoop();
                break;
            }
            shopBuyConfirm();
            shopCmdLoop(SHOP_STR_YESNO, 2, ST_EQ_BUY_YN);
            break;
        case ST_EQ_BUY_YN:
            if(b || shopCursor != 0) {
                equipCancel();
            } else if(partyGold < shopCurPrice) {
                shopDialogue(SHOP_STR_CANTAFFORD);
                equipLoop();
            } else {
                shopDialogue(SHOP_STR_WHOGETS);
                shopCharNames(ST_EQ_BUY_CHAR);
            }
            break;
        case ST_EQ_BUY_CHAR:
            if(b) {
                equipLoop();
                break;
            }
            shopMember = shopCursor;
            if(!equipGiveItem()) {
                shopDialogue(SHOP_STR_NOROOM);
            } else {
                shopPay(shopCurPrice);
                shopDialogue(SHOP_STR_THANKYOU);
            }
            equipLoop();
            break;
        case ST_EQ_SELL_CHAR:
            if(b) {
                equipLoop();
                break;
            }
            shopMember = shopCursor;
            if(equipBuildSellBox() == 0) {
                shopDialogue(SHOP_STR_NOSELL);
                equipLoop();
            } else {
                shopSelectList(FALSE, ST_EQ_SELL_ITEM);
            }
            break;
        case ST_EQ_SELL_ITEM:
            if(b) {
                equipLoop();
                break;
            }
            shopSellConfirm();
            shopCmdLoop(SHOP_STR_YESNO, 2, ST_EQ_SELL_YN);
            break;
        case ST_EQ_SELL_YN:
            if(b || shopCursor != 0) {
                equipCancel();
                break;
            }
            removeEquipment(shopType == SHOP_ARMOR, shopMember, shopSlot);
            dlgAddGold(shopCurPrice);
            drawShopGoldBox();
            equipLoop();
            break;
        case ST_IT_CMD:
            if(b || shopCursor != 0) {
                exitOwMenu();
                break;
            }
            shopDialogue(SHOP_STR_WHATWOULD);
            shopSelectList(FALSE, ST_IT_ITEM);
            break;
        case ST_IT_ITEM:
            if(b) {
                itemLoop();
                break;
            }
            shopBuyConfirm();
            shopCmdLoop(SHOP_STR_YESNO, 2, ST_IT_YN);
            break;
        case ST_IT_YN:
            if(b || shopCursor != 0) {
                shopDialogue(SHOP_STR_TOOBAD);
            } else if(partyGold < shopCurPrice) {
                shopDialogue(SHOP_STR_CANTAFFORD);
            } else if(items[shopCurItem] >= 99) {
                shopDialogue(SHOP_STR_NOROOM);
            } else {
                items[shopCurItem]++;
                shopPay(shopCurPrice);
                shopDialogue(SHOP_STR_THANKYOU);
            }
            itemLoop();
            break;
        case ST_MG_CHAR:
            if(b) {
                exitOwMenu();
                break;
            }
            shopMember = shopCursor;
            shopSelectList(TRUE, ST_MG_SPELL);
            break;
        case ST_MG_SPELL:
            if(b) {
                magicLoop();
                break;
            }
            shopCurItem = shopItemBox[shopCursor];
            if(!magicAssertLearn()) {
                magicLoop();
                break;
            }
            shopBuyConfirm();
            shopCmdLoop(SHOP_STR_YESNO, 2, ST_MG_YN);
            break;
        case ST_MG_YN:
            if(b || shopCursor != 0) {
                shopDialogue(SHOP_STR_TOOBAD);
                magicLoop();
            } else if(partyGold < shopCurPrice) {
                shopDialogue(SHOP_STR_CANTAFFORD);
                magicLoop();
            } else {
                Uint8 magicId = shopCurItem - SHOP_SPELL_START;
                shopPay(shopCurPrice);
                party[shopMember].magic[magicId >> 3][shopSlot] = (magicId & 0x07) + 1;
                magicStart();
            }
            break;
        case ST_CL_START:
            clinicStart();
            break;
        case ST_CL_SEL:
            if(b) {
                exitOwMenu();
                break;
            }
            shopCurItem = shopCursor;
            innClinicConfirm();
            shopCmdLoop(SHOP_STR_YESNO, 2, ST_CL_YN);
            break;
        case ST_CL_YN:
            if(b || shopCursor != 0) {
                clinicStart();
            } else if(innClinicCanAfford()) {
                PLAYER_CHARACTER *c = &party[shopDead[shopCurItem]];
                c->ailment = 0;
                c->current_hp = 1;
                shopDialogue(SHOP_STR_REVIVED);
                eraseShopBox(SHOP_BOX_CMD);
                shopWaitAB(ST_CL_START);
            }
            break;
        case ST_INN_STAY:
            if(b || shopCursor != 0) {
                innExit();
                break;
            }
            innClinicConfirm();
            shopCmdLoop(SHOP_STR_YESNO, 2, ST_INN_YN);
            break;
        case ST_INN_YN:
            if(b || shopCursor != 0) {
                innExit();
            } else if(innClinicCanAfford()) {
                innSilenceSq2();
                innRecover();
                //NES SaveGame's jingle. The save and its "Don't forget" text are skipped, so the box is left empty.
                music_track = MUSIC_SAVE;
                innSilenceSq2();
                Uint8 x, y;
                drawShopBox(SHOP_BOX_DIALOGUE, &x, &y);
                eraseShopBox(SHOP_BOX_CMD);
                memcpy(shopSprBackup, shopSprNesPal, sizeof(shopSprBackup));
                shopStep = ST_INN_FADED;
                shopMode = SHOP_MODE_FADE_OUT;
            }
            break;
        case ST_INN_FADED:
            //The NES doesn't read the pad during the fade.
            shopJoyA = 0;
            shopJoyB = 0;
            shopStep = ST_INN_DONE;
            shopMode = SHOP_MODE_JINGLE;
            break;
        case ST_INN_DONE:
            shopStep = ST_EXIT;
            shopMode = SHOP_MODE_FADE_IN;
            break;
        case ST_EXIT:
        default:
            exitOwMenu();
            break;
    }
}

//NES LoadShopCHRPal and DrawShop.
void setupShopScreen() {
    Uint8 *image = (Uint8 *)LUT_SHOPKEEP_IMAGE;
    Uint8 x, y;

    shopType = ((Uint8 *)LUT_SHOP_TYPES)[shop_id] & 0x07;
    loadShopInventory();

    convertNesTilesToSatTiles((Uint8 *)SHOP_CHR, (Uint8 *)LWRAM_BUFFER, 0x1000);
    setBoxPalette(menu_pal_0, boarderPalette_blue, MSX_BOX_MENU);
    for(Uint8 i = 0; i < 4; i++) {
        menu_pal_1[i] = nes_pal[((Uint8 *)LUT_SHOP_PALETTES)[i]];
        menu_pal_2[i] = nes_pal[((Uint8 *)LUT_SHOP_PALETTES)[4 + i]];
        menu_pal_3[i] = nes_pal[((Uint8 *)BATTLE_BACKDROP_PALETTE_OFFSET)[SHOP_BACKDROP_PAL + (shopType * 4) + i]];
    }
    if(MSX_ON) {
        //The MSX title and gold boxes look like the other boxes.
        msxBoxPalette(MSX_BOX_MENU, menu_pal_1);
        msxBoxPalette(MSX_BOX_MENU, menu_pal_2);
        //SHOPBG has 16 tiles per shop type, the first 2 unused.
        const Uint32 *pal = msxPalette(MSX_ID_SHOP_PAL);
        for(Uint8 i = 0; i < 16; i++) {
            menu_pal_3[i] = pal[i];
        }
        for(Uint8 k = 1; k < SHOPKEEP_TILES; k++) {
            memcpy((Uint8 *)LWRAM_BUFFER + (k + shopType * SHOPKEEP_TILES) * 32,
                msxData(MSX_ID_SHOP_TILES) + (shopType * 16 + k + 2) * 32, 32);
        }
    }

    //NES DrawImageRect: zero tiles are drawn without the additive.
    for(Uint8 row = 0; row < SHOPKEEP_SIZE; row++) {
        for(Uint8 col = 0; col < SHOPKEEP_SIZE; col++) {
            Uint8 tile = *image++;
            if(tile != 0) {
                tile += shopType * SHOPKEEP_TILES;
            }
            drawPlane[SHOPKEEP_Y + row][SHOPKEEP_X + col + CENTER_ADJUSTMENT] = (SHOP_PAL_KEEPER << 12) | tile;
        }
    }

    drawShopBox(SHOP_BOX_TITLE, &x, &y);
    drawComplexString(x, y, getShopString(shopType), SHOP_PAL_TITLE);
    drawShopGoldBox();

    //NES EnterShop entry points.
    shopMode = SHOP_MODE_NONE;
    switch(shopType) {
        case SHOP_WEAPON:
        case SHOP_ARMOR:
            shopDialogue(SHOP_STR_WELCOME);
            equipLoop();
            break;
        case SHOP_WHITE_MAGIC:
        case SHOP_BLACK_MAGIC:
            magicStart();
            break;
        case SHOP_CLINIC:
            clinicStart();
            break;
        case SHOP_INN:
            drawShopBox(SHOP_BOX_DIALOGUE, &x, &y);
            drawComplexString(x, y, innStayStr, SHOP_PAL_BORDER);
            shopCmdLoop(SHOP_STR_YESNO, 2, ST_INN_STAY);
            break;
        default:
            shopDialogue(SHOP_STR_WELCOME);
            itemLoop();
            break;
    }
}

void initShop() {
    for(Uint8 i = 0; i < 4; i++) {
        sprite_make(FRAME_BATTLE_START, 0, 0, (Sint32) CHARACTER_PRIORITY, &shopSprites[i]);
    }
    memcpy(shopSprNesPal, (Uint8 *)BATTLE_CHAR_PALETTE_TABLE, sizeof(shopSprNesPal));
    shopPadPrev = PadData1;
    shopSfxPrevDir = 0;
    if(PadData1 & PAD_U) shopSfxPrevDir |= 0x08;
    if(PadData1 & PAD_D) shopSfxPrevDir |= 0x04;
    if(PadData1 & PAD_L) shopSfxPrevDir |= 0x02;
    if(PadData1 & PAD_R) shopSfxPrevDir |= 0x01;
    shopJoyA = 0;
    shopJoyB = 0;
}

//NES ShopFrame button handling, then the current loop, wait or fade.
void doShop() {
    PadBits pressed = PadData1 & ~shopPadPrev;
    Uint8 dir = 0;

    shopPadPrev = PadData1;
    if(pressed & (PAD_A | PAD_C)) {
        shopJoyA++;
    }
    if(pressed & PAD_B) {
        shopJoyB++;
    }
    if(PadData1 & PAD_U) dir |= 0x08;
    if(PadData1 & PAD_D) dir |= 0x04;

    //NES _ShopFrame_CheckBtns sounds. Only ShopFrame reads the pad.
    if(shopMode == SHOP_MODE_LOOP || shopMode == SHOP_MODE_WAIT) {
        Uint8 sfxDir = dir;
        if(PadData1 & PAD_L) sfxDir |= 0x02;
        if(PadData1 & PAD_R) sfxDir |= 0x01;
        if(shopJoyA || shopJoyB) {
            sfxMenuSel();
        } else if(sfxDir != 0 && sfxDir != shopSfxPrevDir) {
            sfxMenuMove();
        }
        shopSfxPrevDir = sfxDir;
    }

    switch(shopMode) {
        case SHOP_MODE_LOOP:
            if(shopJoyB || shopJoyA) {
                Uint8 b = (shopJoyB != 0);
                shopJoyA = 0;
                shopJoyB = 0;
                shopMode = SHOP_MODE_NONE;
                shopContinue(b);
            } else if(dir != shopPrevDir) {
                shopPrevDir = dir;
                if(dir == 0x08) {
                    shopCursor = (shopCursor == 0) ? shopCursorMax - 1 : shopCursor - 1;
                } else if(dir != 0) {
                    shopCursor = (shopCursor + 1 >= shopCursorMax) ? 0 : shopCursor + 1;
                }
            }
            break;
        case SHOP_MODE_WAIT:
            if(shopJoyA || shopJoyB) {
                shopMode = SHOP_MODE_NONE;
                shopContinue(FALSE);
            }
            break;
        case SHOP_MODE_JINGLE:
            //Faded out until the save jingle ends or A or B is pressed.
            if(shopJoyA || shopJoyB || music_track == MUSIC_SILENCED) {
                shopJoyA = 0;
                shopJoyB = 0;
                shopMode = SHOP_MODE_NONE;
                shopContinue(FALSE);
            }
            break;
        case SHOP_MODE_FADE_OUT:
        case SHOP_MODE_FADE_IN:
            //NES FadeOut/FadeInBatSprPalettes: one shade every 8 frames, until a pass changes nothing.
            if((nesFrameCounter & 0x07) == 0) {
                Uint8 left = (shopMode == SHOP_MODE_FADE_OUT) ? dimShopSprPalettes() : brightenShopSprPalettes();
                uploadShopSprPalettes();
                if(left == 0) {
                    shopMode = SHOP_MODE_NONE;
                    shopContinue(FALSE);
                }
            }
            break;
        default:
            break;
    }
}

void shop_draw() {
    for(Uint8 i = 0; i < 4; i++) {
        drawOBSprite(&shopSprites[i], i, shopSpritePos[i][0], shopSpritePos[i][1]);
    }
    if(shopMode == SHOP_MODE_LOOP) {
        cursor.xPos = FIXED(shopCursLut[shopCursor][0] + 32);
        cursor.yPos = FIXED(shopCursLut[shopCursor][1]);
        player_cursor_draw();
    }
}
