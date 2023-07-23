#pragma once

enum DirectionButtons { DIRECTION_RIGHT, DIRECTION_LEFT, DIRECTION_UP, DIRECTION_DOWN };

void input_update();

bool direction_hold(enum DirectionButtons button);
