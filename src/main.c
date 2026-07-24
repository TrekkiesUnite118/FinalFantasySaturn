#include    <string.h> //memcpy
#define        _SPR2_
#include    <sega_spr.h>
#include    <sega_scl.h> 
#include    <sega_mth.h>
#include    <sega_def.h>
#include    <sega_per.h>

#include    "cd.h"
#include    "graphicrefs.h"
#include    "vdp2structs.h"
#include    "sprite.h"
#include    "scroll.h"
#include    "static.h"
#include    "overworld.h"
#include    "battle.h"
#include    "player.h"
#include    "print.h"
#include    "menu.h"

static inline void ow_layer_init(MAP_DATA *map, Uint8 *tiles_location, Uint16 rawlength, Uint8 numTiles, Uint8 *map_location, Uint16 map_width, Uint16 map_height, Uint8 total_colors, Uint8 *palette, Uint8 num_palettes) {
    map->tile_location = tiles_location;
    map->rawlength = rawlength;
    map->numTiles = numTiles;
    map->map_location = map_location;
    map->map_width = map_width;
    map->map_height = map_height;
    map->total_colors = total_colors;
    map->palette = palette;
    map->num_palettes = num_palettes;
}

static inline void static_screen_init(LAYER *screen, char *tiles_name, Uint16 tiles_num, Uint32 *palette, Uint8 *map_name, Uint16 map_width, Uint16 map_height) {
    screen->tile_name = tiles_name;
    screen->tile_num = tiles_num;
    screen->palette = palette;
    screen->map_name = map_name;
    screen->map_width = map_width;
    screen->map_height = map_height;
}
Uint32 frame = 0;

