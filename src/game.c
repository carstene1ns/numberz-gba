#include <string.h>
#include <assert.h>
#include "posprintf.h" // ext

#include "main.h"
#include "input.h"
#include "game.h"
#include "intro.h"
#include "menu.h"
#include "levels.h"
#include "audio.h"
#include "menu.h"

// from grit
#include "bg-game.h"
#include "ts-game.h"

static OBJ_ATTR obj_buffer[128];

enum state { GAME_SELECT_SRC, GAME_SELECT_TGT, GAME_ANIMATE, GAME_UNDO };

const char* gamemode_desc[] = { "orig", "norm", "hard", "time" };

static struct {
    const char *gamemode_desc;
    int level;
    int level_max;
    const u8 (*levels)[LEVEL_ROWS][LEVEL_COLS];
    const char** passwords;
    int move;
    int points;
    int max_moves;
    struct {
        int x, y;
    } target;
    struct {
        int x, y, num;
    } source;
    struct {
        int x, y, num, points;
        bool enabled;
    } undo;
    struct {
        int x, y;
    } cursor;
    struct {
        int x, y, inc_x, inc_y;
    } anim;
    enum state state;
} game_vars = {
    "", 0, LEVEL_MAX_ORIGINAL, levels_original, passwords_original,
    0, 0, 0,
    { 0, 0 }, //tgt
    { 0, 0, 0 }, //src
    { 0, 0, 0, 0, false },//und
    { 0, 0 }, //cur
    { 0, 0, 0, 0 }, //anim
    0
};

static u8 current_level[LEVEL_ROWS][LEVEL_COLS] = {0};

static int stat_tick = 0;
static int anim_tick = 0;
static int curs_tick = 0;
static int curs_bank = 0;
static bool need_refresh = true;

#define TEXT_LEFT 180
#define DRAW_OFFSET 3
#define NUM_BLOCKS 36
#define BLOCK_SIZE 24
#define BLOCK_BORDER 2
#define BLOCK_OFFSET (BLOCK_SIZE+BLOCK_BORDER)

#define TILE_CURSOR 0
#define TILE_ANIMATE1 2
#define TILE_ANIMATE2 1
#define TILE_SPECIALEND 3
#define OBJECTS_UI TILE_SPECIALEND
#define OBJECTS_GAME NUM_BLOCKS * 2 + TILE_SPECIALEND // block, number, special

#define ID_EMPTY 0
#define ID_SPECIAL 10
#define ID_TARGET 20

static void make_palette() {
    //bank 1 = red
    pal_obj_bank[1][8]= RGB15(31,  0,  0);
    pal_obj_bank[1][6]= RGB15(28,  0,  0);
    pal_obj_bank[1][1]= RGB15(22,  0,  0);
    pal_obj_bank[1][2]= RGB15(16,  0,  0);
    pal_obj_bank[1][7]= RGB15(9,  0,  0);
    //bank 2 = green
    pal_obj_bank[2][8]= RGB15( 0, 31,  0);
    pal_obj_bank[2][6]= RGB15( 0, 28,  0);
    pal_obj_bank[2][1]= RGB15( 0, 22,  0);
    pal_obj_bank[2][2]= RGB15( 0, 16,  0);
    pal_obj_bank[2][7]= RGB15( 0, 9,  0);
    //bank 3 = blue
    pal_obj_bank[3][8]= RGB15( 0,  0, 31);
    pal_obj_bank[3][6]= RGB15( 0,  0, 28);
    pal_obj_bank[3][1]= RGB15( 0,  0, 22);
    pal_obj_bank[3][2]= RGB15( 0,  0, 16);
    pal_obj_bank[3][7]= RGB15( 0,  0, 9);
    //bank 4 = pink
    pal_obj_bank[4][8]= RGB15( 31,  0, 31);
    pal_obj_bank[4][6]= RGB15( 28,  0, 28);
    pal_obj_bank[4][1]= RGB15( 22,  0, 22);
    pal_obj_bank[4][2]= RGB15( 16,  0, 16);
    pal_obj_bank[4][7]= RGB15( 9,  0, 9);
    //bank 5 = gray
    pal_obj_bank[5][8]= RGB15(16, 16, 16);
    pal_obj_bank[5][6]= RGB15(16, 16, 16);
    pal_obj_bank[5][1]= RGB15(16, 16, 16);
    pal_obj_bank[5][2]= RGB15(16, 16, 16);
    pal_obj_bank[5][7]= RGB15(16, 16, 16);
    //bank 6 = white
    pal_obj_bank[6][8]= RGB15(31, 31, 31);
    pal_obj_bank[6][6]= RGB15(31, 31, 31);
    pal_obj_bank[6][1]= RGB15(31, 31, 31);
    pal_obj_bank[6][2]= RGB15(31, 31, 31);
    pal_obj_bank[6][7]= RGB15(31, 31, 31);
}

