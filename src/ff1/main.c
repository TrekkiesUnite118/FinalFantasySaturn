/*******************************************************************
 *
 *                      Main Loop
 *
 * Loads the NES ROM, sets up the Saturn and runs the game's state
 * machine once per frame. Each TO_ state sets up a screen, then the
 * game moves on to the state that runs it.
 *
*******************************************************************/
#include    <string.h> //memcpy
#include    "types.h"
#include    "sys.h"

#include    "cd.h"
#include    "memmap.h"
#include    "graphicrefs.h"
#include    "nesutils.h"
#include    "sprdata.h"
#include    "sprscreen.h"
#include    "rbgscroll.h"
#include    "static.h"
#include    "overworld.h"
#include    "battle.h"
#include    "player.h"
#include    "print.h"
#include    "menu.h"
#include    "vblank.h"
#include    "minimap.h"
#include    "mainmenu.h"
#include    "bugfix.h"
#include    "stdmap.h"
#include    "shop.h"
#include    "save.h"
#include    "audio.h"
#include    "music.h"
#include    "story.h"
#include    "minigame.h"
#include    "sound.h"
#include    "msxgfx.h"
#include    "gfxmenu.h"

/**
 * @brief Loads the ROM, sets up the game, then runs one state per frame.
 *
 * Goes to the SAD screen if FFANTASY.BIN doesn't match what we expect.
 *
 * @return Never returns.
 */
//Frames after a map rebuild before the display comes back, so the previous screen's sprites are gone.
#define MAP_SHOW_FRAMES (3)

