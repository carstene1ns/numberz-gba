#include <string.h>

#include "main.h"
#include "credits.h"
#include "input.h"
#include "intro.h"

static int credits_tick, page, last_page;

static void logic() {
    // return to intro
    if (key_hit(KEY_START))
        restore_intro_state();
}

static void draw() {
    if(credits_tick > 200) {
        credits_tick = 0;
        page ^= 1;
    }
    credits_tick++;

    if (page == last_page) return;
    last_page = page;
    tte_erase_screen();

    if(!page) {
        tte_set_pos(16, 12);
        tte_write("Software used:");
        tte_set_pos(32, 32);
        tte_write("#{cx:0x1000}tonclib#{cx:0} by Jasper Vijn");
        tte_set_pos(32, 40);
        tte_write("#{cx:0x2000}maxmod#{cx:0} by Mukunda Johnson");
        tte_set_pos(32, 48);
        tte_write("#{cx:0x3000}posprintf#{cx:0} by Dan Posluns");
        tte_set_pos(16, 78);
        tte_write("Original Game:");
        tte_set_pos(32, 98);
        tte_write("#{cx:0x4000}zNumbers#{cx:0} by Karl Bartel");
    } else {
        tte_set_pos(16, 12);
        tte_write("Music:");
        tte_set_pos(32, 32);
        tte_write("#{cx:0x1000}a short Tune#{cx:0} by sit");
        tte_set_pos(32, 40);
        tte_write("#{cx:0x2000}Tranze seven#{cx:0}");
        tte_set_pos(96, 48);
        tte_write(" by dr.awesome");
        tte_set_pos(16, 78);
        tte_write("Shoutouts to:");
        tte_set_pos(32, 98);
        tte_write("#{cx:0x4000}gbadev.org, devkitpro.org#{cx:0}");
        tte_set_pos(32, 106);
        tte_write("and all homebrewers");
    }
    tte_set_pos(16, 140);
    tte_write("#{cx:0x5000}made with #{cx:0x1000}<3#{cx:0x5000} by carstene1ns#{cx:0}");
}

void init_credits() {
    // init state
    App.delegate.logic = logic;
    App.delegate.draw = draw;

    default_text_margins();
    tte_erase_screen();

    setup_display(false, true, false);

    credits_tick = 0;
    page = 0;
    last_page = -1;
}
