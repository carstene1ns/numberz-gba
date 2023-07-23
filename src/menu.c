#include "main.h"
#include "menu.h"
#include "intro.h"
#include "credits.h"
#include "game.h"
#include "audio.h"

// GLOBALS
static int current_index = 0;
static int num_options = 0;
const static MenuOption *current_options;
static void (*additional_drawing)() = NULL;
static void (*additional_logic)() = NULL;

static struct {
    int music;
    int sfx;
    bool mute_music;
    bool mute_sfx;
} volume = { 4, 5, false, false };

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

// PROTOTYPES
static void menu_intro_main();
static void menu_intro_start();
static void menu_intro_options();
static void start_game();
static void back_to_intro();

static void menu_pause_main();
static void menu_pause_options();
static void back_to_game();
static void restart_and_back_to_game();

static void draw();
static void select();
static void move_up();
static void move_down();
static void reset_menu();

static void audio_logic();
static void draw_audio_options();
static void toggle_music();
static void toggle_sounds();

const static MenuOption options_intro[] = {
    { 80, 60, "Start", menu_intro_start },
    { 80, 80, "Options", menu_intro_options },
    { 80, 100, "Credits", init_credits },
    { 80, 120, "back to Title", back_to_intro },
};
const static MenuOption options_intro_start[] = {
    { 80, 60, "Begin", start_game },
    { 80, 80, "Password", NULL },
    { 80, 100, "back", menu_intro_main },
};
const static MenuOption options_intro_options[] = {
    { 80, 60, "Mode", NULL },
    { 80, 80, "Music", toggle_music },
    { 80, 100, "Sounds", toggle_sounds },
    { 80, 120, "back", menu_intro_main },
};

const static MenuOption options_pause[] = {
    { 40, 60, "Continue", back_to_game },
    { 40, 72, "Options", menu_pause_options },
    { 40, 86, "Restart", restart_and_back_to_game },
    { 40, 100, "back to Title", init_intro },
};
const static MenuOption options_pause_options[] = {
    { 40, 60, "Mode", NULL },
    { 40, 80, "Music", toggle_music },
    { 40, 100, "Sounds", toggle_sounds },
    { 40, 120, "back", menu_pause_main },
};

// GAME
static void pause_logic() {
    if (key_hit(KEY_UP))
        move_up();
    if (key_hit(KEY_DOWN))
        move_down();
    if (key_hit(KEY_A))
        select();

    // return to game
    if (key_hit(KEY_START))
        back_to_game();
}

void pause_menu() {
    // init state
    App.delegate.logic = pause_logic;
    App.delegate.draw = draw;

    pause_music(true);

    set_windows(16, 36, 144, 92, 0, 0, 0, 0, 10);
    menu_pause_main();
}

static void menu_pause_main() {
    num_options = ARRAY_SIZE(options_pause);
    current_options = options_pause;
    reset_menu();
    additional_drawing = draw_stats;
}

static void menu_pause_options() {
    num_options = ARRAY_SIZE(options_pause_options);
    current_options = options_pause_options;
    reset_menu();
    //additional_logic = audio_logic;
    additional_logic = move_intro_bg;
    additional_drawing = draw_audio_options;
}

static void restart_and_back_to_game() {
    restart_game();
    back_to_game();
}

static void back_to_game() {
    default_text_margins();
    restore_game_state();
    reset_windows();
    setup_blending();
    pause_music(false);
}

// INTRO
static void intro_logic() {
    if (key_hit(KEY_UP))
        move_up();
    if (key_hit(KEY_DOWN))
        move_down();
    if (key_hit(KEY_A))
        select();

    if (key_hit(KEY_START))
        back_to_intro();

    if(additional_logic != NULL)
        additional_logic();
}

void intro_menu() {
    // init state
    App.delegate.logic = intro_logic;
    App.delegate.draw = draw;

    menu_intro_main();
}

static void back_to_intro() {
    default_text_margins();
    restore_intro_state();
}

static void menu_intro_main() {
    num_options = ARRAY_SIZE(options_intro);
    current_options = options_intro;
    set_windows(60, 50, 130, 90, 0, 0, 0, 0, 10);
    reset_menu();
    additional_logic = move_intro_bg;
}

static void menu_intro_start() {
    num_options = ARRAY_SIZE(options_intro_start);
    current_options = options_intro_start;
    set_windows(60, 50, 90, 60, 0, 0, 0, 0, 10);
    reset_menu();
    additional_logic = move_intro_bg;
}

static void start_game() {
    reset_menu();
    init_game(GAME_ORIGINAL, 1);
}

static void menu_intro_options() {
    num_options = ARRAY_SIZE(options_intro_options);
    current_options = options_intro_options;
    reset_menu();
    //additional_logic = audio_logic;
    additional_logic = move_intro_bg;
    additional_drawing = draw_audio_options;
}

// COMMON
static void draw() {
    tte_erase_screen();
    for(int i = 0; i < num_options; i++){
        MenuOption o = current_options[i];
        if (i == current_index) {
            tte_set_pos(o.x-10, o.y);
            tte_write("#{cx:0x4000}=>#{cx:0}");
        }
        tte_set_pos(o.x, o.y);
        if (o.func == NULL)
            tte_printf("#{cx:0x3000}%s#{cx:0}", o.label);
        else
            tte_write(o.label);
    }

    if(additional_drawing != NULL)
        additional_drawing();
}

static void select() {
    // currently not implemented
    if(current_options[current_index].func == NULL) {
        play_sfx(SOUND_FAIL);
        return;
    }

    play_sfx(SOUND_SELECT);
    current_options[current_index].func();
}

static void move_up(){
    if(current_index > 0){
        current_index--;
    } else {
        current_index = num_options - 1;
    }
}

static void move_down(){
    if(current_index < num_options - 1){
        current_index++;
    } else {
        current_index = 0;
    }
}

static void reset_menu(){
    current_index = 0;
    additional_drawing = NULL;
    additional_logic = NULL;
}

static void audio_logic() {
    int sfx_add = 0;
    int mus_add = 0;
    if (key_hit(KEY_LEFT)) {
        if(current_index == 1) {
            mus_add--;
        } else if (current_index == 2) {
            sfx_add--;
        }
    }
    if (key_hit(KEY_RIGHT)) {
        if(current_index == 1) {
            mus_add++;
        } else if (current_index == 2) {
            sfx_add++;
        }
    }

    // set
    volume.music += mus_add;
    volume.sfx += sfx_add;
    // sanitize
    volume.music = CLAMP(volume.music, 0, 6);
    volume.sfx = CLAMP(volume.sfx, 0, 6);
    // apply
    volume_music(volume.music);
    volume_sfx(volume.sfx);
}

static void draw_audio_options() {
#define TOP 80
#define LEFT 130
    // music
    if(volume.mute_music) {
        tte_set_pos(LEFT, TOP);
        tte_write("-----");
    } else {
        for(int i = 0; i < volume.music; i++){
            tte_set_pos(LEFT+i*8, TOP);
            tte_write("o");
        }
    }
    // sound
    if(volume.mute_sfx) {
        tte_set_pos(LEFT, TOP+20);
        tte_write("-----");
    } else {
        for(int i = 0; i < volume.sfx; i++){
            tte_set_pos(LEFT+i*8, TOP+20);
            tte_write("o");
        }
    }
#undef TOP
#undef LEFT
}

static void toggle_music() {
    volume.mute_music = !volume.mute_music;
    mute_music(volume.mute_music);
}

static void toggle_sounds() {
    volume.mute_sfx = !volume.mute_sfx;
    mute_sfx(volume.mute_sfx);
}