void setup_blending() {
    // blend animations
    REG_BLDCNT= BLD_BUILD(BLD_OBJ, BLD_BG2, BLD_STD);
    REG_BLDALPHA= BLDA_BUILD(0x80/8, 0x10/8);
    REG_BLDY= BLDY_BUILD(0x70/8);
}

static void setup_level(enum GameMode mode, int start_level) {
    switch (mode) {
        case GAME_ORIGINAL:
            game_vars.level_max = LEVEL_MAX_ORIGINAL;
            game_vars.levels = levels_original;
            game_vars.passwords = passwords_original;
            break;
        case GAME_NORMAL:
            game_vars.level_max = LEVEL_MAX_NORMAL;
            game_vars.levels = levels_normal;
            game_vars.passwords = passwords_normal;
            break;
        case GAME_HARD:
            game_vars.level_max = LEVEL_MAX_HARD;
            game_vars.levels = levels_hard;
            game_vars.passwords = passwords_hard;
            break;
        default:
            assert(0 && "level mode not handled");
            break;
    }
    game_vars.gamemode_desc = gamemode_desc[mode];
    game_vars.level = start_level;
}

static void load_level() {
    // copy from static data
    memcpy(current_level, game_vars.levels[game_vars.level-1], sizeof(current_level));

    // game has 36 fields
    for(int i = 0; i < LEVEL_ROWS; i++) {
        for(int j = 0; j < LEVEL_COLS; j++) {
            // block
            OBJ_ATTR *blk = &obj_buffer[TILE_SPECIALEND+i*6+j];
            obj_set_attr(blk,
                ATTR0_REG|ATTR0_4BPP|ATTR0_SQUARE,
                ATTR1_SIZE_32,
                ATTR2_PRIO(2) | ATTR2_PALBANK(current_level[i][j]) | 0); // colored bank 0-5, tile 0 is the block
            obj_set_pos(blk, DRAW_OFFSET + j * BLOCK_OFFSET, DRAW_OFFSET + i * BLOCK_OFFSET);

            // number on top
            OBJ_ATTR *num = &obj_buffer[TILE_SPECIALEND+NUM_BLOCKS+i*6+j];
            obj_set_attr(num,
                ATTR0_REG|ATTR0_4BPP|ATTR0_SQUARE,
                ATTR1_SIZE_8,
                ATTR2_PRIO(1) | ATTR2_PALBANK(6) | (current_level[i][j] + 64 + 16)); // bank 6 is white, get tile with number
            int offset = DRAW_OFFSET + BLOCK_SIZE/3;
            obj_set_pos(num, offset + j * BLOCK_OFFSET, offset + i * BLOCK_OFFSET);

            // hide, if empty or add to move counter
            if(current_level[i][j] == ID_EMPTY) {
                obj_hide(blk);
                obj_hide(num);
            } else {
                game_vars.max_moves++;
            }
        }
    }
}

static inline OBJ_ATTR* get_sprite_from_block(int x, int y, bool num) {
    return &obj_buffer[TILE_SPECIALEND + (num ? NUM_BLOCKS : 0) + y * 6 + x];
}

