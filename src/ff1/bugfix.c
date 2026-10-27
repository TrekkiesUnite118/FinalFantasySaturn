#include <string.h>
#include "types.h"

#include "graphicrefs.h"
#include "nesutils.h"
#include "player.h"
#include "menu.h"
#include "sprdata.h"
#include "vblank.h"
#include "music.h"
#include "bugfix.h"
#include "msxgfx.h"

//Pal 0 is the menu blue with white text, pal 1 the same with grey text for fixes that are off.
#define BF_PAL_ON (0)
#define BF_PAL_OFF (1)
#define BF_GREY (0x10)

//Layout in NES tile coords. The boxes reach 3 tiles past each side of the NES screen.
//Japanese lines need a free row above them for dakuten marks, which leaves no room for the title.
#define BF_BOX_X (0xFD)
#define BF_BOX_W (38)
#define BF_TITLE_Y (1)
#define BF_TITLE_H (3)
#ifdef FF1_JP
#define BF_LIST_Y (1)
#define BF_LIST_H (20)
#define BF_ROWS (9)
#define BF_COLS (3)
#define BF_COL_W (12)
#define BF_DESC_Y (21)
#define BF_DESC_H (8)
#define BF_DESC_LINES (3)
#define BF_NAME_W (10)
#else
#define BF_LIST_Y (4)
#define BF_LIST_H (15)
#define BF_ROWS (13)
#define BF_COLS (2)
#define BF_COL_W (18)
#define BF_DESC_Y (19)
#define BF_DESC_H (10)
#define BF_DESC_LINES (6)
#define BF_NAME_W (15)
#endif
#define BF_ROW_H (TEXT_MARK_ROWS + 1)
#define BF_DESC_X (0xFF)
#define BF_DESC_W (34)
//The cursor sits 2 tiles left of the text.
#define BF_CURSOR_X(x) (((Sint32)(x) - 2 + CENTER_ADJUSTMENT) * 8)

#ifdef FF1_JP
static const char *bfNames[BUGFIX_COUNT] = {
    "クリティカルりつ",
    "ぶきの こうか",
    "モンクの アーマー",
    "てきの たいせい",
    "ストライ/セーバー",
    "シェイプ/シェイラ",
    "アンデッドはんてい",
    "れんぞくレベルアップ",
    "ミスの ついかこうか",
    "こんらんの じめつ",
    "ぜんたいまほう",
    "てきの ぞくせい",
    "てきの ねむり",
    "アップの ひょうじ",
    "ならびかえ",
    "こうどうじゅん",
    "ふいうちの ターン",
    "いしと どく",
    "ちんもくと アイテム",
    "フィールドケアルガ",
    "たからばこと 99こ",
    "ギルの じょうげん",
    "エンカウントちいき",
    "にげる かくりつ",
    "ちせい"
};

