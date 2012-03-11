#include "VictoryView.h"

namespace TotalsGame {


void VictoryParticle::Initialize() {
    sizeIndex = Game::rand.randrange(3);
    size = (sizeIndex+1) * 8;
    initialPosition.set(64, 92);
    initialVelocity.setPolar(
                Game::rand.uniform(-0.2f * M_PI, -0.8f * M_PI),
                Game::rand.uniform(65, 75)
                );
}

bool VictoryParticle::IsOnScreen(const Float2 &p) {
    return p.x > -(size) &&
            p.x < (128) &&
            p.y > -(size) &&
            p.y < (128);
}

bool VictoryParticle::Paint(TotalsCube *c, int type, int id, float time) {
    static const PinnedAssetImage *sprites[4][3] =
    {
        {&Diamond_8,&Diamond_16, &Diamond_24},
        {&Ruby_8,&Ruby_16, &Ruby_24},
        {&Emerald_8,&Emerald_16, &Emerald_24},
        {&Coin_8, &Coin_16, &Coin_24}
    };

    const Float2 halfG(0, 32.5);
    Float2 p = initialPosition + initialVelocity * time + halfG * time * time;

    if (IsOnScreen(p)) {
        c->backgroundLayer.setSpriteImage(id, *sprites[type][sizeIndex], 0);
        c->backgroundLayer.moveSprite(id, p.x, p.y);
        return true;
    }
    else if(!c->backgroundLayer.isSpriteHidden(id))
    {
        c->backgroundLayer.resizeSprite(id, 0,0);
    }
    return false;
}


VictoryView::VictoryView(int index) {
    mType = index % 4;
    mOpen = false;
    mOver = false;
}

void VictoryView::Open() {
    if (!mOpen) {
        mOpen = true;
        for(int i=0; i<8; ++i) {
            mParticles[i].Initialize();
        }
        time = 0;
    }
}

void VictoryView::EndUpdates() {
    mOver = true;
}

void VictoryView::Update () {
    if (mOpen && !mOver) {
        time += Game::dt;
    }
}

void VictoryView::Paint () {
    if(mOpen)
    {
        static const AssetImage *narratorTypes[] =
        {
            &Narrator_Diamond,   &Narrator_Ruby, &Narrator_Emerald, &Narrator_Coin
        };
        GetCube()->Image(narratorTypes[mType], Vec2(0,16-narratorTypes[mType]->height));

        for(int i=0; i<8; ++i) {
            mParticles[i].Paint(GetCube(), mType, i, time);
        }
    }
    else
    {
        GetCube()->Image(&Narrator_GetReady, Vec2(0,16-Narrator_GetReady.height));
    }
}



}

