/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "frontend.h"
#include <time.h>
// for MAX HAX
#if __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif


Frontend *Frontend::instance = NULL;

Frontend::Frontend() 
  : frameCount(0),
    idleFrames(0),
    world(b2Vec2(0.0f, 0.0f)),
    mouseBody(NULL),
    mouseJoint(NULL),
    mouseIsAligning(false),
    mouseIsSpinning(false),
    contactListener(ContactListener(*this))
{}


bool Frontend::init(System *_sys)
{
    // MAX HAX - chdir out of a bundle
    #if __APPLE__
    CFBundleRef mainbundle = CFBundleGetMainBundle();
    CFURLRef bundleUrl = CFBundleCopyBundleURL(mainbundle);
    CFStringRef path = CFURLCopyFileSystemPath(bundleUrl, kCFURLPOSIXPathStyle);
    std::string dir = CFStringGetCStringPtr(path, 0);
    CFRelease(path);
    CFRelease(bundleUrl);
    const size_t baseLength = dir.find_last_of('/');
    chdir(dir.substr(0, baseLength).c_str());
    #endif

    instance = this;
    sys = _sys;
    frameCount = 0;
    toggleZoom = false;
    viewExtent = targetViewExtent() * 3.0;
    isRunning = true;
    isRotationFixed = false;

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
                const float spacing = CubeConstants::SIZE * 2.7;
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
        normalViewExtent = CubeConstants::SIZE * 2.5 * sqrtf(sys->opt_numCubes);
    else
        normalViewExtent = CubeConstants::SIZE * 1.4;

    maxViewExtent = normalViewExtent * 10.0f;

    /*
     * Create the rest of the world
     */

    createWalls();

    /*
     * Listen for collisions. This is how we update our neighbor matrix.
     */

    world.SetContactListener(&contactListener);
    
    /*
     * Open our GUI window
     */
     
    if (sys->opt_numCubes > 1) {
        // 2 or more cubes: Large window
        if (!openWindow(800, 600))
            return false;
    } else {    
        // Zero or one cube: Small window
        if (!openWindow(300, 300))
            return false;
    }
    
    overlay.init(&renderer, sys);
    
    return true;
}

void Frontend::numCubesChanged()
{
    unsigned i;

    idleFrames = 0;

    for (i = 0; i < sys->opt_numCubes; i++)
        if (!cubes[i].isInitialized()) {
            // Create new cubes at the mouse cursor for now
            b2Vec2 v = mouseVec(normalViewExtent);
            cubes[i].init(i, &sys->cubes[i], world, v.x, v.y);
            if (isRotationFixed) {
                cubes[i].toggleRotationLock(isRotationFixed);
            }
        }

    for (;i < sys->MAX_CUBES; i++)
        if (cubes[i].isInitialized())
            cubes[i].exit();
}

void Frontend::addCube()
{
    if (sys->opt_numCubes < sys->MAX_CUBES) {
        sys->setNumCubes(sys->opt_numCubes + 1);
        numCubesChanged();
    }
}

void Frontend::removeCube()
{
    if (sys->opt_numCubes > 0) {
        sys->setNumCubes(sys->opt_numCubes - 1);
        numCubesChanged();
    }
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

    float yRatio = renderer.getHeight() / (float)renderer.getWidth();
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
    glfwTerminate();
}

bool Frontend::runFrame()
{
    if (!isRunning || !sys->isRunning())
        return false;
        
    // Simulated hardware VSync
    if (!(frameCount % FRAME_HZ_DIVISOR))
        for (unsigned i = 0; i < sys->opt_numCubes; i++)
            sys->cubes[i].lcdPulseTE();

    animate();
    draw();

    frameCount++;
    idleFrames++;

    return true;
}

