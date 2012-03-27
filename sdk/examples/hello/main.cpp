/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo SDK Example.
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <sifteo.h>
#include "assets.gen.h"

using namespace Sifteo;

void main()
{
    const CubeID myCube(0);
    static VideoBuffer vid;
    
    myCube.enable();
    vid.attach(myCube);
    vid.setMode(vid.BG0_ROM);
}


#if 0

static void onAccelChange(void *context, _SYSCubeID cid)
{
    Byte2 accel = cubes[cid].physicalAccel();

    String<64> str;
    str << "Accel: " << Hex(accel.x + 0x80, 2) << " " << Hex(accel.y + 0x80, 2);
    LOG_STR(str);

    VidMode_BG0 vid(cubes[cid].vbuf);
    vid.BG0_text(Vec2(2,3), Font, str);
    vid.BG0_setPanning(accel / -2);
}

static void init()
{
    Metadata()
        .title("Hello World SDK Example")
        .icon(GameIcon);
    
    // Synchronously load the BootAssets. This should be quick.
    for (unsigned i = 0; i < NUM_CUBES; i++) {

        // XXX: Manually assigning base address for now
        BootAssets.cubes[i].baseAddr = 512 * 4;
        MainAssets.cubes[i].baseAddr = 512 * 6;

        cubes[i].enable(i);
        cubes[i].loadAssets(BootAssets);
    }
    BootAssets.wait();

    // Start asynchronously loading the MainAssets
    for (unsigned i = 0; i < NUM_CUBES; i++) {
        cubes[i].loadAssets(MainAssets);
    }

    for (unsigned i = 0; i < NUM_CUBES; i++) {
        VidMode_BG0 vid(cubes[i].vbuf);
        vid.init();
        vid.clear(WhiteTile);
    }

    int frame = 0;
    while (MainAssets.isLoading()) {

        // Animate Kirby running across the screen as MainAssets loads.
        for (unsigned i = 0; i < NUM_CUBES; i++) {
            VidMode_BG0 vid(cubes[i].vbuf);

            Int2 pan = Vec2(-cubes[i].assetProgress(MainAssets,
                                vid.LCD_width - Kirby.pixelWidth()),
                            -(int)(vid.LCD_height - Kirby.pixelHeight()) / 2);

            LOG_VEC2(pan);
            vid.BG0_drawAsset(Vec2(0,0), Kirby, frame);
            vid.BG0_setPanning(pan);
        }

        if (++frame == Kirby.frames)
            frame = 0;

        /*
         * Frame rate limit: Drawing has higher priority than asset loading,
         * so in order to load assets quickly we need to explicitly leave some
         * time for the system to send asset data over the radio.
         */
        for (unsigned i = 0; i < 4; i++)
            System::paint();
    }
}

void main()
{
    init();

    _SYS_setVector(_SYS_CUBE_ACCELCHANGE, (void*)onAccelChange, NULL);

    for (unsigned i = 0; i < NUM_CUBES; i++) {
        VidMode_BG0 vid(cubes[i].vbuf);
        vid.clear(WhiteTile);
        vid.BG0_text(Vec2(2,1), Font, "Hello World!");
        vid.BG0_drawAsset(Vec2(1,10), Logo);
        onAccelChange(NULL, cubes[i].id());
    } 

    int frame = 0;
    while (1) {
        for (unsigned i = 0; i < NUM_CUBES; i++) {
            VidMode_BG0 vid(cubes[i].vbuf);
            vid.BG0_drawAsset(Vec2(7,6), Ball, frame);
        }
        if (++frame == Ball.frames)
            frame = 0;
        System::paint();
    }
}

#endif
