/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _GL_RENDERER_H
#define _GL_RENDERER_H

#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#   include <windows.h>
#endif

#ifdef __MACH__
#   define HAS_GL_ENTRY(x)   (true)
#   include <OpenGL/gl.h>
#   include <OpenGL/glext.h>
#elif defined (_WIN32)
#   define GLEW_STATIC
#   define HAS_GL_ENTRY(x)   ((x) != NULL)
#   include <GL/glew.h>
#else
#   define GL_GLEXT_PROTOTYPES
#   define HAS_GL_ENTRY(x)   (true)
#   include <GL/gl.h>
#   include <GL/glext.h>
#endif

#ifndef GL_UNSIGNED_SHORT_5_6_5
#   define GL_UNSIGNED_SHORT_5_6_5 0x8363
#endif

#include <Box2D/Box2D.h>
#include <vector>
#include <string>
#include "system.h"


class GLRenderer {
 public:
    bool init();
    void setViewport(int width, int height);

    void beginFrame(float viewExtent, b2Vec2 viewCenter);
    void endFrame();

    void drawBackground(float extent, float scale);
    void drawCube(unsigned id, b2Vec2 center, float angle, float hover,
                  b2Vec2 tilt, uint16_t *framebuffer, b2Mat33 &modelMatrix);
                  
    void beginOverlay();
    void overlayText(unsigned x, unsigned y, const float color[4], const char *str);
    unsigned measureText(const char *str);

  	void takeScreenshot(std::string name) {
		// Screenshots are asynchronous
		pendingScreenshotName = name;
	}
    
    unsigned getWidth() const {
        return viewportWidth;
    }
    
    unsigned getHeight() const {
        return viewportHeight;
    }

 private:
    struct VertexTN {
        GLfloat tx, ty;
        GLfloat nx, ny, nz;
        GLfloat vx, vy, vz;
    };

    struct VertexT {
        GLfloat tx, ty;
        GLfloat vx, vy, vz;
    };
    
    struct Glyph {
        // From the BMFont file format
        uint32_t id;
        uint16_t x;
        uint16_t y;
        uint16_t width;
        uint16_t height;
        int16_t xOffset;
        int16_t yOffset;
        int16_t xAdvance;
        uint8_t page;
        uint8_t channel;
    };

    const Glyph *findGlyph(uint32_t id);
    
    void initCube(unsigned id);
    void cubeTransform(b2Vec2 center, float angle, float hover,
                       b2Vec2 tilt, b2Mat33 &modelMatrix);

    void drawCubeBody();
    void drawCubeFace(unsigned id, uint16_t *framebuffer);
    
    GLuint loadTexture(const uint8_t *pngData, GLenum wrap=GL_CLAMP, GLenum filter=GL_LINEAR);
    GLhandleARB loadShader(GLenum type, const uint8_t *source);
    GLhandleARB linkProgram(GLhandleARB fp, GLhandleARB vp);

    void createRoundedRect(std::vector<VertexTN> &outPolygon, float size, float height, float relRadius);
    void extrudePolygon(const std::vector<VertexTN> &inPolygon, std::vector<VertexTN> &outTristrip);

    void saveTexturePNG(std::string name, unsigned width, unsigned height);
	void saveColorBufferPNG(std::string name);
	
    int viewportWidth, viewportHeight;
    
    GLhandleARB cubeFaceProgram;
    GLuint cubeFaceTexture;
    GLuint cubeFaceHilightTexture;
    GLuint cubeFaceHilightMaskTexture;

    GLhandleARB cubeSideProgram;

    GLhandleARB backgroundProgram;
    GLuint backgroundTexture;
    
    GLuint fontTexture;

    std::vector<VertexTN> faceVA;
    std::vector<VertexTN> sidesVA;
    std::vector<VertexT> textVA;

    struct {
        bool initialized;
        GLuint lcdTexture;
    } cubes[System::MAX_CUBES];
	
	std::string pendingScreenshotName;
};

#endif
