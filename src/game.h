#pragma once

enum GameMode { GAME_ORIGINAL, GAME_NORMAL, GAME_HARD, GAME_TIMEATTACK };

void init_game(enum GameMode mode, int start_level);

void restore_game_state();

void restart_game();

void draw_stats();

void setup_blending();