void set_animation_block(bool show, bool set_palbank, int palbank, bool set_number, int number) {
    OBJ_ATTR *anim = &obj_buffer[TILE_ANIMATE1];
    OBJ_ATTR *anim_num = &obj_buffer[TILE_ANIMATE2];

    if(set_palbank) {
        BFN_SET(anim->attr2, palbank, ATTR2_PALBANK);
    }

    if(set_number) {
        BFN_SET(anim_num->attr2, number + 64 + 16, ATTR2_ID);
    }

    if (show) {
        obj_unhide(anim, ATTR0_REG);
        obj_unhide(anim_num, ATTR0_REG);
    } else {
        obj_hide(anim);
        obj_hide(anim_num);
    }
}

void move_animation_block(int x, int y) {
    OBJ_ATTR *anim = &obj_buffer[TILE_ANIMATE1];
    OBJ_ATTR *anim_num = &obj_buffer[TILE_ANIMATE2];

    obj_set_pos(anim, DRAW_OFFSET + x, DRAW_OFFSET + y);
    int offset = DRAW_OFFSET + BLOCK_SIZE/3;
    obj_set_pos(anim_num, offset + x, offset + y);
}

void show_block(int x, int y, bool show) {
    OBJ_ATTR *blk = get_sprite_from_block(x, y, false);
    OBJ_ATTR *blk_num = get_sprite_from_block(x, y, true);

    if (show) {
        obj_unhide(blk, ATTR0_REG);
        obj_unhide(blk_num, ATTR0_REG);
    } else {
        obj_hide(blk);
        obj_hide(blk_num);
    }
}

void set_block_bank(int x, int y, int palbank_blk, int palbank_num) {
    OBJ_ATTR *blk = get_sprite_from_block(x, y, false);
    OBJ_ATTR *blk_num = get_sprite_from_block(x, y, true);

    BFN_SET(blk->attr2, palbank_blk, ATTR2_PALBANK);
    BFN_SET(blk_num->attr2, palbank_num, ATTR2_PALBANK);
}

void set_block_num(int x, int y, int num_blk, int num_num) {
    OBJ_ATTR *blk = get_sprite_from_block(x, y, false);
    OBJ_ATTR *blk_num = get_sprite_from_block(x, y, true);

    BFN_SET(blk->attr2, num_blk, ATTR2_ID);
    BFN_SET(blk_num->attr2, num_num + 64 + 16, ATTR2_ID);
}

static void animate_cursor() {
    curs_tick++;
    if(curs_tick & 400) {
        if (curs_bank == 6)
            curs_bank = 0;
        else
            curs_bank = 6;
        BFN_SET((&obj_buffer[TILE_CURSOR])->attr2, curs_bank, ATTR2_PALBANK);
        curs_tick = 0;
    }
}

static void move_cursor() {
    game_vars.cursor.x -= direction_hold(DIRECTION_LEFT);
    game_vars.cursor.x += direction_hold(DIRECTION_RIGHT);
    game_vars.cursor.y -= direction_hold(DIRECTION_UP);
    game_vars.cursor.y += direction_hold(DIRECTION_DOWN);

    game_vars.cursor.x = clamp(game_vars.cursor.x, 0, 6);
    game_vars.cursor.y = clamp(game_vars.cursor.y, 0, 6);

    obj_set_pos(&obj_buffer[TILE_CURSOR], game_vars.cursor.x * BLOCK_OFFSET + BLOCK_BORDER,
                game_vars.cursor.y * BLOCK_OFFSET + BLOCK_BORDER);
}

static void hide_cursor(bool hide) {
    OBJ_ATTR *cur= &obj_buffer[TILE_CURSOR];
    if(hide)
        obj_hide(cur);
    else
        obj_unhide(cur, ATTR0_REG);
}

static void mark_targets(int x, int y, bool mark) {
    // check all 36 fields
    for(int i = 0; i < LEVEL_ROWS; i++) {
        for(int j = 0; j < LEVEL_COLS; j++) {
            // only consider free fields
            if (mark && (current_level[i][j] == ID_EMPTY)) {
                if(
                    // vertical
                    (y == i && ABS(x - j) == game_vars.source.num)
                    // horizontal
                    || (x == j && ABS(y - i) == game_vars.source.num)
                    // diagonal
                    || (ABS(x - j) == game_vars.source.num && ABS(y - i) == game_vars.source.num)
                ) {
                    current_level[i][j] = ID_TARGET;
                    obj_unhide(get_sprite_from_block(j, i, false), ATTR0_REG);
                }
            // only consider target fields
            } else if(!mark && (current_level[i][j] == ID_TARGET)) {
                current_level[i][j] = ID_EMPTY;
                obj_hide(get_sprite_from_block(j, i, false));
            }
        }
    }
}

