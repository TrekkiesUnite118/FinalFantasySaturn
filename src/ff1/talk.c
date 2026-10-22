#include "types.h"

#include "player.h"
#include "magic.h"
#include "stdmap.h"
#include "mapobj.h"
#include "dialog.h"
#include "talk.h"

#define OBJID_GARLAND (0x02)
#define OBJID_PRINCESS_1 (0x03)
#define OBJID_BIKKE (0x04)
#define OBJID_ELFPRINCE (0x06)
#define OBJID_ASTOS (0x07)
#define OBJID_NERRICK (0x08)
#define OBJID_SMITH (0x09)
#define OBJID_UNNE (0x0B)
#define OBJID_VAMPIRE (0x0C)
#define OBJID_BAHAMUT (0x0E)
#define OBJID_SUBENGINEER (0x10)
#define OBJID_PRINCESS_2 (0x12)
#define OBJID_TITAN (0x14)
#define OBJID_PIRATETERR_1 (0x3F)
#define OBJID_PIRATETERR_2 (0x40)
#define OBJID_PIRATETERR_3 (0x41)
#define OBJID_BLACKORB (0xCA)

#define BTL_VAMPIRE (0x7C)
#define BTL_ASTOS (0x7D)
#define BTL_BIKKE (0x7E)
#define BTL_GARLAND (0x7F)

#define NORMTELE_SAVEDPRINCESS (0x3F)
#define WPNID_XCALBUR (0x27)

typedef Uint8 (*TALK_FUNC)(void);

static Uint8 *talk; //NES tmp-tmp+3, the object's talk data.
static Uint8 talkIndex; //Map object slot being talked to.
static Uint8 *talkProp; //NES tileprop.

static Uint8 isObjectVisible(Uint8 id) {
    return gameFlags[id] & GMFLG_OBJVISIBLE;
}

static Uint8 checkGameEventFlag(Uint8 id) {
    return gameFlags[id] & GMFLG_EVENT;
}

static void setGameEventFlag(Uint8 id) {
    gameFlags[id] |= GMFLG_EVENT;
}

//NES HideThisMapObject: hides the object being talked to.
static void hideThisMapObject(Uint8 id) {
    gameFlags[id] &= ~GMFLG_OBJVISIBLE;
    mapobj[talkIndex].id = 0;
}

//NES ShowMapObject: shows the first object on the map loaded with this ID.
static void showMapObject(Uint8 id) {
    gameFlags[id] |= GMFLG_OBJVISIBLE;
    for(Uint8 i = 0; i < SM_OBJ_COUNT; i++) {
        if(mapobj[i].rawid == id) {
            mapobj[i].id = id;
            return;
        }
    }
}

//NES TalkBattle: the battle starts when the box closes.
static void talkBattle(Uint8 formation) {
    btlformation = formation;
    talkProp[0] = TP_BATTLEMARKER;
}

//NES TalkNormTeleport: the teleport happens when the box closes.
static void talkNormTeleport(Uint8 teleId) {
    talkProp[1] = teleId;
    talkProp[0] = TP_TELE_NORM;
}

static Uint8 allOrbsLit() {
    return items[ITEM_ORB_START] & items[ITEM_ORB_START + 1] & items[ITEM_ORB_START + 2] & items[ITEM_ORB_START + 3];
}

//The NES leaves A as the high byte of the routine's address.
static Uint8 Talk_None() {
    return 0x92;
}

static Uint8 Talk_Unused() {
    return 0x94;
}

static Uint8 Talk_KingConeria() {
    if(!isObjectVisible(OBJID_PRINCESS_2)) {
        return talk[1];
    }
    if(bridgeVisible) {
        return talk[3];
    }
    bridgeVisible++;
    dlgSfx++;
    return talk[2];
}

static Uint8 Talk_Garland() {
    hideThisMapObject(OBJID_GARLAND);
    talkBattle(BTL_GARLAND);
    return talk[1];
}

static Uint8 Talk_Princess1() {
    hideThisMapObject(OBJID_PRINCESS_1);
    showMapObject(OBJID_PRINCESS_2);
    talkNormTeleport(NORMTELE_SAVEDPRINCESS);
    return talk[1];
}

static Uint8 Talk_Bikke() {
    if(!checkGameEventFlag(OBJID_BIKKE)) {
        setGameEventFlag(OBJID_BIKKE);
        talkBattle(BTL_BIKKE);
        return talk[1];
    }
    if(shipVisible) {
        return talk[3];
    }
    shipVisible++;
    showMapObject(OBJID_PIRATETERR_1);
    showMapObject(OBJID_PIRATETERR_2);
    showMapObject(OBJID_PIRATETERR_3);
    dlgSfx++;
    return talk[2];
}

