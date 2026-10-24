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
#include "romaddr.h"

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
