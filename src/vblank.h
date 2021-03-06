#if    !defined( __V_BLANK_H )
    #define    __V_BLANK_H
    
    /****************************************
    *            include files                *
    ****************************************/
    #include    "per_x.h"
    
    /****************************************
    *            define constants            *
    ****************************************/
    #define PAD_U    TRG_UP
    #define PAD_D    TRG_DOWN
    #define PAD_R    TRG_RIGHT
    #define PAD_L    TRG_LEFT
    #define PAD_A    TRG_A
    #define PAD_B    TRG_B
    #define PAD_C    TRG_C
    #define PAD_S    TRG_START
    #define PAD_X    TRG_X
    #define PAD_Y    TRG_Y
    #define PAD_Z    TRG_Z
    #define PAD_RB    TRG_R
    #define PAD_LB    TRG_L
    
    
    /****************************************
    *        declare external objects        *
    ****************************************/
    extern volatile trigger_t    PadData1;
    extern volatile trigger_t    PadData1E;
    extern volatile trigger_t    PadData2;
    extern volatile trigger_t    PadData2E;

    
    Uint8 canMoveCursor;
    Uint8 canUpdateValue;
    int frame_count_a;
    int frame_count_b;
    
    /****************************************
    *            declare functions            *
    ****************************************/
    void    SetVblank( void );
    
#endif
