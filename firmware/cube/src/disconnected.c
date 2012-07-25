/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Thundercracker cube firmware
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * Implements the cube's behavior while disconnected from the base. Includes
 * an interactive idle screen, battery meter, and power-down timer, as well
 * as the default radio behavior (idle hopping, pairing).
 */

#include <stdint.h>
#include <sifteo/abi/vram.h>
#include "graphics.h"
#include "draw.h"
#include "sensors.h"
#include "sensors_nb.h"
#include "power.h"

extern __bit disc_battery_draw;     // 0 = drawing logo, 1 = drawing battery

#define BATTERY_HEIGHT  19          // Number of scanlines at the top of the display reserved for battery
#define FP_BITS         4           // Fixed point precision, in bits

// Arbitrary horizontal limits, let the logo get off-screen a bit without wrapping
#define X_MIN           (-20)
#define X_MAX           ( 20)

// Hard vertical limits (battery clipping, bottom screen edge)
#define Y_MIN           (BATTERY_HEIGHT - 48)
#define Y_MAX           (BATTERY_HEIGHT + 23)

// The cube's idle timeout, measured in units of 1.57 seconds (sensor_tick_counter overflow rate).
// Reset when entering idle mode, or when the score is incremented.
// Current timeout: 1 minute
#define IDLE_TIMEOUT    38

// Threshold for detecting score-affecting bounces
#define BOUNCE_THR      0x40

#define X_MIN_FP        (X_MIN << FP_BITS)
#define X_MAX_FP        (X_MAX << FP_BITS)
#define Y_MIN_FP        (Y_MIN << FP_BITS)
#define Y_MAX_FP        (Y_MAX << FP_BITS)

// State for bouncing logo, in 16-bit fixed point math.
// Laid out in this same order in overlay memory.
extern int16_t disc_logo_x;
extern int16_t disc_logo_y;
extern int16_t disc_logo_dx;
extern int16_t disc_logo_dy;

// Score for our bouncing logo minigame, in BCD
extern uint8_t disc_score;
extern __bit disc_bounce_type;
extern __bit disc_has_trophy;
extern uint8_t disc_sleep_timer;

// Idle radio state
extern uint8_t disc_hop_timer;
extern uint8_t disc_nb_base;

extern const __code uint8_t img_logo[];
extern const __code uint8_t img_trophy[];
extern const __code uint8_t img_battery[];
extern const __code uint8_t img_battery_bars_1[];
extern const __code uint8_t img_battery_bars_2[];
extern const __code uint8_t img_battery_bars_3[];
extern const __code uint8_t img_battery_bars_4[];


static void draw_logo(void) __naked
{
    // Draw our logo at the standard position
    __asm
        DRAW_XY (1,5)
        mov     dptr, #_img_logo
        ajmp    _draw_image
    __endasm ;
}

static void draw_digit(void) __naked
{
    /*
     * Assembly-callable function to draw one BCD digit, from 'a',
     * at the VRAM address pointed to by DPTR. Advances DPTR to the
     * next tile.
     */
    __asm
        anl     a, #0x0F            ; Mask off digit
        add     a, #0x10            ; Convert digit to font index
        rl      a                   ; Convert to 7:7 format
        movx    @dptr, a
        inc     dptr                ; Skip LSB
        inc     dptr                ; Skip MSB (not written)
        ret
    __endasm ;
}

static void fp_integer(void) __naked
{
    /*
     * Assembly-callable function to extract the integer portion
     * of a 16-bit variable at @r0, and return it in A. Assumes
     * FP_BITS is 4. We're taking the low nybble of the high byte
     * and the high nybble of the low byte.
     *
     * Clobbers r1. Leaves r0 pointing just after the 16-bit value.
     */

    __asm
        mov     a, @r0
        inc     r0
        swap    a
        anl     a, #0x0F
        mov     r1, a

        mov     a, @r0
        inc     r0
        swap    a
        anl     a, #0xF0
        orl     a, r1

        ret
    __endasm ;
}