static const char *bfDescs[BUGFIX_COUNT][BF_DESC_LINES] = {
    {"クリティカルりつに ぶきの のうりょくちではなく",
     "アイテムの ばんごうが つかわれる。",
     "しゅうせい‥‥そうびした ぶきの あたいを つかう。"},
    {"ぶきの ぞくせいや とくこうが だげきに はんえいされない。",
     "しゅうせい‥‥そうびした ぶきの ぞくせいと",
     "とくこうが きくように なる。"},
    {"レベルアップのとき すでの モンクけいは アーマーが",
     "レベルと おなじになり ぼうぐの アーマーが きえる。",
     "しゅうせい‥‥ぼうぐが ないときだけ。"},
    {"デスペルや たいせいまほうで てきの たいせいが",
     "かわっても はんえいされない。",
     "しゅうせい‥‥へんかが はんえいされる。"},
    {"ストライと セーバーは こうげきりょくが",
     "よみこまれず ほぞんも されないため こうかがない。",
     "しゅうせい‥‥こうげきりょくが あがる。"},
    {"シェイプと シェイラは かならず はずれ",
     "てきの かいひりつが さがらない。",
     "しゅうせい‥‥あたれば かいひりつが さがる。"},
    {"ディアけいは キャラの クラスを しゅぞくと よみちがえ",
     "スーパーモンクと まどうしが アンデッドに なる。",
     "しゅうせい‥‥みかたは アンデッドに ならない。"},
    {"いちどの せんとうで あがる レベルは 1だけ。",
     "しゅうせい‥‥けいけんちが あるかぎり",
     "レベルが あがりつづける。"},
    {"いちど あたると おなじ こうげきの ミスでも",
     "じょうたいいじょうが はっせいする。",
     "しゅうせい‥‥あたったときだけ はっせい。"},
    {"さいごの てきが こんらんして じめつしても",
     "すぐに せんとうが おわらない。",
     "しゅうせい‥‥まいターン おわりを チェックする。"},
    {"てきが なかま ぜんたいに かける まほうは",
     "かけた ほんにんには きかない。",
     "しゅうせい‥‥ほんにんにも きく。"},
    {"てきは じぶんの じゃくてんの ぞくせいで こうげきする。",
     "ウィンターウルフは ほのお。",
     "しゅうせい‥‥てきの こうげきは むぞくせい。"},
    {"ねむった てきは へんな あたいで はんていされ",
     "つぎの ターンで めざめる。",
     "しゅうせい‥‥みかたと おなじく さいだいHPで はんてい。"},
    {"レベルアップのとき 99の のうりょくも",
     "アップしたと ひょうじされる。",
     "しゅうせい‥‥あがった ものだけ ひょうじする。"},
    {"せんとうふのうの なかまを うしろに さげるとき",
     "ちがう ばしょを いれかえ じゅんばんが くずれる。",
     "しゅうせい‥‥ただしく ならびかえる。"},
    {"こうどうじゅんは 16かいの ランダムな いれかえで",
     "きまるため みかたが あとに なりやすい。",
     "しゅうせい‥‥こうへいな シャッフル。"},
    {"モンスターの せんせいこうげきの ターンに",
     "ねむりや まひの なかまが かいふくする ことがある。",
     "しゅうせい‥‥その ターン みかたは こうどうできない。"},
    {"いしに なった キャラクターも どくで HPが へる。",
     "しゅうせい‥‥いしの あいだは どくの ダメージなし。",
     ""},
    {"ちんもくすると まほうだけでなく",
     "くすりと もちものも つかえなくなる。",
     "しゅうせい‥‥まほうだけ ふうじる。"},
    {"せんとういがいの ケアルガで せんとうふのうや",
     "いしの キャラクターの HPも かいふくする。",
     "しゅうせい‥‥その キャラクターは えらべない。"},
    {"アイテムが 99こで とれなくても",
     "たからばこが あいたことに なる。",
     "しゅうせい‥‥たからばこは あかないまま。"},
    {"ばあいによって ギルが じょうげんの 999999を こえる。",
     "しゅうせい‥‥つねに じょうげんで とまる。",
     ""},
    {"フィールドで でる てきが すすむ さきではなく",
     "いまいる ばしょで きまる。",
     "しゅうせい‥‥すすんだ さきの ばしょで きまる。"},
    {"にげるとき ちがう あたいを レベルとして よむため",
     "まえの なかまほど にげやすく さいごは ほぼ にげられない。",
     "しゅうせい‥‥にげる ひとの こううんと レベル。"},
    {"ちせいが まほうに えいきょうしない。",
     "しゅうせい‥‥まほうに アドバンスばんの しきを つかう。",
     "クリティカルしゅうせいと あわせ まほうつかいが ゆうりに なる。"}
};
#else
static const char *bfNames[BUGFIX_COUNT] = {
    "Critical Rate",
    "Weapon Effects",
    "Monk Absorb",
    "Enemy Resists",
    "TMPR/SABR",
    "LOCK/LOK2",
    "Class Undead",
    "Multi Level-Up",
    "Ailment On Miss",
    "Confused Hang",
    "Ally Spells",
    "Enemy Element",
    "Enemy Sleep",
    "Stat-Up Report",
    "Party Sort",
    "Turn Shuffle",
    "Surprise Turn",
    "Stone Poison",
    "Mute Items",
    "CUR4 Field",
    "Full Chest",
    "Gold Cap",
    "Encounter Zone",
    "Run Chance",
    "Intelligence"
};

