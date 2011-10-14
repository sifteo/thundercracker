/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "frontend.h"


Frontend::Frontend() 
  : world(b2Vec2(0.0f, 0.0f)),
    mouseBody(NULL),
    mouseJoint(NULL),
    mouseIsAligning(false),
    contactListener(ContactListener(*this))
{}


void Frontend::init(System *_sys)
{
    sys = _sys;
    frameCount = 0;
    toggleZoom = false;
    viewExtent = targetViewExtent() * 3.0;

    SDL_Init(SDL_INIT_VIDEO);
    SDL_WM_SetCaption("Thundercracker", NULL);

    /*
     * Create cubes in a grid. Height is the square root of the number
     * of cubes, rounding down. For sizes up to 3 cubes, this produces
     * a horizontal line layout. For larger layout, it gives us a square.
     */

    if (sys->opt_numCubes) {
        gridH = sqrtf(sys->opt_numCubes);
        gridW = (sys->opt_numCubes + gridH - 1) / gridH;
        for (unsigned y = 0, cubeID = 0; y < gridH && cubeID < sys->opt_numCubes; y++)
            for (unsigned x = 0; x < gridW && cubeID < sys->opt_numCubes; x++, cubeID++) {
                const float spacing = FrontendCube::SIZE * 2.7;
                cubes[cubeID].init(cubeID, &sys->cubes[cubeID], world,
                                   ((gridW - 1) * -0.5 + x) * spacing,
                                   ((gridH - 1) * -0.5 + y) * spacing);
            }
    } else {
        gridW = 0;
        gridH = 0;
    }

    /*
     * The view area should scale with number of cubes. So, scale the
     * linear size of our view with the square root of the number of
     * cubes. We don't just want to make sure they initially fit, but
     * we want there to be a good amount of working space for
     * manipulating the cubes.
     *
     * Special-case one cube, since you don't really need space to
     * work with a single cube.
     *
     * In any case, these are resizable dynamically. It's just nice
     * in everyone's best interest to pick sane defaults :)
     */

    if (sys->opt_numCubes > 1)
        normalViewExtent = FrontendCube::SIZE * 2.5 * sqrtf(sys->opt_numCubes);
    else
        normalViewExtent = FrontendCube::SIZE * 1.4;

    maxViewExtent = normalViewExtent * 10.0f;

    /*
     * Create the rest of the world
     */

    createWalls();

    /*
     * Listen for collisions. This is how we update our neighbor matrix.
     */

    world.SetContactListener(&contactListener);
}

void Frontend::createWalls()
{
    /*
     * Very thick walls, so objects can't tunnel through them and
     * escape the world. The initial locations aren't important,
     * we set those in moveWalls().
     *
     * Use kinematic boxes so that we can move them with velocity.
     * This looks a lot nicer than SetTransform when the simulation is
     * running!
     */

    float e = maxViewExtent;
    float e2 = e * 2;

    walls[0] = newKBox(0, 0, e, e2);  // X+
    walls[1] = newKBox(0, 0, e, e2);  // X-
    walls[2] = newKBox(0, 0, e2, e);  // Y+
    walls[3] = newKBox(0, 0, e2, e);  // Y-
}

void Frontend::moveWalls(bool immediate)
{
    /*
     * Our world is scaled such that the horizontal size is constant
     * (unless changed by the user) but the vertical size changes
     * depending on the window aspect ratio. We'll try to move the
     * walls accordingly.
     */

    float yRatio = viewportHeight / (float)viewportWidth;
    if (yRatio < 0.1)
        yRatio = 0.1;

    float x = maxViewExtent + normalViewExtent;
    float y = maxViewExtent + normalViewExtent * yRatio;

    if (immediate) {
        walls[0]->SetTransform(b2Vec2( x,  0), 0);
        walls[1]->SetTransform(b2Vec2(-x,  0), 0);
        walls[2]->SetTransform(b2Vec2( 0,  y), 0);
        walls[3]->SetTransform(b2Vec2( 0, -y), 0);
    } else {
        const float g = 4.0f;
        pushBodyTowards(walls[0], b2Vec2( x,  0), g);
        pushBodyTowards(walls[1], b2Vec2(-x,  0), g);
        pushBodyTowards(walls[2], b2Vec2( 0,  y), g);
        pushBodyTowards(walls[3], b2Vec2( 0, -y), g);
    }
}
             
