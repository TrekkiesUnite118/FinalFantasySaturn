#include <string.h>
#include "types.h"

#include "graphicrefs.h"
#include "nesutils.h"
#include "memmap.h"
#include "romaddr.h"
#include "player.h"
#include "menu.h"
#include "vblank.h"
#include "music.h"
#include "bugfix.h"
#include "msxgfx.h"
#include "gfxmenu.h"

//Same look as the bug fix menu: white text when on, grey when off.
#define GM_PAL_ON (0)
#define GM_PAL_OFF (1)
#define GM_GREY (0x10)

//Layout in NES tile coords. Japanese lines need a free row above them for dakuten marks.
#define GM_BOX_X (0xFD)
#define GM_BOX_W (38)
#define GM_TEXT_X (0xFF)
//Last text column, leaving a blank column inside the box's right border.
#define GM_TEXT_END (32)
//The cursor sits 2 tiles left of the option.
#define GM_ITEM_X (1)
#define GM_TITLE_Y (1)
#define GM_TITLE_H (3)
#define GM_LIST_Y (GM_TITLE_Y + GM_TITLE_H)
//Down to the second last screen row, leaving the last one blank.
#define GM_LIST_H (25)
#define GM_ITEM_Y (GM_LIST_Y + 1 + TEXT_MARK_ROWS)
#define GM_ROW_H (TEXT_MARK_ROWS + 1)
#define GM_NAME_W (20)
#define GM_VALUE_GAP (2)
#define GM_CURSOR_X ((GM_ITEM_X - 2 + CENTER_ADJUSTMENT) * 8)

static const char gmTitle[] = "OPTIONS";
static const char gmMsxValue[] = "MSX";
#ifdef FF1_JP
static const char gmGfxName[] = "グラフィック";
static const char gmSndName[] = "サウンド";
static const char gmNesValue[] = "FC";

//The Japanese font has no A-Z, so the bridge scene's credit letters are borrowed while the menu is up.
#define GM_CAPS_CHR (BRIDGE_CHR + 0x20 * 16)
#define GM_CAPS_TILE (0x60)
#define GM_CAPS_COUNT (26)
static Uint8 gmSavedTiles[GM_CAPS_COUNT * 32];
#else
static const char gmGfxName[] = "Graphics";
static const char gmSndName[] = "Sound";
static const char gmNesValue[] = "NES";
#endif

enum {
    GM_OPT_GFX,
    GM_OPT_SND,
    GM_OPT_COUNT
};

//Sound switch for the main loop, since loading a driver can't run in the V-blank.
enum {
    GM_SND_NONE,
    GM_SND_NES,
    GM_SND_MSX
};

static PadBits gmPadPrev;
static Uint8 gmCursor;
static volatile Uint8 gmSndRequest;
static Uint8 gmSndShown;

//Upper case text: the menu font in English, the credit letters in Japanese.
static void gmDrawCaps(Uint8 x, Uint8 y, const char *str, Uint8 pal) {
#ifdef FF1_JP
    for(; *str; str++, x++) {
        Uint16 tile = *str >= 'A' && *str <= 'Z' ? GM_CAPS_TILE + (*str - 'A') : 0;
        drawPlane[y][(Uint8)(x + CENTER_ADJUSTMENT)] = (pal << 12) + tile;
    }
#else
    optionDrawText(x, y, str, strlen(str), pal);
#endif
}

static void gmDrawOption(Uint8 opt, const char *name, Uint8 msx) {
    Uint8 msxX = GM_TEXT_END + 1 - (sizeof(gmMsxValue) - 1);
    Uint8 nesX = msxX - GM_VALUE_GAP - (sizeof(gmNesValue) - 1);
    Uint8 y = GM_ITEM_Y + opt * GM_ROW_H;

    optionDrawText(GM_ITEM_X, y, name, GM_NAME_W, GM_PAL_ON);
    gmDrawCaps(nesX, y, gmNesValue, msx ? GM_PAL_OFF : GM_PAL_ON);
    gmDrawCaps(msxX, y, gmMsxValue, msx ? GM_PAL_ON : GM_PAL_OFF);
}

