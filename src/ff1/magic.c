#include "types.h"
#include "battle.h"
#include "player.h"
#include "magic.h"
#include "bugfix.h"

#define MAGIC_BATTLE_MSG_LUT (0x002304D0)
#define ITEM_NAME_PTR_TABLE (0x0022B710)
#define ITEM_NAME_BANK_ADD (0x00220010)
#define ENEMY_ATTACK_NAME_PTR_TABLE (0x0022B610)
#define MAGIC_ITEM_START (0xB0)

#define CATEGORY_UNDEAD (0x08)

//NES ShowAltBattleMessage LUT, ALTBTLMSG to BTLMSG.
static const Uint8 altBattleMsgs[22] = {
    BTLMSG_RUNAWAY, BTLMSG_PARALYZED_B, BTLMSG_SLEEPING, BTLMSG_SILENCED,
    BTLMSG_INEFFECTIVE, BTLMSG_CONFUSED, BTLMSG_SILENCED, BTLMSG_ASLEEP,
    BTLMSG_PARALYZED_A, BTLMSG_DARKNESS, BTLMSG_POISONED, BTLMSG_BROKENTOPIECES,
    BTLMSG_TERMINATED, BTLMSG_CURED, BTLMSG_BREAKSILENCE, BTLMSG_WOKEUP,
    BTLMSG_CURED, BTLMSG_SIGHTRECOVERED, BTLMSG_NEUTRALIZED, BTLMSG_INEFFECTIVENOW,
    BTLMSG_SLAIN, BTLMSG_NOTHINGHAPPENS
};

typedef struct {
    Uint8 isPlayer;
    Uint16 hp;
    Uint16 hpMax;
    Uint8 numHitsMult;
    Uint8 ailments;
    Uint8 category;
    Uint8 magDef;
    Uint8 elemWeak;
    Uint8 elemResist;
    Uint8 morale;
    Uint8 absorb;
    Uint8 evade;
    Uint8 strength;
    Uint8 luck;
} MAGIC_DEFENDER;

static Uint8 spellHitRate;
static Uint8 spellEffectivity;
static Uint8 spellElement;
static Uint8 spellConnected;

//NES math buffer entries. basedamage is not reset between spells.
static Uint16 hitChance;
static Uint16 baseDamage;
static Uint16 magRandHit;

//Intelligence fix: the Dawn of Souls damage formula for spells the party casts.
#define INT_DAMAGE_CAP (9999)
static const Uint8 intMultBreaks[] = {20, 30, 40, 50, 66, 82, 98};
static Uint8 casterUsesInt;
static Uint8 casterIntel;
static Uint8 casterLuck;
static Uint8 curSpellId;

//Dawn of Souls healing tier (the spell's level in its line) by spell ID, 0 for spells without one.
static Uint8 intHealTier(Uint8 spellId) {
    switch(spellId) {
        case 0x00: //CURE
        case 0x13: //HEAL
            return 1;
        case 0x10: //CUR2
        case 0x23: //HEL2
            return 2;
        case 0x20: //CUR3
        case 0x33: //HEL3
            return 3;
        default:
            return 0;
    }
}

//(Spell power + INT) * tier. The target's rand[0, Luck / 2] is added per target.
Uint16 intHealAmount(Uint8 spellId, Uint8 intel) {
    Uint8 tier = intHealTier(spellId);

    if(intel > 99) {
        intel = 99;
    }
    return (getMagicData(spellId)[MAGDATA_EFFECTIVITY] + intel) * tier;
}

Uint8 *getMagicData(Uint8 spellId) {
    return (Uint8*) MAGIC_LUT_LOCATION + (spellId * 8);
}

Uint8 *getItemName(Uint8 itemId) {
    Uint8 *ptr = (Uint8*) ITEM_NAME_PTR_TABLE + (itemId * 2);
    Uint16 nesPtr = ptr[0] | (ptr[1] << 8);
    return (Uint8*) ITEM_NAME_BANK_ADD + nesPtr;
}

//4 character name, 0 terminated.
Uint8 *getSpellName(Uint8 spellId) {
    return getItemName(MAGIC_ITEM_START + spellId);
}

