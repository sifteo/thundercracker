/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
#include <stdio.h>
#include <string.h>
#include "fontdata.h"

using namespace Sifteo;

static Cube cube(0);

static unsigned draw_x;
static unsigned draw_y;


static void draw_glyph(char ch)
{
    uint8_t index = ch - ' ';
    const uint8_t *data = font_data + (index << 3) + index;
    uint8_t escapement = *(data++);
    uint16_t dest = (draw_y << 4) | (draw_x >> 3);
    unsigned shift = draw_x & 7;

    for (unsigned i = 0; i < 8; i++) {
        cube.vbuf.pokeb(dest, cube.vbuf.peekb(dest) | (data[i] << shift));
        dest += 16;
    }

    if (shift) {
        dest += -16*8 + 1;
        shift = 8 - shift;

        for (unsigned i = 0; i < 8; i++) {
            cube.vbuf.pokeb(dest, cube.vbuf.peekb(dest) | (data[i] >> shift));
            dest += 16;
        }
    }

    draw_x += escapement;
}

static unsigned measure_glyph(char ch)
{
    uint8_t index = ch - ' ';
    const uint8_t *data = font_data + (index << 3) + index;
    return data[0];
}

static void draw_text(const char *str)
{
    char c;
    while ((c = *str)) {
        str++;
        draw_glyph(c);
    }
}

static unsigned measure_text(const char *str)
{
    unsigned width = 0;
    char c;
    while ((c = *str)) {
        str++;
        width += measure_glyph(c);
    }
    return width;
}

static void center_text_line(const char *str)
{
    draw_x = (128 - measure_text(str)) >> 1;
    draw_text(str);
    draw_y += 8;
}

static void erase()
{
    for (unsigned i = 0; i < sizeof cube.vbuf.sys.vram.fb / 2; i++)
        cube.vbuf.poke(i, 0);
}

static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
    /*
     * Round to the nearest 5/6 bit color. Note that simple
     * bit truncation does NOT produce the best result!
     */
    uint16_t r5 = ((uint16_t)r * 31 + 128) / 255;
    uint16_t g6 = ((uint16_t)g * 63 + 128) / 255;
    uint16_t b5 = ((uint16_t)b * 31 + 128) / 255;
    return (r5 << 11) | (g6 << 5) | b5;
}

static uint16_t color_lerp(uint8_t alpha) {
    /*
     * Linear interpolation between foreground and background
     */

    const unsigned bg_r = 0x31;
    const unsigned bg_g = 0x31;
    const unsigned bg_b = 0x6f;

    const unsigned fg_r = 0xc7;
    const unsigned fg_g = 0xc7;
    const unsigned fg_b = 0xfc;
    
    const uint8_t invalpha = 0xff - alpha;

    return rgb565( (bg_r * invalpha + fg_r * alpha) / 0xff,
                   (bg_g * invalpha + fg_g * alpha) / 0xff,
                   (bg_b * invalpha + fg_b * alpha) / 0xff );
}
                   
static void fade_in_and_out() {
    const unsigned speed = 4;
    const unsigned hold = 100;
    
    for (unsigned i = 0; i < 0x100; i += speed) {
        cube.vbuf.poke(offsetof(_SYSVideoRAM, colormap) / 2 + 1,
                       color_lerp(i));
        System::paint();
    }

    for (unsigned i = 0; i < hold; i++)
        System::paint();

    for (unsigned i = 0; i < 0x100; i += speed) {
        cube.vbuf.poke(offsetof(_SYSVideoRAM, colormap) / 2 + 1,
                       color_lerp(0xFF - i));
        System::paint();
    }
}

void siftmain()
{
    cube.enable();

    /*
     * Init framebuffer, paint a solid background
     */

    cube.vbuf.init();
    memset(cube.vbuf.sys.vram.words, 0, sizeof cube.vbuf.sys.vram.words);
    cube.vbuf.sys.vram.mode = _SYS_VM_SOLID;
    cube.vbuf.sys.vram.num_lines = 128;
    cube.vbuf.sys.vram.colormap[0] = color_lerp(0);
    cube.vbuf.sys.vram.colormap[1] = color_lerp(0);

    // And wait for it to fully draw
    System::paintSync();

    /*
     * Now set up a letterboxed 128x48 mode
     */

    cube.vbuf.poke(0x3fc/2, 0x3028);
    cube.vbuf.pokeb(offsetof(_SYSVideoRAM, mode), _SYS_VM_FB128);

    /*
     * Draw some text!
     *
     * We do the drawing while the text is invisible (same fg and bg
     * colors), then fade it in and out using palette animation.
     */

    while (1) {
        draw_y = 16;
        erase();
        center_text_line("Welcome to");
        center_text_line("the future of text.");
        fade_in_and_out();

        draw_y = 16;
        erase();
        center_text_line("The future of text");
        center_text_line("is now.");
        fade_in_and_out();

        draw_y = 16;
        erase();
        center_text_line("The future...");
        center_text_line("of text. It is here.");
        fade_in_and_out();

        draw_y = 16;
        erase();
        center_text_line("Do you like");
        center_text_line("the future of text?");
        fade_in_and_out();

        draw_y = 6;
        erase();
        center_text_line("We made all this text");
        center_text_line("just for you.");
        draw_y += 4;
        center_text_line("It was supposed to be");
        center_text_line("a surprise.");
        fade_in_and_out();

        draw_y = 20;
        erase();
        center_text_line("I hope you're happy.");
        fade_in_and_out();

        draw_y = 20;
        erase();
        center_text_line("I really do.");
        fade_in_and_out();

        draw_y = 20;
        erase();
        center_text_line("Are you still here?");
        fade_in_and_out();

        draw_y = 20;
        erase();
        center_text_line("Why are you still here?");
        fade_in_and_out();

        draw_y = 16;
        erase();
        center_text_line("Maybe you're not ready");
        center_text_line("for the future of text.");
        fade_in_and_out();
    }
}
