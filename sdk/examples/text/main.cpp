/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
#include "fontdata.h"
using namespace Sifteo;

static Metadata M = Metadata()
    .title("Text SDK Example");


struct TextRenderer {
    FB128Drawable &fb;
    UByte2 position;

    TextRenderer(FB128Drawable &fb) : fb(fb) {}

    void drawGlyph(char ch) {
        // Specifics of our font format
        uint8_t index = ch - ' ';
        const uint8_t *data = font_data + (index << 3) + index;
        uint8_t escapement = *(data++);
        const Int2 size = {8, 8};

        fb.bitmap(data, 1, position, size);
        position.x += escapement;
    }

    static unsigned measureGlyph(char ch) {
        uint8_t index = ch - ' ';
        const uint8_t *data = font_data + (index << 3) + index;
        return data[0];
    }

    void drawText(const char *str) {
        LOG(("Drawing text: \"%s\" at (%d, %d)\n",
            str, position.x, position.y));

        char c;
        while ((c = *str)) {
            str++;
            drawGlyph(c);
        }
    }

    static unsigned measureText(const char *str) {
        unsigned width = 0;
        char c;
        while ((c = *str)) {
            str++;
            width += measureGlyph(c);
        }
        return width;
    }

    void drawCentered(const char *str) {
        position.x = (LCD_width - measureText(str)) / 2;
        drawText(str);
        position.y += 8;
    }
};


static RGB565 makeColor(uint8_t alpha)
{
    // Linear interpolation between foreground and background

    const RGB565 bg = RGB565::fromRGB(0x31316f);
    const RGB565 fg = RGB565::fromRGB(0xc7c7fc);

    return bg.lerp(fg, alpha);
}


static void fadeInAndOut(Colormap &cm)
{
    const unsigned speed = 4;
    const unsigned hold = 100;
    
    LOG(("~ FADE ~\n"));
    
    for (unsigned i = 0; i < 0x100; i += speed) {
        cm[1] = makeColor(i);
        System::paint();
    }

    for (unsigned i = 0; i < hold; i++)
        System::paint();

    for (unsigned i = 0; i < 0x100; i += speed) {
        cm[1] = makeColor(255 - i);
        System::paint();
    }
}


void main()
{
    const CubeID cube(0);
    static VideoBuffer vid;

    cube.enable();

    /*
     * Init framebuffer, paint a solid background, and wait for it to draw.
     */

    vid.initMode(SOLID_MODE);
    vid.colormap[0] = makeColor(0);
    vid.attach(cube);
    System::paintSync();

    /*
     * Now set up a letterboxed 128x48 mode. This uses windowing to
     * start drawing on scanline 40, and draw a total of 48 scanlines.
     */

    vid.initMode(FB128, 40, 48);
    vid.colormap[0] = makeColor(0);

    /*
     * Draw some text!
     *
     * We do the drawing while the text is invisible (same fg and bg
     * colors), then fade it in and out using palette animation.
     */

    TextRenderer text(vid.fb128);

    while (1) {
        text.position.y = 16;
        text.fb.fill(0);
        text.drawCentered("Welcome to");
        text.drawCentered("the future of text.");
        fadeInAndOut(vid.colormap);

        text.position.y = 16;
        text.fb.fill(0);
        text.drawCentered("The future of text");
        text.drawCentered("is now.");
        fadeInAndOut(vid.colormap);

        text.position.y = 16;
        text.fb.fill(0);
        text.drawCentered("The future...");
        text.drawCentered("of text. It is here.");
        fadeInAndOut(vid.colormap);

        text.position.y = 16;
        text.fb.fill(0);
        text.drawCentered("Do you like");
        text.drawCentered("the future of text?");
        fadeInAndOut(vid.colormap);

        text.position.y = 6;
        text.fb.fill(0);
        text.drawCentered("We made all this text");
        text.drawCentered("just for you.");
        text.position.y += 4;
        text.drawCentered("It was supposed to be");
        text.drawCentered("a surprise.");
        fadeInAndOut(vid.colormap);

        text.position.y = 20;
        text.fb.fill(0);
        text.drawCentered("I hope you're happy.");
        fadeInAndOut(vid.colormap);

        text.position.y = 20;
        text.fb.fill(0);
        text.drawCentered("I really do.");
        fadeInAndOut(vid.colormap);

        text.position.y = 20;
        text.fb.fill(0);
        text.drawCentered("Are you still here?");
        fadeInAndOut(vid.colormap);

        text.position.y = 20;
        text.fb.fill(0);
        text.drawCentered("Why are you still here?");
        fadeInAndOut(vid.colormap);

        text.position.y = 16;
        text.fb.fill(0);
        text.drawCentered("Maybe you're not ready");
        text.drawCentered("for the future of text.");
        fadeInAndOut(vid.colormap);
    }
}
