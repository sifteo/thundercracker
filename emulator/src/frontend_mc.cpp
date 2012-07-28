/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * Max Kaufmann <max@sifteo.com>
 *
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "frontend_mc.h"
#include "gl_renderer.h"
#include "mc_led.h"

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

void FrontendMC::draw(GLRenderer &r)
{
    STATIC_ASSERT(LED::OFF == 0);
    STATIC_ASSERT(LED::RED == 1);
    STATIC_ASSERT(LED::GREEN == 2);
    STATIC_ASSERT(LED::ORANGE == 3);

    static const float ledColors[][3] = {
        { 0.0, 0.0, 0.0 },
        { 3.0, 0.3, 0.3 },
        { 0.1, 3.0, 0.1 },
        { 3.0, 0.9, 0.0 },
    };

    r.drawMC(body->GetPosition(), body->GetAngle(), ledColors[3 & LED::currentColor]);
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