//Spell name, or the enemy special attack name for IDs from 0x42.
Uint8 *getAttackName(Uint8 attackId) {
    if(attackId < ENEMY_SPECIAL_ATTACK_OFFSET) {
        return getSpellName(attackId);
    }
    Uint8 *ptr = (Uint8*) ENEMY_ATTACK_NAME_PTR_TABLE + ((attackId - ENEMY_SPECIAL_ATTACK_OFFSET) * 2);
    Uint16 nesPtr = ptr[0] | (ptr[1] << 8);
    return (Uint8*) ITEM_NAME_BANK_ADD + nesPtr;
}

static void addMagicOp(Uint8 type, Uint16 value) {
    if(magicOpCount < MAGIC_MAX_OPS) {
        magicOps[magicOpCount].type = type;
        magicOps[magicOpCount].value = value;
        magicOpCount++;
    }
}

//Player defenders get "Stopped" instead of "Broken into pieces".
void addMagicAltMessage(Uint8 altMsg, Uint8 defIsPlayer) {
    Uint8 msg = altBattleMsgs[altMsg];
    if(defIsPlayer && msg == BTLMSG_BROKENTOPIECES) {
        msg = BTLMSG_STOPPED;
    }
    addMagicOp(MAGIC_OP_MSG, msg);
}

static Uint16 addCapped(Uint16 value, Uint8 add) {
    Uint32 sum = value + add;
    return sum > 0xFFFF ? 0xFFFF : sum;
}

static Uint16 subFloored(Uint16 value, Uint8 sub) {
    return value > sub ? value - sub : 0;
}

static Uint8 addCapped8(Uint8 value, Uint8 add) {
    Uint16 sum = value + add;
    return sum > 0xFF ? 0xFF : sum;
}

static void calcElemHitChance(MAGIC_DEFENDER *d) {
    hitChance = 148;
    if(spellElement & d->elemResist) {
        hitChance = 0;
    }
    if(spellElement & d->elemWeak) {
        hitChance = addCapped(hitChance, 40);
    }
}

//hit += spell hit rate - magdef, damage = rand[damage, damage*2], magRandHit = rand[0, 200].
static void prepHitAndDamage(MAGIC_DEFENDER *d) {
    hitChance = addCapped(hitChance, spellHitRate);
    hitChance = subFloored(hitChance, d->magDef);
    baseDamage = addCapped(baseDamage, rand_min_max(0, baseDamage & 0xFF));
    magRandHit = rand_min_max(0, 200);
}

static Uint8 spellRollConnects() {
    return magRandHit != 200 && hitChance >= magRandHit;
}

//A connecting roll doubles the damage.
static void applyDamage(MAGIC_DEFENDER *d) {
    if(spellRollConnects()) {
        baseDamage <<= 1;
    }
    d->hp = d->hp > baseDamage ? d->hp - baseDamage : 0;
    addMagicOp(MAGIC_OP_DMG, baseDamage);
}

void setSpellCaster(Uint8 usesInt, Uint8 intel, Uint8 luck) {
    casterUsesInt = usesInt && BUGFIX_ON(BUGFIX_INT_MAGIC);
    casterIntel = intel > 99 ? 99 : intel;
    casterLuck = luck;
}

//Spell power * INT multiplier + (INT + Luck) / 2 + rand[0, Luck / 2].
static Uint16 intBaseDamage() {
    Uint8 mult = 1;

    for(Uint8 i = 0; i < sizeof(intMultBreaks); i++) {
        if(casterIntel >= intMultBreaks[i]) {
            mult++;
        }
    }
    return spellEffectivity * mult + ((casterIntel + casterLuck) >> 1) + rand_min_max(0, casterLuck >> 1);
}

//The hit roll adds INT / 5 instead of doubling the damage.
static void applyIntDamage(MAGIC_DEFENDER *d) {
    hitChance = addCapped(hitChance, spellHitRate);
    hitChance = subFloored(hitChance, d->magDef);
    magRandHit = rand_min_max(0, 200);
    if(spellRollConnects()) {
        baseDamage += casterIntel / 5;
    }
    if(baseDamage > INT_DAMAGE_CAP) {
        baseDamage = INT_DAMAGE_CAP;
    }
    d->hp = d->hp > baseDamage ? d->hp - baseDamage : 0;
    addMagicOp(MAGIC_OP_DMG, baseDamage);
}