bool Frontend::openWindow(int width, int height, bool fullscreen)
{
    glfwOpenWindowHint(GLFW_FSAA_SAMPLES, 4);

    if (!glfwOpenWindow(width, height,   // Pixels
                        8, 8, 8,         // RGB bits
                        0,               // Alpha bits
                        24,              // Depth bits
                        0,               // Stencil bits
                        fullscreen ? GLFW_FULLSCREEN : GLFW_WINDOW)) {
        return false;
    }

    if (!renderer.init())
        return false;

    isFullscreen = fullscreen;
    mouseWheelPos = 0;
        
    glfwSwapInterval(1);
    glfwEnable(GLFW_MOUSE_CURSOR);
    glfwSetWindowTitle(APP_TITLE);

    int w, h;
    glfwGetWindowSize(&w, &h);
    onResize(w, h);
    
    moveWalls(true);
    
    glfwSetWindowSizeCallback(onResize);
    glfwSetKeyCallback(onKey);
    glfwSetMousePosCallback(onMouseMove);
    glfwSetMouseButtonCallback(onMouseButton);
    glfwSetMouseWheelCallback(onMouseWheel);
    glfwSetWindowCloseCallback(onWindowClose);

    return true;
}

void GLFWCALL Frontend::onResize(int width, int height)
{
    instance->idleFrames = 0;
    
    if (width && height) {    
        if (!instance->isFullscreen) {
            // Save this width/height, for restoring windowed mode after fullscreen
            instance->lastWindowW = width;
            instance->lastWindowH = height;
        }

        instance->renderer.setViewport(width, height);
    }
}

int GLFWCALL Frontend::onWindowClose()
{
    instance->isRunning = false;
    return GL_TRUE;
}

void GLFWCALL Frontend::onKey(int key, int state)
{
    if (state == GLFW_PRESS) {
        switch (key) {
        
        case 'H':
        case '/':
        case GLFW_KEY_F1:
            instance->overlay.toggleHelp();
            break;
        
        case 'Q':
        case GLFW_KEY_ESC:
            instance->isRunning = false;
            break;
        
        case '1':
            instance->normalViewExtent = instance->pixelViewExtent();
            break;
        
        case '2':
            instance->normalViewExtent = instance->pixelViewExtent() / 2.0f;
            break;
        
        case 'Z':
            instance->toggleZoom ^= true;
            break;

        case 'F':
            instance->toggleFullscreen();
            break;
                
        case 'S': {
            std::string name = instance->createScreenshotName();
            instance->overlay.postMessage("Saved screenshot \"" + name + "\"");
            instance->renderer.takeScreenshot(name);
            break;
        }

        case 'T':
            instance->sys->opt_turbo ^= true;
            break;

        case 'I':
            instance->overlay.toggleInspector();
            break;

        case 'R':
            /*
             * Intentionally undocumented: Toggle trace mode.
             * If trace mode isn't available, this key is a silent no-op.
             */
            if (instance->sys->isTraceAllowed())
                instance->sys->tracer.setEnabled(!instance->sys->tracer.isEnabled());
            break;
        
        case GLFW_KEY_SPACE:
            if (instance->mouseIsPulling)
                instance->hoverOrRotate();
            break;

        case '-':
            instance->removeCube();
            break;

        case '+':
        case '=':
            instance->addCube();
            break;
        
        case GLFW_KEY_BACKSPACE:
            instance->toggleRotationLock();
            break;

        default:
            return;

        }
        
        // Any handled key resets the idle timer
        instance->idleFrames = 0;
    }   
}

void Frontend::toggleFullscreen()
{
    if (isFullscreen) {
        // Restore last windowed mode
        // Restore last windowed mode
        glfwCloseWindow();            
        openWindow(lastWindowW, lastWindowH, false);
    } else {
        // Full-screen, using the desktop's native video mode
        GLFWvidmode mode;
        glfwGetDesktopMode(&mode);
        glfwCloseWindow();
        openWindow(mode.Width, mode.Height, true);
    }
}           

void GLFWCALL Frontend::onMouseMove(int x, int y)
{
    instance->mouseX = x;
    instance->mouseY = y;
    
    if (instance->toggleZoom)
        instance->idleFrames = 0;    
}

