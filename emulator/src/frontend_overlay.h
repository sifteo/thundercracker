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

#ifndef _FRONTEND_OVERLAY_H
#define _FRONTEND_OVERLAY_H

#include "gl_renderer.h"
#include "system.h"

#include <Box2D/Box2D.h>
#include <string>


struct Color {
    Color() {}
    
    Color(float r, float g, float b, float a=1.0f) {
        set(r, g, b, a);
    }
    
    void set(float r, float g, float b, float a=1.0f) {
        v[0] = r;
        v[1] = g;
        v[2] = b;
        v[3] = a;
    }
    
    float v[4];
};    


class FrontendOverlay {
public:
    FrontendOverlay();

    void init(GLRenderer *renderer, System *sys);

    void draw();
    void drawCubeStatus(FrontendCube *fe, int x, int y);
    void drawCubeInspector(FrontendCube *fe, int x, int y, int width, int height);

    void toggleHelp();
    void toggleInspector();
    void toggleAudioVisualizer();

    void postMessage(std::string msg);

    bool allowIdling() {
        return !visualizerVisible;
    }

private:
    static const unsigned margin = 5;
    static const unsigned lineSpacing = 20;
    
    void moveTo(int _x, int _y) {
        x = _x;
        y = _y;
    }
    
    void text(const Color &c, const char *msg, float align=0.0f);
    void drawHelp();
    void drawRealTimeInfo();
    
    GLRenderer *renderer;
    System *sys;
    
    std::string message;
    unsigned messageTimer;
    bool helpVisible;
    bool inspectorVisible;
    bool visualizerVisible;
    
    ElapsedTime slowTimer;
    ElapsedTime fastTimer;
    ElapsedTime realTimeMessageTimer;
    
    char realTimeMessage[64];
    Color realTimeColor;
    float filteredTimeRatio;
    float visualizerAlpha;

    int x, y;
    
    struct {
        char fps[64];
        EventRateProbe lcd_wr;
        uint32_t flashModifyCount;
    } cubes[System::MAX_CUBES];
};


#endif
