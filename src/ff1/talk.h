/*******************************************************************
 *
 *                      Talking To Map Objects
 *
 * The NES TalkToObject routines: what each townsperson or other map
 * object says and does when the party talks to them, including
 * event flags, item hand-outs and battles.
 *
 *
*******************************************************************/
#ifndef TALK_H
#define TALK_H

#include "types.h"

//NES game_flags bits.
#define GMFLG_OBJVISIBLE (0x01)
#define GMFLG_EVENT (0x02)

Uint8 btlformation; //NES btlformation, set by talk battles.
Uint8 dlgflg_reentermap; //NES dlgflg_reentermap, reload the map once the box closes.

/**
 * @brief Runs the talk routine of a map object.
 *
 * @param index Map object slot (mapobj index) being talked to.
 * @param tileprop Pointer to the tile properties. A routine may set it to
 * start a battle or teleport.
 * @return The dialogue ID to show.
 */
Uint8 smTalkToObject(Uint8 index, Uint8 *tileprop);

#endif
