#include "dialogue.h"
#include "tiles.h"
#include "sound.h"
#include "hud.h"
#include <gbdk/platform.h>

/*
 * Each page = up to 5 lines, separated by '\n'.
 * Max 18 visible characters per line (rendered at window cols 1-18).
 * Text is uppercase only. Digits and punctuation . ! ' , ? are supported.
 * Each scene is a NULL-terminated array of page pointers.
 */

/* ---- Scene 0: INTRO ---- */
static const char intro_p0[] =
    "AFTER 200 YEARS OF\n"
    "WAR, THE GALAXY\n"
    "BURNS. EARTH DIES.\n"
    "YOU ARE HUMANITY'S\n"
    "LAST WARRIOR.";

static const char intro_p1[] =
    "HUNT DOWN EMPEROR\n"
    "XUMUTHI. HE THINKS\n"
    "THE WAR IS WON.\n"
    "SURPRISE IS YOUR\n"
    "ONLY ADVANTAGE.";

static const char * const s_intro[] = { intro_p0, intro_p1, NULL };

/* ---- Scene 1: PRE_BOSS1 ---- */
static const char pre_b1_p0[] =
    "WHO ARE YOU? I DID\n"
    "NOT EXPECT ANOTHER\n"
    "MECHA PILOT HERE.\n"
    "NO MATTER. YOU'LL\n"
    "DIE LIKE THE REST!";

static const char * const s_pre_b1[] = { pre_b1_p0, NULL };

/* ---- Scene 2: POST_BOSS1 ---- */
static const char post_b1_p0[] =
    "YOU PROVED TOUGHER\n"
    "THAN EXPECTED.\n"
    "NO TIME FOR THIS.\n"
    "MY HORDE FIRES NOW\n"
    "YOU ARE NO MATCH.";

static const char * const s_post_b1[] = { post_b1_p0, NULL };

/* ---- Scene 3: PRE_BOSS2 ---- */
static const char pre_b2_p0[] =
    "YOU AGAIN? I DID\n"
    "NOT EXPECT THIS.\n"
    "I SHOULD HAVE DONE\n"
    "THIS WHEN LAST WE\n"
    "MET. PREPARE NOW.";

static const char pre_b2_p1[] =
    "YOU'LL SHARE YOUR\n"
    "COMRADES' FATE.\n"
    "DEATH IN BATTLE IS\n"
    "NOBLE. DRIFT IN\n"
    "SPACE, FOREVER.";

static const char * const s_pre_b2[] = { pre_b2_p0, pre_b2_p1, NULL };

/* ---- Scene 4: POST_BOSS2 ---- */
static const char post_b2_p0[] =
    "AAAARGHH! I WILL\n"
    "NOT BE DEFEATED SO\n"
    "CLOSE TO MY GOAL!\n"
    "MY HORDE WILL KEEP\n"
    "YOU BUSY WHILE I";

static const char post_b2_p1[] =
    "FINISH OFF EARTH.\n"
    "MY GUARD HUNTS YOU\n"
    "DOWN. EVEN IF YOU\n"
    "CATCH ME, YOU WILL\n"
    "STILL BE THE LAST.";

static const char * const s_post_b2[] = { post_b2_p0, post_b2_p1, NULL };

/* ---- Scene 5: PRE_BOSS3 ---- */
static const char pre_fb_p0[] =
    "HOW DID YOU GET\n"
    "HERE?! MY GUARD\n"
    "FAILED. I MUST NOW\n"
    "DEAL WITH YOU\n"
    "MYSELF.";

static const char pre_fb_p1[] =
    "YOUR SKILL AMAZES\n"
    "ME. YOUR KIND HAS\n"
    "NEVER SHOWN SUCH\n"
    "TALENT. PREPARE TO\n"
    "DIE!!!";

static const char * const s_pre_fb[] = { pre_fb_p0, pre_fb_p1, NULL };

/* ---- Scene 6: MID_BOSS3 ---- */
static const char mid_fb_p0[] =
    "NOOOOO! THIS CAN'T\n"
    "BE. FINAL BATTLE\n"
    "FOR US BOTH. I USE\n"
    "ALL RESERVES NOW.\n"
    "MINIONS, KILL HIM!";

static const char * const s_mid_fb[] = { mid_fb_p0, NULL };

/* ---- Scene 7: POST_BOSS3 ---- */
static const char post_fb_p0[] =
    "AAAAARRGGHHH! TO\n"
    "THINK THAT I WILL\n"
    "DIE AT THE HANDS\n"
    "OF A TINY HUMAN\n"
    "MECHA PILOT.";

