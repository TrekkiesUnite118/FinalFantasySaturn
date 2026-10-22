/*******************************************************************
 *
 *                      NES Bug Fix Options
 *
 * Optional fixes for bugs in the NES game, and the menu to switch
 * them on and off (R from the main menu).
 *
 * Game code checks a fix with BUGFIX_ON(id).
 *
*******************************************************************/
#ifndef BUGFIX_H
#define BUGFIX_H

#include "datatypes.h"

//NES bug fixes the player can switch on. All off is NES behavior.
enum {
    BUGFIX_CRIT_RATE = 0,
    BUGFIX_WEAPON_EFFECTS,
    BUGFIX_MONK_ABSORB,
    BUGFIX_ENEMY_RESISTS,
    BUGFIX_TMPR_SABR,
    BUGFIX_LOCK,
    BUGFIX_CLASS_UNDEAD,
    BUGFIX_MULTI_LEVEL_UP,
    BUGFIX_AILMENT_ON_MISS,
    BUGFIX_CONFUSED_HANG,
    BUGFIX_ALLY_SPELLS,
    BUGFIX_ENEMY_ELEMENT,
    BUGFIX_ENEMY_SLEEP,
    BUGFIX_STAT_UP_REPORT,
    BUGFIX_PARTY_SORT,
    BUGFIX_TURN_SHUFFLE,
    BUGFIX_SURPRISE_TURN,
    BUGFIX_STONE_POISON,
    BUGFIX_MUTE_ITEMS,
    BUGFIX_CUR4_FIELD,
    BUGFIX_FULL_CHEST,
    BUGFIX_GOLD_CAP,
    BUGFIX_ENCOUNTER_ZONE,
    BUGFIX_RUN_CHANCE,
    BUGFIX_INT_MAGIC,
    BUGFIX_COUNT
};

#define BUGFIX_BYTES ((BUGFIX_COUNT + 7) >> 3)
//Nonzero when the fix is on.
#define BUGFIX_ON(id) (bugFixes[(id) >> 3] & (1 << ((id) & 7)))

//One bit per fix, by BUGFIX_ id.
extern Uint8 bugFixes[BUGFIX_BYTES];

/**
 * @brief Draws the bug fix menu: title, list of fixes and the description box.
 */
void setupBugFixScreen(void);

/**
 * @brief Puts the cursor on the first fix.
 */
void initBugFixMenu(void);

/**
 * @brief Runs one frame of the bug fix menu.
 *
 * A or C switches the fix under the cursor, B goes back to the main menu.
 */
void doBugFixMenu(void);

/**
 * @brief Draws the cursor.
 */
void bugfix_draw(void);

#endif