static Uint8 Talk_ElfDoc() {
    if(checkGameEventFlag(OBJID_ELFPRINCE)) {
        return talk[3];
    }
    if(!items[ITEM_HERB]) {
        return talk[1];
    }
    items[ITEM_HERB]--;
    setGameEventFlag(OBJID_ELFPRINCE);
    dlgSfx++;
    return talk[2];
}

static Uint8 Talk_ElfPrince() {
    if(!checkGameEventFlag(OBJID_ELFPRINCE)) {
        return talk[3];
    }
    if(items[ITEM_MYSTIC_KEY]) {
        return talk[2];
    }
    items[ITEM_MYSTIC_KEY]++;
    dlgSfx++;
    return talk[1];
}

static Uint8 Talk_Astos() {
    if(!items[ITEM_CROWN]) {
        return talk[1];
    }
    items[ITEM_CRYSTAL]++;
    hideThisMapObject(OBJID_ASTOS);
    talkBattle(BTL_ASTOS);
    dlgSfx++;
    return talk[2];
}

static Uint8 Talk_Nerrick() {
    if(!items[ITEM_TNT]) {
        return talk[1];
    }
    items[ITEM_TNT]--;
    canalVisible = 0;
    hideThisMapObject(OBJID_NERRICK);
    dlgSfx++;
    return talk[2];
}

static Uint8 Talk_Smith() {
    Uint8 *slot;

    if(checkGameEventFlag(OBJID_SMITH)) {
        return talk[3];
    }
    if(!items[ITEM_ADAMANT]) {
        return talk[1];
    }
    slot = dlgFindEmptySlot(FALSE);
    if(!slot) {
        return DLGID_DONTBEGREEDY;
    }
    *slot = WPNID_XCALBUR;
    setGameEventFlag(OBJID_SMITH);
    items[ITEM_ADAMANT]--;
    dlgSfx++;
    return talk[2];
}

static Uint8 Talk_Matoya() {
    if(items[ITEM_HERB]) {
        return talk[3];
    }
    if(items[ITEM_CRYSTAL]) {
        items[ITEM_HERB]++;
        items[ITEM_CRYSTAL]--;
        dlgSfx++;
        return talk[2];
    }
    if(checkGameEventFlag(OBJID_ELFPRINCE)) {
        return talk[3];
    }
    return talk[1];
}

static Uint8 Talk_Unne() {
    if(checkGameEventFlag(OBJID_UNNE)) {
        return talk[3];
    }
    if(!items[ITEM_SLAB]) {
        return talk[1];
    }
    items[ITEM_SLAB]--;
    setGameEventFlag(OBJID_UNNE);
    dlgSfx++;
    return talk[2];
}

static Uint8 Talk_Vampire() {
    hideThisMapObject(OBJID_VAMPIRE);
    talkBattle(BTL_VAMPIRE);
    return talk[1];
}

static Uint8 Talk_Sarda() {
    if(items[ITEM_ROD] || isObjectVisible(OBJID_VAMPIRE)) {
        return talk[2];
    }
    items[ITEM_ROD]++;
    dlgSfx++;
    return talk[1];
}

//NES DoClassChange.
static void doClassChange() {
    for(Uint8 i = 0; i < 4; i++) {
        party[i].char_class += 6;
    }
    dlgflg_reentermap++;
}

static Uint8 Talk_Bahamut() {
    if(checkGameEventFlag(OBJID_BAHAMUT)) {
        return talk[3];
    }
    if(!items[ITEM_TAIL]) {
        return talk[1];
    }
    items[ITEM_TAIL]--;
    setGameEventFlag(OBJID_BAHAMUT);
    doClassChange();
    dlgSfx++;
    return talk[2];
}

static Uint8 Talk_ifvis() {
    return isObjectVisible(talk[0]) ? talk[2] : talk[1];
}

static Uint8 Talk_SubEng() {
    if(!items[ITEM_OXYALE]) {
        return talk[1];
    }
    hideThisMapObject(OBJID_SUBENGINEER);
    return talk[2];
}

static Uint8 Talk_CubeBot() {
    if(items[ITEM_CUBE]) {
        return talk[2];
    }
    items[ITEM_CUBE]++;
    dlgSfx++;
    return talk[1];
}

