#include "Anim.h"
#include "GameStateMachine.h"
#include "assets.gen.h"

enum AnimIndex
{
    AnimIndex_Tile1Idle,
    AnimIndex_Tile1SlideL,
    AnimIndex_Tile1SlideR,
    AnimIndex_Tile1OldWord,
    AnimIndex_Tile1NewWord,
    AnimIndex_Tile1EndofRoundScored,
    AnimIndex_Tile1ShuffleScored,

    AnimIndex_Tile2Idle,
    AnimIndex_Tile2SlideL,
    AnimIndex_Tile2SlideR,
    AnimIndex_Tile2OldWord,
    AnimIndex_Tile2NewWord,
    AnimIndex_Tile2EndofRoundScored,
    AnimIndex_Tile2ShuffleScored,

    AnimIndex_Tile3Idle,
    AnimIndex_Tile3SlideL,
    AnimIndex_Tile3SlideR,
    AnimIndex_Tile3OldWord,
    AnimIndex_Tile3NewWord,
    AnimIndex_Tile3EndofRoundScored,
    AnimIndex_Tile3ShuffleScored,

    NumAnimIndexes
};


struct AnimObjData
{
    const AssetImage *mAsset;
    const AssetImage *mAltAsset;
    uint32_t mInvisibleFrames; // bitmask
    unsigned char mNumFrames;
    const Vec2 *mPositions;
};

struct AnimData
{
    float mDuration;
    bool mLoop;
    unsigned char mNumObjs;
    const AnimObjData *mObjs;
};

// FIXME write a tool to hide all this array/struct nesting ugliness
// FIXME reuse stuff with indexing
const static Vec2 positions[] =
{

    Vec2(2, 2),
    Vec2(8, 2),
    Vec2(7, 2),
    Vec2(6, 2),
    Vec2(5, 2),
    Vec2(4, 2),
    Vec2(3, 2),
    Vec2(2, 2),
    Vec2(2, 2),
    Vec2(2, 2),
    Vec2(2, 2),
    Vec2(3, 2),
    Vec2(4, 2),
    Vec2(5, 2),
    Vec2(6, 2),
    Vec2(7, 2),
    Vec2(8, 2),
    Vec2(8, 2),
    Vec2(8, 2),
    Vec2(8, 2),
    Vec2(7, 2),
    Vec2(6, 2),
    Vec2(5, 2),
    Vec2(4, 2),
    Vec2(3, 2),
    Vec2(2, 2),
};

const static AnimObjData animObjData[] =
{
    // AnimIndex_Tile2Idle
    { &Tile2, &Tile2, 0x0, 1, &positions[0]},
    { &Tile2, &Tile2, 0x0, 1, &positions[1]},

    // AnimIndex_Tile2SlideL
    { &Tile2, &Tile2, 0x0, 10, &positions[7]},
    { &Tile2, &Tile2, 0x0, 7, &positions[2]},

    // AnimIndex_Tile2SlideR
    { &Tile2, &Tile2, 0x0, 7, &positions[10]},
    { &Tile2, &Tile2, 0x0, 10, &positions[16]},

    // AnimIndex_Tile2OldWord
    { &Tile2Glow, &Tile2, 0x0, 1, &positions[0]},
    { &Tile2Glow, &Tile2, 0x0, 1, &positions[1]},
};

