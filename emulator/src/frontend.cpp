/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "frontend.h"


class QueryCallback : public b2QueryCallback {
 public:
    QueryCallback(const b2Vec2& point)
        : mPoint(point), mFixture(NULL) {}

    bool ReportFixture(b2Fixture *fixture) {
        b2Body *body = fixture->GetBody();

        if (body->GetType() == b2_dynamicBody &&
            fixture->TestPoint(mPoint)) {
            mFixture = fixture;
            return false;
        }
        return true;
    }
            
    b2Vec2 mPoint;
    b2Fixture *mFixture;
};


void Frontend::init(System *_sys)
{
    sys = _sys;
    frameCount = 0;
    toggleZoom = false;
    viewExtent = targetViewExtent() * 3.0;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_WM_SetCaption("Thundercracker", NULL);

    /*
     * Create cubes
     */

    for (unsigned i = 0; i < sys->opt_numCubes; i++) {
        // Put all the cubes in a line
        float x =  ((sys->opt_numCubes - 1) * -0.5 + i) * FrontendCube::SIZE * 2.7;

        cubes[i].init(&sys->cubes[i], world, x, 0);
    }

    /*
     * Create the rest of the world
     */

    b2BodyDef groundDef;
    ground = world.CreateBody(&groundDef);

    float extent = normalViewExtent();
    float extent2 = extent * 2.0f;

    // Thick walls
    newStaticBox(extent2, 0, extent, extent2);   // X+
    newStaticBox(-extent2, 0, extent, extent2);  // X-
    newStaticBox(0, extent2, extent2, extent);   // Y+
    newStaticBox(0, -extent2, extent2, extent);  // Y-
}

void Frontend::newStaticBox(float x, float y, float hw, float hh)
{
    b2BodyDef bodyDef;
    bodyDef.position.Set(x, y);
    b2Body *body = world.CreateBody(&bodyDef);

    b2PolygonShape box;
    box.SetAsBox(hw, hh);

    b2FixtureDef fixtureDef;
    fixtureDef.shape = &box;
    body->CreateFixture(&fixtureDef);
}

void Frontend::exit()
{             
    SDL_Quit();
}

void Frontend::run()
{
    if (!onResize(800, 600))
        return;

    while (1) {
        SDL_Event event;
    
        // Drain the GUI event queue
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                
            case SDL_QUIT:
                return;

            case SDL_VIDEORESIZE:
                if (!onResize(event.resize.w, event.resize.h))
                    return;
                break;

            case SDL_KEYDOWN:
                onKeyDown(event.key);
                break;

            case SDL_MOUSEMOTION:
                mouseX = event.motion.x;
                mouseY = event.motion.y;
                onMouseMove();
                break;

            case SDL_MOUSEBUTTONDOWN:
                mouseX = event.button.x;
                mouseY = event.button.y;
                onMouseDown(event.button.state);
                break;

            case SDL_MOUSEBUTTONUP:
                mouseX = event.button.x;
                mouseY = event.button.y;
                onMouseUp(event.button.state);
                break;

            }
        }

        if (sys->time.isPaused())
            SDL_Delay(50);
        
        if (!(frameCount % FRAME_HZ_DIVISOR)) {
            for (unsigned i = 0; i < sys->opt_numCubes; i++)
                sys->cubes[i].lcd.pulseTE();
        }

        animate();
        draw();
        frameCount++;
    }
}

bool Frontend::onResize(int width, int height)
{
    if (!(width && height))
        return true;

    surface = SDL_SetVideoMode(width, height, 0, SDL_OPENGL | SDL_RESIZABLE);
    if (surface == NULL) {
        fprintf(stderr, "Error creating SDL surface!\n");
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);

    viewportWidth = width;
    viewportHeight = height;
    glViewport(0, 0, width, height);
    
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);

    for (unsigned i = 0; i < sys->opt_numCubes; i++)
        cubes[i].initGL();

    return true;
}

void Frontend::onKeyDown(SDL_KeyboardEvent &evt)
{
    switch (evt.keysym.sym) {
        
    case 'z':
        toggleZoom = !toggleZoom;
        break;

    default:
        break;
    }
}

void Frontend::onMouseMove()
{
    if (mouseJoint)
        mouseJoint->SetTarget(mouseVec(normalViewExtent()));
}

void Frontend::onMouseDown(int buttons)
{
    if (!mouseJoint) {
        // Test a small bounding box at the mouse hotspot

        b2AABB aabb;
        b2Vec2 mouse = mouseVec(normalViewExtent());
        b2Vec2 size(0.001f, 0.001f);
        aabb.lowerBound = mouse - size;
        aabb.upperBound = mouse + size;

        QueryCallback callback(mouse);
        world.QueryAABB(&callback, aabb);

        if (callback.mFixture) {
            // Create a mouse joint (a spring)

            b2Body *body = callback.mFixture->GetBody();
            b2MouseJointDef md;

            md.bodyA = ground;
            md.bodyB = body;
            md.target = mouse;
            md.maxForce = 1000.0f;
            mouseJoint = (b2MouseJoint*) world.CreateJoint(&md);
            body->SetAwake(true);
        }
    }
}

void Frontend::onMouseUp(int buttons)
{
    if (mouseJoint) {
        world.DestroyJoint(mouseJoint);
        mouseJoint = NULL;
    }
}

void Frontend::animate()
{
    /*
     * All of our animation is fixed timestep.
     */

    const float easeSpeed = 0.1;
    const float timeStep = 1.0f / 60.0f;
    const int velocityIterations = 6;
    const int positionIterations = 2;

    world.Step(timeStep, velocityIterations, positionIterations);

    viewExtent += easeSpeed * (targetViewExtent() - viewExtent);
    viewCenter += easeSpeed * (targetViewCenter() - viewCenter);
}

void Frontend::draw()
{
    // Background
    glClearColor(0.2, 0.2, 0.4, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    // Orthogonal camera
    glLoadIdentity();
    glScalef(1.0f / viewExtent,
             -viewportWidth / (float)viewportHeight / viewExtent,
             1.0f / viewExtent);
    glTranslatef(-viewCenter.x, -viewCenter.y, 0);

    // All cubes
    for (unsigned i = 0; i < sys->opt_numCubes; i++)
        cubes[i].draw();

    SDL_GL_SwapBuffers();
}

float Frontend::zoomedViewExtent() {
    return FrontendCube::SIZE * 1.25;
}

float Frontend::normalViewExtent() {
    return FrontendCube::SIZE * (sys->opt_numCubes * 1.75);
}

float Frontend::targetViewExtent() {
    return toggleZoom ? zoomedViewExtent() : normalViewExtent();
}    

b2Vec2 Frontend::targetViewCenter() {
    // When zooming in/out, the pixel under the cursor stays the same.
    return toggleZoom ? (mouseVec(normalViewExtent()) - mouseVec(targetViewExtent()))
        : b2Vec2(0.0, 0.0);
}

b2Vec2 Frontend::mouseVec(float viewExtent) {
    int halfWidth = viewportWidth / 2;
    int halfHeight = viewportHeight / 2;
    float mouseScale = viewExtent / (float)halfWidth;
    return  b2Vec2((mouseX - halfWidth) * mouseScale,
                   (mouseY - halfHeight) * mouseScale);
}
