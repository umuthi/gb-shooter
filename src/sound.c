#include "sound.h"
#include <gbdk/platform.h>

/*
 * Direct hardware register SFX — no audio library.
 *
 * CH1 (NR1x): Square wave + frequency sweep  → laser shoot
 * CH2 (NR2x): Square wave                    → player hit
 * CH4 (NR4x): Noise (LFSR)                   → explosion
 *
 * NR12/NR22/NR42 envelope format:
 *   bits 7:4 = initial volume (0-15)
 *   bit  3   = direction (0=decrease, 1=increase)
 *   bits 2:0 = step time in 1/64s  (0 = envelope off)
 *
 * NR10 sweep format:
 *   bits 6:4 = sweep period in 128Hz ticks
 *   bit  3   = direction (0=freq increase, 1=freq decrease)
 *   bits 2:0 = shift amount
 *
 * NR43 noise format:
 *   bits 7:4 = clock shift s
 *   bit  3   = LFSR width (0=15-bit wide/rumble, 1=7-bit narrow/hiss)
 *   bits 2:0 = divisor code r  (r=0 uses r=0.5)
 *   noise freq ≈ 524288 / r / 2^(s+1)
 */

void sound_init(void) {
    NR52_REG = 0x80;  /* Master sound on */
    NR51_REG = 0xFF;  /* All channels to both L and R outputs */
    NR50_REG = 0x77;  /* Max volume both sides */
}

void sfx_shoot(void) {
    /*
     * Laser zap: starts at ~2 kHz, sweeps upward rapidly, fast volume decay.
     * The rising sweep gives the classic sci-fi "pew" character.
     */
    NR10_REG = 0x1A;  /* Sweep: period=1 (fast), increase, shift=2        */
    NR11_REG = 0x80;  /* 50% duty, length counter unused                  */
    NR12_REG = 0xF2;  /* Vol=15, decrease, step=2 → decays in ~0.47s      */
    NR13_REG = 0xC0;  /* Freq low byte  — combined with high → ~2048 Hz   */
    NR14_REG = 0x87;  /* Trigger, no length enable, freq high bits = 7     */
}

void sfx_explosion(void) {
    /*
     * Deep space explosion: 15-bit LFSR for wide-spectrum rumble,
     * slow envelope decay so it lingers.
     */
    NR41_REG = 0x00;  /* Length counter (unused — no length enable below)  */
    NR42_REG = 0xF5;  /* Vol=15, decrease, step=5 → ~1.17s decay           */
    NR43_REG = 0x43;  /* Clock shift=4, 15-bit LFSR, divisor=3 → low rumble*/
    NR44_REG = 0x80;  /* Trigger, no length enable                          */
}

void sfx_player_hit(void) {
    /*
     * Hull impact warning: 12.5% duty (buzzy/harsh), medium pitch,
     * moderate decay so player clearly feels the hit.
     */
    NR21_REG = 0x00;  /* 12.5% duty — harsher buzzing tone                 */
    NR22_REG = 0xB4;  /* Vol=11, decrease, step=4 → ~0.69s decay           */
    NR23_REG = 0xD6;  /* Freq low byte  — combined with high → ~440 Hz     */
    NR24_REG = 0x86;  /* Trigger, no length enable, freq high bits = 6      */
}

void sfx_pickup(void) {
    /* Short high-pitched chime on CH1 */
    NR10_REG = 0x00;
    NR11_REG = 0xC0;  /* 75% duty */
    NR12_REG = 0xF1;  /* Vol=15, decrease, 1 step */
    NR13_REG = 0x50;
    NR14_REG = 0xC7;  /* Trigger + length, freq high=7 */
}

void sfx_type(void) {
    /* Very short high-pitched tick for dialogue typing — CH1 with length enable */
    NR10_REG = 0x00;  /* no frequency sweep */
    NR11_REG = 0xF8;  /* 75% duty, max length (63) so it stops quickly */
    NR12_REG = 0x52;  /* Vol=5, decrease, step=2 → very brief */
    NR13_REG = 0x00;  /* freq low byte */
    NR14_REG = 0xC7;  /* Trigger + length enable, freq high=7 → ~4186 Hz */
}

void sfx_bomb(void) {
    /* Big screen-clearing explosion on CH4 */
    NR41_REG = 0x00;
    NR42_REG = 0xF7;  /* Vol=15, decrease, 7 steps → ~1.64s */
    NR43_REG = 0x11;  /* shift=1, 15-bit LFSR, div=1 → wide rumble */
    NR44_REG = 0x80;  /* Trigger, no length */
}
