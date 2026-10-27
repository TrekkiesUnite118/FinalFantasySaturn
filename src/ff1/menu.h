/*******************************************************************
 *
 *                      Menu Screen
 *
 * Boxes and text on the NBG1 menu layer, used by the title, party
 * creation, prelude, battle and menu screens. Draws go to menuPlane,
 * or to drawPlane while it's redirected.
 *
 * Also the battle message area, drawn a row at a time like the NES.
 *
*******************************************************************/
#ifndef MENU_H
#define MENU_H

#include    "types.h"
#include    "romaddr.h"

//NES lut_PtyGenBuf entry.
typedef struct {
    Uint8 class;
    Uint8 padding;
    Uint8 name[4]; //While we'll support 6 this is for reading data from the NES ROM.
    Uint8 name_xPos;
    Uint8 name_yPos;
    Uint8 class_xPos;
    Uint8 class_yPos;
    Uint8 sprite_xPos;
    Uint8 sprite_yPos;
    Uint8 box_xPos;
    Uint8 box_yPos;
    Uint8 cursor_xPos;
    Uint8 cursor_yPos;
} PARTY_GEN;


typedef struct {
    Uint8 header;
    Uint8 xPos;
    Uint8 yPos;
    Uint8 ptr_low;
    Uint8 ptr_high;
} COMMAND_BOX_TEXT;

typedef struct {
    Uint8 header;
    Uint8 xPos;
    Uint8 yPos;
    Uint8 width;
    Uint8 height;
    COMMAND_BOX_TEXT text[5];
} COMMAND_BOX;

typedef struct {
    Uint8 header;
    Uint8 xPos;
    Uint8 yPos;
    Uint8 width;
    Uint8 height;
    Uint8 text_header;
    Uint8 text_xPos;
    Uint8 text_yPos;
} BATTLE_INFO_BOX;

typedef struct {
    Uint8 header;
    Uint8 xPos;
    Uint8 yPos;
    Uint8 width;
    Uint8 height;
} BATTLE_BOX;

//Columns added to every X, to center the NES screen.
#define  CENTER_ADJUSTMENT (4)
//Rows above the text used by Japanese dakuten marks.
#ifdef FF1_JP
#define  TEXT_MARK_ROWS (1)
#else
#define  TEXT_MARK_ROWS (0)
#endif
#define  NAME_BOX_EXTENSION (2)
#define  BOX_TOP_LEFT_CORNER_CHAR (0xF7)
#define  BOX_TOP_EDGE_CHAR (0xF8)
#define  BOX_TOP_RIGHT_CORNER_CHAR (0xF9)
#define  BOX_LEFT_EDGE_CHAR (0xFA)
#define  BOX_RIGHT_EDGE_CHAR (0xFB)
#define  BOX_BOTTOM_LEFT_CORNER_CHAR (0xFC)
#define  BOX_BOTTOM_EDGE_CHAR (0xFD)
#define  BOX_BOTTOM_RIGHT_CORNER_CHAR (0xFE)
#define  BOX_BLANK_CHAR (0xFF)

#define  TILESET_2BPP_SIZE (0x800)
#define  TILESET_4BPP_SIZE (0x1000)

#define  BASIC_TEXT_ITEM_CNT (27)
#define  BASIC_TEXT_WEAPON_CNT (40)
#define  BASIC_TEXT_ARMOR_CNT (40)
#define  BASIC_TEXT_GOLDITEM_CNT (68)


//NBG1 map.
extern Uint16 menuPlane[64][64];
//Where box and text draws go. menuPlane unless redirected.
extern Uint16 (*drawPlane)[64];

//Unused.
extern Uint8 canAnimatePrelude;

//NES box border palettes.
extern Uint8 boarderPalette_black[4];
extern Uint8 boarderPalette_blue[4];
extern Uint8 boarderPalette_red[4];