const static AnimData animData[] =
{

    //AnimIndex_Tile1Idle,
    { 1.f, true, 2, &animObjData[0]},
    //AnimIndex_Tile1SlideL,
    { 1.f, true, 2, &animObjData[0]},
    //AnimIndex_Tile1SlideR,
    { 1.f, true, 2, &animObjData[0]},
    //AnimIndex_Tile1OldWord,
    { 1.f, true, 2, &animObjData[0]},
    //AnimIndex_Tile1NewWord,
    { 1.f, true, 2, &animObjData[0]},
    //AnimIndex_Tile1EndofRoundScored,
    { 1.f, true, 2, &animObjData[0]},
    //AnimIndex_Tile1ShuffleScored,
    { 1.f, true, 2, &animObjData[0]},

    // AnimIndex_Tile2Idle
    { 1.f, true, 2, &animObjData[0]},

    // AnimIndex_Tile2SlideL
    { 1.f, false, 2, &animObjData[2]},

    // AnimIndex_Tile2SlideR
    { 1.f, false, 2, &animObjData[4]},

    // AnimIndex_Tile2OldWord
    { 1.f, true, 2, &animObjData[6]},

    //AnimIndex_Tile2NewWord,
    { 1.f, true, 2, &animObjData[6]},
    //AnimIndex_Tile2EndofRoundScored,
    { 1.f, true, 2, &animObjData[0]},
    //AnimIndex_Tile2ShuffleScored,
    { 1.f, true, 2, &animObjData[0]},

    //AnimIndex_Tile3Idle,
    { 1.f, true, 2, &animObjData[0]},
    //AnimIndex_Tile3SlideL,
    { 1.f, true, 2, &animObjData[0]},
    //AnimIndex_Tile3SlideR,
    { 1.f, true, 2, &animObjData[0]},
    //AnimIndex_Tile3OldWord,
    { 1.f, true, 2, &animObjData[0]},
    //AnimIndex_Tile3NewWord,
    { 1.f, true, 2, &animObjData[0]},
    //AnimIndex_Tile3EndofRoundScored,
    { 1.f, true, 2, &animObjData[0]},
    //AnimIndex_Tile3ShuffleScored,
    { 1.f, true, 2, &animObjData[0]},

};

