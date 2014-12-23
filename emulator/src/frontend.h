/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * Micah Elizabeth Scott <micah@misc.name>
 *
 * Copyright <c> 2011 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef _FRONTEND_H
#define _FRONTEND_H

#include "gl_renderer.h"
#include "system.h"
#include "frontend_cube.h"
#include "frontend_mc.h"
#include "frontend_overlay.h"
#include "tinythread.h"

#include <Box2D/Box2D.h>
#include <string>
#include <GL/glfw.h>


class FrameRateController {
public:
    FrameRateController();

    double getEndFrameSleepTime();

    void setTargetFPS(double target) {
        targetFPS = target;
    }

private:
    double lastTimestamp;
    double accumulator;
    double targetFPS;
};


class Frontend {
 public:
    Frontend();

    bool init(System *sys);

    static bool runFrame();
    static void exit();

    static void postMessage(std::string msg);

 private:
    /*
     * Number of real frames per virtual LCD frame (Assume 60Hz
     * real, 30 Hz virtual). This doesn't have any bearing on the
     * rendering frame rate of the cubes, it's just used for
     * generating vertical sync signals.
     */
    static const unsigned FRAME_HZ_DIVISOR = 2;

    class MousePicker : public b2QueryCallback {
    public:
        void test(b2World &world, b2Vec2 point);
        bool ReportFixture(b2Fixture *fixture);
        b2Vec2 mPoint;
        FrontendCube *mCube;
        FrontendMC *mMC;
    };

    class ContactListener : public b2ContactListener {
    public:
        ContactListener(Frontend &fe) : frontend(fe) {};
        void BeginContact(b2Contact *contact);
        void EndContact(b2Contact *contact);
    private:
        Frontend &frontend;
        void updateSensors(b2Contact *contact, bool touching);
    };

    void animate();
    void draw();
    void updateCubeCount();

    bool openWindow(int width, int height, bool fullscreen=false);
    void toggleFullscreen();

    static void GLFWCALL onResize(int width, int height);
    static void GLFWCALL onKey(int key, int state);
    static void GLFWCALL onMouseMove(int x, int y);
    static void GLFWCALL onMouseButton(int button, int state);
    static void GLFWCALL onMouseWheel(int pos);
    static int GLFWCALL onWindowClose();

    void onMouseDown(int button);
    void onMouseUp(int button);
    void hoverOrRotate();

    void scaleViewExtent(float ratio);
    void createWalls();
    void moveWalls(bool immediate=false);
    void pushBodyTowards(b2Body *b, b2Vec2 target, float gain);

    static float viewExtentForCubeCount(unsigned num);
    static b2Vec2 getCubeGridLoc(unsigned index, unsigned total);
    b2Body *newKBox(float x, float y, float hw, float hh);
    unsigned cubeID(FrontendCube *cube);

    float zoomedViewExtent();
    float targetViewExtent();
    float pixelViewExtent();
    unsigned pixelZoomMode();
    float getYRatio();

    b2Vec2 targetViewCenter();
    b2Vec2 mouseVec(float viewExtent);
    b2Vec2 worldToScreen(b2Vec2 world);
    float worldToScreen(float x);

    std::string createScreenshotName();
    void drawOverlay();

    void toggleRotationLock();
    void postVolumeMessage();
    void postBatteryMessage();

    System *sys;
    unsigned frameCount;
    unsigned idleFrames;

    // Object counts, local to the Frontend. (May lag the simulation thread)
    unsigned cubeCount;

    FrontendMC mc;
    FrontendCube cubes[System::MAX_CUBES];

    bool toggleZoom;
    bool isFullscreen;
    bool isRunning;
    bool isRotationFixed;
    bool isAnimatingNewCubeLayout;

    int mouseX, mouseY;
    int mouseWheelPos;
    unsigned lastWindowW, lastWindowH;

    float viewExtent;
    float normalViewExtent;
    float maxViewExtent;
    b2Vec2 viewCenter;

    b2World world;
    b2Body *walls[4];

    b2Body *mouseBody;
    b2RevoluteJoint *mouseJoint;
    float spinTarget;
    bool mouseIsAligning;
    bool mouseIsSpinning;
    bool mouseIsPulling;
    bool mouseIsTilting;

    MousePicker mousePicker;
    ContactListener contactListener;
    FrameRateController frControl;

    GLRenderer renderer;
    FrontendOverlay overlay;

    static Frontend *instance;
    static tthread::mutex instanceLock;
};

#endif
