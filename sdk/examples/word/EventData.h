#ifndef EVENTDATA_H
#define EVENTDATA_H

#include <sifteo.h>
using namespace Sifteo;

union EventData
{
    EventData() {}
    struct
    {
        const char* mWord;
        int mOffLengthIndex;
    } mNewAnagram;

    struct
    {
        const char* mWord;
        Cube::ID mCubeIDStart;
    } mWordFound;

    struct
    {
        Cube::ID mCubeIDStart;
    } mWordBroken;

    struct
    {
        bool mFirst;
    } mEnterState;
};

#endif // EVENTDATA_H
