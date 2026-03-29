#include "pickup.h"
#include "tiles.h"
#include "player.h"
#include "sound.h"
#include <gbdk/platform.h>

Pickup pickups[PICKUP_COUNT];

static uint8_t rand8(void) {
    static uint8_t seed = 0x71;
    seed ^= seed << 3;
    seed ^= seed >> 5;
    return seed;
}

void pickups_init(void) {
    uint8_t i;
    for (i = 0; i < PICKUP_COUNT; i++) {
        pickups[i].active = 0;
        pickups[i].x = 0;
        pickups[i].y = 0;
        pickups[i].type = 0;
        move_sprite(PICKUP_OAM_BASE + i, 0, 0);
        set_sprite_prop(PICKUP_OAM_BASE + i, 0x00U); /* OBP0 */
    }
}

void pickup_try_spawn(uint8_t x, uint8_t y) {
    uint8_t r;
    uint8_t type;
    uint8_t i;
    uint8_t tile;

    r = rand8();
    if (r < 51) {
        type = PICKUP_TYPE_HEART;
    } else if (r < 115) {
        type = PICKUP_TYPE_POWER;
    } else if (r < 153) {
        type = PICKUP_TYPE_BOMB;
    } else {
        return; /* no drop */
    }

    /* Find free slot */
    for (i = 0; i < PICKUP_COUNT; i++) {
        if (!pickups[i].active) {
            pickups[i].active = 1;
            pickups[i].x = x;
            pickups[i].y = y;
            pickups[i].type = type;

            if (type == PICKUP_TYPE_HEART) {
                tile = SPR_HEART;
            } else if (type == PICKUP_TYPE_POWER) {
                tile = SPR_PICKUP_POWER;
            } else {
                tile = SPR_PICKUP_BOMB;
            }
            pickups[i].base_tile = tile;
            set_sprite_tile(PICKUP_OAM_BASE + i, tile);
            set_sprite_prop(PICKUP_OAM_BASE + i, 0x00U); /* OBP0 */
            move_sprite(PICKUP_OAM_BASE + i, pickups[i].x, pickups[i].y);
            return;
        }
    }
}

void pickups_update(void) {
    uint8_t i;
    for (i = 0; i < PICKUP_COUNT; i++) {
        if (!pickups[i].active) continue;
        pickups[i].y += 1;
        if (pickups[i].y > 152) {
            pickups[i].active = 0;
            move_sprite(PICKUP_OAM_BASE + i, 0, 0);
        } else {
            if (anim_frame_changed)
                set_sprite_tile(PICKUP_OAM_BASE + i, pickups[i].base_tile + anim_frame);
            move_sprite(PICKUP_OAM_BASE + i, pickups[i].x, pickups[i].y);
        }
    }
}
