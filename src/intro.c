#include <string.h>

#include "main.h"
#include "intro.h"
#include "input.h"
#include "menu.h"
#include "game.h"
#include "audio.h"

#include "bg-intro.h"

static int x, y;
static int bg_tick, bg_dir;

static void logic() {
    // return from intro
    if (key_hit(KEY_START)) {
        reset_windows();
        intro_menu();
    }

    move_intro_bg();
}

static void draw() {
    tte_erase_screen();

    tte_set_pos(90, 50);
    tte_write("numberz");
    tte_set_pos(90, 60);
    tte_write("#{cx:0x6000}2umb3rz");
    tte_set_pos(90, 70);
    tte_write("#{cx:0x7000}number7");
    tte_set_pos(90, 78);
    tte_write("#{cx:0x8000}2um43r7");
    tte_set_pos(90, 87);
    tte_write("#{cx:0x4000}v " VERSION);
    tte_set_pos(75, 120);
    tte_write("#{cx:0x3000}Press Start!#{cx:0}");
}

void move_intro_bg() {
    if(bg_tick & 10) {
        bg_tick = 0;
        x += 1 * bg_dir;
        y += 2 * bg_dir;
        if (x > 256 || y > 512 || x < 0 || y < 0)
            bg_dir *= -1;
        REG_BG2HOFS = x;
        REG_BG2VOFS = y;
    }
    bg_tick++;
}

void restore_intro_state() {
    // init state
    App.delegate.logic = logic;
    App.delegate.draw = draw;

    setup_display(true, true, false);

    default_text_margins();
    tte_erase_screen();
    set_windows(86, 46, 61, 46, 70, 119, 98, 10, 10);
}

void init_intro() {
    restore_intro_state();

    // Background
    // Load palette
    GRIT_CPY(pal_bg_mem, bg_introPal);
    // Load tiles into CBB 1
    LZ77UnCompVram(bg_introTiles, tile_mem[1]);
    // Load map into SBB 16
    LZ77UnCompVram(bg_introMap, se_mem[16]);
    // set up BG2 for a 4bpp 32x32t map, using charblock 1 and screenblock 16
    REG_BG2CNT = BG_CBB(1) | BG_SBB(16) | BG_4BPP | BG_REG_32x32 | BG_PRIO(3);

    // text palette
    pal_bg_bank[6][15] = RGB15(28, 25, 23);
    pal_bg_bank[7][15] = RGB15(15, 18, 13);
    pal_bg_bank[8][15] = RGB15(3, 5, 8);

    play_music(MUSIC_INTRO);

    x = 0;
    y = 0;
    bg_tick = 0;
    bg_dir = 1;
}