void Frontend::pushBodyTowards(b2Body *b, b2Vec2 target, float gain)
{
    b->SetLinearVelocity(gain * (target - b->GetPosition()));
}

b2Body *Frontend::newKBox(float x, float y, float hw, float hh)
{
    b2BodyDef bodyDef;
    bodyDef.position.Set(x, y);
    bodyDef.type = b2_kinematicBody;
    bodyDef.allowSleep = false;
    b2Body *body = world.CreateBody(&bodyDef);

    b2PolygonShape box;
    box.SetAsBox(hw, hh);

    b2FixtureDef fixtureDef;
    fixtureDef.shape = &box;
    body->CreateFixture(&fixtureDef);

    return body;
}

unsigned Frontend::cubeID(FrontendCube *cube)
{
    return (unsigned)(cube - &cubes[0]);
}

void Frontend::exit()
{             
    SDL_Quit();
}

void Frontend::run()
{
    if (sys->opt_numCubes > 1) {
        // 2 or more cubes: Large window
        if (!onResize(800, 600))
            return;
    } else {    
        // Zero or one cube: Small window
        if (!onResize(300, 300))
            return;
    }

    while (sys->isRunning()) {
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
                break;

            case SDL_MOUSEBUTTONDOWN:
                mouseX = event.button.x;
                mouseY = event.button.y;
                onMouseDown(event.button.button);
                break;

            case SDL_MOUSEBUTTONUP:
                mouseX = event.button.x;
                mouseY = event.button.y;
                onMouseUp(event.button.button);
                break;

            }
        }

        if (!(frameCount % FRAME_HZ_DIVISOR)) {
            for (unsigned i = 0; i < sys->opt_numCubes; i++)
                sys->cubes[i].lcdPulseTE();
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

    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4); 
    SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);

    surface = SDL_SetVideoMode(width, height, 0, SDL_OPENGL | SDL_RESIZABLE);

    if (surface == NULL) {
        // First failure? Try without FSAA.

        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0); 
        surface = SDL_SetVideoMode(width, height, 0, SDL_OPENGL | SDL_RESIZABLE);
        if (surface) {
            fprintf(stderr, "Warning: FSAA not available. Your cubes will have crunchy edges :(\n");
        }
    }

    if (surface == NULL) {
        fprintf(stderr, "Error creating SDL surface!\n");
        return false;
    }

    viewportWidth = width;
    viewportHeight = height;
    moveWalls(true);

    /*
     * Assume we totally lost the OpenGL context, and reallocate things.
     * XXX: This is not always necessary. It's platform-specific.
     */

    if (!renderer.init())
        return false;

    renderer.setViewport(width, height);

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

