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

static inline void layer_init(LAYER *layer, char *tiles_name, Uint16 tiles_num, Uint32 *palette, char *map_name, Uint16 map_width, Uint16 map_height) {
    layer->tile_name = tiles_name;
    layer->tile_num = tiles_num;
    layer->palette = palette;
    layer->map_name = map_name;
    layer->map_width = map_width;
    layer->map_height = map_height;
}

static inline void ow_layer_init(LAYER *layer, char *tiles_name, Uint16 tiles_num, Uint32 *palette, Uint8 *map_plane, Uint16 map_width, Uint16 map_height) {
    layer->tile_name = tiles_name;
    layer->tile_num = tiles_num;
    layer->palette = palette;
    layer->map_name = map_plane;
    layer->map_width = map_width;
    layer->map_height = map_height;
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
    cd_load("FFANTASY.BIN", (void *)LWRAM, 262160);
    state = TO_TITLE_STATE;
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
    STATIC_SCREEN titleScreen;
    STATIC_SCREEN charSelectScreen;
    STATIC_SCREEN charNameScreen;
    level1.player_startx = MTH_FIXED(2296);
    level1.player_starty = MTH_FIXED(2545);

    print_init();
    player_ow_init(level1.player_startx, level1.player_starty);
    
    //The original game relied on the randomness of NES RAM on 
    //Power On. The Saturn clears it's RAM on start up, so to
    //try and mimick the NES behavior I'm getting the 
    //seconds value from the RTC.
    SCL_SetSpriteMode(SCL_TYPE5,SCL_MIX,SCL_SP_WINDOW);
    setRngSeed();

    while(1) {
        frame++;
        player_input();
        //print_num(frame, 0, 0);
       
        SPR_2OpenCommand(SPR_2DRAW_PRTY_OFF);
        switch(state) {
            case TO_TITLE_STATE:
                static_screen_init((&titleScreen.bg), menu_name, menu_num, menu_pal, title_name, title_width, title_height);
                static_init(&titleScreen);
                sound_init();
                if(!sound_inited) {
                    sound_init();
                    sound_inited = 1;
                }
                print_init();
                player_cursor_init(MTH_FIXED(104), MTH_FIXED(81));
                sound_cdda(2);
                player_cursor_draw();
                draw_response_rate();
                state = TITLE_STATE;
                break;
            case TO_CHAR_SELECT_STATE:
                sprite_deleteall();
                static_screen_init((&charSelectScreen.bg), menu_name, menu_num, menu_pal, cselect_name, cselect_width, cselect_height);
                update_static(&charSelectScreen);
                print_init();
                player_cursor_init(MTH_FIXED(80), MTH_FIXED(55));
                sound_cdda(2);
                player_select_init();

                state = CHAR_SELECT_STATE;
                break;
            case TO_CHAR_NAME_STATE: 
                sprite_deleteall();    
                static_screen_init((&charNameScreen.bg), menu_name, menu_num, menu_pal, cname_name, cname_width, cname_height);
                update_static(&charNameScreen);
                print_init();
                player_cursor_init(MTH_FIXED(64), MTH_FIXED(71));
                sound_cdda(2);
                state = CHAR_NAME_STATE;
                break;
            case TO_OW_STATE:
                sprite_deleteall();
                static_disable();
                print_init();
                ow_layer_init(&(level1.bg_far), owmap_name, owmap_num, owmap_pal, mapPlane, 256, 256);
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
                print_init();
                switch(currentBackDrop) {
                    case GRASS_BACKDROP:
                        static_screen_init((&battleScreen.bg), grass_name, grass_num, grass_pal, bMap_name, bMap_width, bMap_height);
                        break;
                    case MARSH_CAVE_BACKDROP:
                        static_screen_init((&battleScreen.bg), mcave_name, mcave_num, mcave_pal, bMap_name, bMap_width, bMap_height);
                        break;
                    case CAVE_BACKDROP:
                        static_screen_init((&battleScreen.bg), cave_name, cave_num, cave_pal, bMap_name, bMap_width, bMap_height);
                        break;
                    case WATER_BACKDROP:
                        static_screen_init((&battleScreen.bg), water_name, water_num, water_pal, bMap_name, bMap_width, bMap_height);
                        break;
                    case FOREST_BACKDROP:
                        static_screen_init((&battleScreen.bg), forest_name, forest_num, forest_pal, bMap_name, bMap_width, bMap_height);
                        break;
                    case TEMPLE_BACKDROP:
                        static_screen_init((&battleScreen.bg), temple_name, temple_num, temple_pal, bMap_name, bMap_width, bMap_height);
                        break;
                    case DESERT_BACKDROP:
                        static_screen_init((&battleScreen.bg), desert_name, desert_num, desert_pal, bMap_name, bMap_width, bMap_height);
                        break;
                    case MARSH_BACKDROP:
                        static_screen_init((&battleScreen.bg), marsh_name, marsh_num, marsh_pal, bMap_name, bMap_width, bMap_height);
                        break;
                    case EARTH_CAVE_BACKDROP:
                        static_screen_init((&battleScreen.bg), ecave_name, ecave_num, ecave_pal, bMap_name, bMap_width, bMap_height);
                        break;
                    case CASTLE_BAKDROP:
                        static_screen_init((&battleScreen.bg), castle_name, castle_num, castle_pal, bMap_name, bMap_width, bMap_height);
                        break;
                    case RIVER_BACKDROP:
                        static_screen_init((&battleScreen.bg), river_name, river_num, river_pal, bMap_name, bMap_width, bMap_height);
                        break;
                    case TOWER_BACKDROP:
                        static_screen_init((&battleScreen.bg), tower_name, tower_num, tower_pal, bMap_name, bMap_width, bMap_height);;
                        break;
                    case SHRINE_BACKDROP:
                        static_screen_init((&battleScreen.bg), sshrine_name, sshrine_num, sshrine_pal, bMap_name, bMap_width, bMap_height);
                        break;
                    case ICE_CAVE_BACKDROP:
                        static_screen_init((&battleScreen.bg), icave_name, icave_num, icave_pal, bMap_name, bMap_width, bMap_height);
                        break;
                    case VOLCANO_BACKDROP:
                        static_screen_init((&battleScreen.bg), volcano_name, volcano_num, volcano_pal, bMap_name, bMap_width, bMap_height);
                        break;
                    case WATERFALL_BACKDROP:
                        static_screen_init((&battleScreen.bg), waterfall_name, waterfall_num, waterfall_pal, bMap_name, bMap_width, bMap_height);
                        break;
                    default:
                        static_screen_init((&battleScreen.bg), waterfall_name, waterfall_num, waterfall_pal, bMap_name, bMap_width, bMap_height);
                        break;
                }

                static_init(&battleScreen);
                
                player_bat_init(MTH_FIXED(200), MTH_FIXED(40));
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
                player_bat_draw();
                enemy_bat_draw();
                print_display();
                break;
        }

        SPR_2CloseCommand();
        
        SCL_DisplayFrame();
    }
    return 0;
}