static void fp_integrate_axis(void) __naked
{
    /*
     * Assembly-callable utility to integrate one 16-bit axis.
     * Called with r0 pointed to either disc_logo_dx or disc_logo_dy.
     *
     * Assumes the inverse of our acceleration for this axis
     * is stored in r3:2.
     *
     * Clobbers r0, r2, r3, a.
     */

    __asm
        clr     c
        mov     a, @r0              ; DX/DY low
        subb    a, r2               ; Subtract inverse-acceleration low
        mov     @r0, a
        mov     r2, a               ; Save a copy

        inc     r0
        mov     a, @r0              ; DX/DY high
        subb    a, r3
        mov     @r0, a
        mov     r3, a               ; Save a copy

        mov     a, r0
        add     a, #(256 - 5)       ; Back to X/Y low
        mov     r0, a

        mov     a, @r0              ; X/Y low
        add     a, r2
        mov     @r0, a
        inc     r0
        mov     a, @r0              ; X/Y high
        addc    a, r3
        mov     @r0, a

        ret
    __endasm ;
}

static void fp_bounce_axis(void) __naked
{
    /*
     * Assembly-callable utility to invert the velocity of one 16-bit axis.
     * Called with r0 pointed to either disc_logo_dx or disc_logo_dy.
     *
     * Also handles scoring for our mini-game. Bounces are classified as "large" or
     * "small" bounces. Small bounces never affect your score. Large bounces are good
     * (increase score) if disc_bounce_type=1. They are bad (clear score) if
     * disc_bounce_type=0.
     *
     * To detect how 'big' the bounce is, ideally we'd be looking at the 16-bit
     * absolute value of the velocity we just inverted. To save space, though, we
     * take advantage of the fact that velocities are usually smaller than 0x100,
     * and we just XOR the high and low byte. This gives us an approximate absolute
     * value that works fine in the range we care about.
     *
     * Clobbers r0, r1, a.
     *
     * Returns by jumping to fp_bounce_axis_ret.
     */

    __asm
        clr     c
        clr     a
        subb    a, @r0                      ; DX/DY low
        mov     @r0, a
        mov     r1, a                       ; Save low byte

        clr     a
        inc     r0
        subb    a, @r0                      ; DX/DY high
        mov     @r0, a
        xrl     a, r1                       ; XOR high and low bytes

        add     a, #(256 - BOUNCE_THR)
        jnc     1$                          ; Ignore small bounces

        clr     a                           ; Increment or clear score?
        jnb     _disc_bounce_type, 2$

        mov     a, _disc_score
        anl     a, #0xF0                    ; If the score is at least 10, reset sleep timer
        jz      3$
        acall   _disc_reset_sleep_timer
3$:

        jb      _disc_has_trophy, 1$        ; No score increment if we already have trophy

        mov     a, _disc_score              ; BCD increment
        inc     a
        clr     c                           ; (DA relies on both C and AC flags)
        clr     ac
        da      a
        mov     _disc_has_trophy, c         ; Acquire trophy after rollover from 99

2$:     mov     _disc_score, a
1$:
        ajmp    fp_bounce_axis_ret
    __endasm ;
}

static void sub_s8_from_s16(void) __naked
{
    /*
     * Assembly-callable math utility, with two entry points.
     *
     * Adds or subtracts signed 8-bit value (in 'a') to/from a signed 16-bit value in r3:r2.
     */

    __asm

        cpl     a       ; Twos complement
        inc     a

_add_s8_to_s16::

        jb      acc.7, 1$

        ; Positive
        add     a, r2
        mov     r2, a
        mov     a, r3
        addc    a, #0
        mov     r3, a
        ret

1$:     ; Negative
        add     a, r2
        mov     r2, a
        mov     a, r3
        addc    a, #0xFF
        mov     r3, a
        ret
    __endasm ;
}

