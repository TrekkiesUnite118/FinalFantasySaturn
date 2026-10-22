/*******************************************************************
 *
 *                      Battle Magic
 *
 * Spell, item and attack names and the NES spell effects, for spells
 * cast on enemies or on the party.
 *
 * Each cast fills magicOps with what the battle screen shows.
 *
*******************************************************************/
#ifndef MAGIC_H
#define MAGIC_H
#include "datatypes.h"

#include "types.h"

//Byte offsets into getMagicData's 8 bytes.
#define MAGDATA_HITRATE (0)
#define MAGDATA_EFFECTIVITY (1)
#define MAGDATA_ELEMENT (2)
#define MAGDATA_TARGET (3)
#define MAGDATA_EFFECT (4)
#define MAGDATA_GRAPHIC (5)
#define MAGDATA_PALETTE (6)

#define SPELL_FIRE (0x04)
#define DRINK_HEAL (0x40)
#define DRINK_PURE (0x41)

//NES item IDs.
#define ITEM_LUTE (0x01)
#define ITEM_CROWN (0x02)
#define ITEM_CRYSTAL (0x03)
#define ITEM_HERB (0x04)
#define ITEM_MYSTIC_KEY (0x05)
#define ITEM_TNT (0x06)
#define ITEM_ADAMANT (0x07)
#define ITEM_SLAB (0x08)
#define ITEM_RUBY (0x09)
#define ITEM_ROD (0x0A)
#define ITEM_FLOATER (0x0B)
#define ITEM_CHIME (0x0C)
#define ITEM_TAIL (0x0D)
#define ITEM_CUBE (0x0E)
#define ITEM_BOTTLE (0x0F)
#define ITEM_OXYALE (0x10)
#define ITEM_ORB_START (0x12)
#define ITEM_TENT (0x16)
#define ITEM_CABIN (0x17)
#define ITEM_HOUSE (0x18)
#define ITEM_HEAL (0x19)
#define ITEM_PURE (0x1A)
#define ITEM_SOFT (0x1B)
#define ITEM_STOP (0x1C)
#define ITEM_WEAPON_START (0x1C)
#define ITEM_ARMOR_START (0x44)

//Values of MAGDATA_TARGET.
#define MAGIC_TARGET_ALL_ENEMIES (0x01)
#define MAGIC_TARGET_ONE_ENEMY (0x02)
#define MAGIC_TARGET_SELF (0x04)
#define MAGIC_TARGET_ALL_PARTY (0x08)

#define ALTBTLMSG_SILENCED_1 (0x03)
#define ALTBTLMSG_INEFFECTIVE (0x04)
#define ALTBTLMSG_CONFUSED (0x05)
#define ALTBTLMSG_TERMINATED (0x0C)
#define ALTBTLMSG_CURED_1 (0x0D)
#define ALTBTLMSG_INEFFECTIVENOW (0x13)
#define ALTBTLMSG_SLAIN (0x14)
#define ALTBTLMSG_NOTHINGHAPPENS (0x15)

enum __magicoptype__ {
    MAGIC_OP_MSG,   //Bottom message, a respond delay, then it's undrawn.
    MAGIC_OP_DMG,   //Damage box, stays until the turn's boxes are cleared.
    MAGIC_OP_DELAY,
    MAGIC_OP_ERASE  //Erase the defending enemy's graphic.
};

#define MAGIC_MAX_OPS (16)

typedef struct {
    Uint8 type;
    Uint16 value;
} MAGIC_OP;

//What a spell shows on its current target, in order.
MAGIC_OP magicOps[MAGIC_MAX_OPS];
Uint8 magicOpCount;

/**
 * @brief Gets a spell's data from the ROM.
 *
 * @param spellId Spell number.
 * @return Pointer to the spell's 8 bytes (MAGDATA_).
 */
Uint8 *getMagicData(Uint8 spellId);

/**
 * @brief Gets an item's name from the ROM.
 *
 * @param itemId Item number.
 * @return Pointer to the name.
 */
Uint8 *getItemName(Uint8 itemId);

/**
 * @brief Gets a spell's name from the ROM.
 *
 * @param spellId Spell number.
 * @return Pointer to the 4 character name, 0 terminated.
 */
Uint8 *getSpellName(Uint8 spellId);

/**
 * @brief Gets the name of a spell or an enemy special attack.
 *
 * @param attackId Spell number, or enemy special attack from $42.
 * @return Pointer to the name.
 */
Uint8 *getAttackName(Uint8 attackId);

/**
 * @brief Adds a battle message to magicOps.
 *
 * Players get "Stopped" in place of "Broken into pieces".
 *
 * @param altMsg Message (ALTBTLMSG_).
 * @param defIsPlayer TRUE if the target is a party member.
 */
void addMagicAltMessage(Uint8 altMsg, Uint8 defIsPlayer);

/**
 * @brief Sets the caster's stats for the next casts.
 *
 * @param usesInt TRUE if INT adds to the spell. Only counts when BUGFIX_INT_MAGIC is on.
 * @param intel Caster's INT, capped at 99.
 * @param luck Caster's luck.
 */
void setSpellCaster(Uint8 usesInt, Uint8 intel, Uint8 luck);

/**
 * @brief Gets a healing spell's amount with INT added.
 *
 * Ex: (spell power + INT) * the spell's tier.
 *
 * @param spellId Spell number.
 * @param intel Caster's INT, capped at 99.
 * @return Heal amount before the target's luck roll.
 */
Uint16 intHealAmount(Uint8 spellId, Uint8 intel);

/**
 * @brief Casts a spell on an enemy (NES Battle_CastMagicOnEnemy).
 *
 * Elemental resist changes are only kept when BUGFIX_ENEMY_RESISTS is on.
 *
 * @param spellId Spell number.
 * @param enemyIndex Enemy to cast on.
 */
void castSpellOnEnemy(Uint8 spellId, Uint8 enemyIndex);

/**
 * @brief Casts a spell on a party member (NES Battle_CastMagicOnPlayer).
 *
 * The class is used as the enemy category, so HARM hits Masters and
 * Wizards, unless BUGFIX_CLASS_UNDEAD is on. Strength changes only
 * count when BUGFIX_TMPR_SABR is on.
 *
 * @param spellId Spell number.
 * @param playerIndex Party member to cast on.
 */
void castSpellOnPlayer(Uint8 spellId, Uint8 playerIndex);

#endif
