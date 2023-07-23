#include <tonc.h>
#include <string.h>

#include "audio.h"
#include "main.h"
#include "intro.h"
#include "menu.h"
#include "game.h"
#include "input.h"

// global app struct
struct app App;

void default_text_margins() {
    tte_set_margins(8, 8, SCREEN_WIDTH-8, SCREEN_HEIGHT-8);
}

void setup_display(bool background, bool text, bool objects) {
    REG_DISPCNT = DCNT_MODE0;
    if(background) REG_DISPCNT |= DCNT_BG2;
    if(text) REG_DISPCNT |= DCNT_BG3;
    if(objects) REG_DISPCNT |= DCNT_OBJ | DCNT_OBJ_1D;
}

void reset_windows() {
    REG_DISPCNT &= ~(DCNT_WIN0|DCNT_WIN1);

    REG_WIN0H = 0;
    REG_WIN0V =  0;
    REG_WIN0CNT = 0;
    REG_WIN1H = 0;
    REG_WIN1V =  0;
    REG_WIN1CNT = 0;

    REG_WINOUTCNT = 0;

    REG_WININ = WININ_BUILD(0, 0);
    REG_WINOUT = WINOUT_BUILD(0, 0);

    REG_BLDCNT = 0;
    REG_BLDY = 0;
}

void set_windows(int x0, int y0, int w0, int h0, int x1, int y1, int w1, int h1, int bldy) {
    REG_DISPCNT |= DCNT_WIN0|DCNT_WIN1;

    REG_WIN0H = x0<<8 | (x0+w0);
    REG_WIN0V =  y0<<8 | (y0+h0);
    REG_WIN0CNT = WIN_ALL|WIN_BLD;

    REG_WIN1H = x1<<8 | (x1+w1);
    REG_WIN1V =  y1<<8 | (y1+h1);
    REG_WIN1CNT = WIN_ALL|WIN_BLD;

    REG_WINOUTCNT = WIN_ALL;

    REG_WININ = WININ_BUILD(WIN_BG2|WIN_BG3|WIN_BLD, WIN_BG2|WIN_BG3|WIN_BLD);
    REG_WINOUT = WINOUT_BUILD(WIN_BG2|WIN_BG3, WIN_BG2|WIN_BG3);

    REG_BLDCNT = (BLD_ALL&~BIT(3))|BLD_BLACK;
    REG_BLDY = bldy;
}


void init() {
    // setup graphics, interrupts, etc.
    setup_display(false, true, false);
    IRQ_INIT();
    irq_enable(II_VBLANK);
    sqran(2310);

    // use last background and last screen block for text
    tte_init_se(3, BG_CBB(0)|BG_SBB(31), 0, CLR_YELLOW, 14, NULL, NULL);
    tte_init_con();
    default_text_margins();
    // move into foreground
    REG_BG3CNT |= BG_PRIO(0);

    // font palette
    pal_bg_bank[1][15]= CLR_RED;
    pal_bg_bank[2][15]= CLR_GREEN;
    pal_bg_bank[3][15]= CLR_BLUE;
    pal_bg_bank[4][15]= CLR_WHITE;
    pal_bg_bank[5][15]= CLR_MAG;

    pal_bg_bank[1][14]= CLR_GRAY;
    pal_bg_bank[2][14]= CLR_GRAY;
    pal_bg_bank[3][14]= CLR_GRAY;
    pal_bg_bank[4][14]= CLR_GRAY;
    pal_bg_bank[5][14]= CLR_GRAY;

    audio_init();

    memset(&App, 0, sizeof(struct app));

    // start with intro
    init_intro();
}

int main() {
    init();

    // main loop
    while(1) {
        VBlankIntrWait();

        // update
        audio_frame();
        input_update();
        App.delegate.logic();
        App.delegate.draw();
    }

    // end
    return 0;
}
