/*******************************************************************
 *
 *                      Shops
 *
 * The NES shop screen: weapon, armor, magic and item shops, clinics,
 * inns and the caravan. The shop is picked by shop_id.
 *
 *
 *
*******************************************************************/
#ifndef SHOP_H
#define SHOP_H

#include "types.h"

#define LUT_SHOP_STRINGS (0x00238010) //Bank $0E $8000, pointer table then strings.
#define LUT_SHOP_DATA (0x00238310) //Bank $0E $8300, pointers to each shop's inventory.
#define SHOP_BANK_ADD (0x00238010 - 0x8000)
#define LUT_SHOP_BOX (0x0023AC50) //X, Y, width and height tables, 5 boxes each.
#define LUT_SHOPKEEP_IMAGE (0x0023ACAC) //10x10 tiles.
#define LUT_SHOP_PALETTES (0x0023EB84) //Title and gold box palettes.
#define LUT_SHOP_TYPES (0x0023EBC5)
#define SHOP_CHR (0x00224010) //Bank $09 $8000, shop CHR followed by the menu CHR.
#define SHOP_BACKDROP_PAL (0x40) //Shop palettes in the battle backdrop palette table.

#define SHOP_ID_CARAVAN (70)

enum __shoptype__ {
    SHOP_WEAPON,
    SHOP_ARMOR,
    SHOP_WHITE_MAGIC,
    SHOP_BLACK_MAGIC,
    SHOP_CLINIC,
    SHOP_INN,
    SHOP_ITEM,
    SHOP_CARAVAN
};

/**
 * @brief Draws the shop screen for shop_id and starts its first menu.
 *
 * Loads the shop graphics and palettes, the shopkeeper, the title and
 * gold boxes, and the shop's inventory.
 */
void setupShopScreen(void);

/**
 * @brief Sets up the party sprites and pad state for the shop.
 */
void initShop(void);

/**
 * @brief Runs one frame of the shop. Called each frame in the SHOP state.
 */
void doShop(void);

/**
 * @brief Draws the party sprites and the cursor.
 */
void shop_draw(void);

#endif