static void unselect_block() {
    int x = game_vars.source.x;
    int y = game_vars.source.y;
    int num = game_vars.source.num;

    // turn palettes back
    set_block_bank(x, y, num, 6);
    mark_targets(x, y, false);

    play_sfx(SOUND_DESELECT);

    game_vars.state = GAME_SELECT_SRC;
    need_refresh = true;
}

static void select_block_src(int x, int y) {
    // check if selectable
    if(current_level[y][x] == ID_EMPTY || current_level[y][x] > ID_SPECIAL) {
        play_sfx(SOUND_FAIL);
        return;
    }

    // set source
    game_vars.source.x = x;
    game_vars.source.y = y;
    game_vars.source.num = current_level[y][x];

    // turn palettes
    set_block_bank(x, y, 0, game_vars.source.num);

    // show all targets
    mark_targets(x, y, true);

    play_sfx(SOUND_SELECT);

    game_vars.state = GAME_SELECT_TGT;
    need_refresh = true;
}

static void select_block_tgt(int x, int y) {
    // check if same
    if(y == game_vars.source.y && x == game_vars.source.x) {
        unselect_block();
        return;
    }
    // check if selectable
    if(current_level[y][x] != ID_TARGET) {
        play_sfx(SOUND_FAIL);
        return;
    }

    game_vars.undo.enabled = false;

    // set target
    game_vars.target.x = x;
    game_vars.target.y = y;

    // hide all targets and source
    mark_targets(game_vars.source.x, game_vars.source.y, false);
    show_block(game_vars.source.x, game_vars.source.y, false);

    // setup animation
    int num = current_level[game_vars.source.y][game_vars.source.x];
    anim_tick = num * BLOCK_OFFSET;
    if(num > 2) anim_tick /= 2;
    set_animation_block(true, true, num, true, num);
    game_vars.anim.inc_x = ((x - game_vars.source.x) * BLOCK_OFFSET) / anim_tick;
    game_vars.anim.inc_y = ((y - game_vars.source.y) * BLOCK_OFFSET) / anim_tick;
    game_vars.anim.x = game_vars.source.x * BLOCK_OFFSET - game_vars.anim.inc_x;
    game_vars.anim.y = game_vars.source.y * BLOCK_OFFSET - game_vars.anim.inc_y;
    move_animation_block(game_vars.anim.x, game_vars.anim.y);

    play_sfx(SOUND_ANIMATE);

    game_vars.state = GAME_ANIMATE;
    hide_cursor(true);
    need_refresh = true;
}

static void animate() {
    game_vars.anim.x += game_vars.anim.inc_x;
    game_vars.anim.y += game_vars.anim.inc_y;
    move_animation_block(game_vars.anim.x, game_vars.anim.y);

    anim_tick--;
    need_refresh = true;
}

static void end_turn() {
    int x = game_vars.target.x;
    int y = game_vars.target.y;

    // move block to end position
    move_animation_block(x, y);
    set_animation_block(false, false, 0, false, 0);

    // mark target done
    int num = current_level[game_vars.source.y][game_vars.source.x];
    current_level[y][x] = ID_SPECIAL + num;
    show_block(x, y, true);
    set_block_bank(x, y, 5, num);
    set_block_num(x, y, 0, current_level[y][x] - ID_SPECIAL);

    // mark source free
    x = game_vars.source.x;
    y = game_vars.source.y;
    current_level[y][x] = ID_EMPTY;
    set_block_bank(x, y, 0, 6);
    set_block_num(x, y, 0, 0);

    // end of animation
    game_vars.state = GAME_SELECT_SRC;
    // enable undo
    game_vars.undo.enabled = true;
    game_vars.undo.x = game_vars.source.x;
    game_vars.undo.y = game_vars.source.y;
    game_vars.undo.num = game_vars.source.num;
    game_vars.undo.points = game_vars.source.num;

    hide_cursor(false);
    game_vars.move++;
    game_vars.points += game_vars.source.num;
    need_refresh = true;
}