static void disc_reset_sleep_timer(void)
{
    disc_sleep_timer = sensor_tick_counter_high + IDLE_TIMEOUT;
}

static void disc_reset_radio_state(void)
{
    /*
     * Put the radio on a predictable channel and address, based
     * on our hardware ID. Use the default idle address first. 
     *
     * Schedule the first hop to occur between 1 and 2 TF0 ticks
     * from now. There's a one-tick uncertainty in our timer, and
     * we never want to hop too soon after powerup.
     */

    radio_idle_hop = 0;
    disc_hop_timer = sensor_tick_counter_high + 2;
    return radio_set_idle_addr();
}

static uint8_t disc_find_neighbored_base(void) __naked
{
    /*
     * Look for a base among the filtered neighbor results. If we find one,
     * returns its neighbor packet byte. If not, returns zero.
     */

    __asm
        mov     r0, #(_ack_data + RF_ACK_NEIGHBOR)
1$:
        mov     a, @r0
        mov     dpl, a
        anl     a, #NB_BASE_MASK        ; Tests presence bit plus ID >= 24
        cjne    a, #NB_BASE_MASK, 2$
        ret
2$:
        inc     r0
        cjne    r0, #(_ack_data + RF_ACK_NEIGHBOR + 4), 1$

        mov     dpl, #0                 ; Found nothing
        ret
    __endasm ;
}


void disconnected_init(void)
{
    /*
     * Reset system state.
     * Returns by tailcall to graphics_render().
     */

    radio_connected = 0;
    nb_tx_id = 0;

    /*
     * Initialize disconnected-mode state
     */

    vram.mode = _SYS_VM_BG0_ROM;
    vram.flags = _SYS_VF_TOGGLE;

    disc_battery_draw = 0;
    disc_has_trophy = 0;

    disc_logo_x = 0;
    disc_logo_y = BATTERY_HEIGHT << FP_BITS;
    disc_logo_dx = 0;
    disc_logo_dy = 0;
    disc_score = 0;
    disc_nb_base = 0;

    disc_reset_sleep_timer();
    disc_reset_radio_state();

    /*
     * First frame: Update the whole screen, but draw only the
     * logo. If we do either of our usual partial-screen updates,
     * the first frame will have a brief flash of garbage on it.
     */

    draw_clear();
    draw_logo();
    vram.num_lines = 128;

    return graphics_render();
}


