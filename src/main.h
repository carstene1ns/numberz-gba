#pragma once

#include <tonc.h>

// global app struct
struct app {
    struct {
        void (*logic)(void);
        void (*draw)(void);
    } delegate;
};
extern struct app App;

void default_text_margins();

void setup_display(bool background, bool text, bool objects);

void set_windows(int x0, int y0, int w0, int h0, int x1, int y1, int w1, int h1, int bldy);

void reset_windows();

#define VERSION "0.1.0"