//NBG1 palettes 0-6, set by each screen's setup. 5 and 6 are the MSX battle's extra enemy palettes.
extern Uint32 menu_pal_0[];
extern Uint32 menu_pal_1[];
extern Uint32 menu_pal_2[];
extern Uint32 menu_pal_3[];
extern Uint32 menu_pal_4[];
extern Uint32 menu_pal_5[16];
extern Uint32 menu_pal_6[16];

/**
 * @brief Sets a box and text palette's 4 colors: the NES ones, or the screen's MSX box colors.
 *
 * @param dst Palette to set.
 * @param nesPal The 4 NES colors.
 * @param msxStyle MSX_BOX_ style for MSX mode.
 */
void setBoxPalette(Uint32 *dst, const Uint8 *nesPal, Uint8 msxStyle);

/**
 * @brief Sets up the NBG1 menu layer with the menu tiles, palettes and menuPlane.
 */
void static_init(void);

/**
 * @brief Sets up the NBG1 menu layer with the given tiles.
 *
 * @param tiles 256 tiles to use in place of the menu tiles.
 */
void static_init_tiles(Uint8 *tiles);

//Palette for battle boxes and text.
extern Uint8 BATTLE_MENU_PALETTE;

/**
 * @brief Draws a bordered box with a blank inside.
 *
 * @param xPos Left column.
 * @param yPos Top row.
 * @param width Width in tiles, including the border.
 * @param height Height in tiles, including the border.
 * @param palNum Palette.
 */
void drawBox(Uint8 xPos, Uint8 yPos, Uint8 width, Uint8 height, Uint8 palNum);

/**
 * @brief Draws only a box's bottom edge.
 *
 * @param xPos Left column.
 * @param yPos Row.
 * @param width Width in tiles.
 * @param palNum Palette.
 */
void drawBottomBoxLine(Uint8 xPos, Uint8 yPos, Uint8 width, Uint8 palNum);

#ifdef FF1_JP
/**
 * @brief Gets the dakuten or handakuten mark drawn above a Japanese character.
 *
 * @param ch Character.
 * @return The mark tile, or a blank for characters without one.
 */
Uint8 jpKanaMark(Uint8 ch);

/**
 * @brief Gets the tile a Japanese character draws on its own row.
 *
 * @param ch Character.
 * @return The base kana tile for kana $48-$79, otherwise the character.
 */
Uint8 jpKanaBase(Uint8 ch);

/**
 * @brief Converts one UTF-8 kana, digit or "!?/LEHP" character to a Japanese ROM text code.
 *
 * @param src String position, moved past the character.
 * @return The text code, or a space for characters the font lacks.
 */
Uint8 jpTextCode(const char **src);
#endif

/**
 * @brief Draws an NES string, expanding DTE pairs.
 *
 * Byte $01 starts a new line. $00 ends the string. Japanese text has no DTE,
 * and draws its dakuten marks on the row above.
 *
 * @param xPos Left column.
 * @param yPos Top row.
 * @param stringPtr String.
 * @param palNum Palette.
 * @param lineSpacing Rows to move down on each new line.
 */
void drawTextLine(Uint8 xPos, Uint8 yPos, Uint8 *stringPtr, Uint8 palNum, Uint8 lineSpacing);

/**
 * @brief Draws the prelude text to menuPlane, like drawTextLine.
 *
 * Rows 15 and 20 are skipped on the US ROM.
 *
 * @param xPos Left column.
 * @param yPos Top row.
 * @param stringPtr String.
 * @param palNum Palette.
 * @param lineSpacing Rows to move down on each new line.
 */
void drawPreludeTextLine(Uint8 xPos, Uint8 yPos, Uint8 *stringPtr, Uint8 palNum, Uint8 lineSpacing);

/**
 * @brief Sets up the battle palettes and draws the enemy, party and stat boxes.
 *
 * @param battleValue Backdrop number, for its palette.
 */
void setupBattleScreen(Uint8 battleValue);

/**
 * @brief Loads the enemy tiles and their two palettes into menu palettes 3 and 4.
 *
 * @param patternData NES enemy tiles.
 * @param pal_id_0 First battle palette number.
 * @param pal_id_1 Second battle palette number.
 */