static const char *bfDescs[BUGFIX_COUNT][BF_DESC_LINES] = {
    {"The critical hit rate is set to",
     "the weapon number in the item",
     "list, not the weapon crit stat.",
     "",
     "Fix - use the crit stat of the",
     "equipped weapon."},
    {"Weapon elements and monster type",
     "bonuses never apply to physical",
     "attacks.",
     "",
     "Fix - attacks use the element and",
     "type bonus of the equipped weapon."},
    {"At level up, an unarmed BB/Master",
     "gets absorb equal to their level,",
     "replacing the absorb of any armor.",
     "",
     "Fix - only when no armor is worn.",
     ""},
    {"XFER and resist spells change the",
     "resistances of an enemy, but the",
     "change is never used.",
     "",
     "Fix - the changes take effect.",
     ""},
    {"TMPR and SABR do nothing, because",
     "the attack power of the target is",
     "never loaded or saved.",
     "",
     "Fix - they raise attack power.",
     ""},
    {"LOCK and LOK2 always miss, so the",
     "evade of an enemy is never",
     "lowered.",
     "",
     "Fix - they can hit and lower",
     "enemy evade."},
    {"HARM spells read the class of a",
     "character as a monster type, so",
     "Masters and Wizards count as",
     "undead.",
     "",
     "Fix - characters are never undead."},
    {"Only one level is gained per",
     "battle, no matter how much EXP is",
     "earned.",
     "",
     "Fix - keep gaining levels until",
     "the EXP is used up."},
    {"Once one enemy hit connects, later",
     "misses in the same attack can",
     "still inflict its ailment.",
     "",
     "Fix - only hits inflict ailments.",
     ""},
    {"If the last enemy kills itself",
     "while confused, the battle does",
     "not end right away.",
     "",
     "Fix - check for the end of battle",
     "after every turn."},
    {"An enemy spell that targets all of",
     "its group skips the caster itself.",
     "",
     "Fix - the caster is included.",
     "",
     ""},
    {"Enemies attack with their own",
     "elemental weakness, so FrWolves",
     "attack with fire.",
     "",
     "Fix - enemy attacks have no",
     "element."},
    {"Sleeping enemies wake up on their",
     "next turn, as the wake check reads",
     "a broken value.",
     "",
     "Fix - wake if max HP beats a roll,",
     "like the party."},
    {"At level up, a stat already at 99",
     "is still reported as going up.",
     "",
     "Fix - only real gains are shown.",
     "",
     ""},
    {"Moving fallen members to the back",
     "swaps the wrong slots, which can",
     "scramble the party order.",
     "",
     "Fix - the party sorts properly.",
     ""},
    {"Turn order is made from 16 random",
     "swaps, which tends to leave the",
     "party near the end.",
     "",
     "Fix - a fair shuffle.",
     ""},
    {"When the party is surprised, a",
     "sleeping or paralyzed member can",
     "still recover in that round.",
     "",
     "Fix - the party gets no turns in",
     "the surprise round."},
    {"Poison still drains the HP of a",
     "stoned character.",
     "",
     "Fix - stone stops poison damage.",
     "",
     ""},
    {"Silence blocks Item and Drink in",
     "battle, not just Magic.",
     "",
     "Fix - only Magic is blocked.",
     "",
     ""},
    {"Outside battle, CUR4 refills the",
     "HP of a dead or stoned character.",
     "",
     "Fix - it cannot target them.",
     "",
     ""},
    {"A chest is marked as opened even",
     "if its item was not taken because",
     "you already had 99.",
     "",
     "Fix - the chest stays closed.",
     ""},
    {"Some gold totals can go past the",
     "999999 limit.",
     "",
     "Fix - gold is always capped.",
     "",
     ""},
    {"Overworld battles use the zone of",
     "the tile you are leaving, not the",
     "one you are entering.",
     "",
     "Fix - use the tile entered.",
     ""},
    {"Running reads the wrong value for",
     "level. Front slots run easily, the",
     "last slot almost never escapes.",
     "",
     "Fix - Luck against the level of",
     "the runner."},
    {"INT has no effect on spells.",
     "Fix - attack and healing spells",
     "use the Dawn of Souls formulas.",
     "With the Critical Rate fix, this",
     "can shift class balance heavily",
     "toward magic users."}
};

static const char bfTitle[] = "BUG FIXES";
#endif

Uint8 bugFixes[BUGFIX_BYTES];

static Uint8 bfCursor;
static Uint8 bfPrevDir;
static PadBits bfPadPrev;

#ifdef FF1_JP
//UTF-8 to the Japanese ROM's text codes, padded with spaces to len.
static void bfText(Uint8 *dst, const char *src, Uint8 len) {
    Uint8 i = 0;

    for(; *src && i < len; i++) {
        dst[i] = jpTextCode(&src);
    }
    for(; i < len; i++) {
        dst[i] = 0xFF;
    }
    dst[len] = 0x00;
}
#else
//ASCII to the NES menu font, padded with spaces to len.
static void bfText(Uint8 *dst, const char *src, Uint8 len) {
    Uint8 i = 0;

    for(; src[i] && i < len; i++) {
        char ch = src[i];
        if(ch >= 'A' && ch <= 'Z') {
            dst[i] = 0x8A + (ch - 'A');
        } else if(ch >= 'a' && ch <= 'z') {
            dst[i] = 0xA4 + (ch - 'a');
        } else if(ch >= '0' && ch <= '9') {
            dst[i] = 0x80 + (ch - '0');
        } else if(ch == '.') {
            dst[i] = 0xC0;
        } else if(ch == ',') {
            dst[i] = 0xBF;
        } else if(ch == '-') {
            dst[i] = 0xC2;
        } else if(ch == '/') {
            dst[i] = 0x7A;
        } else {
            dst[i] = 0xFF;
        }
    }
    for(; i < len; i++) {
        dst[i] = 0xFF;
    }
    dst[len] = 0x00;
}
#endif