bool animPaint(AnimType animT,
               VidMode_BG0_SPR_BG1 &vid,
               BG1Helper *bg1,
               float animTime,
               const AnimParams *params)
{
    const static AssetImage* fonts[] =
    {
        &Font1Letter, &Font2Letter, &Font3Letter,
    };
    const AssetImage& font = *fonts[GameStateMachine::getCurrentMaxLettersPerCube() - 1];
    AnimIndex anim = (AnimIndex)(animT + NumAnimTypes * (GameStateMachine::getCurrentMaxLettersPerCube() - 1));
    STATIC_ASSERT(NumAnimTypes * MAX_LETTERS_PER_CUBE == NumAnimIndexes);
    STATIC_ASSERT(arraysize(animData) == NumAnimIndexes);
    const AnimData &data = animData[anim];

    float animPct =
            data.mLoop ?
                fmodf(animTime, data.mDuration)/data.mDuration :
                MIN(1.f, animTime/data.mDuration);
    const int MAX_ROWS = 16, MAX_COLS = 16;
    for (unsigned i = 0; i < data.mNumObjs; ++i)
    {
        const AnimObjData &objData = data.mObjs[i];
        unsigned char frame =
                (unsigned char) ((float)objData.mNumFrames * animPct);
        frame = MIN(frame, objData.mNumFrames - 1);
        // clip to screen
        Vec2 pos(objData.mPositions[frame]);
        // FIXME write utility AABB class
        if (pos.x >= MAX_ROWS || pos.y >= MAX_COLS)
        {
            continue; // totally offscreen
        }
        pos.x = MAX(pos.x, 0);
        pos.y = MAX(pos.y, 0);
        Vec2 clipOffset = pos - objData.mPositions[frame];
        if (clipOffset.x >= (int)objData.mAsset->width ||
            clipOffset.y >= (int)objData.mAsset->height)
        {
            continue; // totally offscreen
        }
        Vec2 size(objData.mAsset->width, objData.mAsset->height);
        size.x -= abs<int>(clipOffset.x);
        size.y -= abs<int>(clipOffset.y);
        size.x = MIN(size.x, MAX_ROWS - pos.x);
        size.y = MIN(size.y, MAX_COLS - pos.y);
        ASSERT(size.x > 0);
        ASSERT(size.y > 0);
        ASSERT(objData.mAsset);
        unsigned assetFrame = 0;
        if (anim != AnimIndex_Tile2Idle )
        {
            DEBUG_LOG(("anim time:\t%f\tpct:%f\tframe:\t%d\n", animTime, animPct, frame));
        }

        unsigned fontFrame = font.frames + 1;
        if (params && params->mLetters && bg1)
        {
            if (i < GameStateMachine::getCurrentMaxLettersPerCube())
            {
                fontFrame = params->mLetters[i] - (int)'A';
            }
        }

        if (fontFrame < font.frames)
        {
            Vec2 letterPos(pos);
            letterPos.y += 3; // TODO
            vid.BG0_drawPartialAsset(pos, clipOffset, size, *objData.mAsset, assetFrame);
            bg1->DrawPartialAsset(letterPos, Vec2(0,0), Vec2(size.x, font.height), font, fontFrame);
        }
        else
        {
            vid.BG0_drawPartialAsset(pos, clipOffset, size, *objData.mAltAsset, assetFrame);
        }

        switch (anim)
        {
        case AnimIndex_Tile2SlideL:
            break;

        default:
            break;
        }
    }

    // TODO fold border painting into the paint code
    const bool leftNeighbor = params ? params->mLeftNeighbor : false;
    const bool rightNeighbor = params ? params->mRightNeighbor : false;
    if (leftNeighbor || (rightNeighbor && animT != AnimType_NewWord && animT != AnimType_OldWord))
    {
        // don't draw left border
        vid.BG0_drawPartialAsset(Vec2(0, 14), Vec2(1, 0), Vec2(16, 2), BorderBottom);
    }
    else if (bg1)
    {
        // draw left border
        vid.BG0_drawPartialAsset(Vec2(0, 2), Vec2(0, 1), Vec2(2, 14), BorderLeft);
        bg1->DrawPartialAsset(Vec2(0, 1), Vec2(0, 0), Vec2(2, 1), BorderLeft);
        bg1->DrawPartialAsset(Vec2(1, 14), Vec2(0, 0), Vec2(1, 2), BorderBottom);
        vid.BG0_drawPartialAsset(Vec2(2, 14), Vec2(1, 0), Vec2(14, 2), BorderBottom);
    }

    if (rightNeighbor || (leftNeighbor && animT != AnimType_NewWord && animT != AnimType_OldWord))
    {
        // don't draw right border
        vid.BG0_drawPartialAsset(Vec2(0, 0), Vec2(0, 0), Vec2(16, 2), BorderTop);
    }
    else if (bg1)
    {
        // draw right border
        vid.BG0_drawPartialAsset(Vec2(14, 0), Vec2(0, 0), Vec2(2, 14), BorderRight);
        bg1->DrawPartialAsset(Vec2(14, 14), Vec2(0, 16), Vec2(2, 1), BorderRight);
        bg1->DrawPartialAsset(Vec2(14, 0), Vec2(16, 0), Vec2(1, 2), BorderTop);
        vid.BG0_drawPartialAsset(Vec2(0, 0), Vec2(0, 0), Vec2(14, 2), BorderTop);
    }


    const LevelProgressData &progressData =
            GameStateMachine::getInstance().getLevelProgressData();

    const static AssetImage *CheckMarkImagesBottom[] =
    {
        0,
        &BorderSlotBlank,
        &BorderSlotNormal,
        &BorderSlotBonus,

    };

    const static AssetImage *CheckMarkImagesTop[] =
    {
        0,
        &BorderSlotBlank,
        &BorderSlotNormal,
        &BorderSlotBonus,

    };

    const unsigned TopRowStartIndex = arraysize(LevelProgressData::mPuzzleProgress)/2;
    for (unsigned i = 0; i < arraysize(LevelProgressData::mPuzzleProgress); ++i)
    {
        if (i < TopRowStartIndex)
        {
            // row 1, bottom
            const AssetImage *image =
                    CheckMarkImagesBottom[(int)progressData.mPuzzleProgress[i]];
            if (image)
            {
                vid.BG0_drawAsset(Vec2(2 + i * 2, 14), *image);
            }
        }
        else
        {
            // row 2, top
            const AssetImage *image =
                    CheckMarkImagesTop[(int)progressData.mPuzzleProgress[i]];
            if (image)
            {
                vid.BG0_drawAsset(Vec2(2 + i * 2, 0), *image);
            }
        }
    }

    // finished?
    return data.mLoop || animTime <= data.mDuration;
}