void disconnected_poll(void)
{
    /*
     * Handle one main loop cycle, in disconnected mode.
     * Returns by tailcall to graphics_render().
     */

    /*
     * Explicit frame toggle: Easier to quiesce after connection, as compared
     * to if we were using _SYS_VF_CONTINUOUS.
     */
    __asm
        mov     dptr, #_SYS_VA_FLAGS
        movx    a, @dptr
        xrl     a, #_SYS_VF_TOGGLE
        movx    @dptr, a
    __endasm ;

    if (disc_battery_draw) {
        /*
         * Update battery indicator (and the scoreboard, if it's visible)
         *
         * We don't draw the battery indicator at all until we've finished
         * sampling (battery_v is nonzero).
         */

        disc_battery_draw = 0;

        draw_clear();
        vram.num_lines = BATTERY_HEIGHT;

        /*
         * Battery indicator image
         */

        __asm
            mov     a, (_ack_data + RF_ACK_BATTERY_V)   ; Skip if we have no samples yet
            jz      2$

            DRAW_XY (11, 0)                             ; Draw battery outline
            mov     dptr, #_img_battery
            acall   _draw_image

            mov     dptr, #_img_battery_bars_4          ; Pick a bars image based on the battery voltage
            mov     a, (_ack_data + RF_ACK_BATTERY_V)
            addc    a, #(256 - BATTERY_THRESHOLD_3)
            jc      1$
            mov     dptr, #_img_battery_bars_3
            addc    a, #(BATTERY_THRESHOLD_3 - BATTERY_THRESHOLD_2)
            jc      1$
            mov     dptr, #_img_battery_bars_2
            addc    a, #(BATTERY_THRESHOLD_2 - BATTERY_THRESHOLD_1)
            jc      1$
            mov     dptr, #_img_battery_bars_1
        1$:

            DRAW_XY (12, 1)                             ; Draw the correct battery bars image
            acall   _draw_image

        2$:
        __endasm ;

        /*
         * Score counter (two-digit BCD) or trophy image
         */

        __asm

            jnb     _disc_has_trophy, 4$

            ; Draw trophy

            DRAW_XY (1, 0)
            mov     dptr, #_img_trophy
            acall   _draw_image
            sjmp    3$
        4$:

            ; Draw two-digit score counter

            mov     dptr, #XY(1,1)
            mov     a, _disc_score                      ; First, look at tens digit
            swap    a
            anl     a, #0x0F
            jz      3$                                  ; Hide if score < 10
            acall   _draw_digit
            mov     a, _disc_score
            acall   _draw_digit
        3$:
        __endasm ;

        return graphics_render();
    }

    /*
     * Update the rest of the screen (bouncing logo).
     *
     * We do this separately from the battery indicator, so we can pan the
     * logo independently from the battery indicator.
     */

    disc_battery_draw = 1;

    draw_clear();
    draw_logo();

    __asm

        ; Draw over the rest of the screen, starting just below the battery meter

        mov     dptr, #_SYS_VA_FIRST_LINE
        mov     a, #BATTERY_HEIGHT
        movx    @dptr, a
        inc     dptr
        mov     a, #(128 - BATTERY_HEIGHT)
        movx    @dptr, a

        ; Keep the integer portion of the X/Y location in (r6, r7).
        ; This will form the basis for the BG0 panning, plus we use it as
        ; part of the spring force which returns the logo to the center.
        ;
        ; Also, extract the integer portion of the velocity as (r4, r5).
        ; That will be used below in the damping calculations.

fp_bounce_axis_ret:

        mov     r0, #_disc_logo_x
        acall   _fp_integer
        mov     r6, a
        acall   _fp_integer
        mov     r7, a
        acall   _fp_integer
        mov     r4, a
        acall   _fp_integer
        mov     r5, a

        ; Convert these signed integer coordinates into unsigned BG0 panning amounts,
        ; and write them into VRAM.

        mov     a, r6
        jnb     acc.7, 3$           ; X >= 0
        add     a, #144
3$:     mov     dptr, #_SYS_VA_BG0_XY
        movx    @dptr, a

        mov     a, r7
        jnb     acc.7, 4$           ; Y >= 0
        add     a, #144
4$:     inc     dptr
        movx    @dptr, a

        ; Bounds checks and bouncing.
        ; We can test the integer version of our X/Y coordinates. If they
        ; are out of range, clamp the underlying FP value and invert the velocity.

        clr     _disc_bounce_type

        ; ---- X axis

        mov     a, r6                               ; Examine X axis
        jb      acc.7, 5$                           ; X negative? Test X_MIN

        add     a, #(255 - X_MAX)                   ; Test X_MAX
        jnc     6$

        mov     _disc_logo_x+0, #(X_MAX_FP >> 0)    ; Clamp to X_MAX
        mov     _disc_logo_x+1, #(X_MAX_FP >> 8)
        sjmp    7$                                  ; Bounce

5$:     add     a, #(257 - X_MIN)                   ; Test X_MIN
        jc      6$

        mov     _disc_logo_x+0, #(X_MIN_FP >> 0)    ; Clamp to X_MIN
        mov     _disc_logo_x+1, #(X_MIN_FP >> 8)
7$:     mov     r0, #_disc_logo_dx                  ; X bounce
        ajmp    _fp_bounce_axis
6$:

        ; ---- Y axis

        mov     a, r7                               ; Examine Y axis
        jb      acc.7, 8$                           ; Y negative? Test Y_MIN

        add     a, #(255 - Y_MAX)                   ; Test Y_MAX
        jnc     9$

        mov     _disc_logo_y+0, #(Y_MAX_FP >> 0)    ; Clamp to Y_MAX
        mov     _disc_logo_y+1, #(Y_MAX_FP >> 8)
        sjmp    10$                                 ; Bounce

8$:     add     a, #(257 - Y_MIN)                   ; Test Y_MIN
        jc      9$

        mov     _disc_logo_y+0, #(Y_MIN_FP >> 0)    ; Clamp to Y_MIN
        mov     _disc_logo_y+1, #(Y_MIN_FP >> 8)
        setb    _disc_bounce_type                   ; Bounces off of Y_MIN are good for score
        
10$:    mov     r0, #_disc_logo_dy                  ; Y bounce
        ajmp    _fp_bounce_axis
9$:

        ; Motion equations. This includes a spring force returning
        ; the logo to the center, an accelerometer force, plus some damping.
        ;
        ; Equivalent to:
        ;    disc_logo_x += (disc_logo_dx -= x + (disc_logo_dx >> FP_BITS) - ack_data.accel[0]);
        ;    disc_logo_y += (disc_logo_dy -= -BATTERY_HEIGHT + y + (disc_logo_dy >> FP_BITS) - ack_data.accel[1]);

        ; ---- X axis

        clr     a                                   ; Zero out accumulator
        mov     r2, a
        mov     r3, a
        mov     a, r6                               ; Spring return force
        acall   _add_s8_to_s16
        mov     a, r4                               ; Damping force
        acall   _add_s8_to_s16
        mov     a, (_ack_data + RF_ACK_ACCEL + 0)   ; Tilt force
        acall   _sub_s8_from_s16
        mov     r0, #_disc_logo_dx                  ; Integrate velocity and position
        acall   _fp_integrate_axis

        ; ---- Y axis

        mov     r2, #(256 - BATTERY_HEIGHT)         ; Compensate for BATTERY_HEIGHT
        mov     r3, #0xFF
        mov     a, r7                               ; Spring return force
        acall   _add_s8_to_s16
        mov     a, r5                               ; Damping force
        acall   _add_s8_to_s16
        mov     a, (_ack_data + RF_ACK_ACCEL + 1)   ; Tilt force
        acall   _sub_s8_from_s16
        mov     r0, #_disc_logo_dy                  ; Integrate velocity and position
        acall   _fp_integrate_axis

    __endasm ;
    
    /*
     * Check sleep timer
     */

    if (disc_sleep_timer == sensor_tick_counter_high)
        vram.mode = _SYS_VM_SLEEP;

    /*
     * Radio:
     *
     *   1. Check whether we're neighbored to a base. If so, we're pairing.
     *      Set a fixed address, and a channel derived from the neighbor ID.
     *
     *   2. If not, implement idle hopping. Once per TF0 period, hop to our
     *      alternate channel or back. This prevents us from getting stuck
     *      on a channel with lots of interference.
     */

    {
        uint8_t nb_base = disc_find_neighbored_base();

        if (nb_base == disc_nb_base) {
            // No change in neighbored base
            if (nb_base == 0 && disc_hop_timer == sensor_tick_counter_high) {
                // Idle for 1+ seconds. Hop to/from the alternate channel.
                ++disc_hop_timer;
                radio_idle_hop ^= 1;
                radio_set_idle_addr();
            }
        } else {
            disc_nb_base = nb_base;
            if (nb_base) {
                // New neighbored base
                CCPDATIA = nb_base & 0x1F;
                radio_set_pairing_addr();
            } else {
                // We no longer have a neighbored base
                disc_reset_radio_state();
            }
        }
    }

    return graphics_render();
}

