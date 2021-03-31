#include    <string.h> //memcpy
#define        _SPR2_
#include    <sega_spr.h>
#include    <sega_scl.h> 
#include    <sega_mth.h>
#include     <sega_def.h>

#include    "cd.h"
#include    "graphicrefs.h"
#include     "vdp2structs.h"
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
    state = TO_OW_STATE;
    sprite_init();
    overworld_init();
    
    LEVEL level1;
    STATIC_SCREEN battleScreen;
    level1.player_startx = MTH_FIXED(2296);
    level1.player_starty = MTH_FIXED(2545);

    print_init();
    player_ow_init(level1.player_startx, level1.player_starty);
    char name1[6] = "TREKKY";
    char name2[6] = "PONUT ";
    char name3[6] = "XL2   ";
    char name4[6] = "VBT   ";
    char_init(0, 0, name1);
    char_init(1, 1, name2);
    char_init(4, 2, name3);
    char_init(5, 3, name4);

    player_bat_init(MTH_FIXED(200), MTH_FIXED(40));
    
    
    SCL_SetSpriteMode(SCL_TYPE5,SCL_MIX,SCL_SP_WINDOW);

    while(1) {
        
        
        frame++;
        player_input();
        //print_num(frame, 0, 0);
        
        SPR_2OpenCommand(SPR_2DRAW_PRTY_OFF);
        switch(state) {
            case TO_OW_STATE:
                
                static_disable();
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
                scroll_disable();
                
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
                
                sound_cdda(4);
                state = BAT_STATE;
                break;
            case OW_STATE:
                player_ow_draw();
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
