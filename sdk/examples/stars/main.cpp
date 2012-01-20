/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
#include "assets.gen.h"

#define NUM_CUBES 3

using namespace Sifteo;

struct FVec2 {
    float x, y;
};

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

        VidMode_BG0 vid(cube.vbuf);
        vid.init();
        vid.BG0_drawAsset(Vec2(0,0), Background);
 
        // Clear BG1/SPR before switching modes
        _SYS_vbuf_fill(&cube.vbuf.sys, _SYS_VA_BG1_BITMAP/2, 0, 16);
        _SYS_vbuf_fill(&cube.vbuf.sys, _SYS_VA_BG1_TILES/2,
                       cube.vbuf.indexWord(Font.index), 32);
        _SYS_vbuf_fill(&cube.vbuf.sys, _SYS_VA_SPR, 0, 8*5/2);
        
        // Allocate 16x2 tiles on BG1 for text at the bottom of the screen
        _SYS_vbuf_fill(&cube.vbuf.sys, _SYS_VA_BG1_BITMAP/2 + 14, 0xFFFF, 2);

        // Switch modes
        _SYS_vbuf_pokeb(&cube.vbuf.sys, offsetof(_SYSVideoRAM, mode), _SYS_VM_BG0_SPR_BG1);
    }
    
    void update(float timeStep)
    {
        /*
         * Action triggers
         */
        
        switch (frame) {
        
        case 0:
            text.x = 0;
            text.y = -20;
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
            text.x = -4;
            text.y = 128;
            textTarget.x = -4;
            textTarget.y = 128-16;
            
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
         
        _SYSAccelState accel;
        _SYS_getAccel(cube.id(), &accel);
        
        /*
         * Update starfield animation
         */
        
        for (unsigned i = 0; i < numStars; i++) {
            resizeSprite(i, 8, 8);
            setSpriteImage(i, Star.index + (frame % Star.frames) * Star.width * Star.height);
            moveSprite(i, stars[i].x + (64 - 3.5f), stars[i].y + (64 - 3.5f));
           
            stars[i].x += timeStep * (stars[i].vx + accel.x * starTiltSpeed);
            stars[i].y += timeStep * (stars[i].vy + accel.y * starTiltSpeed);
            
            if (stars[i].x > 80 || stars[i].x < -80 ||
                stars[i].y > 80 || stars[i].y < -80)
                initStar(i);
        }
        
        /*
         * Update global animation
         */

        frame++;
        fpsTimespan += timeStep;

        VidMode_BG0 vid(cube.vbuf);
        bg.x += timeStep * (accel.x * bgTiltSpeed);
        bg.y += timeStep * (accel.y * bgTiltSpeed - bgScrollSpeed);
        vid.BG0_setPanning(Vec2(bg.x + 0.5f, bg.y + 0.5f));
        
        text.x += (textTarget.x - text.x) * textSpeed;
        text.y += (textTarget.y - text.y) * textSpeed;
        panBG1(text.x + 0.5f, text.y + 0.5f);
    }

private:   
    struct {
        float x, y, vx, vy;
    } stars[numStars];
    
    Cube &cube;
    unsigned frame;
    FVec2 bg, text, textTarget;
    
    float fpsTimespan;
    
    void moveSprite(int id, int x, int y)
    {
        uint8_t xb = -x;
        uint8_t yb = -y;

        uint16_t word = ((uint16_t)xb << 8) | yb;
        uint16_t addr = ( offsetof(_SYSVideoRAM, spr[0].pos_y)/2 +
                        sizeof(_SYSSpriteInfo)/2 * id ); 

        _SYS_vbuf_poke(&cube.vbuf.sys, addr, word);
    }

    void resizeSprite(int id, int w, int h)
    {
        uint8_t xb = -w;
        uint8_t yb = -h;

        uint16_t word = ((uint16_t)xb << 8) | yb;
        uint16_t addr = ( offsetof(_SYSVideoRAM, spr[0].mask_y)/2 +
                          sizeof(_SYSSpriteInfo)/2 * id ); 

        _SYS_vbuf_poke(&cube.vbuf.sys, addr, word);
    }

    void setSpriteImage(int id, int tile)
    {
        uint16_t word = VideoBuffer::indexWord(tile);
        uint16_t addr = ( offsetof(_SYSVideoRAM, spr[0].tile)/2 +
                          sizeof(_SYSSpriteInfo)/2 * id ); 

        _SYS_vbuf_poke(&cube.vbuf.sys, addr, word);
    }

    void panBG1(int8_t x, int8_t y)
    {
        _SYS_vbuf_poke(&cube.vbuf.sys, offsetof(_SYSVideoRAM, bg1_x) / 2,
                       ((uint8_t)x) | ((uint16_t)(uint8_t)y << 8));
    }

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
        stars[id].x = 0;
        stars[id].y = 0;
        
        gRandom.randint(0, 10);
        
        float angle = gRandom.uniform(0, 2 * M_PI);
        float speed = gRandom.uniform(starEmitSpeed * 0.5f, starEmitSpeed);
    
        stars[id].vx = cosf(angle) * speed;
        stars[id].vy = sinf(angle) * speed;
    }
};


static const char* sideNames[] = {
  "top", "left", "bottom", "right"  
};

void neighbor_add(_SYSCubeID c0, _SYSSideID s0, _SYSCubeID c1, _SYSSideID s1) {
    LOG(("neighbor add:\t%d/%s\t%d/%s\n", c0, sideNames[s0], c1, sideNames[s1]));
}

void neighbor_remove(_SYSCubeID c0, _SYSSideID s0, _SYSCubeID c1, _SYSSideID s1) {
    LOG(("neighbor remove:\t%d/%s\t%d/%s\n", c0, sideNames[s0], c1, sideNames[s1]));
}

void accel(_SYSCubeID c) {
    LOG(("accelerometer changed\n"));
}

void siftmain()
{
    LOG(("HELLO, WORLD\n"));
    //_SYS_vectors.cubeEvents.accelChange = accel;
    _SYS_vectors.neighborEvents.add = neighbor_add;
    _SYS_vectors.neighborEvents.remove = neighbor_remove;
    
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
