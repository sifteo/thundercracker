#include <sifteo.h>
using namespace Sifteo;

void main()
{
    Array<Volume, 64> gGameList;
    Volume::list(Volume::T_GAME, gGameList);

    if (gGameList.empty()) {
        LOG("LAUNCHER: No games installed\n");
        while (1)
            System::paint();
    } else {
        gGameList[0].exec();
    }
}
