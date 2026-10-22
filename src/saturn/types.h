/*******************************************************************
 *
 *                      Base Types
 *
 * Base types and fixed point for code outside the hardware layer,
 * without SBL headers.
 *
 * Blocks copy SBL's definitions under SBL's own guards, so either may
 * be included first.
 *
*******************************************************************/
#ifndef SAT_TYPES_H
#define SAT_TYPES_H

#ifndef SEGA_XPT_H
#define SEGA_XPT_H
typedef unsigned    char    Uint8;
typedef signed      char    Sint8;
typedef unsigned    short   Uint16;
typedef signed      short   Sint16;
typedef unsigned    long    Uint32;
typedef signed      long    Sint32;
typedef float               Float32;
typedef double              Float64;
typedef int                 Int;
typedef int                 Bool;
#ifndef NULL
#define NULL    ((void *)0)
#endif
enum BooleanLogic { FALSE = 0, TRUE = 1 };
enum BooleanSwitch { OFF = 0, ON = 1 };
enum Judgement { OK = 0, NG = -1 };
#define MAX(x, y)       ((x) > (y) ? (x) : (y))
#define MIN(x, y)       ((x) < (y) ? (x) : (y))
#define ABS(x)          ((x) < 0 ? -(x) : (x))
#endif

#ifndef SEGA_DEF_H
#define SEGA_DEF_H
#define  RGB16_COLOR(r,g,b)  (Rgb16)(((b)<<10) + ((g)<<5) + (r) + 0x8000)
#define  RGB32_COLOR(r,g,b)  (Rgb32)(((b)<<16) + ((g)<<8) + (r) + 0x80000000)
typedef  Uint16	     Rgb16;
typedef  Uint32	     Rgb32;
typedef struct  XyInt {
	 Sint16      x;
	 Sint16      y;
} XyInt;
#endif

//SEGA_MTH.H typedefs Fixed32 unguarded: in hardware-layer files include it before this header.
#ifndef SEGA_MTH_H
typedef Sint32 Fixed32;
#endif
Fixed32 MTH_IntToFixed(Sint32 x);
Sint32  MTH_FixedToInt(Fixed32 x);

//Same text as SEGA_SCL.H's FIXED, so including both is an identical redefinition.
#ifndef FIXED
#define FIXED(x)	      ((Fixed32)((x) * 65536.0))
#endif

#define fixedToInt(x) MTH_FixedToInt(x)
#define intToFixed(x) MTH_IntToFixed(x)

#endif