void initEnemyPatternData(Uint8 *patternData, Uint8 pal_id_0, Uint8 pal_id_1);

/**
 * @brief Draws an enemy's tiles.
 *
 * @param enemy_num Enemy graphic. 0 and 1 are small (4x4 tiles), 2 and 3 are large (6x6 tiles).
 * @param palette_assignment 0 for menu palette 3, 1 for menu palette 4.
 * @param xPos Left column.
 * @param yPos Top row.
 */
void drawEnemy(Uint8 enemy_num, Uint8 palette_assignment, Uint8 xPos, Uint8 yPos);

/**
 * @brief Draws the bottom edges of the enemy and party boxes.
 */
void mainBattleBoxBottoms();

/**
 * @brief Draws the enemy and party boxes with clear insides, and the backdrop.
 */
void drawMainBattleBoxes();

/**
 * @brief Draws the title screen: continue, new game, respond rate and copyright.
 */
void setupTitleScreen();

/**
 * @brief Draws the party creation boxes and class names and sets up the character sprites.
 */
void setupPartyCreateScreen();

/**
 * @brief Draws the lineup screen's box (NES EnterLineupMenu).
 */
void setupLineupScreen();

/**
 * @brief Draws the name entry screen's letter and name boxes.
 */
void setupNameEntryScreen();

/**
 * @brief Draws the prelude text and restarts its animation.
 */
void setupPreludeScreen();

/**
 * @brief Runs one frame of the prelude, fading the text in a line at a time.
 */
void preludeAnimate();

/**
 * @brief Clears menuPlane.
 */
void clearMenuPlane();

/**
 * @brief Draws the party creation character sprites and names.
 */
void player_select_draw();

/**
 * @brief Resets the party creation classes to Fighter, Thief, Black Belt and Red Mage.
 */
void resetPartyInitBuff();

/**
 * @brief Changes a party creation slot to the next class and draws its name.
 *
 * @param cur_char_create Party slot.
 */
void incrementPartyInitBuff(Uint8 cur_char_create);

/**
 * @brief Changes a party creation slot to the previous class and draws its name.
 *
 * @param cur_char_create Party slot.
 */
void decrementPartyInitBuff(Uint8 cur_char_create);

/**
 * @brief Gets a party creation slot's cursor X.
 *
 * @param cur_char_create Party slot.
 * @return Screen X in pixels.
 */
Uint8 getPartyCreateCursorPosX(Uint8 cur_char_create);

/**
 * @brief Gets a party creation slot's cursor Y.
 *
 * @param cur_char_create Party slot.
 * @return Screen Y in pixels.
 */
Uint8 getPartyCreateCursorPosY(Uint8 cur_char_create);

/**
 * @brief Draws one row of the battle backdrop.
 *
 * @param row Backdrop row, 0-3.
 */
void drawBattleBackdropRow(Uint8 row);

/**
 * @brief Gets the ailment a party member's status box shows.
 *
 * @param currChar Party member.
 * @return First set ailment bit, 0-6. 7 for none.
 */
Uint8 determineAilmentBit(Uint8 currChar);

/**
 * @brief Draws a number right aligned in 4 columns.
 *
 * @param num Number.
 * @param xPos Left column.
 * @param yPos Row.
 */
void drawHpNum(Uint32 num, Uint8 xPos, Uint8 yPos);

/**
 * @brief Draws every party member's ailment and HP.
 */
void updateAilmentandHP();

/**
 * @brief Draws a party member's ailment, or "HP", and their HP.
 *
 * @param index Party member.
 */
void drawAilmentAndHP(Uint8 index);

/**
 * @brief Draws a party member's name, ailment and HP in their stat box.
 *
 * @param index Party member.
 */
void drawCharacterStatus(Uint8 index);

/**
 * @brief Draws a battle message from the ROM.
 *
 * @param xPos Left column.
 * @param yPos Row.
 * @param msg_id Message number, from 1.
 * @param pal Unused. BATTLE_MENU_PALETTE is used.
 */
void drawBattleMsg(Uint8 xPos, Uint8 yPos, Uint8 msg_id, Uint8 pal);