static void check_undo() {
    if (!game_vars.undo.enabled) {
        play_sfx(SOUND_FAIL);
        return;
    }
    game_vars.undo.enabled = false;

    // hide target
    show_block(game_vars.target.x, game_vars.target.y, false);

    // setup animation
    anim_tick = game_vars.undo.num * BLOCK_OFFSET;
    if(game_vars.undo.num > 2) anim_tick /= 2;
    set_animation_block(true, true, game_vars.undo.num, true, game_vars.undo.num);
    game_vars.anim.inc_x = ((game_vars.undo.x - game_vars.target.x) * BLOCK_OFFSET) / anim_tick;
    game_vars.anim.inc_y = ((game_vars.undo.y - game_vars.target.y) * BLOCK_OFFSET) / anim_tick;
    game_vars.anim.x = game_vars.target.x * BLOCK_OFFSET - game_vars.anim.inc_x;
    game_vars.anim.y = game_vars.target.y * BLOCK_OFFSET - game_vars.anim.inc_y;
    move_animation_block(game_vars.anim.x, game_vars.anim.y);

    play_sfx(SOUND_DESELECT);

    game_vars.state = GAME_UNDO;
    hide_cursor(true);
    need_refresh = true;
}

static void end_undo() {
    int x = game_vars.undo.x;
    int y = game_vars.undo.y;

    // move block to end position
    move_animation_block(x, y);
    set_animation_block(false, false, 0, false, 0);

    // mark source usable
    current_level[y][x] = current_level[game_vars.target.y][game_vars.target.x] - ID_SPECIAL;
    show_block(x, y, true);
    set_block_bank(x, y, current_level[y][x], 6);
    set_block_num(x, y, 0, current_level[y][x]);

    // mark target free
    x = game_vars.target.x;
    y = game_vars.target.y;
    current_level[y][x] = ID_EMPTY;
    show_block(x, y, false);
    set_block_bank(x, y, 0, 6);
    set_block_num(x, y, 0, 0);

    // end of animation
    game_vars.state = GAME_SELECT_SRC;
    hide_cursor(false);
    game_vars.move--;
    game_vars.points -= game_vars.undo.points;
    need_refresh = true;
}

static void check_win(bool force) {
    // won?
    if(game_vars.move == game_vars.max_moves || force) {
        play_sfx(SOUND_WIN);

        game_vars.level += 1;
        // end game
        if(game_vars.level > game_vars.level_max)
            init_intro(); // fixme: win message

        restart_game();
    }
}

void restart_game() {
    // reset moves
    game_vars.max_moves = 0;
    game_vars.move = 0;
    game_vars.undo.enabled = false;

    need_refresh = true;

    load_level();
}

static void logic() {
    animate_cursor();

    if (game_vars.state == GAME_UNDO) {
        if (anim_tick > 0)
            animate();
        else
            end_undo();
    } else if (game_vars.state == GAME_ANIMATE) {
        if (anim_tick > 0)
            animate();
        else
            end_turn();
    } else if (game_vars.state == GAME_SELECT_TGT) {
        move_cursor();

        // select block
        if (key_hit(KEY_A))
            select_block_tgt(game_vars.cursor.x, game_vars.cursor.y);

        // unselect block
        if (key_hit(KEY_B))
            unselect_block();

        // open menu
        if (key_hit(KEY_START)) {
            unselect_block();
            pause_menu();
        }
    } else if (game_vars.state == GAME_SELECT_SRC) {
        check_win(false);

        move_cursor();

        // select block
        if (key_hit(KEY_A))
            select_block_src(game_vars.cursor.x, game_vars.cursor.y);

        // undo
        if (key_hit(KEY_B))
            check_undo();

        // open menu
        if (key_hit(KEY_START))
            pause_menu();
    }
}