static void gmDrawOptions() {
    gmSndShown = gmSndRequest ? gmSndRequest == GM_SND_MSX : sndMsx;
    gmDrawOption(GM_OPT_GFX, gmGfxName, gfxMsx);
    gmDrawOption(GM_OPT_SND, gmSndName, gmSndShown);
}

static void gmPlaceCursor() {
    cursor.xPos = FIXED(GM_CURSOR_X);
    cursor.yPos = FIXED((GM_ITEM_Y + gmCursor * GM_ROW_H) * 8);
}

void setupGfxScreen() {
    setBoxPalette(menu_pal_0, boarderPalette_blue, MSX_BOX_MENU);
    setBoxPalette(menu_pal_1, boarderPalette_blue, MSX_BOX_MENU);
    menu_pal_1[3] = nes_pal[GM_GREY];

#ifdef FF1_JP
    Uint8 *tiles = (Uint8 *)LWRAM_MENU_TILES + GM_CAPS_TILE * 32;
    memcpy(gmSavedTiles, tiles, sizeof(gmSavedTiles));
    convertNesTilesToSatTiles((Uint8 *)GM_CAPS_CHR, tiles, GM_CAPS_COUNT * 16);
#endif
    drawBox(GM_BOX_X, GM_TITLE_Y, GM_BOX_W, GM_TITLE_H, GM_PAL_ON);
    gmDrawCaps(GM_TEXT_X, GM_TITLE_Y + 1, gmTitle, GM_PAL_ON);
    drawBox(GM_BOX_X, GM_LIST_Y, GM_BOX_W, GM_LIST_H, GM_PAL_ON);
    gmDrawOptions();
}

void initGfxMenu() {
    gmCursor = GM_OPT_GFX;
    gmSndRequest = GM_SND_NONE;
    player_cursor_init(FIXED(GM_CURSOR_X), FIXED(GM_ITEM_Y * 8));
    gmPadPrev = PadData1;
}

void leaveGfxMenu() {
#ifdef FF1_JP
    memcpy((Uint8 *)LWRAM_MENU_TILES + GM_CAPS_TILE * 32, gmSavedTiles, sizeof(gmSavedTiles));
#endif
}

void doGfxMenu() {
    PadBits pressed = PadData1 & ~gmPadPrev;
    Uint8 cur = gmCursor == GM_OPT_GFX ? gfxMsx : sndMsx;
    Uint8 msx = cur;
    gmPadPrev = PadData1;

    //Waits out a sound switch, then shows how it went.
    if(gmSndRequest) {
        return;
    }
    if(gmSndShown != sndMsx) {
        gmDrawOptions();
    }
    if(pressed & (PAD_U | PAD_D)) {
        gmCursor = (gmCursor + 1) % GM_OPT_COUNT;
        sfxMenuMove();
        return;
    } else if(pressed & (PAD_A | PAD_C)) {
        msx = !cur;
    } else if(pressed & PAD_L) {
        msx = FALSE;
    } else if(pressed & PAD_R) {
        msx = TRUE;
    } else if(pressed & PAD_B) {
        sfxMenuSel();
        leaveGfxMenu();
        state = TO_MAIN_MENU;
        return;
    } else {
        return;
    }
    if(gmCursor == GM_OPT_GFX) {
        sfxMenuSel();
        gfxMsx = msx && msxAvailable;
    } else {
        msx = msx && musicMsxAvailable();
        if(msx == sndMsx) {
            sfxMenuSel();
        } else {
            gmSndRequest = msx ? GM_SND_MSX : GM_SND_NES;
        }
    }
    gmDrawOptions();
}

void gfxmenu_draw() {
    Uint8 request = gmSndRequest;

    //The select sound plays in the new driver.
    if(request) {
        musicSetMsx(request == GM_SND_MSX);
        sfxMenuSel();
        gmSndRequest = GM_SND_NONE;
    }
    gmPlaceCursor();
    player_cursor_draw();
}
