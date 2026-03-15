#include "dialogue.h"
#include "tiles.h"
#include "sound.h"
#include "hud.h"
#include <gbdk/platform.h>

/* ---- Text data — each scene is a single page, lines separated by '\n' ----
 * Max 5 lines per page, max 18 visible chars per line (cols 1-18 of window).
 * Uppercase only; space maps to TILE_BLANK (white, blends with text bg).
 */
static const char scene_0[] =   /* INTRO */
    "YOUR PLANET HAS\n"
    "FALLEN UNDER THE\n"
    "SHADOW OF THEIR\n"
    "FLEET. YOU MUST\n"
    "FIGHT THEM NOW.";

static const char scene_1[] =   /* PRE_BOSS1 */
    "WARNING. ENEMY\n"
    "COMMAND SHIP\n"
    "DETECTED AHEAD.\n"
    "PREPARE FOR\n"
    "HEAVY COMBAT.";

static const char scene_2[] =   /* POST_BOSS1 */
    "COMMAND SHIP DOWN.\n"
    "BUT THIS WAS THEIR\n"
    "FIRST WAVE. MORE\n"
    "THREATS AWAIT IN\n"
    "THE DARK AHEAD.";

static const char scene_3[] =   /* PRE_BOSS2 */
    "SENSORS DETECT A\n"
    "MASSIVE WARSHIP.\n"
    "ITS FIREPOWER\n"
    "DWARFS ANYTHING\n"
    "SEEN BEFORE.";

static const char scene_4[] =   /* POST_BOSS2 */
    "THE WARSHIP FALLS.\n"
    "YET THE ALIEN\n"
    "SIGNAL GROWS\n"
    "STRONGER. SOMETHING\n"
    "LARGER IS COMING.";

static const char scene_5[] =   /* PRE_BOSS3 */
    "THE MOTHER SHIP.\n"
    "SOURCE OF THE\n"
    "INVASION SIGNAL.\n"
    "DESTROY THIS AND\n"
    "IT ENDS FOREVER.";

static const char scene_6[] =   /* MID_BOSS3 */
    "IT REGENERATES!\n"
    "THE SHIP ABSORBED\n"
    "THE DAMAGE AND\n"
    "REPAIRED ITSELF.\n"
    "PUSH THROUGH NOW!";

static const char scene_7[] =   /* POST_BOSS3 */
    "MOTHER SHIP DOWN.\n"
    "THE INVASION FLEET\n"
    "HAS NO COMMAND.\n"
    "YOUR MISSION IS\n"
    "COMPLETE.";

static const char scene_8[] =   /* ENDING */
    "AGAINST ALL ODDS\n"
    "YOU SAVED YOUR\n"
    "HOME PLANET TODAY.\n"
    "THE STARS ARE\n"
    "SAFE FOREVER.";

/* Lookup table — one page per scene */
static const char * const scenes[DIALOGUE_COUNT] = {
    scene_0, scene_1, scene_2, scene_3, scene_4,
    scene_5, scene_6, scene_7, scene_8
};

/* ---- State ---- */
static uint8_t  dl_scene;
static uint8_t  dl_done;
static uint8_t  dl_page_done;
static uint16_t dl_str_pos;    /* offset into current scene string */
static uint8_t  dl_row;        /* current text row (0-4)  */
static uint8_t  dl_col;        /* current text col (0-17) */
static uint8_t  dl_frame_cnt;
static uint8_t  dl_blink;      /* cursor blink counter */
static uint8_t  dl_sfx_cnt;    /* counts non-space chars for SFX cadence */

/* ---- Helpers ---- */
static void fill_row(uint8_t row, uint8_t tile) {
    uint8_t buf[20];
    uint8_t i;
    for (i = 0; i < 20; i++) buf[i] = tile;
    set_win_tiles(0, row, 20, 1, buf);
}