static void applyAilments(MAGIC_DEFENDER *d) {
    if(spellEffectivity & d->ailments) {
        addMagicAltMessage(ALTBTLMSG_INEFFECTIVE, d->isPlayer);
    }
    d->ailments |= spellEffectivity;
}

//NES BtlMag_PerformSpellEffect jump table, including its bugs.
static void doSpellEffect(MAGIC_DEFENDER *d, Uint8 effect) {
    switch(effect) {
        case 0x00:
            addMagicAltMessage(ALTBTLMSG_INEFFECTIVE, d->isPlayer);
            break;
        case 0x01: //Damage
            spellConnected = TRUE;
            hitChance = 148;
            baseDamage = casterUsesInt ? intBaseDamage() : spellEffectivity;
            if(spellElement & d->elemResist) {
                hitChance = 0;
                baseDamage >>= 1;
            }
            if(spellElement & d->elemWeak) {
                hitChance = addCapped(hitChance, 40);
                Uint32 dmg = baseDamage + (baseDamage >> 1);
                baseDamage = dmg > 0xFFFF ? 0xFFFF : dmg;
            }
            if(casterUsesInt) {
                applyIntDamage(d);
                break;
            }
            prepHitAndDamage(d);
            applyDamage(d);
            break;
        case 0x02: //Damage undead, no elemental checks.
            hitChance = 148;
            if(casterUsesInt) {
                if(!(d->category & CATEGORY_UNDEAD)) {
                    break;
                }
                baseDamage = intBaseDamage();
                spellConnected = TRUE;
                applyIntDamage(d);
                break;
            }
            baseDamage = spellEffectivity;
            prepHitAndDamage(d);
            if(!(d->category & CATEGORY_UNDEAD)) {
                baseDamage = 0;
                break;
            }
            spellConnected = TRUE;
            applyDamage(d);
            break;
        case 0x03: //Inflict ailment
            calcElemHitChance(d);
            prepHitAndDamage(d);
            if(spellRollConnects()) {
                applyAilments(d);
                spellConnected = TRUE;
            }
            break;
        case 0x04: //Slow. Still connects when the multiplier can't go lower.
            calcElemHitChance(d);
            prepHitAndDamage(d);
            if(spellRollConnects()) {
                spellConnected = TRUE;
                d->numHitsMult--;
                if(d->numHitsMult & 0x80) {
                    d->numHitsMult++;
                }
            }
            break;
        case 0x05: //Lower morale
            calcElemHitChance(d);
            prepHitAndDamage(d);
            if(spellRollConnects()) {
                d->morale = d->morale >= spellEffectivity ? d->morale - spellEffectivity : 0;
                spellConnected = TRUE;
            }
            break;
        case 0x06:
        case 0x07: //Recover HP, rand[eff, eff*2] capped at 255. Nothing on a dead target.
            spellConnected = TRUE;
            if(casterUsesInt && intHealTier(curSpellId) && !(d->ailments & AIL_DEAD)) {
                Uint32 hp = d->hp + intHealAmount(curSpellId, casterIntel) + rand_min_max(0, d->luck >> 1);
                d->hp = hp >= d->hpMax ? d->hpMax : hp;
            } else if(!(d->ailments & AIL_DEAD)) {
                Uint8 heal = addCapped8(rand_min_max(0, spellEffectivity), spellEffectivity);
                d->hp = addCapped(d->hp, heal);
                if(d->hp >= d->hpMax) {
                    d->hp = d->hpMax;
                }
            }
            break;
        case 0x08: //Cure ailment
            if(d->ailments & spellEffectivity) {
                d->ailments &= ~spellEffectivity;
                spellConnected = TRUE;
            }
            break;
        case 0x09: //Absorb up
            d->absorb = addCapped8(d->absorb, spellEffectivity);
            spellConnected = TRUE;
            break;
        case 0x0A: //Elemental resist
            d->elemResist |= spellEffectivity;
            spellConnected = TRUE;
            break;
        case 0x0B: //Attack up
        case 0x0D: //Attack up 2. Its hit rate bonus goes to an unused attacker variable.
            d->strength = addCapped8(d->strength, spellEffectivity);
            spellConnected = TRUE;
            break;
        case 0x0C: //Fast, has no effect past a multiplier of 2.
            spellConnected = TRUE;
            d->numHitsMult++;
            if(d->numHitsMult >= 3) {
                spellConnected = FALSE;
                d->numHitsMult = 2;
            }
            break;
        case 0x0E: //Evade down. NES bug: always misses after rolling.
            calcElemHitChance(d);
            prepHitAndDamage(d);
            if(BUGFIX_ON(BUGFIX_LOCK) && spellRollConnects()) {
                d->evade = d->evade > spellEffectivity ? d->evade - spellEffectivity : 0;
                spellConnected = TRUE;
            }
            break;
        case 0x0F: //Cure all
            d->hp = d->hpMax;
            d->ailments = 0;
            spellConnected = TRUE;
            break;
        case 0x10: //Evade up
            d->evade = addCapped8(d->evade, spellEffectivity);
            spellConnected = TRUE;
            break;
        case 0x11: //Remove resist. Weak and resistant gives 188 instead of 40.
            {
                hitChance = 148;
                if(spellElement & d->elemResist) {
                    hitChance = 0;
                }
                if(spellElement & d->elemWeak) {
                    hitChance = 188;
                }
                hitChance = addCapped(hitChance, spellHitRate);
                hitChance = subFloored(hitChance, d->magDef);
                Uint8 roll = rand_min_max(0, 200);
                if(roll != 200) {
                    magRandHit = roll;
                    if(hitChance >= magRandHit) {
                        d->elemResist = 0;
                        spellConnected = TRUE;
                    }
                }
            }
            break;
        case 0x12: //Inflict ailment 2, hits a non-resistant target with 300 HP or less.
            if(!(spellElement & d->elemResist) && d->hp <= 300) {
                applyAilments(d);
                spellConnected = TRUE;
            }
            break;
        default:
            break;
    }
}