void Frontend::onMouseDown(int button)
{
    if (button == 4) {
        // Scroll up / zoom in
        scaleViewExtent(0.9f);
        return;
    }

    if (button == 5) {
        // Scroll down / zoom out
        scaleViewExtent(1.1f);
        return;
    }

    if (!mouseBody) {
        mousePicker.test(world, mouseVec(normalViewExtent));

        if (mousePicker.mCube) {
            // This body represents the mouse itself now as a physical object
            b2BodyDef mouseDef;
            mouseDef.type = b2_kinematicBody;
            mouseDef.position = mousePicker.mPoint;
            mouseDef.allowSleep = false;
            mouseBody = world.CreateBody(&mouseDef);

            if (button == 3 || (SDL_GetModState() & KMOD_SHIFT)) {
                /*
                 * If this was a right-click or shift-click, go into tilting mode
                 */
                
                mouseIsTilting = true;

            } else {
                /*
                 * Otherwise, the mouse is pulling.
                 *
                 * We represent our mouse as a kinematic body. This
                 * lets us pull/push a cube, or by grabbing it at an
                 * edge or corner, rotate it intuitively.
                 */
                
                mouseIsPulling = true;

                /*
                 * Pick an attachment point. If we're close to the center,
                 * snap our attachment point to the center of mass, and
                 * turn on a servo that tries to orient the cube to a
                 * multiple of 90 degrees.
                 */
                
                b2Vec2 anchor = mousePicker.mPoint;
                b2Vec2 center = mousePicker.mCube->body->GetWorldCenter();
                float centerDist = b2Distance(anchor, center);
                const float centerSize = FrontendCube::SIZE * FrontendCube::CENTER_SIZE;

                if (centerDist < centerSize) {
                    anchor = center;
                    mouseIsAligning = true;
                    
                    /*
                     * This notion of center distance is also used to
                     * simulate touch. The closer to the exact center,
                     * the stronger the touch signal will be. This is
                     * a bit of a hack, but I'm hoping it mimicks what
                     * happens with actual hardware- you get a lot of
                     * false 'touch' signals when users manipulate the
                     * cubes, since you'd like to be able to fling
                     * cubes around by their faces without worrying
                     * about whether you're 'touching' them or
                     * not. So, the cube firmware will be responsible
                     * for doing various kinds of filtering to
                     * determine what counts as a touch.
                     */

                    mousePicker.mCube->setTouch(1.0f - centerDist / centerSize);
                }

                // Glue it to the point we picked, with a revolute joint

                b2RevoluteJointDef jointDef;
                jointDef.Initialize(mousePicker.mCube->body, mouseBody, anchor);
                jointDef.motorSpeed = 0.0f;
                jointDef.maxMotorTorque = 10.0f;
                jointDef.enableMotor = true;
                mouseJoint = (b2RevoluteJoint*) world.CreateJoint(&jointDef);
            }
        }
    }
}

void Frontend::onMouseUp(int button)
{
    if (mouseBody) {
        if (mousePicker.mCube) {
            // Reset tilt
            mousePicker.mCube->setTiltTarget(b2Vec2(0.0f, 0.0f));

            // Reset touch
            mousePicker.mCube->setTouch(0.0f);
        }

        /* Mouse state reset */

        if (mouseJoint)
            world.DestroyJoint(mouseJoint);
        world.DestroyBody(mouseBody);
        mouseJoint = NULL;
        mouseBody = NULL;
        mousePicker.mCube = NULL;
        mouseIsAligning = false;
        mouseIsPulling = false;
        mouseIsTilting = false;
    }
}

void Frontend::animate()
{
    /*
     * All of our animation is fixed timestep.
     */

    const float timeStep = 1.0f / 60.0f;
    const int velocityIterations = 10;
    const int positionIterations = 8;

    if (mouseIsPulling) {
        pushBodyTowards(mouseBody, mouseVec(normalViewExtent), 50.0f);
    }

    if (mouseIsAligning) {
        /*
         * Turn on a joint motor, to try and push the cube toward its
         * closest multiple of 90 degrees.
         */
        
        const float gain = 20.0f;

        float cubeAngle = mouseJoint->GetBodyA()->GetAngle();
        float fractional = fmod(cubeAngle + M_PI/4, M_PI/2);
        if (fractional < 0) fractional += M_PI/2;
        float error = fractional - M_PI/4;
        mouseJoint->SetMotorSpeed(gain * error);
    }

    if (mouseIsTilting) {
        /*
         * Animate the tilt, using a pair of Euler angles based on the distance
         * between the mouse's current position and its initial grab point.
         */

        if (mousePicker.mCube) {
            const float maxTilt = 80.0f;
            b2Vec2 mouseDiff = mouseVec(normalViewExtent) - mouseBody->GetWorldCenter();
            b2Vec2 tiltTarget = (maxTilt / FrontendCube::SIZE) * mouseDiff; 
            tiltTarget.x = b2Clamp(tiltTarget.x, -maxTilt, maxTilt);
            tiltTarget.y = b2Clamp(tiltTarget.y, -maxTilt, maxTilt);        

            // Rotate it into the cube's coordinates
            b2Vec2 local = b2Mul(b2Rot(-mousePicker.mCube->body->GetAngle()), tiltTarget);

            mousePicker.mCube->setTiltTarget(local);
        }
    }
        
    /* Local per-cube animations */
    for (unsigned i = 0; i < sys->opt_numCubes; i++)
        cubes[i].animate();

    /* Animated viewport centering/zooming */
    {
        const float gain = 0.1;

        viewExtent += gain * (targetViewExtent() - viewExtent);
        viewCenter += gain * (targetViewCenter() - viewCenter);

        moveWalls();
    }

    world.Step(timeStep, velocityIterations, positionIterations);
}