static const char post_fb_p1[] =
    "MY MIND RETURNS TO\n"
    "MY HOME ACROSS THE\n"
    "UNIVERSE. THIS IS\n"
    "LIKELY OUR LAST\n"
    "MEETING. I COMMEND";

static const char post_fb_p2[] =
    "YOU. YOU ARE RARE\n"
    "AMONG YOUR KIND.\n"
    "PERHAPS THERE IS\n"
    "MORE TO YOUR KIND\n"
    "THAN I FIRST KNEW.";

static const char * const s_post_fb[] = { post_fb_p0, post_fb_p1, post_fb_p2, NULL };

/* ---- Scene 8: ENDING ---- */
static const char ending_p0[] =
    "HIS MIND GONE, THE\n"
    "HORDE WENT STILL.\n"
    "THEY DRIFTED ON IN\n"
    "THEIR LAST COURSE,\n"
    "SILENT FOREVER.";

static const char ending_p1[] =
    "ALL WERE UNDER HIS\n"
    "PSYCHIC COMMAND.\n"
    "THE PILOT HEADED\n"
    "TO EARTH. HUMANITY\n"
    "MIGHT YET SURVIVE.";

static const char * const s_ending[] = { ending_p0, ending_p1, NULL };

/* ---- Master scene table ---- */
static const char * const * const scenes[DIALOGUE_COUNT] = {
    s_intro,   s_pre_b1,  s_post_b1,
    s_pre_b2,  s_post_b2, s_pre_fb,
    s_mid_fb,  s_post_fb, s_ending
};

/* ---- Typing state ---- */
static uint8_t  dl_scene;
static uint8_t  dl_page;
static uint8_t  dl_done;
static uint8_t  dl_page_done;
static uint16_t dl_str_pos;
static uint8_t  dl_row;
static uint8_t  dl_col;
static uint8_t  dl_frame_cnt;
static uint8_t  dl_blink;
static uint8_t  dl_sfx_cnt;

/* ---- Helpers ---- */

static void fill_row(uint8_t row, uint8_t tile) {
    uint8_t buf[20];
    uint8_t i;
    for (i = 0; i < 20; i++) buf[i] = tile;
    set_win_tiles(0, row, 20, 1, buf);
}

static uint8_t char_to_tile(char c) {
    if (c >= '0' && c <= '9') return (uint8_t)(TILE_DIGIT_0 + (c - '0'));
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
    case '\'': return TILE_APOSTROPHE;
    case ',': return TILE_COMMA;
    case '?': return TILE_QUESTION;
    default:  return TILE_BLANK;
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
    const char *p = scenes[dl_scene][dl_page];
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
    for (i = 13; i < 18; i++) fill_row(i, TILE_BLANK);
    set_win_tiles(19, 17, 1, 1, &blank);
    dl_str_pos   = 0;
    dl_row       = 0;
    dl_col       = 0;
    dl_frame_cnt = 0;
    dl_page_done = 0;
    dl_blink     = 0;
    dl_sfx_cnt   = 0;
}

/* ---- Public API ---- */

void dialogue_start(uint8_t scene_id) {
    uint8_t i;
    dl_scene = scene_id;
    dl_page  = 0;
    dl_done  = 0;

    for (i = 0; i < 40; i++) move_sprite(i, 0, 0);
    SCY_REG = 0;
    SCX_REG = 0;

    for (i = 0; i < 12; i++) fill_row(i, TILE_BLACK);
    fill_row(12, TILE_DASH);
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
        /* Blink the advance cursor */
        dl_blink++;
        if (dl_blink >= 60) dl_blink = 0;
        {
            uint8_t cur = (dl_blink < 30) ? TILE_STAR_SM : TILE_BLANK;
            set_win_tiles(19, 17, 1, 1, &cur);
        }
        if (joy_pressed & (J_A | J_B | J_START)) {
            if (scenes[dl_scene][dl_page + 1] != NULL) {
                dl_page++;
                page_init();
            } else {
                dl_done = 1;
            }
        }
        return;
    }

    /* Skip to end of page on button press */
    if (joy_pressed & (J_A | J_B | J_START)) {
        dump_remaining();
        return;
    }

    /* Advance one character every 3 frames */
    dl_frame_cnt++;
    if (dl_frame_cnt < 3) return;
    dl_frame_cnt = 0;

    p = scenes[dl_scene][dl_page];
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
