#pragma once

#include "sifteo.h"
#include "Game.h"
#include "View.h"
#include "assets.gen.h"

namespace TotalsGame {

struct VictoryParticle {
    uint8_t size, sizeIndex;
    Float2 initialPosition;
    Float2 initialVelocity;

    void Initialize();

    bool IsOnScreen(const Float2 &p);

    bool Paint(TotalsCube *c, int type, int id, float time);
};

class VictoryView : public View {

    bool mOpen;
    bool mOver;
    VictoryParticle mParticles[8];
    uint8_t mType;
    float time;

public:
    VictoryView(int index);

    void Open();

    void EndUpdates();
    void Update ();

    void Paint ();
};

}