/**
 * @brief Draws "Monsters perished".
 *
 * @param xPos Left column.
 * @param yPos Row.
 * @param pal Unused. BATTLE_MENU_PALETTE is used.
 */
void drawMonstersPerished(Uint8 xPos, Uint8 yPos, Uint8 pal);

/**
 * @brief Draws "<Name> party perished".
 *
 * @param xPos Left column.
 * @param yPos Row.
 * @param name_ptr Party leader's name.
 * @param pal Palette.
 */
void drawPartyPerished(Uint8 xPos, Uint8 yPos, Uint8 *name_ptr, Uint8 pal);

/**
 * @brief Adds a battle message to a string.
 *
 * @param buffer String to add to.
 * @param len Current length.
 * @param msg_id Message number, from 1.
 * @return New length. The string isn't terminated.
 */
Uint8 appendBattleMsg(Uint8 *buffer, Uint8 len, Uint8 msg_id);

/**
 * @brief Adds a character name to a string, without trailing blanks.
 *
 * @param buffer String to add to.
 * @param len Current length.
 * @param name_ptr Name, up to 6 characters.
 * @return New length. The string isn't terminated.
 */
Uint8 appendTrimmedName(Uint8 *buffer, Uint8 len, Uint8 *name_ptr);

/**
 * @brief Adds a number without leading zeros to a string (NES DrawBattle_Number).
 *
 * @param buffer String to add to.
 * @param len Current length.
 * @param num Number.
 * @return New length. The string isn't terminated.
 */
Uint8 appendNumber(Uint8 *buffer, Uint8 len, Uint16 num);

/**
 * @brief Draws an NES complex string, with its control codes (NES DrawComplexString).
 *
 * @param xPos Left column.
 * @param yPos Top row.
 * @param str String.
 * @param palNum Palette.
 */
void drawComplexString(Uint8 xPos, Uint8 yPos, Uint8 *str, Uint8 palNum);

/**
 * @brief Writes a number right aligned, with leading zeros as spaces (NES PrintNumber).
 *
 * @param buffer Set to the string, 0 terminated. Needs digits + 1 bytes.
 * @param num Number.
 * @param digits Width.
 */
void formatNumber(Uint8 *buffer, Uint32 num, Uint8 digits);

/**
 * @brief Gets an item's price from the ROM (NES LoadPrice).
 *
 * @param itemId Item number.
 * @return Price in gold.
 */
Uint16 getItemPrice(Uint8 itemId);

/**
 * @brief Gets an NES lut_MenuText string from the ROM.
 *
 * @param id String number.
 * @return Pointer to the string.
 */
Uint8 *getMenuString(Uint8 id);

/**
 * @brief Clears an area to transparent.
 *
 * @param xPos Left column.
 * @param yPos Top row.
 * @param width Width in tiles.
 * @param height Height in tiles.
 */
void clearArea(Uint8 xPos, Uint8 yPos, Uint8 width, Uint8 height);

/**
 * @brief Empties the battle message area and drops any queued draws (NES ClearBattleMessageBuffer).
 */
void msgReset();

/**
 * @brief Starts a battle message block. Draws until msgEndBlock go into it.
 */
void msgBeginBlock();

/**
 * @brief Ends a battle message block and queues it to be drawn.
 */
void msgEndBlock();

/**
 * @brief Removes the newest battle message blocks (NES UndrawNBattleBlocks).
 *
 * @param count Number of blocks to remove.
 */
void msgUndraw(Uint8 count);

/**
 * @brief Gets the number of battle message blocks shown.
 *
 * @return Block count.
 */
Uint8 msgGetBlockCount();

/**
 * @brief Checks if battle message draws are still queued.
 *
 * @return TRUE while drawing or undrawing.
 */
Uint8 msgBusy();

/**
 * @brief Runs one frame of the queued battle message draws. Call this in V-blank before menuPlane is uploaded.
 *
 * Drawing shows 1 row a frame top down, undrawing 2 rows a frame bottom up.
 */
void msgAnimate();

#endif