static uint8_t char_to_tile(char c) {
    switch (c) {
    case 'A': return TILE_LETTER_A;
    case 'B': return TILE_LETTER_B;
    case 'C': return TILE_LETTER_C;
    case 'D': return TILE_LETTER_D;
    case 'E': return TILE_LETTER_E;
    case 'F': return TILE_LETTER_F;
    case 'G': return TILE_LETTER_G;
    case 'H': return TILE_LETTER_H;
    case 'I': return TILE_LETTER_I;
    case 'K': return TILE_LETTER_K;
    case 'L': return TILE_LETTER_L;
    case 'M': return TILE_LETTER_M;
    case 'N': return TILE_LETTER_N;
    case 'O': return TILE_LETTER_O;
    case 'P': return TILE_LETTER_P;
    case 'R': return TILE_LETTER_R;
    case 'S': return TILE_LETTER_S;
    case 'T': return TILE_LETTER_T;
    case 'U': return TILE_LETTER_U;
    case 'V': return TILE_LETTER_V;
    case 'W': return TILE_LETTER_W;
    case 'X': return TILE_LETTER_X;
    case 'Y': return TILE_LETTER_Y;
    case 'Z': return TILE_LETTER_Z;
    case '.': return TILE_PERIOD;
    case '!': return TILE_EXCLAIM;
    default:  return TILE_BLANK;  /* space and unknowns = transparent */
    }
}

static void render_char(char c) {
    uint8_t tile;
    if (c == '\n') {
        dl_row++;
        dl_col = 0;
        return;
    }
    if (dl_col < 18 && dl_row < 5) {
        tile = char_to_tile(c);
        set_win_tiles(1 + dl_col, 13 + dl_row, 1, 1, &tile);
        if (c != ' ') {
            dl_sfx_cnt++;
            if (dl_sfx_cnt >= 3) {
                dl_sfx_cnt = 0;
                sfx_type();
            }
        }
    }
    dl_col++;
}

static void dump_remaining(void) {
    const char *p = scenes[dl_scene];
    char c;
    while ((c = p[dl_str_pos]) != '\0') {
        render_char(c);
        dl_str_pos++;
    }
    dl_page_done = 1;
}

static void page_init(void) {
    uint8_t i;
    uint8_t blank = TILE_BLANK;
    /* Clear text rows */
    for (i = 13; i < 18; i++) fill_row(i, TILE_BLANK);
    /* Hide cursor */
    set_win_tiles(19, 17, 1, 1, &blank);

    dl_str_pos  = 0;
    dl_row      = 0;
    dl_col      = 0;
    dl_frame_cnt = 0;
    dl_page_done = 0;
    dl_blink     = 0;
    dl_sfx_cnt   = 0;
}

/* ---- Public API ---- */

void dialogue_start(uint8_t scene_id) {
    uint8_t i;

    dl_scene = scene_id;
    dl_done  = 0;

    /* Hide all OAM sprites */
    for (i = 0; i < 40; i++) move_sprite(i, 0, 0);

    /* Freeze scroll */
    SCY_REG = 0;
    SCX_REG = 0;

    /* Image area: rows 0-11 solid black */
    for (i = 0; i < 12; i++) fill_row(i, TILE_BLACK);
    /* Separator */
    fill_row(12, TILE_DASH);
    /* Text area: rows 13-17 white background (letters render on white) */
    for (i = 13; i < 18; i++) fill_row(i, TILE_BLANK);

    move_win(HUD_WX, 0);
    SHOW_WIN;

    page_init();
}

void dialogue_update(uint8_t joy_pressed) {
    const char *p;
    char c;

    if (dl_done) return;

    if (dl_page_done) {
        /* Blink cursor (TILE_STAR_SM) at col 19, row 17 */
        dl_blink++;
        if (dl_blink >= 60) dl_blink = 0;
        {
            uint8_t cur = (dl_blink < 30) ? TILE_STAR_SM : TILE_BLANK;
            set_win_tiles(19, 17, 1, 1, &cur);
        }
        /* Any button: advance */
        if (joy_pressed & (J_A | J_B | J_START)) {
            dl_done = 1;
        }
        return;
    }

    /* A/B/START pressed while typing → dump all remaining text */
    if (joy_pressed & (J_A | J_B | J_START)) {
        dump_remaining();
        return;
    }

    /* Advance one character every 3 frames */
    dl_frame_cnt++;
    if (dl_frame_cnt < 3) return;
    dl_frame_cnt = 0;

    p = scenes[dl_scene];
    c = p[dl_str_pos];
    if (c == '\0') {
        dl_page_done = 1;
        return;
    }
    render_char(c);
    dl_str_pos++;
}

uint8_t dialogue_is_done(void) {
    return dl_done;
}
