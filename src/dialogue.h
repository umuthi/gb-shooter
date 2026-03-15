#ifndef DIALOGUE_H
#define DIALOGUE_H

#include <gbdk/platform.h>

/* Scene identifiers */
#define DIALOGUE_INTRO      0
#define DIALOGUE_PRE_BOSS1  1
#define DIALOGUE_POST_BOSS1 2
#define DIALOGUE_PRE_BOSS2  3
#define DIALOGUE_POST_BOSS2 4
#define DIALOGUE_PRE_BOSS3  5
#define DIALOGUE_MID_BOSS3  6
#define DIALOGUE_POST_BOSS3 7
#define DIALOGUE_ENDING     8
#define DIALOGUE_COUNT      9

void dialogue_start(uint8_t scene_id);
void dialogue_update(uint8_t joy_pressed);
uint8_t dialogue_is_done(void);

#endif
