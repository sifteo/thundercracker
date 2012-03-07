/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
#include "assets.gen.h"

using namespace Sifteo;

Math::Random gRandom;

class StarDemo {
public:

    static const unsigned numStars = 8;
    
    static const float textSpeed = 0.2f;
    static const float bgScrollSpeed = 10.0f;
    static const float bgTiltSpeed = 1.0f;
    static const float starEmitSpeed = 60.0f;
    static const float starTiltSpeed = 3.0f;

    StarDemo(Cube &cube)
        : cube(cube)
    {}

    void init()
    {
        frame = 0;
        bg.x = 0;
        bg.y = 0;
        
        for (unsigned i = 0; i < numStars; i++)
            initStar(i);

        VidMode_BG0_SPR_BG1 vid(cube.vbuf);
        vid.clear();
        vid.BG0_drawAsset(Vec2(0,0), Background);
        
        // Allocate 16x2 tiles on BG1 for text at the bottom of the screen
        _SYS_vbuf_fill(&cube.vbuf.sys, _SYS_VA_BG1_BITMAP/2 + 14, 0xFFFF, 2);

        vid.set();
    }
    
    void update(float timeStep)
    {
        /*
         * Action triggers
         */
        
        switch (frame) {
        
        case 0:
            text.set(0, -20);
            textTarget = text;
            break;
        
        case 100:
            writeText(" Hello traveler ");
            textTarget.y = 8;
            break;

        case 200:
            textTarget.y = -20;
            break;
            
        case 250:
            writeText(" Tilt me to fly ");
            textTarget.y = 8;
            break;
            
        case 350:
            textTarget.y = -20;
            break;
        
        case 400:
            writeText(" Enjoy subspace ");
            textTarget.y = 64-8;
            break;
        
        case 500:
            textTarget.x = 128;
            break;
        
        case 550:
            text.x = 128;
            textTarget.x = 0;
            writeText(" and be careful ");
            break;

        case 650:
            textTarget.y = -20;
            break;
        
        case 800: {
            text.set(-4, 128);
            textTarget.set(-4, 128-16);
            
            /*
             * This is the *average* FPS so far, as measured by the game.
             * If the cubes aren't all running at the same frame rate, this
             * will be roughly the rate of the slowest cube, due to how the
             * flow control in System::paint() works.
             */

            String<17> buf;
            float fps = frame / fpsTimespan;
            buf << (int)fps << "." << Fixed((int)(fps * 100) % 100, 2, true)
                << " FPS avg        ";
            writeText(buf);
            break;
        }
        
        case 900:
            textTarget.y = 128;
            break;
            
        case 2048:
            frame = 0;
            fpsTimespan = 0;
            break;
        }
       
        /*
         * We respond to the accelerometer
         */
         
        Vec2 accel = cube.physicalAccel();
        Float2 tilt(accel.x, accel.y);
        tilt *= starTiltSpeed;
        
        /*
         * Update starfield animation
         */
        
        VidMode_BG0_SPR_BG1 vid(cube.vbuf);
        for (unsigned i = 0; i < numStars; i++) {
            const Float2 center(64 - 3.5f, 64 - 3.5f);
            vid.resizeSprite(i, 8, 8);
            vid.setSpriteImage(i, Star.index + (frame % Star.frames) * Star.width * Star.height);
            vid.moveSprite(i, stars[i].pos + center);
            
            stars[i].pos += timeStep * (stars[i].velocity + tilt);
            
            if (stars[i].pos.x > 80 || stars[i].pos.x < -80 ||
                stars[i].pos.y > 80 || stars[i].pos.y < -80)
                initStar(i);
        }
        
        /*
         * Update global animation
         */

        frame++;
        fpsTimespan += timeStep;

        Float2 bgVelocity(accel.x * bgTiltSpeed, accel.y * bgTiltSpeed - bgScrollSpeed);
        bg += timeStep * bgVelocity;
        vid.BG0_setPanning(Vec2::round(bg));
        
        text += (textTarget - text) * textSpeed;
        vid.BG1_setPanning(Vec2::round(text));
    }

private:   
    struct {
        Float2 pos, velocity;
    } stars[numStars];
    
    Cube &cube;
    unsigned frame;
    Float2 bg, text, textTarget;
    float fpsTimespan;

    void writeText(const char *str)
    {
        // Text on BG1, in the 16x2 area we allocated
        uint16_t addr = offsetof(_SYSVideoRAM, bg1_tiles) / 2;
        char c;
        
        while ((c = *(str++))) {
            uint16_t index = (c - ' ') * 2 + Font.index;
            cube.vbuf.pokei(addr, index);
            cube.vbuf.pokei(addr + 16, index + 1);
            addr++;
        }
    }

    void initStar(int id)
    {
        float angle = gRandom.uniform(0, 2 * M_PI);
        float speed = gRandom.uniform(starEmitSpeed * 0.5f, starEmitSpeed);    
        stars[id].pos.set(0, 0);
        stars[id].velocity.setPolar(angle, speed);
    }
};


void siftmain()
{
    static Cube cubes[] = { Cube(0), Cube(1) };
    static StarDemo demos[] = { StarDemo(cubes[0]), StarDemo(cubes[1]) };
    
    for (unsigned i = 0; i < arraysize(cubes); i++) {
        cubes[i].enable();
        cubes[i].loadAssets(GameAssets);

        VidMode_BG0_ROM rom(cubes[i].vbuf);
        rom.init();
        rom.BG0_text(Vec2(1,1), "Loading...");
        rom.BG0_text(Vec2(1,13), "zomg it's full\nof stars!!!");
    }

    for (;;) {
        bool done = true;

        for (unsigned i = 0; i < arraysize(cubes); i++) {
            VidMode_BG0_ROM rom(cubes[i].vbuf);
            rom.BG0_progressBar(Vec2(0,7), cubes[i].assetProgress(GameAssets, VidMode_BG0::LCD_width), 2);
            if (!cubes[i].assetDone(GameAssets))
                done = false;
        }

        System::paint();

        if (done)
            break;
    }

    for (unsigned i = 0; i < arraysize(demos); i++)
        demos[i].init();
    
    float lastTime = System::clock();
    while (1) {
        float now = System::clock();
        float dt = now - lastTime;
        lastTime = now;

        for (unsigned i = 0; i < arraysize(demos); i++)
            demos[i].update(dt);
        
        System::paint();
    }
}