void draw_stats() {
    static char buf[32];
    tte_set_pos(TEXT_LEFT, 18);
    tte_write("Level:");
    posprintf(buf, "#{cx:0x1000}%s%2d#{cx:0}", game_vars.gamemode_desc, game_vars.level);
    tte_set_pos(TEXT_LEFT, 30);
    tte_write(buf);
    tte_set_pos(TEXT_LEFT, 56);
    tte_write("Moves:");
    posprintf(buf, "#{cx:0x2000}%2d/%2d#{cx:0}", game_vars.move, game_vars.max_moves);
    tte_set_pos(TEXT_LEFT+10, 68);
    tte_write(buf);
    tte_set_pos(TEXT_LEFT-4, 96);
    tte_write("Points:");
    posprintf(buf, "#{cx:0x2000}%6d#{cx:0}", game_vars.points);
    tte_set_pos(TEXT_LEFT-2, 108);
    tte_write(buf);
#if 0
    tte_set_pos(TEXT_LEFT, 136);
    tte_write("Passw:");
    posprintf(buf, "#{cx:0x4000}%s#{cx:0}", game_vars.passwords[game_vars.level]);
    tte_set_pos(TEXT_LEFT+10, 148);
    tte_write(buf);
#endif
}

static void draw() {
    if (need_refresh) {
        oam_copy(oam_mem, obj_buffer, OBJECTS_GAME);
        need_refresh = false;
    } else
        oam_copy(oam_mem, obj_buffer, OBJECTS_UI);

    if(stat_tick & 1000) {
        stat_tick = 0;
        draw_stats();
    }
    stat_tick++;
}

void restore_game_state() {
    // init state
    App.delegate.logic = logic;
    App.delegate.draw = draw;

    setup_display(true, true, true);

    default_text_margins();
    tte_erase_screen();
    stat_tick = 1000;
}

void init_game(enum GameMode mode, int start_level) {
    restore_game_state();

    // Background
    // Load palette
    GRIT_CPY(pal_bg_mem, bg_gamePal);
    // Load tiles into CBB 1
    LZ77UnCompVram(bg_gameTiles, tile_mem[1]);
    // Load map into SBB 16
    LZ77UnCompVram(bg_gameMap, se_mem[16]);
    // set up BG2 for a 4bpp 32x32t map, using charblock 1 and screenblock 16
    REG_BG2CNT = BG_CBB(1) | BG_SBB(16) | BG_4BPP | BG_REG_32x32 | BG_PRIO(3);
    REG_BG2HOFS = 0;
    REG_BG2VOFS = 0;

    // Objects
    // Load palette
    GRIT_CPY(pal_obj_mem, ts_gamePal);
    make_palette();
    // Load tiles into CBB 0
    LZ77UnCompVram(ts_gameTiles, tile_mem[4]);

    // init sprites
    oam_init(obj_buffer, 128);

    setup_blending();

    // cursor
    OBJ_ATTR *cur = &obj_buffer[TILE_CURSOR];
    obj_set_attr(cur,
        ATTR0_REG|ATTR0_4BPP|ATTR0_SQUARE,
        ATTR1_SIZE_32,
        ATTR2_PRIO(0) | ATTR2_PALBANK(0) | 16); // bank 0 is normal, get tile with number
    obj_set_pos(cur, 1, 1);

    // special animation tiles
    OBJ_ATTR *anim = &obj_buffer[TILE_ANIMATE1];
    obj_set_attr(anim,
        ATTR0_REG|ATTR0_4BPP|ATTR0_SQUARE|ATTR0_BLEND,
        ATTR1_SIZE_32,
        ATTR2_PRIO(0) | ATTR2_PALBANK(0) | 0); // bank 0 is normal, tile 0 is the block
    obj_hide(anim);
    OBJ_ATTR *anim_num = &obj_buffer[TILE_ANIMATE2];
    obj_set_attr(anim_num,
        ATTR0_REG|ATTR0_4BPP|ATTR0_SQUARE|ATTR0_BLEND,
        ATTR1_SIZE_8,
        ATTR2_PRIO(0) | ATTR2_PALBANK(6) | (0 + 64 + 16)); // bank 6 is white, tile is "0"
    obj_hide(anim_num);

    game_vars.points = 0;
    setup_level(mode, start_level);
    restart_game();

    game_vars.state = GAME_SELECT_SRC;

    play_music(MUSIC_INGAME);
}