//NES BtlMag_PrintMagicMessage.
static void addSpellMessage(MAGIC_DEFENDER *d, Uint8 spellId) {
    Uint8 *msgLut = (Uint8*) MAGIC_BATTLE_MSG_LUT;

    if(!spellConnected) {
        addMagicAltMessage(ALTBTLMSG_INEFFECTIVE, d->isPlayer);
    } else if(spellId >= ENEMY_SPECIAL_ATTACK_OFFSET || spellId == 0x41) {
        addMagicOp(MAGIC_OP_DELAY, 0);
    } else if(spellId == 0x40) {
        addMagicOp(MAGIC_OP_MSG, BTLMSG_HPUP);
    } else if(msgLut[spellId] != 0) {
        addMagicOp(MAGIC_OP_MSG, msgLut[spellId]);
    } else {
        addMagicOp(MAGIC_OP_DELAY, 0);
    }
}

//NES BtlMag_HandleAilmentChanges, bit 0 (dead) first. A death ends the messages.
static void addAilmentChangeMessages(MAGIC_DEFENDER *d, Uint8 origAilments) {
    for(Uint8 bit = 0; bit < 8; bit++) {
        Uint8 mask = 1 << bit;
        Uint8 x = 8 - bit;

        if(!((origAilments ^ d->ailments) & mask)) {
            continue;
        }
        if(d->ailments & mask) {
            if(bit == 0) {
                addMagicAltMessage(d->isPlayer ? ALTBTLMSG_SLAIN : ALTBTLMSG_TERMINATED, d->isPlayer);
                break;
            }
            addMagicAltMessage(ALTBTLMSG_CONFUSED - 1 + x, d->isPlayer);
        } else {
            addMagicAltMessage(ALTBTLMSG_CURED_1 - 1 + x, d->isPlayer);
        }
    }
}

