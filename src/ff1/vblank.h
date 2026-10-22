/*******************************************************************
 *
 *                      V-Blank Routines
 *
 * FF1's V-blank in and out work. V-blank in runs the audio frame and
 * VRAM writes. V-blank out reads the pad and runs one frame of the
 * current state's game logic.
 *
 *
*******************************************************************/
#if    !defined( __V_BLANK_H )
    #define    __V_BLANK_H

    #include    "pad.h"

    //Cursor and value repeat limits. Set when a held button moves the cursor or changes a value,
    //cleared 8 frames later.
    Uint8 canMoveCursor;
    Uint8 canUpdateValue;
    //Counts 0 to 60 in V-blank in, then wraps.
    Uint8 frameCount;
    Uint16 nesFrameCounter; //NES framecounter, incremented once per frame outside of battle.
    //Frame counters for canUpdateValue and canMoveCursor.
    int frame_count_a;
    int frame_count_b;

    //Frames since the last message finished drawing, for the battle respond rate.
    Uint8 respondRateCounter;

    /****************************************
    *            declare functions            *
    ****************************************/
    /**
     * @brief Resets the frame counters, starts the pad and installs the
     * V-blank routines.
     */
    void    SetVblank( void );
    
#endif