void GLFWCALL Frontend::onMouseButton(int button, int state)
{
    if (state == GLFW_PRESS)
        instance->onMouseDown(button);
    else if (state == GLFW_RELEASE)
        instance->onMouseUp(button);

    instance->idleFrames = 0;
}

void GLFWCALL Frontend::onMouseWheel(int pos)
{
    float scale;
    int delta = pos - instance->mouseWheelPos;
    instance->mouseWheelPos = pos;
    
    if (delta > 0)
        scale = powf(0.9f, delta);
    else
        scale = powf(1.1f, -delta);
        
    instance->scaleViewExtent(scale);
    instance->idleFrames = 0;
}

void Frontend::onMouseDown(int button)
{
    idleFrames = 0;
    
    if (button == GLFW_MOUSE_BUTTON_RIGHT && mouseIsPulling)
        hoverOrRotate();
    
    if (!mouseBody) {
        mousePicker.test(world, mouseVec(normalViewExtent));

        if (mousePicker.mCube) {
            // This body represents the mouse itself now as a physical object
            b2BodyDef mouseDef;
            mouseDef.type = b2_kinematicBody;
            mouseDef.position = mousePicker.mPoint;
            mouseDef.allowSleep = false;
            mouseBody = world.CreateBody(&mouseDef);

            bool shift = glfwGetKey(GLFW_KEY_LSHIFT) || glfwGetKey(GLFW_KEY_RSHIFT);
            bool ctrl = glfwGetKey(GLFW_KEY_LCTRL) || glfwGetKey(GLFW_KEY_RCTRL);
            
            if (button == GLFW_MOUSE_BUTTON_RIGHT || shift) {
                /*
                 * If this was a right-click or shift-click, go into tilting mode
                 */
                
                mouseIsTilting = true;
                
            } else if (button == GLFW_MOUSE_BUTTON_MIDDLE || ctrl) {
                /*
                 * Ctrl-click or middle-click taps the cube's touch sensor
                 */

                mousePicker.mCube->setTouch(true);

            } else {
                /*
                 * Otherwise, the mouse is pulling.
                 *
                 * We represent our mouse as a kinematic body. This
                 * lets us pull/push a cube, or by grabbing it at an
                 * edge or corner, rotate it intuitively.
                 */
                
                mouseIsPulling = true;
                mousePicker.mCube->setHoverTarget(CubeConstants::HOVER_SLIGHT);
                
                /*
                 * Pick an attachment point. If we're close to the center,
                 * snap our attachment point to the center of mass, and
                 * turn on a servo that tries to orient the cube to a
                 * multiple of 90 degrees.
                 */
                
                b2Vec2 anchor = mousePicker.mPoint;
                b2Vec2 center = mousePicker.mCube->getBody()->GetWorldCenter();
                float centerDist = b2Distance(anchor, center);
                const float centerSize = CubeConstants::SIZE * CubeConstants::CENTER_SIZE;

                if (centerDist < centerSize) {
                    // Center-drag to align
                    anchor = center;
                    mouseIsAligning = true;
                }

                // Glue it to the point we picked, with a revolute joint

                b2RevoluteJointDef jointDef;
                jointDef.Initialize(mousePicker.mCube->getBody(), mouseBody, anchor);
                jointDef.motorSpeed = 0.0f;
                jointDef.maxMotorTorque = 100.0f;
                jointDef.enableMotor = false;
                mouseJoint = (b2RevoluteJoint*) world.CreateJoint(&jointDef);
            }
        }
    }
}

void Frontend::hoverOrRotate()
{
    /*
     * While a cube is grabbed, the spacebar or right mouse button can
     * be used to convert the grab into a hover. The cube will lift farther
     * off the ground, and it will be able to pass over other cubes.
     * You can use this to easily pick up a cube and drop it on the other side
     * of another cube, or to place it between two other cubes.
     *
     * If you perform this action again while the cube is already hovering,
     * it will do a quick rotate in-place.
     */

    if (mousePicker.mCube->isHovering()) {
        if (!mouseIsSpinning) {
            spinTarget = mousePicker.mCube->getBody()->GetAngle();
            mouseIsSpinning = true;
        }
        spinTarget += M_PI/2;
    } else {
        mousePicker.mCube->setHoverTarget(CubeConstants::HOVER_FULL);
    }
}

