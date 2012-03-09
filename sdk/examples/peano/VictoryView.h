#pragma once

#include "sifteo.h"
#include "Game.h"
#include "View.h"
#include "assets.gen.h"

namespace TotalsGame {

struct VictoryParticle {
    uint8_t size, sizeIndex;
    Float2 position;
    Float2 velocity;

    void Initialize() {
        sizeIndex = Game::rand.randrange(3);
        size = (sizeIndex+1) * 8;
        position.set(64, 92);
        velocity.setPolar(
                    Game::rand.uniform(-0.2f * M_PI, -0.8f * M_PI),
                    Game::rand.uniform(65, 75)
                    );
    }

    void Update() {
        while(Game::dt > 0.25f) {
            velocity.y += 0.25f * 65;
            position += 0.25f * velocity;
            Game::dt -= 0.25f;
        }
        velocity.y += Game::dt * 65;
        position += Game::dt * velocity;
    }

    bool IsOnScreen() {
        return position.x > -size &&
                position.x < 128 &&
                position.y > -size &&
                position.y < 128;
    }

    bool Paint(TotalsCube *c, int type, int id) {
        static const PinnedAssetImage *sprites[4][3] =
        {
            {&Diamond_8,&Diamond_16, &Diamond_24},
            {&Ruby_8,&Ruby_16, &Ruby_24},
            {&Emerald_8,&Emerald_16, &Emerald_24},
            {&Coin_8, &Coin_16, &Coin_24}
        };

        if (IsOnScreen()) {
            c->backgroundLayer.setSpriteImage(id, *sprites[type][sizeIndex], 0);
            c->backgroundLayer.moveSprite(id, position.x, position.y);
            return true;
        }
        else if(!c->backgroundLayer.isSpriteHidden(id))
        {
            c->backgroundLayer.resizeSprite(id, 0,0);
        }
        return false;
    }
};

class VictoryView : public View {

    bool mOpen;
    bool mOver;
    VictoryParticle mParticles[8];
    uint8_t mType;

public:
    VictoryView(int index) {
        mType = index % 4;
        mOpen = false;
        mOver = false;
    }

    void Open() {
        if (!mOpen) {
            mOpen = true;
            for(int i=0; i<8; ++i) {
                mParticles[i].Initialize();
            }
        }
    }

    void EndUpdates() {
        mOver = true;
    }

    void Update () {
        if (mOpen && !mOver) {
            for(int i=0; i<8; ++i) {
                mParticles[i].Update();
            }
        }
    }

    void Paint () {
        if(mOpen)
        {
            static const AssetImage *narratorTypes[] =
            {
                &Narrator_Diamond,   &Narrator_Ruby, &Narrator_Emerald, &Narrator_Coin
            };
            GetCube()->Image(narratorTypes[mType], Vec2(0,16-narratorTypes[mType]->height));

            for(int i=0; i<8; ++i) {
                mParticles[i].Paint(GetCube(), mType, i);
            }
        }
        else
        {
            GetCube()->Image(&Narrator_GetReady, Vec2(0,16-Narrator_GetReady.height));
        }
    }

};

}