static void performSpellEffect(MAGIC_DEFENDER *d, Uint8 spellId) {
    Uint8 *data = getMagicData(spellId);
    Uint8 origAilments = d->ailments;

    spellHitRate = data[MAGDATA_HITRATE];
    spellEffectivity = data[MAGDATA_EFFECTIVITY];
    spellElement = data[MAGDATA_ELEMENT];
    spellConnected = FALSE;
    curSpellId = spellId;

    doSpellEffect(d, data[MAGDATA_EFFECT]);
    addSpellMessage(d, spellId);

    if(d->hp == 0 || (d->hp & 0x8000)) {
        d->ailments |= AIL_DEAD;
    }
    addAilmentChangeMessages(d, origAilments);
}

//NES Battle_CastMagicOnEnemy. Elemental resist isn't saved, the NES XFER/AFIR bug.
void castSpellOnEnemy(Uint8 spellId, Uint8 enemyIndex) {
    ENEMY_DATA *enemy = &enemies[enemyIndex];
    MAGIC_DEFENDER d;

    d.isPlayer = FALSE;
    d.hp = enemy->hp;
    d.hpMax = enemy->hp_max;
    d.numHitsMult = enemy->num_hits_mult;
    d.ailments = enemy->ailment;
    d.category = enemy->category;
    d.magDef = enemy->mag_def;
    d.elemWeak = enemy->elem_weak;
    d.elemResist = enemy->elem_resist;
    d.morale = enemy->morale;
    d.absorb = enemy->rom_absorb;
    d.evade = enemy->evade;
    d.strength = enemy->strength;
    d.luck = 0;

    performSpellEffect(&d, spellId);

    enemy->hp = d.hp;
    enemy->absorb = d.absorb;
    enemy->num_hits_mult = d.numHitsMult;
    enemy->ailment = d.ailments;
    enemy->morale = d.morale;
    enemy->evade = d.evade;
    enemy->strength = d.strength;
    if(BUGFIX_ON(BUGFIX_ENEMY_RESISTS)) {
        enemy->elem_resist = d.elemResist;
    }

    //A killed or stoned enemy is removed from battle, which is 0 HP in the port.
    if(d.ailments & (AIL_DEAD | AIL_STONE)) {
        addMagicOp(MAGIC_OP_ERASE, enemyIndex);
        enemy->hp = 0;
    }
}

//NES Battle_CastMagicOnPlayer. The class is used as the category, so HARM hits Masters and
//Wizards. Strength isn't loaded or saved, so attack up does nothing on players.
void castSpellOnPlayer(Uint8 spellId, Uint8 playerIndex) {
    BATTLE_CHARACTER *ch = &battle_char_stats[playerIndex];
    MAGIC_DEFENDER d;

    d.isPlayer = TRUE;
    d.hp = ch->hp;
    d.hpMax = party[playerIndex].max_hp;
    d.numHitsMult = ch->numhitsmult;
    d.ailments = ch->ailment;
    d.category = BUGFIX_ON(BUGFIX_CLASS_UNDEAD) ? 0 : ch->char_class;
    d.magDef = ch->mag_def;
    d.elemWeak = ch->elem_weak;
    d.elemResist = ch->elem_resist;
    d.morale = 0;
    d.absorb = ch->absorb;
    d.evade = ch->evade_percent;
    d.strength = BUGFIX_ON(BUGFIX_TMPR_SABR) ? ch->damage : 0;
    d.luck = ch->luck;

    performSpellEffect(&d, spellId);

    ch->ailment = d.ailments;
    ch->mag_def = d.magDef;
    ch->hp = d.hp;
    ch->elem_resist = d.elemResist;
    ch->numhitsmult = d.numHitsMult;
    ch->absorb = d.absorb;
    ch->evade_percent = d.evade;
    if(BUGFIX_ON(BUGFIX_TMPR_SABR)) {
        ch->damage = d.strength;
    }

    //Immobilized players lose their command.
    if(d.ailments & (AIL_DEAD | AIL_STONE | AIL_STUN | AIL_SLEEP)) {
        playerActions[playerIndex].action = NO_ACTION;
    }
}
