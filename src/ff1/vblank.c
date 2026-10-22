/*----------------------------------------------------------------------------
 *  V_Blank.c -- V-Blank���荞�ݏ��������[�`���T���v��
 *  Copyright(c) 1994 SEGA
 *  Written by K.M on 1994-05-16 Ver.1.00
 *  Updated by K.M on 1994-09-21 Ver.1.00
 *
 *  UsrVblankStart()    �FV-Blank�J�n���荞�ݏ����T���v��
 *  UsrVblankEnd()    �FV-Blank�I�����荞�ݏ����T���v��
 *
 *----------------------------------------------------------------------------
 */

#include    "types.h"
#include    "video.h"
#include    "sys.h"
#include    "rbgscroll.h"
#include    "static.h"
#include    "vblank.h"
#include    "menu.h"
#include    "player.h"
#include    "battle.h"
#include    "cd.h"
#include    "minimap.h"
#include    "mainmenu.h"
#include    "stdmap.h"
#include    "shop.h"
#include    "wipe.h"
#include    "save.h"
#include    "nesaudio.h"
#include    "music.h"
#include    "story.h"
#include    "minigame.h"
#include    "bugfix.h"

static void UsrVblankIn(void);
static void UsrVblankOutFirst(void);
static void UsrVblankOut(void);

int frame_count_a = 0;
int frame_count_b = 0;

void SetVblank(void) {
    canUpdateValue = 0;
    canMoveCursor = 0;
    respondRateCounter = 0;
    frameCount = 0;
    padInit();
    sysSetVblank(UsrVblankIn, UsrVblankOutFirst, UsrVblankOut);
}


static void UsrVblankIn(void) {
    nesaudio_vblank_in();
    
    if(frameCount == 60) {
        frameCount = 0;
    } else {
        frameCount++;
    }
    //The NES respond delay starts after its boxes finish drawing.
    if(!msgBusy()) {
        respondRateCounter++;
    }
    //Chaos dissolve tile row erase. VRAM A1 has no CPU access cycles during display.
    if(chaosEraseAddr != 0) {
        *chaosEraseAddr = 0;
        chaosEraseAddr = 0;
    }
    if(state == MINIMAP) {
        minimap_vblank_in();
    } else if(state == STORY) {
        story_vblank_in();
    }
    //Back screen color table entry set up by static_init.
    if(battleBackColorPending) {
        *(volatile Uint16 *)(VID_VRAM_A0 + 0x80000 - 2) = battleBackColor;
        battleBackColorPending = 0;
    }
    sm_vblank_in();
}

//The NES framecounter isn't touched by the battle transition or battle code.
static Uint8 isBattleState(Uint8 s) {
    switch(s) {
        case BAT_TRANSITION:
        case TO_BAT_STATE:
        case BAT_STATE:
        case TO_BAT_MENU_STATE:
        case BAT_MENU_STATE:
        case TO_BAT_EXECUTE_STATE:
        case BAT_EXECUTE_STATE:
        case TO_BAT_VICTORY_STATE:
        case BAT_VICTORY_STATE:
        case TO_BAT_RUN_AWAY:
        case BAT_RUN_AWAY:
        case TO_GAME_OVER:
        case GAME_OVER:
        case BAT_FADE_IN:
        case BAT_DRAW_STATUS:
        case BAT_FADE_OUT:
        case BAT_INIT_MSG:
        case BAT_CHAOS_EXIT:
            return TRUE;
        default:
            return FALSE;
    }
}

static Uint16 (*savedPlane)[64];

//Runs before SBL's V-blank-out work, so the haze table lines up with the display's first H-blank.
static void UsrVblankOutFirst(void) {
    //The main loop may be building a message block in msgPlane.
    savedPlane = drawPlane;
    drawPlane = menuPlane;

    scroll_vblank_out();
}

static void UsrVblankOut(void) {
    //The NES game waits inside PlaySFX_Error.
    if(sfxErrorFrames) {
        drawPlane = savedPlane;
        return;
    }
    //The minigame counts the NES framecounter's low byte itself.
    if(!isBattleState(state) && state != TO_MINIGAME && state != MINIGAME) {
        nesFrameCounter++;
    }
    frame_count_a++;
    frame_count_b++;
    if(frame_count_a == 8) {
        canUpdateValue = 0;
        frame_count_a = 0;
    }
     if(frame_count_b == 8) {
        canMoveCursor = 0;
        frame_count_b = 0;
    }

    padRead();

    player_input();

    if(state == PRELUDE) {
        preludeAnimate();
    }

    if(state != OW_STATE && state != TO_OW_STATE && state != BAT_TRANSITION && state != OW_PAL_CYCLE && state != TO_SM_STATE && state != SM_STATE && state != WIPE && state != SM_PAL_CYCLE) {
        msgAnimate();
        update_static();
    }

    if(state == BAT_MENU_STATE) {
        
        doBattleMenu();
    } else if(state == BAT_EXECUTE_STATE) {
        doBattleExec();
    } else if(state == BAT_VICTORY_STATE || state == BAT_RUN_AWAY || state == GAME_OVER || state == BAT_CHAOS_EXIT) {
        doBattleEnd();
    } else if(state == BAT_FADE_IN || state == BAT_FADE_OUT) {
        doBattleFade();
    } else if(state == BAT_DRAW_STATUS) {
        doBattleDrawStatus();
    } else if(state == BAT_TRANSITION) {
        doBattleTransition();
    } else if(state == OW_PAL_CYCLE) {
        doOwPalCycle();
    } else if(state == LINEUP_MENU) {
        doLineupMenu();
    } else if(state == MINIMAP) {
        doMinimap();
    } else if(state == MAIN_MENU) {
        doMainMenu();
    } else if(state == SAVE_MENU) {
        doSaveMenu();
    } else if(state == STATUS_MENU) {
        doStatusMenu();
    } else if(state == BUGFIX_MENU) {
        doBugFixMenu();
    } else if(state == MAGIC_MENU) {
        doMagicMenu();
    } else if(state == EQUIP_MENU) {
        doEquipMenu();
    } else if(state == ITEM_MENU) {
        doItemMenu();
    } else if(state == SM_STATE) {
        doStandardMap();
    } else if(state == SHOP) {
        doShop();
    } else if(state == WIPE) {
        doWipe();
    } else if(state == SM_PAL_CYCLE) {
        doSmPalCycle();
    } else if(state == STORY) {
        doStory();
    } else if(state == MINIGAME) {
        doMinigame();
    } else if(state == BOOT_MSG) {
        doBootMessage();
    }

    drawPlane = savedPlane;
}
