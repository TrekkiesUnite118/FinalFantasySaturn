/*******************************************************************
 *
 *                      Control Pads
 *
 * Reads the digital pads on ports 1 and 2 with SBL's peripheral
 * library.
 *
*******************************************************************/
#ifndef SAT_PAD_H
#define SAT_PAD_H

#include "types.h"

//Saturn digital pad bits (SBL PER_DGT_*).
#define PAD_U    (1 << 12)
#define PAD_D    (1 << 13)
#define PAD_R    (1 << 15)
#define PAD_L    (1 << 14)
#define PAD_A    (1 << 10)
#define PAD_B    (1 << 8)
#define PAD_C    (1 << 9)
#define PAD_S    (1 << 11)
#define PAD_X    (1 << 6)
#define PAD_Y    (1 << 5)
#define PAD_Z    (1 << 4)
#define PAD_RB   (1 << 7)
#define PAD_LB   (1 << 3)

typedef Uint16 PadBits;

//Held buttons and buttons newly pressed this frame, for ports 1 and 2.
extern volatile Uint16 PadData1;
extern volatile Uint16 PadData1E;
extern volatile Uint16 PadData2;
extern volatile Uint16 PadData2E;

/**
 * @brief Opens the pad ports.
 */
void padInit(void);

/**
 * @brief Reads both ports. Call once per frame from V-blank out.
 *
 * A port with no pad reads as no buttons.
 */
void padRead(void);

#endif