void optionDrawText(Uint8 x, Uint8 y, const char *str, Uint8 len, Uint8 pal) {
    Uint8 buffer[BF_DESC_W + 1];

    bfText(buffer, str, len);
    drawComplexString(x, y, buffer, pal);
}

static Uint8 bfX(Uint8 id) {
    return (id / BF_ROWS) * BF_COL_W;
}

static Uint8 bfY(Uint8 id) {
    return BF_LIST_Y + 1 + TEXT_MARK_ROWS + (id % BF_ROWS) * BF_ROW_H;
}

static void bfDrawName(Uint8 id) {
    optionDrawText(bfX(id), bfY(id), bfNames[id], BF_NAME_W, BUGFIX_ON(id) ? BF_PAL_ON : BF_PAL_OFF);
}

static void bfDrawDesc() {
    for(Uint8 i = 0; i < BF_DESC_LINES; i++) {
        Uint8 y = BF_DESC_Y + 2 + i * BF_ROW_H;
        if(TEXT_MARK_ROWS) {
            //Clears the previous description's marks.
            optionDrawText(BF_DESC_X, y - 1, "", BF_DESC_W, BF_PAL_ON);
        }
        optionDrawText(BF_DESC_X, y, bfDescs[bfCursor][i], BF_DESC_W, BF_PAL_ON);
    }
}

void setupBugFixScreen() {
    setBoxPalette(menu_pal_0, boarderPalette_blue, MSX_BOX_MENU);
    setBoxPalette(menu_pal_1, boarderPalette_blue, MSX_BOX_MENU);
    menu_pal_1[3] = nes_pal[BF_GREY];

#ifndef FF1_JP
    drawBox(BF_BOX_X, BF_TITLE_Y, BF_BOX_W, BF_TITLE_H, BF_PAL_ON);
    optionDrawText(BF_DESC_X, BF_TITLE_Y + 1, bfTitle, sizeof(bfTitle) - 1, BF_PAL_ON);
#endif
    drawBox(BF_BOX_X, BF_LIST_Y, BF_BOX_W, BF_LIST_H, BF_PAL_ON);
    drawBox(BF_BOX_X, BF_DESC_Y, BF_BOX_W, BF_DESC_H, BF_PAL_ON);

    for(Uint8 i = 0; i < BUGFIX_COUNT; i++) {
        bfDrawName(i);
    }
    bfCursor = 0;
    bfDrawDesc();
}

void initBugFixMenu() {
    player_cursor_init(FIXED(BF_CURSOR_X(bfX(0))), FIXED(bfY(0) * 8));
    bfPrevDir = 0;
    bfPadPrev = PadData1;
}

static void bfMoveCursor() {
    Uint8 dir = PadData1 & (PAD_U | PAD_D | PAD_L | PAD_R) ? 1 : 0;
    Uint8 row = bfCursor % BF_ROWS;
    Uint8 col = bfCursor / BF_ROWS;

    if(dir == bfPrevDir) {
        return;
    }
    bfPrevDir = dir;
    if(!dir) {
        return;
    }
    if(PadData1 & PAD_L) {
        col = col > 0 ? col - 1 : BF_COLS - 1;
    } else if(PadData1 & PAD_R) {
        col = col + 1 < BF_COLS ? col + 1 : 0;
    }
    //The last column can be shorter.
    Uint8 rows = col == BF_COLS - 1 ? BUGFIX_COUNT - col * BF_ROWS : BF_ROWS;
    if(row >= rows) {
        row = rows - 1;
    }
    if(PadData1 & PAD_U) {
        row = row > 0 ? row - 1 : rows - 1;
    } else if(PadData1 & PAD_D) {
        row = row + 1 < rows ? row + 1 : 0;
    }
    bfCursor = col * BF_ROWS + row;
    sfxMenuMove();
    bfDrawDesc();
}

void doBugFixMenu() {
    PadBits pressed = PadData1 & ~bfPadPrev;
    bfPadPrev = PadData1;

    if(pressed & (PAD_A | PAD_C)) {
        sfxMenuSel();
        bugFixes[bfCursor >> 3] ^= 1 << (bfCursor & 7);
        bfDrawName(bfCursor);
    } else if(pressed & PAD_B) {
        sfxMenuSel();
        state = TO_MAIN_MENU;
    } else {
        bfMoveCursor();
    }
}

void bugfix_draw() {
    cursor.xPos = FIXED(BF_CURSOR_X(bfX(bfCursor)));
    cursor.yPos = FIXED(bfY(bfCursor) * 8);
    player_cursor_draw();
}
