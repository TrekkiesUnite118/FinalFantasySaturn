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

#include    <machine.h>
#include    <sega_mth.h>
#include    <sega_spr.h>
#include    <sega_scl.h>
#include    <sega_xpt.h>
#include    <sega_int.h>
#include    "per_x.h"
#include    "scroll.h"
#include    "vblank.h"
#include    "menu.h"
#include    "player.h"
#include    "battle.h"
#include    "cd.h"

volatile trigger_t    PadData1  = 0x0000;
volatile trigger_t    PadData1E = 0x0000;
volatile trigger_t    PadData2  = 0x0000;
volatile trigger_t    PadData2E = 0x0000;

SysPort    *__port = NULL;
void UsrVblankIn(void);
void UsrVblankOut(void);

frame_count_a = 0;
frame_count_b = 0;

void SetVblank(void) {
    canUpdateValue = 0;
    canMoveCursor = 0;
    respondRateCounter = 0;
    frameCount = 0;
    __port = PER_OpenPort();
    
    /* V-Blank���荞�݃��[�`���̓o�^ */
    INT_ChgMsk(INT_MSK_NULL,INT_MSK_VBLK_IN | INT_MSK_VBLK_OUT);
    INT_SetScuFunc(INT_SCU_VBLK_IN,UsrVblankIn);
    INT_SetScuFunc(INT_SCU_VBLK_OUT,UsrVblankOut);
    INT_ChgMsk(INT_MSK_VBLK_IN | INT_MSK_VBLK_OUT,INT_MSK_NULL);
}


void UsrVblankIn(void) {
    
    if(frameCount == 60) {
        frameCount = 0;
    } else {
        frameCount++;
    }
    respondRateCounter++;
    SCL_VblankStart();
}

void UsrVblankOut(void) {
    SCL_VblankEnd();
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
    
    if( __port != NULL ){
        const SysDevice    *device;
        PER_GetPort( __port );
        
        if(( device = PER_GetDeviceR( &__port[0], 0 )) != NULL ){
            trigger_t    prev = PadData1;
            trigger_t    current = PER_GetTrigger( device );
            
            PadData1  = current;
            PadData1E = PER_GetPressEdge( prev, current );
        }
        else{
            PadData1 = PadData1E = 0;
        }
        
        if(( device = PER_GetDeviceR( &__port[1], 0 )) != NULL ){
            trigger_t    prev = PadData2;
            trigger_t    current = PER_GetTrigger( device );
            
            PadData2  = current;
            PadData2E = PER_GetPressEdge( prev, current );
        }
        else{
            PadData2 = PadData2E = 0;
        }
    }

    player_input();

    if(state == PRELUDE) {
        preludeAnimate();
    }

    if(state != OW_STATE && state != TO_OW_STATE) {
        update_static();
    }

    if(state == BAT_MENU_STATE) {
        
        doBattleMenu();
    } else if(state == BAT_EXECUTE_STATE) {
        doBattleExec();
    } else if(state == BAT_VICTORY_STATE) {
        
        party_animate_battle_cheer();
    }
}
