/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * Max Kaufmann <max@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "frontend_mc.h"
#include "gl_renderer.h"
#include "led.h"
#include "ostime.h"
#include "volume.h"

LEDSequencer FrontendMC::led;

FrontendMC::FrontendMC()
    : body(0) {}

void FrontendMC::init(b2World& world, float x, float y)
{
    /*
     * Note: The MC intentionally has very different physical
     *       properties than the cubes. We don't want the MC
     *       to be super-spinny like the cubes, so we apply
     *       a more typical amount of angular damping, and
     *       it needs to feel a bit "heavier" overall.
     */
    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.linearDamping = 80.0f;
    bodyDef.angularDamping = 1000.0f;
    bodyDef.position.Set(x, y);
    body = world.CreateBody(&bodyDef);

    b2PolygonShape box;
    const b2Vec2 boxSize = 0.96f * b2Vec2(
        MCConstants::SIZEX,
        MCConstants::SIZEY
    );
    box.SetAsBox(boxSize.x, boxSize.y);

    bodyFixtureData.type = FixtureData::T_MC;
    bodyFixtureData.ptr.mc = this;

    b2FixtureDef fixtureDef;
    fixtureDef.shape = &box;
    fixtureDef.density = 1.0f;
    fixtureDef.friction = 0.8f;
    fixtureDef.userData = &bodyFixtureData;
    bodyFixture = body->CreateFixture(&fixtureDef);
    
    // Attach neighbor fixtures
    initNeighbor(0, 1.0f);
    initNeighbor(1, -1.0f);
}

void FrontendMC::exit()
{
    if (body) {
        body->GetWorld()->DestroyBody(body);
        body = 0;
    }
}

void FrontendMC::animate()
{
    // Simulate a fixed-rate IRQ for the LED sequencer

    const double tickPeriod = 1.0 / LEDSequencer::TICK_HZ;
    double now = OSTime::clock();
    double dt = now - nextLEDTick;
    unsigned numTicks = dt / tickPeriod;

    if (numTicks > 10) {
        // Lagging too far; Jump ahead.
        numTicks = 1;
        nextLEDTick = now;
    }

    if (numTicks) {
        // Integrate the LED color over the duration of this frame

        ledColor[0] = 0;
        ledColor[1] = 0;
        ledColor[2] = 0;

        for (unsigned i = 0; i != numTicks; ++i) {
            LEDSequencer::LEDState state;
            led.tickISR(state);
            accumulateLEDColor(state, ledColor);
        }

        ledColor[0] /= numTicks;
        ledColor[1] /= numTicks;
        ledColor[2] /= numTicks;

        nextLEDTick += tickPeriod * numTicks;
    }
}

void FrontendMC::draw(GLRenderer &r)
{
    float vol = Volume::systemVolume() / float(Volume::MAX_VOLUME);
    r.drawMC(body->GetPosition(), body->GetAngle(), ledColor, vol);
}

void FrontendMC::accumulateLEDColor(LEDSequencer::LEDState state, float color[3])
{
    color[0] += state.red * (3.0f / 0xFFFF) + state.green * (0.1f / 0xFFFF);
    color[1] += state.red * (0.3f / 0xFFFF) + state.green * (3.0f / 0xFFFF);
    color[2] += state.red * (0.3f / 0xFFFF) + state.green * (0.1f / 0xFFFF);
}

void FrontendMC::initNeighbor(unsigned id, float x)
{
    /*
     * Neighbor sensors are implemented using Box2D sensors. These are
     * four additional fixtures attached to the same body.
     */

    b2CircleShape circle;
    circle.m_p.Set(x * MCConstants::NEIGHBOR_X, 0);
    circle.m_radius = MCConstants::NEIGHBOR_RADIUS;

    neighborFixtureData[id].type = FixtureData::T_MC_NEIGHBOR;
    neighborFixtureData[id].ptr.mc = this;
    neighborFixtureData[id].side = id;

    b2FixtureDef fixtureDef;
    fixtureDef.shape = &circle;
    fixtureDef.isSensor = true;
    fixtureDef.userData = &neighborFixtureData[id];
    body->CreateFixture(&fixtureDef);
}

void LED::init()
{
    FrontendMC::led.init();
}

void LED::set(const LEDPattern *pattern, bool immediate)
{
    FrontendMC::led.setPattern(pattern, immediate);
}
