/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "frontend.h"

const float FrontendCube::SIZE;


FrontendCube::FrontendCube()
    : body(0) {}

void FrontendCube::init(unsigned _id, Cube::Hardware *_hw, b2World &world, float x, float y)
{
    id = _id;
    hw = _hw;
    lastLcdCookie = 0;
    
    initBody(world, x, y);
    
    initNeighbor(Cube::Neighbors::TOP, 0, -1);
    initNeighbor(Cube::Neighbors::BOTTOM, 0, 1);

    initNeighbor(Cube::Neighbors::LEFT, -1, 0);
    initNeighbor(Cube::Neighbors::RIGHT, 1, 0);

    tiltTarget.Set(0,0);
    tiltVector.Set(0,0);

    setHoverTarget(HOVER_NONE);
    hover = hoverTarget;    
}

void FrontendCube::exit()
{
    if (body) {
        body->GetWorld()->DestroyBody(body);
        body = NULL;
    }
}

void FrontendCube::initBody(b2World &world, float x, float y)
{
    /*
     * Pick our physical properties carefully: We want the cubes to
     * stop when you let go of them, but a little bit of realistic
     * physical jostling is nice.  Most importantly, we need a
     * corner-drag to actually rotate the cubes effortlessly. This
     * means relatively high linear damping, and perhaps lower angular
     * damping.
     */ 

    b2BodyDef bodyDef;
    bodyDef.type = b2_dynamicBody;
    bodyDef.linearDamping = 30.0f;
    bodyDef.angularDamping = 5.0f;
    bodyDef.position.Set(x, y);
    body = world.CreateBody(&bodyDef);

    b2PolygonShape box;
    const float boxSize = SIZE * 0.96;    // Compensate for polygon 'skin'
    box.SetAsBox(boxSize, boxSize);
    
    bodyFixtureData.type = FixtureData::T_CUBE;
    bodyFixtureData.cube = this;

    b2FixtureDef fixtureDef;
    fixtureDef.shape = &box;
    fixtureDef.density = 1.0f;
    fixtureDef.friction = 0.8f;
    fixtureDef.userData = &bodyFixtureData;
    bodyFixture = body->CreateFixture(&fixtureDef);
}

void FrontendCube::initNeighbor(Cube::Neighbors::Side side, float x, float y)
{
    /*
     * Neighbor sensors are implemented using Box2D sensors. These are
     * four additional fixtures attached to the same body.
     */

    b2CircleShape circle;
    circle.m_p.Set(x * NEIGHBOR_CENTER * SIZE, y * NEIGHBOR_CENTER * SIZE);
    circle.m_radius = NEIGHBOR_RADIUS * SIZE;

    neighborFixtureData[side].type = FixtureData::T_NEIGHBOR;
    neighborFixtureData[side].cube = this;
    neighborFixtureData[side].side = side;

    b2FixtureDef fixtureDef;
    fixtureDef.shape = &circle;
    fixtureDef.isSensor = true;
    fixtureDef.userData = &neighborFixtureData[side];
    body->CreateFixture(&fixtureDef);
}

void FrontendCube::draw(GLRenderer &r)
{
    const uint16_t *framebuffer;

    /*
     * We only want to upload a new framebuffer image to the GPU if the LCD
     * has actually been written to. Use the LCD hardware's pixel counter as
     * a cookie to quickly do this test.
     *
     * Note that we must use the pixel count instead of the write count- the
     * pixel count tells us when *any* pixel has been touched, whereas the write
     * count usually only tells us enough to know when a frame has begun.
     *
     * Additionally, if the LCD is invisible, send the renderer a blank black
     * texture. To capture all of this state, we build a single 32-bit cookie
     * that combines a 31-bit pixel count and a blanking bit.
     */
     
    uint32_t cookie = hw->lcd.isVisible() ? (hw->lcd.getPixelCount() & 0x7FFFFFFF) : ((uint32_t)-1);
    
    if (cookie != lastLcdCookie) {
        lastLcdCookie = cookie;
        static const uint16_t blackness[hw->lcd.WIDTH * hw->lcd.HEIGHT] = { 0 };
        framebuffer = hw->lcd.isVisible() ? hw->lcd.fb_mem : blackness;
    } else {
        framebuffer = NULL;
    }

    r.drawCube(id, body->GetPosition(), body->GetAngle(),
               hover, tiltVector, framebuffer, modelMatrix);
}

void FrontendCube::setTiltTarget(b2Vec2 angles)
{
    /* Target tilt angles that we're animating toward, as YX Euler angles in degrees */
    tiltTarget = angles;
}

void FrontendCube::setHoverTarget(float h)
{
    hoverTarget = h;

    /*
     * When a cube is high enough to clear non-hovering cubes,
     * it won't collide with them. This goes for all fixtures- the
     * cube body, plus the neighbor sensors.
     */
     
    b2Filter filter;    
    filter.categoryBits = 1 << isHovering();
    filter.maskBits = filter.categoryBits;

    for (b2Fixture *f = body->GetFixtureList(); f; f = f->GetNext())
        f->SetFilterData(filter);
}

void FrontendCube::updateNeighbor(bool touching, unsigned mySide,
                                  unsigned otherSide, unsigned otherCube)
{
    if (touching)
        hw->neighbors.setContact(mySide, otherSide, otherCube);
    else
        hw->neighbors.clearContact(mySide, otherSide, otherCube);
}

void FrontendCube::animate()
{
    /* Animated tilt */
    const float tiltGain = 0.25f;
    tiltVector += tiltGain * (tiltTarget - tiltVector);

    /* Animated hover */
    const float hoverGain = 0.5f;
    hover += hoverGain * (hoverTarget - hover);
    
    /*
     * Make a 3-dimensional acceleration vector which also accounts
     * for gravity. We're now measuring it in G's, so we apply an
     * arbitrary conversion from our simulated units (Box2D "meters"
     * per timestep) to something realistic.
     */

    b2Vec3 accelG = accel.measure(body, 0.06f);

    /*
     * Now use the same matrix we computed while rendering the last frame, and
     * convert this acceleration into device-local coordinates. This will take into
     * account tilting, as well as the cube's angular orientation.
     */

    /*
     * In our coordinate system, +Z is toward the camera, and -Z is
     * down, into the table. +X is to the right, and +Y is towards the
     * bottom. This measurement's polarity and magnitude should try
     * hard to mimic the physical hardware's accelerometer orientation
     * and sensitivity.
     */

    /* XXX: Real 3-axis support! This hardcoded Z is a total hack. */

    b2Vec3 accelLocal = modelMatrix.Solve33(accelG);
    hw->setAcceleration(-accelLocal.x, -accelLocal.y, -accelLocal.z);
}

void FrontendCube::computeAABB(b2AABB &aabb)
{
    bodyFixture->GetShape()->ComputeAABB(&aabb, body->GetTransform(), 0);
}

AccelerationProbe::AccelerationProbe()
{
    for (unsigned i = 0; i < filterWidth; i++)
        velocitySamples[i].Set(0,0);
    head = 0;
}

b2Vec3 AccelerationProbe::measure(b2Body *body, float unitsToGs)
{
    b2Vec2 prevV = velocitySamples[head];
    b2Vec2 currentV = body->GetLinearVelocity();
    velocitySamples[head] = currentV;
    head = (head + 1) % filterWidth;

    // Take a finite derivative over the width of our filter window
    b2Vec2 accel2D = currentV - prevV;

    return b2Vec3(accel2D.x * unitsToGs, accel2D.y * unitsToGs, -1.0f);
}
