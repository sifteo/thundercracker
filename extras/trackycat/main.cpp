#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

static AssetSlot MainSlot = AssetSlot::allocate()
    .bootstrap(GameAssets);

static Metadata M = Metadata()
    .title("Trackycat~")
    .cubeRange(1);

static const AssetTracker *mods[] = { &Bubbles, &Vox, &Jungle1, &Guitar };
static unsigned current = 0;
static AudioTracker tracker;

void onTouch(void * unused, unsigned cube)
{
    if (CubeID(cube).isTouching()) {
        current = (current + 1) % arraysize(mods);
        tracker.stop();
        tracker.play(*mods[current]);
    }
}

void main()
{
    const CubeID cube(0);
    static VideoBuffer vid;

    tracker.play(*mods[current]);
    Events::cubeTouch.set(onTouch);

    vid.initMode(BG0);
    vid.attach(cube);

    while (1) {
        unsigned frame = SystemTime::now().cycleFrame(0.5, Cat.numFrames());
        vid.bg0.image(vec(0,0), Cat, frame);
        System::paint();
    }
}

