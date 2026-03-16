#ifndef HUD_H
#define HUD_H

#include <gbdk/platform.h>

/* Window layer: y=136, x=7 (hardware offset means wx=7 = left edge) */
#define HUD_WY  136
#define HUD_WX  7

/* Score is a 4-digit number (0-9999) */
extern uint16_t score;

/* Score multiplier — incremented by collecting over-max pickups; reset on hit */
extern uint8_t score_multiplier;

void hud_init(void);
void hud_add_score(uint8_t points);
void hud_update(void);
uint8_t hud_enemy_pts(void);
uint8_t hud_boss_pts(void);

#endif
