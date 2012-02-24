#pragma once

#include "sifteo.h"
#include "Game.h"
#include "View.h"
#include "assets.gen.h"

namespace TotalsGame {

struct VictoryParticle {
    uint8_t size;
    Float2 position;
    Float2 velocity;

    void Initialize() {
        size = 8 * (1 + Game::rand.randrange(3));
        position.set(64, 92);
        velocity.setPolar(
                    Game::rand.uniform(-0.2f * M_PI, -0.8f * M_PI),
                    Game::rand.uniform(65, 75)
                    );
    }

    void Update(float dt) {
        while(dt > 0.25f) {
            velocity.y += 0.25f * 65;
            position += 0.25f * velocity;
            dt -= 0.25f;
        }
        velocity.y += dt * 65;
        position += dt * velocity;
    }

    bool IsOnScreen() {
        int hw = size/2;
        return position.x > -hw &&
                position.x < 128 + hw &&
                position.y > -hw &&
                position.y < 128 + hw;
    }

    bool Paint(TotalsCube *c, int type) {
        static const PinnedAssetImage *sprites[4][3] =
        {
            {&Diamond_8,&Diamond_16, &Diamond_24},
            {&Ruby_8,&Ruby_16, &Ruby_24},
            {&Emerald_8,&Emerald_16, &Emerald_24},
            {&Coin_8, &Coin_16, &Coin_24}
        };

        if (IsOnScreen()) {
            c.Image(
                        &sprites[type][size],
                        int(position.x - size/2),    //todo floor
                        int(position.y - size/2),    //todo floor
                        0, 0, size, size
                        );
            return true;
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
    VictoryView(TotalsCube *c, int index): View(c) {
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

    void Update (float dt) {
        if (mOpen && !mOver) {
            /* TODO need this? nah
            if (OkayToPaint()) {
                int yPad = 32;
                Cube.Image("narrator_" + mType, 0, yPad, 0, yPad, 128, 128-yPad);
                for(int i=0; i<mParticles.Length; ++i) {
                    mParticles[i].Paint(Cube, mType);
                }
                Cube.Paint();
            } */
            for(int i=0; i<8; ++i) {
                mParticles[i].Update(dt);
            }
        }
    }

    void Paint () {
        if(mOpen)
        {
            static const AssetImage *narratorTypes =
            {
                &Narrator_Diamond,   &Narrator_Ruby, &Narrator_Emerald, &Narrator_Coin
            };
            c.Image(narratorTypes[mType]);

            for(int i=0; i<8; ++i) {
                mParticles[i].Paint(GetCube(), mType);
            }
        }
        else
        {
            c.Image(&Narrator_Getready);
        }
    }

};

}