static Uint8 Talk_Princess2() {
    if(items[ITEM_LUTE]) {
        return talk[2];
    }
    items[ITEM_LUTE]++;
    dlgSfx++;
    return talk[1];
}

static Uint8 Talk_Fairy() {
    if(items[ITEM_OXYALE]) {
        return talk[2];
    }
    items[ITEM_OXYALE]++;
    dlgSfx++;
    return talk[1];
}

static Uint8 Talk_Titan() {
    if(!items[ITEM_RUBY]) {
        return talk[1];
    }
    items[ITEM_RUBY]--;
    hideThisMapObject(OBJID_TITAN);
    dlgSfx++;
    return talk[2];
}

static Uint8 Talk_CanoeSage() {
    if(hasCanoe || !items[ITEM_ORB_START + 3]) {
        return talk[2];
    }
    hasCanoe++;
    dlgSfx++;
    return talk[1];
}

static Uint8 Talk_norm() {
    return talk[1];
}

static Uint8 Talk_Replace() {
    hideThisMapObject(talk[0]);
    showMapObject(talk[3]);
    return talk[1];
}

static Uint8 Talk_CoOGuy() {
    hideThisMapObject(talk[0]);
    return talk[1];
}

static Uint8 Talk_fight() {
    hideThisMapObject(talk[0]);
    talkBattle(talk[3]);
    return talk[1];
}

static Uint8 Talk_ifitem() {
    return items[talk[0]] ? talk[1] : talk[2];
}

static Uint8 Talk_Invis() {
    if(!isObjectVisible(OBJID_PRINCESS_2) && !items[ITEM_LUTE]) {
        return talk[1];
    }
    return talk[2];
}

static Uint8 Talk_ifbridge() {
    return bridgeVisible ? talk[1] : talk[2];
}

static Uint8 Talk_ifevent() {
    return checkGameEventFlag(talk[0]) ? talk[2] : talk[1];
}

static Uint8 Talk_GoBridge() {
    if(isObjectVisible(OBJID_PRINCESS_2) && !bridgeVisible) {
        return talk[1];
    }
    return talk[2];
}

static Uint8 Talk_BlackOrb() {
    if(!allOrbsLit()) {
        return talk[2];
    }
    hideThisMapObject(OBJID_BLACKORB);
    dlgSfx += 2;
    return talk[1];
}

static Uint8 Talk_4Orb() {
    return allOrbsLit() ? talk[1] : talk[2];
}

static Uint8 Talk_ifcanoe() {
    return hasCanoe ? talk[1] : talk[2];
}

static Uint8 Talk_ifcanal() {
    return canalVisible ? talk[2] : talk[1];
}

static Uint8 Talk_ifkeytnt() {
    if(items[ITEM_MYSTIC_KEY] && !items[ITEM_TNT]) {
        return talk[1];
    }
    return talk[2];
}

static Uint8 Talk_ifearthvamp() {
    if(!isObjectVisible(OBJID_VAMPIRE) && !items[ITEM_ORB_START + 3]) {
        return talk[1];
    }
    return talk[2];
}

static Uint8 Talk_ifairship() {
    return airshipVisible ? talk[2] : talk[1];
}

static Uint8 Talk_ifearthfire() {
    if(items[ITEM_ORB_START + 3] && !items[ITEM_ORB_START]) {
        return talk[1];
    }
    return talk[2];
}

static Uint8 Talk_CubeBotBad() {
    if(items[ITEM_CUBE]) {
        return talk[2];
    }
    items[ITEM_CUBE]++;
    return talk[1];
}

static Uint8 Talk_Chime() {
    if(!checkGameEventFlag(OBJID_UNNE)) {
        return talk[3];
    }
    if(items[ITEM_CHIME]) {
        return talk[2];
    }
    items[ITEM_CHIME]++;
    dlgSfx++;
    return talk[1];
}

