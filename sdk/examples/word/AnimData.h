#ifndef ANIMDATA_H
#define ANIMDATA_H

#include <sifteo.h>

enum AnimIndex
{
    AnimIndex_2TileSlideL,
    AnimIndex_2TileSlideR,

    NumAnimIndexes
};

enum AnimObjIndex
{
    AnimObjIndex_Tile1,
    AnimObjIndex_Tile2,
    AnimObjIndex_Tile3,
};

enum AnimObjFlags
{
    AnimObjFlags_None,
    AnimObjFlags_Hide = (1 << 0),
};

struct AnimData
{
    Vec2 mPos;
    bool mVisible;
};

bool animDataGet(AnimIndex anim, AnimObjIndex obj, AnimData &data);


#endif // ANIMDATA_H
