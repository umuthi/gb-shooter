#ifndef TILES_H
#define TILES_H

#include <gbdk/platform.h>

/* BG / Window tile indices */
#define TILE_BLANK        0   /* all color-0 pixels → black space background */
#define TILE_STAR_SM      1
#define TILE_STAR_LG      2
#define TILE_DIGIT_0      3   /* digits 3-12 */
#define TILE_HEART        13  /* used as WIN tile in HUD strip */
#define TILE_BLACK        14  /* all color-3 pixels → solid black (overlay bg) */
#define TILE_DASH         15  /* horizontal divider line */

/* Letter tiles */
#define TILE_LETTER_A     16
#define TILE_LETTER_E     17
#define TILE_LETTER_G     18
#define TILE_LETTER_H     19
#define TILE_LETTER_M     20
#define TILE_LETTER_O     21
#define TILE_LETTER_P     22
#define TILE_LETTER_R     23
#define TILE_LETTER_S     24
#define TILE_LETTER_T     25
#define TILE_LETTER_V     26
#define TILE_LETTER_B     27  /* for HUD bomb display */
#define TILE_LETTER_U     28
#define TILE_LETTER_D     29
#define TILE_BAR          30  /* solid block for boss health bar */

/* Dialogue letter tiles — added for cutscene system */
#define TILE_LETTER_C     31
#define TILE_LETTER_F     32
#define TILE_LETTER_I     33
#define TILE_LETTER_K     34
#define TILE_LETTER_L     35
#define TILE_LETTER_N     36
#define TILE_LETTER_W     37
#define TILE_LETTER_Y     38
#define TILE_PERIOD       39
#define TILE_EXCLAIM      40
#define TILE_LETTER_X     41
#define TILE_LETTER_Z     42
#define TILE_APOSTROPHE   43
#define TILE_COMMA        44
#define TILE_QUESTION     45

/* Sprite tile indices (loaded into sprite VRAM separately).
   Frame 0 and frame 1 are always consecutive so anim_frame (0 or 1) can be added. */
#define SPR_PLAYER        0   /* frame 0 */
#define SPR_PLAYER_1      1   /* frame 1 */
#define SPR_BULLET        2
#define SPR_ENEMY1        3   /* straight pattern, frame 0 */
#define SPR_ENEMY1_1      4   /* frame 1 */
#define SPR_ENEMY2        5   /* zigzag pattern, frame 0 */
#define SPR_ENEMY2_1      6   /* frame 1 */
#define SPR_ENEMY3        7   /* swoop/diagonal pattern, frame 0 */
#define SPR_ENEMY3_1      8   /* frame 1 */
#define SPR_ENEMY4        9   /* sine/kamikaze pattern, frame 0 */
#define SPR_ENEMY4_1      10  /* frame 1 */
#define SPR_EXPLOSION_0   11
#define SPR_EXPLOSION_1   12
#define SPR_HEART         13  /* frame 0 */
#define SPR_HEART_1       14  /* frame 1 */
#define SPR_PICKUP_POWER  15  /* frame 0 */
#define SPR_PICKUP_POWER_1 16 /* frame 1 */
#define SPR_PICKUP_BOMB   17  /* frame 0 */
#define SPR_PICKUP_BOMB_1 18  /* frame 1 */
#define SPR_BOSS_TL       19  /* boss 2x2 metasprite — top-left */
#define SPR_BOSS_TR       20
#define SPR_BOSS_BL       21
#define SPR_BOSS_BR       22

extern const uint8_t bg_tiles[];
extern const uint8_t sprite_tiles[];

#define BG_TILES_COUNT    46
#define SPR_TILES_COUNT   23

void tiles_load(void);

#endif
