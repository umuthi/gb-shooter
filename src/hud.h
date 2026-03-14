#ifndef HUD_H
#define HUD_H

#include <gbdk/platform.h>

/* Window layer: y=136, x=7 (hardware offset means wx=7 = left edge) */
#define HUD_WY  136
#define HUD_WX  7

/* Score is a 4-digit number (0-9999) */
extern uint16_t score;

void hud_init(void);
void hud_add_score(uint8_t points);
void hud_draw_lives(uint8_t lives);
void hud_draw_score(void);
void hud_update(void);

#endif
