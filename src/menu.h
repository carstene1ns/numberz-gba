#pragma once

typedef struct {
    u8 x;
    u8 y;
    char label[16];
    void (*func)();
} MenuOption;

void intro_menu();

void pause_menu();