//NES lut_MapObjTalkJumpTbl, by object ID.
static const TALK_FUNC lut_MapObjTalkJumpTbl[0xD0] = {
    Talk_None,
    Talk_KingConeria, Talk_Garland, Talk_Princess1, Talk_Bikke, Talk_ElfDoc, Talk_ElfPrince, Talk_Astos,
    Talk_Nerrick, Talk_Smith, Talk_Matoya, Talk_Unne, Talk_Vampire, Talk_Sarda, Talk_Bahamut, Talk_ifvis,
    Talk_SubEng, Talk_CubeBot, Talk_Princess2, Talk_Fairy, Talk_Titan, Talk_CanoeSage, Talk_norm, Talk_norm,
    Talk_Replace, Talk_Replace, Talk_fight, Talk_fight, Talk_fight, Talk_fight, Talk_fight, Talk_Unused,

    Talk_ifvis, Talk_ifvis, Talk_ifvis, Talk_ifitem, Talk_ifvis, Talk_ifvis, Talk_Invis, Talk_ifbridge,
    Talk_ifvis, Talk_ifvis, Talk_ifvis, Talk_ifvis, Talk_ifitem, Talk_ifvis, Talk_ifitem, Talk_ifevent,
    Talk_ifvis, Talk_ifvis, Talk_GoBridge, Talk_ifvis, Talk_4Orb, Talk_norm, Talk_norm, Talk_ifvis,
    Talk_ifvis, Talk_norm,

    Talk_4Orb, Talk_4Orb, Talk_4Orb, Talk_4Orb, Talk_4Orb,

    Talk_norm,
    Talk_norm, Talk_norm, Talk_ifevent, Talk_ifevent, Talk_ifevent, Talk_norm, Talk_ifevent, Talk_ifitem,
    Talk_norm, Talk_ifevent, Talk_ifevent, Talk_ifitem, Talk_ifevent, Talk_ifevent, Talk_ifevent, Talk_ifevent,
    Talk_ifevent, Talk_ifevent, Talk_norm, Talk_ifcanoe, Talk_ifitem, Talk_ifevent, Talk_ifevent, Talk_norm,
    Talk_norm, Talk_ifcanal, Talk_norm, Talk_norm, Talk_ifitem, Talk_ifitem, Talk_norm, Talk_ifcanal,
    Talk_ifkeytnt, Talk_norm, Talk_ifcanal, Talk_norm, Talk_norm, Talk_norm, Talk_norm, Talk_norm,
    Talk_ifvis, Talk_norm, Talk_ifearthvamp, Talk_ifitem, Talk_ifvis, Talk_ifearthvamp, Talk_norm, Talk_norm,
    Talk_ifitem, Talk_ifairship, Talk_norm, Talk_ifevent, Talk_ifitem, Talk_norm, Talk_norm, Talk_norm,
    Talk_4Orb, Talk_4Orb, Talk_4Orb, Talk_4Orb, Talk_4Orb, Talk_4Orb, Talk_4Orb, Talk_ifitem,
    Talk_ifearthfire, Talk_ifitem, Talk_norm, Talk_norm, Talk_CoOGuy, Talk_norm, Talk_norm, Talk_norm,
    Talk_norm, Talk_norm, Talk_norm, Talk_norm, Talk_norm, Talk_norm, Talk_norm, Talk_norm,
    Talk_norm, Talk_norm, Talk_norm, Talk_norm, Talk_ifitem, Talk_norm, Talk_norm, Talk_norm,
    Talk_norm, Talk_norm, Talk_ifitem, Talk_ifevent, Talk_norm, Talk_norm, Talk_norm, Talk_norm,
    Talk_norm, Talk_norm, Talk_CubeBotBad, Talk_norm, Talk_norm, Talk_norm, Talk_norm, Talk_norm,
    Talk_norm, Talk_ifitem, Talk_norm, Talk_norm, Talk_norm, Talk_norm, Talk_norm, Talk_ifevent,
    Talk_norm, Talk_norm, Talk_norm, Talk_norm, Talk_norm, Talk_norm, Talk_norm, Talk_norm,
    Talk_norm, Talk_norm, Talk_norm, Talk_Chime, Talk_ifevent, Talk_ifevent, Talk_ifevent, Talk_ifevent,
    Talk_ifevent, Talk_ifevent, Talk_ifevent, Talk_ifevent, Talk_ifevent, Talk_ifevent, Talk_ifevent, Talk_ifevent,
    Talk_ifevent, Talk_ifevent, Talk_BlackOrb, Talk_norm, Talk_norm, Talk_norm, Talk_norm, Talk_norm
};

//NES TalkToObject. Returns the dialogue ID; tileprop may be set to start a battle or teleport.
Uint8 smTalkToObject(Uint8 index, Uint8 *tileprop) {
    Uint8 id = mapobj[index].id;

    talk = (Uint8 *)LUT_MAP_OBJ_TALK + id * 4;
    talkIndex = index;
    talkProp = tileprop;
    return lut_MapObjTalkJumpTbl[id]();
}