int main() {
    int battle_rng_seeded = 0;
    int encounter_rng_seeded = 0;
    //Blank the BIOS license screen during loading (TVMD DISP off). SCL turns it back on in sprite_init.
    sysDisplayOff();
    cd_init();
    //16-byte iNES header plus 256KB of PRG ROM.
    cd_load("FFANTASY.BIN", (void *)LWRAM, 0x40010);
    msxInit();
    audio_init(musicInit, musicFrame);
    sound_init();

    Uint8 *nes_head_ptr = (Uint8*) LWRAM;
    Uint8 nesHead1 = nes_head_ptr[0];
    Uint8 nesHead2 = nes_head_ptr[1];
    Uint8 nesHead3 = nes_head_ptr[2];
    if(nesHead1 != 0x4E || nesHead2 != 0x45 || nesHead3 != 0x53) {
        state = TO_SAD;
    } else {
        state = TO_PRELUDE;
    }
    Uint8 *battleBackdropPatternData = (Uint8 *)BATTLE_TILE_OFFSET;
    convertNesTilesToSatTiles(battleBackdropPatternData, (Uint8 *)LWRAM_MENU_TILES, 0x800);
    convertNesTilesToSatTiles((Uint8 *)MENU_TILE_OFFSET, (Uint8 *)LWRAM_MENU_TILES + 0x1000, 0x800);
    initMapSpriteData((Uint8 *)MAPMAN_SPRITE_OFFSET);
    initBattleSpriteData((Uint8 *)BATTLE_SPRITE_OFFSET);
    initWeaponSpriteData((Uint8 *)WEAPON_SPRITE_OFFSET);

    sprite_init();
    overworld_init();
    saveInit();
    saveBootCheck();
    if(state == TO_PRELUDE && saveBootMessagePending()) {
        state = TO_BOOT_MSG;
    }

    newGameInit();

    print_init();
    sprite_window_mode();

    //The original game relied on the randomness of NES RAM on
    //Power On. The Saturn clears its RAM on start up, so to
    //try and mimick the NES behavior I'm getting the
    //seconds value from the RTC.
    setRngSeed();

    while(1) {
        frameBegin();
        //Set by A+B+C+Start in player_input.
        if(softResetPending) {
            softResetPending = FALSE;
            softReset();
        }
        updateBattleSprites();
        switch(state) {
            case TO_SAD:
                static_init();
                
                static_disable();
                print_init();
                print_string("SATURN SAD!", 14, 2, 11);
                print_string("SATURN wanted to play Final Fantasy,", 15, 2, 36);
                print_string("but NES ROM is corrupt!", 16, 2, 23);
                print_string("The SH-2s will now weep...", 17, 2, 26);
                sound_cdda(2);
                state = SAD;
                break;
            case SAD:
                print_display();
                break;
            case TO_BOOT_MSG:
                clearMenuPlane();
                setupBootMessage();
                static_init();
                state = BOOT_MSG;
                break;
            case BOOT_MSG:
                break;
            case TO_PRELUDE:
                clearMenuPlane();
                setupPreludeScreen();
                static_init();
                state = PRELUDE;
                //NES IntroTitlePrepare.
                music_track = MUSIC_PRELUDE;
                break;
            case PRELUDE:
                break;
            case TO_TITLE_STATE:
                if(!battle_rng_seeded) {
                    seedBattleRng();
                    battle_rng_seeded = 1;
                }
                clearMenuPlane();
                setupTitleScreen();
                print_init();
                player_cursor_init(FIXED(104), FIXED(88));
                draw_response_rate();
                static_init();
                //NES EnterIntroStory silences the APU on exit, then IntroTitlePrepare restarts the music.
                musicApuWrite(0x4015, 0);
                music_track = MUSIC_PRELUDE;
                player_cursor_draw();
                state = TITLE_STATE;
                break;
            case TO_CHAR_SELECT_STATE:
                clearMenuPlane();
                setupPartyCreateScreen();
                print_init();
                player_cursor_init(FIXED(80), FIXED(55));
                state = CHAR_SELECT_STATE;
                break;
            case TO_CHAR_NAME_STATE: 
                clearMenuPlane();  
                setupNameEntryScreen();
                print_init();
                player_cursor_init(FIXED(64), FIXED(80));
                state = CHAR_NAME_STATE;
                break;
            case TO_OW_STATE:
                //Rebuilding the map takes several frames, so nothing is shown until it's ready.
                sysBlankDisplay();
                if(!encounter_rng_seeded) {
                    seedEncounterCounter();
                    encounter_rng_seeded = 1;
                }
                determine_curr_char();
                update_player_sprites();
                static_disable();
                print_init();
                scroll_init();
                player_ow_enter();
                sysShowDisplay(MAP_SHOW_FRAMES);
                break;
            case WIPE:
            case SM_PAL_CYCLE:
            case OW_PAL_CYCLE:
                //The NES turns sprites off for the palette cycle.
                break;
            case TO_MAIN_MENU:
                scroll_disable();
                clearMenuPlane();
                setupMainMenuScreen();
                initMainMenu();
                if(mmFromMap) {
                    mmFromMap = FALSE;
                    music_track = MUSIC_MENU;
                    musicApuWrite(0x4015, 0);
                }
                static_init();
                state = MAIN_MENU;
                break;
            case TO_LINEUP_MENU:
                scroll_disable();
                clearMenuPlane();
                //NES EnterLineupMenu.
                musicApuWrite(0x4015, 0);
                setupLineupScreen();
                initLineupMenu();
                music_track = MUSIC_CRYSTAL;
                static_init();
                state = LINEUP_MENU;
                break;
            case LINEUP_MENU:
                lineup_draw();
                break;
            case TO_MINIMAP:
                scroll_disable();
                initMinimap();
                state = MINIMAP;
                break;
            case MINIMAP:
                minimap_draw();
                break;
            case MAIN_MENU:
                mainmenu_draw();
                break;
            case TO_SAVE_MENU:
                clearMenuPlane();
                setupSaveScreen();
                static_init();
                state = SAVE_MENU;
                break;
            case SAVE_MENU:
                save_draw();
                break;
            case TO_BUGFIX_MENU:
                scroll_disable();
                clearMenuPlane();
                setupBugFixScreen();
                initBugFixMenu();
                static_init();
                state = BUGFIX_MENU;
                break;
            case BUGFIX_MENU:
                bugfix_draw();
                break;
            case TO_GFX_MENU:
                scroll_disable();
                clearMenuPlane();
                setupGfxScreen();
                initGfxMenu();
                static_init();
                state = GFX_MENU;
                break;
            case GFX_MENU:
                gfxmenu_draw();
                break;
            case TO_STATUS_MENU:
                scroll_disable();
                clearMenuPlane();
                setupStatusScreen();
                initStatusMenu();
                static_init();
                state = STATUS_MENU;
                break;
            case STATUS_MENU:
                status_draw();
                break;
            case TO_MAGIC_MENU:
                scroll_disable();
                setupMagicScreen();
                initMagicMenu();
                static_init();
                state = MAGIC_MENU;
                break;
            case MAGIC_MENU:
                magic_draw();
                break;
            case TO_EQUIP_MENU:
                scroll_disable();
                clearMenuPlane();
                setupEquipScreen();
                initEquipMenu();
                static_init();
                state = EQUIP_MENU;
                break;
            case EQUIP_MENU:
                equip_draw();
                break;
            case TO_ITEM_MENU:
                scroll_disable();
                setupItemScreen();
                initItemMenu();
                static_init();
                state = ITEM_MENU;
                break;
            case ITEM_MENU:
                item_draw();
                break;
            case TO_SM_STATE:
                sysBlankDisplay();
                determine_curr_char();
                scroll_disable();
                static_disable();
                update_player_sprites();
                sm_init(); //Sets the state.
                sysShowDisplay(MAP_SHOW_FRAMES);
                break;
            case SM_STATE:
                sm_draw();
                break;
            case TO_SHOP:
                scroll_disable();
                clearMenuPlane();
                //NES EnterShop.
                musicApuWrite(0x4015, 0);
                setupShopScreen();
                initShop();
                music_track = MUSIC_SHOP;
                static_init_tiles((Uint8 *)LWRAM_BUFFER);
                state = SHOP;
                break;
            case SHOP:
                shop_draw();
                break;
            case TO_STORY:
                scroll_disable();
                clearMenuPlane();
                setupStoryScreen(MSX_ON);
                state = STORY;
                break;
            case STORY:
                story_draw();
                break;
            case TO_MINIGAME:
                scroll_disable();
                clearMenuPlane();
                //NES LoadBridgeSceneGFX.
                storySelect(FALSE);
                setupStoryScreen(MSX_ON);
                setupMinigame();
                state = MINIGAME;
                break;
            case MINIGAME:
                minigame_draw();
                break;
            case TO_BAT_STATE:
                scroll_disable();
                clearMenuPlane();  
                copyPlayerStatsToBattleStats();
                print_init();
                battleBackdropPatternData = (Uint8 *)BATTLE_TILE_OFFSET + currentBackDrop * 0x800;
                convertNesTilesToSatTiles(battleBackdropPatternData, (Uint8 *)LWRAM_MENU_TILES, 0x120);
                setupBattleScreen(currentBackDrop);
                msgReset();
                determineSurprise();
                enemy_bat_draw();
                initBattleFadePalettes(currentBackDrop);
                setBattleShade(4);
                static_init();
                player_bat_init();
                //NES PrepBattleVarsAndEnterBattle.
                btlFollowupMusic = MUSIC_BATTLE;
                music_track = MUSIC_BATTLE;
                battleFadeFrame = 0;
                state = BAT_FADE_IN;
                break;
            case BAT_FADE_IN:
                break;
            case BAT_DRAW_STATUS:
                party_crouch();
                party_dead();
                player_bat_draw();
                break;
            case BAT_FADE_OUT:
                //Sprites stop being drawn once black, so none are left on screen when their palettes are restored.
                if(battleFadeShade < 4) {
                    player_bat_draw();
                }
                break;
            case OW_STATE:
                player_ow_draw();
                break;
            case BAT_TRANSITION:
                //The NES turns sprites off for the transition.
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
                    respondRateCounter = 0;
                    state = BAT_INIT_MSG;
                } else {
                    state = TO_BAT_MENU_STATE;
                }
                break;
            case BAT_INIT_MSG:
                party_crouch();
                party_dead();
                player_bat_draw();
                //NES DrawBtlMsg_ClearCombatBoxes: respond delay, then undraw.
                if(!msgBusy() && respondRateCounter >= response_rate_lut[getResponseRate()]) {
                    clearMessageArea();
                    state = isSurprised ? TO_BAT_EXECUTE_STATE : TO_BAT_MENU_STATE;
                }
                break;
            case TO_BAT_MENU_STATE:
                player_cursor_init(FIXED(120), FIXED(160));
                player_bat_draw();
                clearMessageArea();
                drawMenu();
                state = BAT_MENU_STATE;
                bat_menu_state = INIT;
                break;

            case BAT_MENU_STATE:
                player_bat_draw();
                if(drawBattleCursor == TRUE) {
                    
                   player_cursor_draw();
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
          
                print_display();
                break;
            case TO_BAT_VICTORY_STATE:
                copyBattleStatsToPlayerStats();
                updateAilmentandHP();
                player_bat_draw();
                clearMessageArea();
                print_display();
                initBattleEnd(BATTLE_END_CHEER);
                state = BAT_VICTORY_STATE;
                break;
            case BAT_VICTORY_STATE:
                updateAilmentandHP();
                player_bat_draw();
                print_display();
                break;
            case TO_GAME_OVER:
                player_bat_draw();
                updateAilmentandHP();
                clearMessageArea();
                //NES GameOver.
                btlFollowupMusic = MUSIC_GAME_OVER;
                music_track = MUSIC_GAME_OVER;
                drawPartyPerishedMessage();

                print_display();
                initBattleEnd(BATTLE_END_WAIT_INPUT);
                state = GAME_OVER;
                break;
            case GAME_OVER:
                player_bat_draw();
                updateAilmentandHP();
          
                print_display();
                break;
            case GAME_RESTART:
                static_disable();
                print_init();
                newGameInit();
                state = TO_TITLE_STATE;
                break;
            case TO_BAT_RUN_AWAY:
                copyBattleStatsToPlayerStats();
                player_bat_draw();
                updateAilmentandHP();
                //The attacker's name box stays up with the message, and the party flips after it's erased.
                drawRunSuccessMessage();
                print_display();
                initBattleEnd(BATTLE_END_DELAY);
                state = BAT_RUN_AWAY;
                break;
            case BAT_RUN_AWAY:
                player_bat_draw();
                updateAilmentandHP();
          
                print_display();
                break;
        }

        frameEnd();
    }
    return 0;
}