void Frontend::scaleViewExtent(float ratio)
{
    normalViewExtent = b2Clamp<float>(normalViewExtent * ratio,
                                      FrontendCube::SIZE * 0.1, maxViewExtent);
}

void Frontend::draw()
{
    renderer.beginFrame(viewExtent, viewCenter);

    float ratio = std::max(1.0f, viewportHeight / (float)viewportWidth);
    renderer.drawBackground(viewExtent * ratio * 50.0f, 0.2f);

    for (unsigned i = 0; i < sys->opt_numCubes; i++)
        cubes[i].draw(renderer);

    renderer.endFrame();
}

float Frontend::zoomedViewExtent() {
    /*
     * Default scaling uses the X axis, but here we want to make sure
     * the whole of a cube is visible, as applicable. So, scale using
     * whichever axis is smaller.
     */

    float scale = (viewportHeight < viewportWidth)
        ? viewportWidth / (float) viewportHeight : 1.0f;
        
    if (sys->opt_numCubes > 1) {
        // Zoom in one one cube
        return scale * FrontendCube::SIZE * 1.1;
    } else {
        // High zoom on our one and only cube
        return scale * FrontendCube::SIZE * 0.2;
    }
}

float Frontend::targetViewExtent() {
    return toggleZoom ? zoomedViewExtent() : normalViewExtent;
}    

b2Vec2 Frontend::targetViewCenter() {
    // When zooming in/out, the pixel under the cursor stays the same.
    return toggleZoom ? (mouseVec(normalViewExtent) - mouseVec(targetViewExtent()))
        : b2Vec2(0.0, 0.0);
}

b2Vec2 Frontend::mouseVec(float viewExtent) {
    int halfWidth = viewportWidth / 2;
    int halfHeight = viewportHeight / 2;
    float mouseScale = viewExtent / (float)halfWidth;
    return  b2Vec2((mouseX - halfWidth) * mouseScale,
                   (mouseY - halfHeight) * mouseScale);
}

void Frontend::MousePicker::test(b2World &world, b2Vec2 point)
{
    // Test overlap of a very small AABB
    b2AABB aabb;
    b2Vec2 size(0.001f, 0.001f);
    aabb.lowerBound = point - size;
    aabb.upperBound = point + size;

    mPoint = point;
    mCube = NULL;

    world.QueryAABB(this, aabb);
}

bool Frontend::MousePicker::ReportFixture(b2Fixture *fixture)
{
    FixtureData *fdat = (FixtureData *) fixture->GetUserData();

    if (fdat && fdat->type == fdat->T_CUBE && fixture->TestPoint(mPoint)) {
        mCube = fdat->cube;
        return false;
    }

    return true;
}
        
void Frontend::ContactListener::BeginContact(b2Contact *contact)
{
    updateSensors(contact);
}

void Frontend::ContactListener::EndContact(b2Contact *contact)
{
    updateSensors(contact);
}

void Frontend::ContactListener::updateSensors(b2Contact *contact)
{
    FixtureData *fdatA = (FixtureData *) contact->GetFixtureA()->GetUserData();
    FixtureData *fdatB = (FixtureData *) contact->GetFixtureB()->GetUserData();

    if (fdatA && fdatB
        && fdatA->type == fdatA->T_NEIGHBOR
        && fdatB->type == fdatB->T_NEIGHBOR) {
        bool touching = contact->IsTouching();

        /*
         * Update interaction between two neighbor sensors
         */

        unsigned cubeA = frontend.cubeID(fdatA->cube);
        unsigned cubeB = frontend.cubeID(fdatB->cube);

        fdatA->cube->updateNeighbor(touching, fdatA->side, fdatB->side, cubeB);
        fdatB->cube->updateNeighbor(touching, fdatB->side, fdatA->side, cubeA);
    }
}
