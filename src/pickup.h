#ifndef PICKUP_H
#define PICKUP_H
#include <gbdk/platform.h>

#define PICKUP_COUNT      4
#define PICKUP_OAM_BASE   17

#define PICKUP_TYPE_HEART  0
#define PICKUP_TYPE_POWER  1
#define PICKUP_TYPE_BOMB   2

typedef struct {
    uint8_t x, y;
    uint8_t type;
    uint8_t active;
    uint8_t base_tile; /* SPR_HEART/SPR_PICKUP_POWER/SPR_PICKUP_BOMB — set on spawn */
} Pickup;

extern Pickup pickups[PICKUP_COUNT];

void pickups_init(void);
void pickup_try_spawn(uint8_t x, uint8_t y);
void pickups_update(void);
#endif