int main() {
    int sound_inited = 0;
    cd_init();
    cd_load("FFANTASY.BIN", (void *)LWRAM, 0x40010);

    Uint8 *nes_head_ptr = (Uint8*) LWRAM;
    Uint8 nesHead1 = nes_head_ptr[0];
    Uint8 nesHead2 = nes_head_ptr[1];
    Uint8 nesHead3 = nes_head_ptr[2];
    if(nesHead1 != 0x4E && nesHead2 != 0x45 && nesHead3 != 0x53) {
        state = TO_SAD;
    } else {
        state = TO_PRELUDE;
    }
    Uint8 *battleBackdropPatternData = BATTLE_TILE_OFFSET;
    convertNesTilesToSatTiles(battleBackdropPatternData, (Uint8 *)LWRAM_MENU_TILES, 0x800);

    Uint8 *menuPatternData = MENU_TILE_OFFSET;
    convertNesTilesToSatTiles(menuPatternData, (Uint8 *)LWRAM_MENU_TILES + 0x1000, 0x800);
    
    Uint8 *patternData = MAPMAN_SPRITE_OFFSET;
    initMapSpriteData(patternData);
    
    Uint8 *battlePatternData = BATTLE_SPRITE_OFFSET;
    initBattleSpriteData(battlePatternData);

    Uint8 *weaponPatternData = WEAPON_SPRITE_OFFSET;
    initWeaponSpriteData(weaponPatternData);

    sprite_init();
    overworld_init();
    
    char name1[6] = "      ";
    char name2[6] = "      ";
    char name3[6] = "      ";
    char name4[6] = "      ";
    char_init(0, 0, name1);
    char_init(1, 1, name2);
    char_init(2, 2, name3);
    char_init(3, 3, name4);
    
    LEVEL level1;
    STATIC_SCREEN battleScreen;
    level1.player_startx = MTH_FIXED(2296);
    level1.player_starty = MTH_FIXED(2545);

    Uint16 *lwram_ptr = (Uint16 *)LWRAM_ROM_END;
    print_init();
    
    //The original game relied on the randomness of NES RAM on 
    //Power On. The Saturn clears it's RAM on start up, so to
    //try and mimick the NES behavior I'm getting the 
    //seconds value from the RTC.
    SCL_SetSpriteMode(SCL_TYPE5,SCL_MIX,SCL_SP_WINDOW);
    setRngSeed();

    player_ow_init(level1.player_startx, level1.player_starty);
    while(1) {
        frame++;
        //print_num(frame, 0, 0);
       
        SPR_2OpenCommand(SPR_2DRAW_PRTY_ON);
        switch(state) {
            case TO_SAD:
                static_init();
                
                static_disable();
                sound_init();
                if(!sound_inited) {
                    sound_init();
                    sound_inited = 1;
                }
                print_init();
                sound_cdda(6);
                print_string("SATURN SAD!", 14, 2, 11);
                print_string("SATURN wanted to play Final Fantasy,", 15, 2, 36);
                print_string("but NES ROM is corrupt!", 16, 2, 23);
                print_string("The SH-2s will now weep...", 17, 2, 26);
                state = SAD;
                break;
            case SAD:
                print_display();
                break;
            case TO_PRELUDE:
                clearMenuPlane();
                setupPreludeScreen();
                static_init();
                state = PRELUDE;
                sound_init();
                if(!sound_inited) {
                    sound_init();
                    sound_inited = 1;
                }
                sound_cdda(2);
                break;
            case PRELUDE:
                break;
            case TO_TITLE_STATE:
                clearMenuPlane();
                setupTitleScreen();
                print_init();
                player_cursor_init(MTH_FIXED(104), MTH_FIXED(88));
                draw_response_rate();
                static_init();
                sound_cdda(2);
                player_cursor_draw();
                state = TITLE_STATE;
                break;
            case TO_CHAR_SELECT_STATE:
                sprite_deleteall();
                clearMenuPlane();
                setupPartyCreateScreen();
                print_init();
                player_cursor_init(MTH_FIXED(80), MTH_FIXED(55));
                state = CHAR_SELECT_STATE;
                break;
            case TO_CHAR_NAME_STATE: 
                sprite_deleteall();  
                clearMenuPlane();  
                setupNameEntryScreen();
                print_init();
                player_cursor_init(MTH_FIXED(64), MTH_FIXED(80));
          
                //sound_cdda(2);
                state = CHAR_NAME_STATE;
                break;
            case TO_OW_STATE:
                determine_curr_char();
                update_player_sprites();
                sprite_deleteall();
                static_disable();
                print_init();
                ow_layer_init(&(level1.bg),(Uint8 *) LWRAM + 0x8010, 0x1000, 128, mapPlane, 256, 256, 32, (Uint8 *) LWRAM + 0x390, 8);
                scroll_init(&level1);
                if(!sound_inited) {
                    sound_init();
                    sound_inited = 1;
                }
                sound_cdda(3);
                state = OW_STATE;
                break;
            case TO_BAT_STATE:
                sprite_deleteall();
                scroll_disable();
                clearMenuPlane();  
                copyPlayerStatsToBattleStats();
                print_init();
                battleBackdropPatternData = BATTLE_TILE_OFFSET + currentBackDrop * 0x800;
                convertNesTilesToSatTiles(battleBackdropPatternData, (Uint8 *)LWRAM_MENU_TILES, 0x120);
                setupBattleScreen(currentBackDrop);
                determineSurprise();
                enemy_bat_draw();
                static_init();
                player_bat_init();
                sound_cdda(4);
                state = BAT_STATE;
                break;
            case OW_STATE:
                player_ow_draw();
                break;
            case TITLE_STATE:
                player_cursor_draw();
                draw_response_rate();
          
                print_display();
                break;
            case CHAR_SELECT_STATE:
                player_cursor_draw();
                player_select_draw();
          
                print_display();
                break;
            case CHAR_NAME_STATE:
                player_cursor_draw();
                draw_char_name();
                print_display();
          
                break;
            case BAT_STATE:
                party_crouch();
                party_dead(); 
                player_bat_draw();
                updateAilmentandHP();
                if(isSurprised || isPreemptive) {
                    drawInitMessage();
                } else {
                    state = TO_BAT_MENU_STATE;
                }

          
               // print_display();
                break;
            case TO_BAT_MENU_STATE:
                player_cursor_init(MTH_FIXED(120), MTH_FIXED(160));
                player_bat_draw();
                clearMessageArea();
                drawMenu();
           
                state = BAT_MENU_STATE;
                bat_menu_state = INIT;
               // print_display();
                break;

            case BAT_MENU_STATE:
                player_bat_draw();
                if(drawBattleCursor == TRUE) {
                    
                   player_cursor_draw();
                }
                if(drawTargetNames == TRUE) {
                    drawEnemyNames();
                }
          
                print_display();
                break;

            case TO_BAT_EXECUTE_STATE:
                player_bat_draw();
                clearMessageArea();
            
                determineBattleOrder();
                turn_exec_state = BETWEEN_TURN;
                state = BAT_EXECUTE_STATE;
                print_display();
                break;

            case BAT_EXECUTE_STATE:
                player_bat_draw();
                updateAilmentandHP();
          
                print_display();
                break;
            case TO_BAT_VICTORY_STATE:
                copyBattleStatsToPlayerStats();
                updateAilmentandHP();
                player_bat_draw();
                clearMessageArea();
                drawMonstersPerishedMessage();
                player_bat_stand();
                sound_cdda(5);
           
                print_display();
                state = BAT_VICTORY_STATE;
                break;
            case BAT_VICTORY_STATE:
                updateAilmentandHP();
                player_bat_draw();
                print_display();
                break;
            case TO_GAME_OVER:
                player_bat_draw();
                enemy_bat_draw();
                updateAilmentandHP();
                clearMessageArea();
                sound_cdda(6);
                print_string("Game Over", 26, 5, 9);
          
                print_display();
                state = GAME_OVER;
                break;
            case GAME_OVER:
                player_bat_draw();
                enemy_bat_draw();
                updateAilmentandHP();
          
                print_display();
                break;
            case GAME_RESTART:
                sprite_deleteall();
                static_disable();
                print_init();
                char_init(0, 0, name1);
                char_init(1, 1, name2);
                char_init(2, 2, name3);
                char_init(3, 3, name4);
                reset();
            
                state = TO_TITLE_STATE;
                break;
            case TO_BAT_RUN_AWAY:
                copyBattleStatsToPlayerStats();
                party_flip();
                player_bat_draw();
                enemy_bat_draw();
                updateAilmentandHP();
                clearMessageArea();
                drawRunSuccessMessage();
                print_display();
                state = BAT_RUN_AWAY;
                break;
            case BAT_RUN_AWAY:
                player_bat_draw();
                enemy_bat_draw();
                updateAilmentandHP();
          
                print_display();
                break;
        }

        SPR_2CloseCommand();
        
        SCL_DisplayFrame();
    }
    return 0;
}