void Frontend::onMouseUp(int button)
{
    bool eitherButton = glfwGetMouseButton(GLFW_MOUSE_BUTTON_LEFT) ||
                        glfwGetMouseButton(GLFW_MOUSE_BUTTON_RIGHT);
        
    idleFrames = 0;
                        
    if (mouseBody && !eitherButton) {
        // All buttons released
    
        if (mousePicker.mCube) {
            mousePicker.mCube->setTiltTarget(b2Vec2(0.0f, 0.0f));
            mousePicker.mCube->setTouch(false);
            mousePicker.mCube->setHoverTarget(CubeConstants::HOVER_NONE);                
        }

        /* Mouse state reset */

        if (mouseJoint)
            world.DestroyJoint(mouseJoint);
        world.DestroyBody(mouseBody);
        mouseJoint = NULL;
        mouseBody = NULL;
        mousePicker.mCube = NULL;
        mouseIsAligning = false;
        mouseIsSpinning = false;
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
        idleFrames = 0;
    }

    if (mouseIsAligning || mouseIsSpinning) {
        /*
         * Spinning and aligning cubes. Both are implemented using a joint motor.
         */
        
        const float gain = 20.0f;
        float cubeAngle = mouseJoint->GetBodyA()->GetAngle();
        float error;
        
        if (mouseIsSpinning) {
            /*
             * Rotate the cube toward 'spinTarget'. When we get there, we disable spin mode.
             */
            error = cubeAngle - spinTarget;
            idleFrames = 0;
            if (fabs(error) < 0.001f)
                mouseIsSpinning = false; 
        
        } else if (mouseIsAligning) {
            /*
             * Continuously apply a force toward the nearest 90-degree multiple.
             */
            float fractional = fmod(cubeAngle + M_PI/4, M_PI/2);
            if (fractional < 0) fractional += M_PI/2;
            error = fractional - M_PI/4;
        }
        
        mouseJoint->EnableMotor(true);
        mouseJoint->SetMotorSpeed(gain * error);
    } else if (mouseJoint) {
        mouseJoint->EnableMotor(false);
    }

    if (mouseIsTilting) {
        /*
         * Animate the tilt, using a pair of Euler angles based on the distance
         * between the mouse's current position and its initial grab point.
         */

        if (mousePicker.mCube) {
            const float maxTilt = 80.0f;
            b2Vec2 mouseDiff = mouseVec(normalViewExtent) - mouseBody->GetWorldCenter();
            b2Vec2 tiltTarget = (maxTilt / CubeConstants::SIZE) * mouseDiff; 
            tiltTarget.x = b2Clamp(tiltTarget.x, -maxTilt, maxTilt);
            tiltTarget.y = b2Clamp(tiltTarget.y, -maxTilt, maxTilt);        

            // Rotate it into the cube's coordinates
            b2Vec2 local = b2Mul(b2Rot(-mousePicker.mCube->getBody()->GetAngle()), tiltTarget);

            mousePicker.mCube->setTiltTarget(local);
            idleFrames = 0;
        }
    }
        
    /* Local per-cube animations */
    for (unsigned i = 0; i < sys->opt_numCubes; i++)
        cubes[i].animate();

    /* Animated viewport centering/zooming */
    {
        const float gain = 0.1;
        int pzoom = pixelZoomMode();
        
        // Pull toward a pixel-accurate zoom, if we're close
        if (pzoom)
            normalViewExtent += gain * (pixelViewExtent() / pzoom - normalViewExtent);

        viewExtent += gain * (targetViewExtent() - viewExtent);
        viewCenter += gain * (targetViewCenter() - viewCenter);

        moveWalls();
    }

    world.Step(timeStep, velocityIterations, positionIterations);
}

float Frontend::pixelViewExtent()
{
    // Calculate the viewExtent which would give a 1:1 pixel mapping
    return renderer.getWidth() * (CubeConstants::LCD_SIZE / (2.0f * Cube::LCD::WIDTH));
}

