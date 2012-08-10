#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

static AssetSlot MainSlot = AssetSlot::allocate()
    .bootstrap(MyAssets);

static Metadata M = Metadata()
    .title("Drum Kit")
    .package("com.sifteo.extras.drumkit", "1.0")
    .icon(Icon)
    .cubeRange(1);

VideoBuffer vid[1];
MotionBuffer<> motion[1];
CubeID drumCube(0);
Float2 drumVel, drumPos;

void activateDrum()
{
    AudioChannel(0).play(DrumSample);
    drumVel.y += 5;
}

void onTouch(void*, unsigned id)
{
    if (id == drumCube && drumCube.isTouching())
        activateDrum();
}

void main()
{
    auto& v = vid[drumCube];
    v.initMode(BG0_BG1);
    v.attach(drumCube);
    motion[drumCube].attach(drumCube);

    for (unsigned y = 0; y < 18; y += 3)
        for (unsigned x = 0; x < 18; x += 3)
            v.bg0.image(vec(x,y), Background);

    v.bg1.fillMask(vec(0,0), DrumIcon.tileSize());
    v.bg1.image(vec(0,0), DrumIcon);

    Events::cubeTouch.set(onTouch);

    bool prevWobble;

    while (1) {
        MotionMedian median;
        median.calculate(motion[drumCube], 10);
        bool wobble = median.range().len2() > (prevWobble ? 1000 : 5000);
        LOG_INT(wobble);
        if (wobble && !prevWobble)
            activateDrum();
        prevWobble = wobble;

        int pan = SystemTime::now().cycleFrame(2.0f, 24);
        vid[drumCube].bg0.setPanning(vec(pan, pan));

        drumPos += drumVel;
        drumVel += drumPos * -0.1;
        drumVel *= 0.9f;
        v.bg1.setPanning(-LCD_center + DrumIcon.pixelExtent() + drumPos);

        System::paint();
    }
}
