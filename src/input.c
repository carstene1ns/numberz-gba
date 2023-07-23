#include "main.h"
#include "input.h"

#define WAIT_INITIAL 14
#define WAIT_REPEAT 6
static int counter[4] = { WAIT_INITIAL, WAIT_INITIAL, WAIT_INITIAL, WAIT_INITIAL };

void input_update() {
    key_poll();

    for(int i = 0; i < 4; i++) {
        int key = BIT(i + 4);
        if (key_held(key)) {
            if (counter[i] == 0)
                counter[i] = WAIT_REPEAT;
            counter[i]--;
        } else {
            counter[i] = WAIT_INITIAL;
        }
    }
}

bool direction_hold(enum DirectionButtons button) {
    switch (button) {
        case DIRECTION_RIGHT:
            return (key_hit(KEY_RIGHT) || counter[0] == 0);
            break;

        case DIRECTION_LEFT:
            return (key_hit(KEY_LEFT) || counter[1] == 0);
            break;

        case DIRECTION_UP:
            return (key_hit(KEY_UP) || counter[2] == 0);
            break;

        case DIRECTION_DOWN:
            return (key_hit(KEY_DOWN) || counter[3] == 0);
            break;
    }

    return false;
}