unsigned Frontend::pixelZoomMode()
{
    /*
     * If we're close to an integer multiple zoom, we try to be pixel accurate.
     *
     * This returns zero if we're not close to an integer zoom, otherwise it returns
     * the integer zoom factor that we're close to. When this returns a nonzero number,
     * both the viewport and the cubes themselves should attempt to align themselves
     * to pixel boundaries.
     */
    
    const float threshold = 0.2f;
    const unsigned maxMultiplier = 4;
    float extent1to1 = pixelViewExtent();
       
    // Perform a relative distance test on several integer multiples
    for (unsigned multiplier = 1; multiplier <= maxMultiplier; multiplier++) {
        float extent = extent1to1 / multiplier;
        float absDist = extent - normalViewExtent;
        float relDist = fabs(absDist) / extent;

        if (relDist < threshold)
            return multiplier;
    }
    
    return 0;
}

void Frontend::scaleViewExtent(float ratio)
{
    normalViewExtent = b2Clamp<float>(normalViewExtent * ratio,
                                      CubeConstants::SIZE * 0.1, maxViewExtent);
}

void Frontend::draw()
{
    // Pixel zoom mode, with respect to current view rather than target view.
    int effectivePixelZoomMode = fabs(viewExtent - normalViewExtent) < 1e-3 ? pixelZoomMode() : 0;
    
    renderer.beginFrame(viewExtent, viewCenter, effectivePixelZoomMode);

    float ratio = std::max(1.0f, renderer.getHeight() / (float)renderer.getWidth());
    renderer.drawBackground(viewExtent * ratio * 50.0f, 0.2f);

    for (unsigned i = 0; i < sys->opt_numCubes; i++) {
        if (cubes[i].draw(renderer)) {
            // We found a cube that isn't idle.
            idleFrames = 0;
        }
    }

    renderer.beginOverlay();

    // Per-cube status overlays
    for (unsigned i = 0; i < sys->opt_numCubes;  i++) {
        FrontendCube &c = cubes[i]; 

        // Overlays are positioned relative to the cube's AABB.
        b2AABB aabb;    
        c.computeAABB(aabb);
        const float distance = 1.1f;
        b2Vec2 extents = aabb.GetExtents();

        b2Vec2 bottomCenter = worldToScreen(c.getBody()->GetPosition() +
            b2Vec2(0, distance * extents.y));

        b2Vec2 topRight = worldToScreen(c.getBody()->GetPosition() +
            b2Vec2(distance * extents.x, -distance * extents.y));

        overlay.drawCubeStatus(&c,
            0.5f + bottomCenter.x,
            0.5f + bottomCenter.y);

        overlay.drawCubeInspector(&c,
            0.5f + topRight.x,
            0.5f + topRight.y,
            0.5f + worldToScreen(1.5f),
            0.5f + worldToScreen(3.0f));
    }

    // Fixed portion of the overlay, should be topmost.
    overlay.draw();
        
    /*
     * If we haven't had any interesting input (user interaction, cube rendering) in a while,
     * assume we're idle and throttle the frame rate way down. Otherwise, render at a zippy but
     * bounded refresh rate.
     *
     * This throttling is vital when we can't sync to the host's vertical refresh, and the idling
     * is really nice to avoid killing laptop batteries...
     */
    frControl.setTargetFPS(idleFrames > 100 ? 10.0 : 75.0);
        
    renderer.endFrame();
    frControl.endFrame();
}

b2Vec2 Frontend::worldToScreen(b2Vec2 world)
{
    // Convert world coordinates to screen coordinates
    
    world -= viewCenter;
    return b2Vec2(renderer.getWidth() / 2, renderer.getHeight() / 2)
                  + renderer.getWidth() * ((0.5f / viewExtent) * world);
}

float Frontend::worldToScreen(float world)
{
    // Convert a scalar distance from world to screen coordinates
    
    return renderer.getWidth() * ((0.5f / viewExtent) * world);
}

