#include "bullet.h"
#include "tiles.h"
#include <gbdk/platform.h>

Bullet bullets[BULLET_COUNT];

void bullets_init(void) {
    uint8_t i;
    for (i = 0; i < BULLET_COUNT; i++) {
        bullets[i].active = 0;
        bullets[i].x = 0;
        bullets[i].y = 0;
        move_sprite(BULLET_OAM_BASE + i, 0, 0);
        set_sprite_tile(BULLET_OAM_BASE + i, SPR_BULLET);
        set_sprite_prop(BULLET_OAM_BASE + i, 0x10U); /* OBP1 → shade 3 (black) */
    }
}

uint8_t bullet_spawn(uint8_t x, uint8_t y) {
    uint8_t i;
    for (i = 0; i < BULLET_COUNT; i++) {
        if (!bullets[i].active) {
            bullets[i].x = x;
            bullets[i].y = y;
            bullets[i].active = 1;
            return 1;
        }
    }
    return 0; /* all slots full */
}

void bullets_update(void) {
    uint8_t i;
    Bullet *b;
    for (i = 0; i < BULLET_COUNT; i++) {
        b = &bullets[i];
        if (!b->active) continue;
        if (b->y < BULLET_SPEED + 8) {
            b->active = 0;
            move_sprite(BULLET_OAM_BASE + i, 0, 0);
        } else {
            b->y -= BULLET_SPEED;
            move_sprite(BULLET_OAM_BASE + i, b->x, b->y);
        }
    }
}