float Frontend::zoomedViewExtent()
{
    /*
     * Default scaling uses the X axis, but here we want to make sure
     * the whole of a cube is visible, as applicable. So, scale using
     * whichever axis is smaller.
     */

    float scale = (renderer.getHeight() < renderer.getWidth())
        ? renderer.getWidth() / (float) renderer.getHeight() : 1.0f;
        
    if (sys->opt_numCubes > 1) {
        // Zoom in one one cube
        return scale * CubeConstants::SIZE * 1.1;
    } else {
        // High zoom on our one and only cube
        return scale * CubeConstants::SIZE * 0.2;
    }
}

float Frontend::targetViewExtent()
{
    return toggleZoom ? zoomedViewExtent() : normalViewExtent;
}    

b2Vec2 Frontend::targetViewCenter()
{
    // When zooming in/out, the pixel under the cursor stays the same.
    return toggleZoom ? (mouseVec(normalViewExtent) - mouseVec(targetViewExtent()))
        : b2Vec2(0.0, 0.0);
}

b2Vec2 Frontend::mouseVec(float viewExtent)
{
    int halfWidth = renderer.getWidth() / 2;
    int halfHeight = renderer.getHeight() / 2;
    float mouseScale = viewExtent / (float)halfWidth;
    return  b2Vec2((mouseX - halfWidth) * mouseScale,
                   (mouseY - halfHeight) * mouseScale);
}

std::string Frontend::createScreenshotName()
{
        char buffer[128];
    time_t t = time(NULL);
        const struct tm *now = localtime(&t);
        strftime(buffer, sizeof buffer, "siftulator-%Y%m%d-%H%M%S", now);
        return std::string(buffer);
}

void Frontend::toggleRotationLock() {
    isRotationFixed = !isRotationFixed;

    overlay.postMessage(std::string("Rotation lock ")
        + (isRotationFixed ? "on" : "off"));

    for (unsigned i = 0; i < sys->opt_numCubes; i++) {
        cubes[i].toggleRotationLock(isRotationFixed);
    }
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
    updateSensors(contact, true);
}

void Frontend::ContactListener::EndContact(b2Contact *contact)
{
    updateSensors(contact, false);
}

void Frontend::ContactListener::updateSensors(b2Contact *contact, bool touching)
{
    FixtureData *fdatA = (FixtureData *) contact->GetFixtureA()->GetUserData();
    FixtureData *fdatB = (FixtureData *) contact->GetFixtureB()->GetUserData();

    if (fdatA && fdatB
        && fdatA->type == fdatA->T_NEIGHBOR
        && fdatB->type == fdatB->T_NEIGHBOR) {

        /*
         * Update interaction between two neighbor sensors
         */

        unsigned cubeA = frontend.cubeID(fdatA->cube);
        unsigned cubeB = frontend.cubeID(fdatB->cube);

        fdatA->cube->updateNeighbor(touching, fdatA->side, fdatB->side, cubeB);
        fdatB->cube->updateNeighbor(touching, fdatB->side, fdatA->side, cubeA);
    }
}

FrameRateController::FrameRateController()
    : lastTimestamp(0), accumulator(0) {}
    
void FrameRateController::endFrame()
{
    /*
     * We try to synchronize to the host's vertical sync.
     * If the video drivers don't support that, this governor
     * prevents us from running way too fast.
     */

    /*
     * How far ahead we need to be on average, in seconds,
     * before we start inserting delays
     */
    const double slack = 0.25;
    
    /*
     * How far ahead or behind are we allowed to get?
     */
    const double limit = 1.0;

    double now = glfwGetTime();
    const double minPeriod = 1.0 / targetFPS;
    double thisPeriod = now - lastTimestamp;
    lastTimestamp = now;
    
    accumulator += minPeriod - thisPeriod;
    
    if (accumulator < -limit)
        accumulator = -limit;        
    else if (accumulator > slack) {
        if (accumulator > limit)
            accumulator = limit;
        glfwSleep(accumulator - slack);
    }
}
