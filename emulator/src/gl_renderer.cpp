/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * Sifteo Thundercracker simulator
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include "mc_audiovisdata.h"
#include "gl_renderer.h"
#include "frontend.h"
#include "lodepng.h"
#include "cube_flash_model.h"
#include "frontend_bmfont.h"
#include "frontend_model.h"


bool GLRenderer::init()
{
#ifdef GLEW_STATIC
    GLenum err = glewInit();
    if (GLEW_OK != err) {
        /* Problem: glewInit failed, something is seriously wrong. */
        fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
        return false;
    }
#endif

    if (!HAS_GL_ENTRY(glCreateShaderObjectARB)) {
        fprintf(stderr, "Error: Shader support not available\n");
        return false;
    }
        
    /*
     * Compile our shaders
     */ 

    cubeFaceProgFiltered = loadCubeFaceProgram("#define FILTER\n");
    cubeFaceProgUnfiltered = loadCubeFaceProgram("");

    extern const uint8_t cube_body_fp[];
    extern const uint8_t cube_body_vp[];
    GLhandleARB cubeBodyFP = loadShader(GL_FRAGMENT_SHADER, cube_body_fp);
    GLhandleARB cubeBodyVP = loadShader(GL_VERTEX_SHADER, cube_body_vp);
    cubeBodyProgram = linkProgram(cubeBodyFP, cubeBodyVP);

    extern const uint8_t mc_face_fp[];
    extern const uint8_t mc_face_vp[];
    GLhandleARB mcFaceFP = loadShader(GL_FRAGMENT_SHADER, mc_face_fp);
    GLhandleARB mcFaceVP = loadShader(GL_VERTEX_SHADER, mc_face_vp);
    mcFaceProgram = linkProgram(mcFaceFP, mcFaceVP);
    glUseProgramObjectARB(mcFaceProgram);
    glUniform1iARB(glGetUniformLocationARB(mcFaceProgram, "normalmap"), 0);
    glUniform1iARB(glGetUniformLocationARB(mcFaceProgram, "lightmap"), 1);
    mcFaceLEDLocation = glGetUniformLocationARB(mcFaceProgram, "led");

    extern const uint8_t background_fp[];
    extern const uint8_t background_vp[];
    GLhandleARB backgroundFP = loadShader(GL_FRAGMENT_SHADER, background_fp);
    GLhandleARB backgroundVP = loadShader(GL_VERTEX_SHADER, background_vp);
    backgroundProgram = linkProgram(backgroundFP, backgroundVP);
    glUseProgramObjectARB(backgroundProgram);
    glUniform1iARB(glGetUniformLocationARB(backgroundProgram, "texture"), 0);
    glUniform1iARB(glGetUniformLocationARB(backgroundProgram, "lightmap"), 1);
    glUniform1iARB(glGetUniformLocationARB(backgroundProgram, "logo"), 2);

    extern const uint8_t scope_fp[];
    extern const uint8_t scope_vp[];
    GLhandleARB scopeFP = loadShader(GL_FRAGMENT_SHADER, scope_fp);
    GLhandleARB scopeVP = loadShader(GL_VERTEX_SHADER, scope_vp);
    scopeProgram = linkProgram(scopeFP, scopeVP);
    glUseProgramObjectARB(scopeProgram);
    glUniform1iARB(glGetUniformLocationARB(scopeProgram, "sampleBuffer"), 0);
    glUniform1iARB(glGetUniformLocationARB(scopeProgram, "background"), 1);
    scopeAlphaAttr = glGetUniformLocationARB(scopeProgram, "alphaAttr");

    /*
     * Load models
     */

    extern const uint8_t model_cube_body[];
    extern const uint8_t model_cube_face[];
    extern const uint8_t model_mc_body[];
    extern const uint8_t model_mc_face[];
    extern const uint8_t model_mc_volume[];

    loadModel(model_cube_body, cubeBody);
    loadModel(model_cube_face, cubeFace);
    loadModel(model_mc_body, mcBody);
    loadModel(model_mc_face, mcFace);
    loadModel(model_mc_volume, mcVolume);

    /*
     * Load textures
     */

    extern const uint8_t img_cube_face_hilight[];
    extern const uint8_t img_mc_face_normals[];
    extern const uint8_t img_mc_face_light[];
    extern const uint8_t img_wood[];
    extern const uint8_t img_bg_light[];
    extern const uint8_t img_scope_bg[];
    extern const uint8_t img_logo[];
    extern const uint8_t ui_font_data_0[];

    cubeFaceHilightTexture = loadTexture(img_cube_face_hilight);
    mcFaceNormalsTexture = loadTexture(img_mc_face_normals);
    mcFaceLightTexture = loadTexture(img_mc_face_light);
    backgroundTexture = loadTexture(img_wood, GL_REPEAT);
    bgLightTexture = loadTexture(img_bg_light);
    fontTexture = loadTexture(ui_font_data_0, GL_CLAMP, GL_NEAREST);
    scopeSampleTexture = 0;
    scopeBackgroundTexture = loadTexture(img_scope_bg, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR);

    logoTexture = loadTexture(img_logo, GL_CLAMP, GL_LINEAR_MIPMAP_LINEAR);
    GLfloat logoBorder[4] = { 0.5f, 0.0f, 0.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, logoBorder);

    /*
     * Per-cube initialization is lazy
     */

    for (unsigned i = 0; i < System::MAX_CUBES; i++) {
        cubes[i].fbInitialized = false;
        cubes[i].flashInitialized = false;
    }

    return true;
}

GLhandleARB GLRenderer::loadCubeFaceProgram(const char *prefix)
{
    /*
     * About our shader for rendering the LCD... We could probably do
     * something more complicated with simulating the subpixels in the
     * LCD screens, but right now I just have two goals:
     *
     *   1. See each pixel distinctly at >100% zooms
     *   2. No visible aliasing artifacts
     *
     * This uses a simple algorithm that decomposes texture coordinates
     * into a fractional and whole pixel portion, and applies a smooth
     * nonlinear interpolation across pixel boundaries:
     *
     * http://www.iquilezles.org/www/articles/texture/texture.htm
     */

    extern const uint8_t cube_face_fp[];
    extern const uint8_t cube_face_vp[];
    GLhandleARB cubeFaceFP = loadShader(GL_FRAGMENT_SHADER, cube_face_fp, prefix);
    GLhandleARB cubeFaceVP = loadShader(GL_VERTEX_SHADER, cube_face_vp, prefix);
    GLhandleARB prog = linkProgram(cubeFaceFP, cubeFaceVP);

    glUseProgramObjectARB(prog);
    glUniform1fARB(glGetUniformLocationARB(prog, "LCD_SIZE"), CubeConstants::LCD_SIZE);
    glUniform1iARB(glGetUniformLocationARB(prog, "hilight"), 0);
    glUniform1iARB(glGetUniformLocationARB(prog, "lcd"), 1);

    return prog;
}
    
GLhandleARB GLRenderer::loadShader(GLenum type, const uint8_t *source, const char *prefix)
{
    const GLchar *stringArray[2];
    stringArray[0] = (const GLchar *) prefix;
    stringArray[1] = (const GLchar *) source;

    GLhandleARB shader = glCreateShaderObjectARB(type);
    glShaderSourceARB(shader, 2, stringArray, NULL);
    glCompileShaderARB(shader);

    GLint isCompiled = false;
    glGetObjectParameterivARB(shader, GL_COMPILE_STATUS, &isCompiled);
    if (!isCompiled) {
        GLchar log[1024];
        GLint logLength = sizeof log;

        glGetInfoLogARB(shader, logLength, &logLength, log);
        log[logLength] = '\0';

        fprintf(stderr, "Error: Shader compile failed.\n%s", log);
    }

    return shader;
}

GLhandleARB GLRenderer::linkProgram(GLhandleARB fp, GLhandleARB vp)
{
    GLhandleARB program = glCreateProgramObjectARB();
    glAttachObjectARB(program, vp);
    glAttachObjectARB(program, fp);
    glLinkProgramARB(program);

    GLint isLinked = false;
    glGetObjectParameterivARB(program, GL_LINK_STATUS, &isLinked);
    if (!isLinked) {
        GLchar log[1024];
        GLint logLength = sizeof log;

        glGetInfoLogARB(program, logLength, &logLength, log);
        log[logLength] = '\0';

        fprintf(stderr, "Error: Shader link failed.\n%s", log);
    }

    return program;
}

void GLRenderer::setViewport(int width, int height)
{
    glViewport(0, 0, width, height);
    viewportWidth = width;
    viewportHeight = height;
}

void GLRenderer::beginFrame(float viewExtent, b2Vec2 viewCenter, unsigned pixelZoomMode)
{
    /*
     * Camera.
     *
     * Our simulation (and our mouse input!) is really 2D, but we use
     * perspective in order to make the whole scene feel a bit more
     * real, and especially to make tilting believable.
     *
     * We scale this so that the X axis is from -viewExtent to
     * +viewExtent at the level of the cube face.
     */
     
    currentFrame.viewExtent = viewExtent;
    currentFrame.pixelZoomMode = pixelZoomMode;

    float aspect = viewportHeight / (float)viewportWidth;
    float yExtent = aspect * viewExtent;

    float zPlane = CubeConstants::SIZE * CubeConstants::HEIGHT;
    float zCamera = 5.0f;
    float zNear = 1.0f;
    float zFar = 10.0f;
    float zDepth = zFar - zNear;
    
    GLfloat proj[16] = {
        zCamera / viewExtent,
        0,
        0,
        0,

        0,
        -zCamera / yExtent,
        0,
        0,

        0,
        0,
        -(zFar + zNear) / zDepth,
        -1.0f,

        0,
        0,
        -2.0f * (zFar * zNear) / zDepth,
        0,
    };

    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(proj);
    glTranslatef(-viewCenter.x, -viewCenter.y, -zCamera);
    glScalef(1, 1, 1.0 / viewExtent);
    glTranslatef(0, 0, -zPlane);
    glMatrixMode(GL_MODELVIEW);

    glDisable(GL_BLEND);
    glDisable(GL_LIGHTING);
    glEnable(GL_CULL_FACE);
    glShadeModel(GL_SMOOTH);

    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
}

void GLRenderer::endFrame()
{
    glfwSwapBuffers();
}

void GLRenderer::beginOverlay()
{
    /*
     * Handle screenshots here, since we don't want
     * the screenshot to include overlay text. Save the
     * full screenshot, and end our pending screenshot request.
     */
    if (!pendingScreenshotName.empty()) {
        saveColorBufferPNG(pendingScreenshotName + ".png");
        pendingScreenshotName.clear();
    }

    // Do our text rendering in pixel coordinates
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glScalef(1, -1, 1);
    glTranslatef(-1, -1, 0);
    glScalef(2.0f / viewportWidth, 2.0f / viewportHeight, 1);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glShadeModel(GL_FLAT);    
}

int GLRenderer::measureText(const char *str)
{
    int x = 0, w = 0;
    uint32_t id;
    
    while ((id = (uint8_t) *(str++))) {
        const FrontendBMFont::Glyph *g = FrontendBMFont::findGlyph(id);
        if (g) {
            w = MAX(w, x + g->xOffset + g->width);
            x += g->xAdvance;
        }
    }
    
    return w;
}

void GLRenderer::overlayText(int x, int y, const float color[4], const char *str)
{
    const float TEXTURE_WIDTH = 128.0f;
    const float TEXTURE_HEIGHT = 256.0f;
   
    overlayVA.clear();

    uint32_t id;
    while ((id = (uint8_t) *(str++))) {
        const FrontendBMFont::Glyph *g = FrontendBMFont::findGlyph(id);
        if (g) {
            VertexT a, b, c, d;
            
            a.tx = g->x / TEXTURE_WIDTH;
            a.ty = g->y / TEXTURE_HEIGHT;
            a.vx = x + g->xOffset;
            a.vy = y + g->yOffset;
            a.vz = 0;
            
            b = a;
            b.tx += g->width / TEXTURE_WIDTH;
            b.vx += g->width;
            
            d = a;
            d.ty += g->height / TEXTURE_HEIGHT;
            d.vy += g->height;
            
            c = b;
            c.ty = d.ty;
            c.vy = d.vy;
            
            overlayVA.push_back(a);
            overlayVA.push_back(b);
            overlayVA.push_back(c);
            
            overlayVA.push_back(a);
            overlayVA.push_back(c);
            overlayVA.push_back(d);
        
            x += g->xAdvance;
        }
    }

    glUseProgramObjectARB(0);
    glColor4fv(color);
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, fontTexture);
    glInterleavedArrays(GL_T2F_V3F, 0, &overlayVA[0]);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei) overlayVA.size());
    glDisable(GL_TEXTURE_2D);
}         

void GLRenderer::overlayRect(int x, int y, int w, int h,
    const float color[4], GLhandleARB program)
{
    glUseProgramObjectARB(program);
    glColor4fv(color);
    overlayRect(x, y, w, h);
}

void GLRenderer::overlayRect(int x, int y, int w, int h)
{
    overlayVA.clear();
    VertexT a, b, c, d;
           
    a.tx = 0;
    a.ty = 0;
    a.vx = x;
    a.vy = y;
    a.vz = 0;
            
    b = a;
    b.vx += w;
    b.tx = 1;
    
    d = a;
    d.vy += h;
    d.ty = 1;
    
    c = b;
    c.vy = d.vy;
    c.ty = 1;
            
    overlayVA.push_back(a);
    overlayVA.push_back(b);
    overlayVA.push_back(c);
         
    overlayVA.push_back(a);
    overlayVA.push_back(c);
    overlayVA.push_back(d);

    glInterleavedArrays(GL_T2F_V3F, 0, &overlayVA[0]);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei) overlayVA.size());
}

void GLRenderer::drawDefaultBackground(float extent, float scale)
{
    float tc = scale * extent;
    VertexTN bg[] = {
        {-tc,  tc,   0.0f, 0.0f, 1.0f,   -extent,  extent, 0.0f },
        { tc,  tc,   0.0f, 0.0f, 1.0f,    extent,  extent, 0.0f },
        {-tc, -tc,   0.0f, 0.0f, 1.0f,   -extent, -extent, 0.0f },
        { tc, -tc,   0.0f, 0.0f, 1.0f,    extent, -extent, 0.0f },
    };

    glLoadIdentity();
    glUseProgramObjectARB(backgroundProgram);
    
    // For speed, don't bother with depth writes
    glDisable(GL_DEPTH_TEST);
    
    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, backgroundTexture);

    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, bgLightTexture);

    glActiveTexture(GL_TEXTURE2);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, logoTexture);

    glInterleavedArrays(GL_T2F_N3F_V3F, 0, bg);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    /*
     * Clean up GL state.
     */

    glActiveTexture(GL_TEXTURE2);
    glDisable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE1);
    glDisable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_DEPTH_TEST);
}

void GLRenderer::drawSolidBackground(const float color[4])
{
    glClearColor(color[0], color[1], color[2], color[3]);
    glClear(GL_COLOR_BUFFER_BIT);
}

void GLRenderer::initCubeFB(unsigned id)
{
    GLCube &cube = cubes[id];
    
    cube.fbInitialized = true;

    glGenTextures(NUM_LCD_TEXTURES, &cube.texFiltered[0]);
    glGenTextures(NUM_LCD_TEXTURES, &cube.texAccurate[0]);
    cube.currentLcdTexture = 0;

    /*
     * To reduce GL state thrashing, plus to work around a specific
     * GPU driver bug (pivotal 23521255) we use separate textures for
     * filtered vs. pixel-accurate modes. Each texture has filtering
     * and mipgen state that we never change during the lifetime of the
     * texture.
     */
    
    for (unsigned i = 0; i < NUM_LCD_TEXTURES; i++) {
        initCubeTexture(cube.texAccurate[i], true);
        initCubeTexture(cube.texFiltered[i], false);
    }
}

void GLRenderer::initCubeTexture(GLuint name, bool pixelAccurate)
{
    glBindTexture(GL_TEXTURE_2D, name);

    glTexImage2D(GL_TEXTURE_2D, 0, 3, Cube::LCD::WIDTH, Cube::LCD::HEIGHT,
                 0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    // Enabled dynamically only when tilted
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, 0.0f);

    if (pixelAccurate) {
        /*
         * Pixel-accurate zooming and transforms. Disable filtering entirely.
         * (The shader's nonlinear sampling becomes a no-op here)
         */

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);        

    } else {
        /*
         * Default high-quality filtering. We rely on linear sampling, plus
         * a higher-order polynomial filter implemented in a fragment program.
         *
         * We rely on mipmaps (with autogeneration) for good-looking
         * minification, and our nonlinear texture sampling (in the
         * fragment program) for good-looking rotation and magnification.
         *
         * We also use anisotropic filtering if we're tilted at all.
         */

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    }    
}

void GLRenderer::cubeTransform(b2Vec2 center, float angle, float hover,
                               b2Vec2 tilt, GLRenderer::CubeTransformState &tState)
{
    /*
     * The whole modelview matrix is ours to set up as a cube->world
     * transform. We use this for rendering, and we also stow it for
     * use in converting acceleration data back to cube coordinates.
     */

    tState.isTilted = false;
    tState.nonPixelAccurate = false;
     
    /*
     * Are we rotating to a non-right-angle? Not pixel accurate.
     * But if we're really close to a right angle, nudge it to line
     * up exactly so we can stay accurate.
     */
     
    float fractional = fmod(angle + M_PI/4, M_PI/2);
    if (fractional < 0) fractional += M_PI/2;
    fractional -= M_PI/4;

    if (fabs(fractional) > 0.02f)
        tState.nonPixelAccurate = true;
    else
        angle -= fractional;

    /*
     * Build the OpenGL transform
     */

    glLoadIdentity();
    glTranslatef(center.x, center.y, 0.0f);
    glRotatef(angle * (180.0f / M_PI), 0,0,1);
   
    const float tiltDeadzone = 5.0f;
    const float height = CubeConstants::HEIGHT;

    if (tilt.x > tiltDeadzone) {
        glTranslatef(CubeConstants::SIZE, 0, height * CubeConstants::SIZE);
        glRotatef(tilt.x - tiltDeadzone, 0,1,0);
        glTranslatef(-CubeConstants::SIZE, 0, -height * CubeConstants::SIZE);
        tState.isTilted = true;
        tState.nonPixelAccurate = true;
    }
    if (tilt.x < -tiltDeadzone) {
        glTranslatef(-CubeConstants::SIZE, 0, height * CubeConstants::SIZE);
        glRotatef(tilt.x + tiltDeadzone, 0,1,0);
        glTranslatef(CubeConstants::SIZE, 0, -height * CubeConstants::SIZE);
        tState.isTilted = true;
        tState.nonPixelAccurate = true;
    }

    if (tilt.y > tiltDeadzone) {
        glTranslatef(0, CubeConstants::SIZE, height * CubeConstants::SIZE);
        glRotatef(-tilt.y + tiltDeadzone, 1,0,0);
        glTranslatef(0, -CubeConstants::SIZE, -height * CubeConstants::SIZE);
        tState.isTilted = true;
        tState.nonPixelAccurate = true;
    }
    if (tilt.y < -tiltDeadzone) {
        glTranslatef(0, -CubeConstants::SIZE, height * CubeConstants::SIZE);
        glRotatef(-tilt.y - tiltDeadzone, 1,0,0);
        glTranslatef(0, CubeConstants::SIZE, -height * CubeConstants::SIZE);
        tState.isTilted = true;
        tState.nonPixelAccurate = true;
    }

    /* Save a copy of the transformation, before scaling it by our size. */
    GLfloat mat[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, mat);
    tState.modelMatrix->ex.x = mat[0];
    tState.modelMatrix->ex.y = mat[1];
    tState.modelMatrix->ex.z = mat[2];
    tState.modelMatrix->ey.x = mat[4];
    tState.modelMatrix->ey.y = mat[5];
    tState.modelMatrix->ey.z = mat[6];
    tState.modelMatrix->ez.x = mat[8];
    tState.modelMatrix->ez.y = mat[9];
    tState.modelMatrix->ez.z = mat[10];

    /* Now scale it */
    glScalef(CubeConstants::SIZE, CubeConstants::SIZE, CubeConstants::SIZE);
    
    /* Hover is relative to cube size, so apply that now. */
    if (hover > 1e-3) {
        glTranslatef(0.0f, 0.0f, hover);
        tState.nonPixelAccurate = true;
    }
}


void GLRenderer::drawCube(unsigned id, b2Vec2 center, float angle, float hover,
                          b2Vec2 tilt, const uint16_t *framebuffer, bool framebufferChanged,
                          b2Mat33 &modelMatrix)
{
    /*
     * Draw one cube, and place its modelview matrix in 'modelmatrix'.
     * If framebuffer==NULL, don't reupload the framebuffer, it hasn't changed.
     */

    if (!cubes[id].fbInitialized) {
        initCubeFB(id);
        
        // Re-upload framebuffer, even if the LCD hasn't changed
        framebufferChanged = true;
    }

    if (currentFrame.pixelZoomMode) {
        // Nudge the position to a pixel grid boundary        
        float pixelSize = currentFrame.viewExtent * 2.0f / viewportWidth;
        center.x = round(center.x / pixelSize) * pixelSize;
        center.y = round(center.y / pixelSize) * pixelSize;

        // Center adjustment for odd-sized viewports
        if (viewportWidth & 1)
            center.x -= pixelSize * 0.5f;
        if (viewportHeight & 1)
            center.y -= pixelSize * 0.5f;
    }
    
    CubeTransformState tState;
    tState.modelMatrix = &modelMatrix;
    cubeTransform(center, angle, hover, tilt, tState);

    bool pixelAccurate = currentFrame.pixelZoomMode && !tState.nonPixelAccurate;
    if (pixelAccurate != cubes[id].pixelAccurate || tState.isTilted != cubes[id].isTilted) {
        /*
          * Re-upload framebuffer if the filtering mode has changed.
          * We may need to regenerate mipmaps, and we definitely need to
          * change texture state. Avoid doing all this work on every frame.
          *
          * (Also, now that we're using separate textures for the accurate vs. non-accurate
          * case, we do need to re-upload because we'll be using a totally separate GL texture
          * name.)
          */
        framebufferChanged = true;
        cubes[id].pixelAccurate = pixelAccurate;
        cubes[id].isTilted = tState.isTilted;
    }

    drawCubeBody();
    drawCubeFace(id, framebufferChanged ? framebuffer : NULL);
}

void GLRenderer::drawMC(b2Vec2 center, float angle, const float led[3], float volume)
{
    glLoadIdentity();
    glTranslatef(center.x, center.y, 0.0f);
    glRotatef(180 + angle * (180.0f / M_PI), 0,0,1);
    glScalef(CubeConstants::SIZE, CubeConstants::SIZE, CubeConstants::SIZE);

    /*
     * Top surface, with normalmap and lightmap, and the current LED color.
     */

    glUseProgramObjectARB(mcFaceProgram);

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, mcFaceNormalsTexture);

    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, mcFaceLightTexture);

    glUniform3fv(mcFaceLEDLocation, 1, led);

    drawModel(mcFace);

    glActiveTexture(GL_TEXTURE1);
    glDisable(GL_TEXTURE_2D);

    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);

    glUseProgramObjectARB(cubeBodyProgram);
    drawModel(mcBody);

    glTranslatef(0.8f * (1.0f - volume), 0, 0);
    drawModel(mcVolume);
}

void GLRenderer::drawCubeBody()
{
    glUseProgramObjectARB(cubeBodyProgram);
    drawModel(cubeBody);
}

void GLRenderer::drawCubeFace(unsigned id, const uint16_t *framebuffer)
{
    GLCube &cube = cubes[id];

    glUseProgramObjectARB(cube.pixelAccurate ? cubeFaceProgUnfiltered : cubeFaceProgFiltered);

    /*
     * Set up samplers
     */

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, cubeFaceHilightTexture);

    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);

    if (framebuffer) {
        /*
         * Next LCD texture in the ring.
         *
         * By avoiding reuploading a texture every frame, we can keep our
         * graphics pipeline stalls down to a minimum. GPU drivers hate it
         * when we touch a resource that the GPU might still be using :)
         */

        cube.currentLcdTexture = (cube.currentLcdTexture + 1) % NUM_LCD_TEXTURES;
    }

    glBindTexture(GL_TEXTURE_2D, (cube.pixelAccurate ?
        cube.texAccurate : cube.texFiltered)[cube.currentLcdTexture]);

    if (framebuffer) {
        /*
         * Update the texture's image and anisotropy.
         */
        
        if (!cube.pixelAccurate) {
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, cube.isTilted ? 5.0f : 0.0f);
        }

        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0,
                        Cube::LCD::WIDTH, Cube::LCD::HEIGHT,
                        GL_RGB, GL_UNSIGNED_SHORT_5_6_5, framebuffer);
    }

    if (!pendingScreenshotName.empty()) {
        // Opportunistically grab this texture for a screenshot
        char suffix[32];
        snprintf(suffix, sizeof suffix, "-cube%02d.png", id);
        saveTexturePNG(pendingScreenshotName + std::string(suffix),
                       Cube::LCD::WIDTH, Cube::LCD::HEIGHT);
    }

    /*
     * Just one draw. Our shader handles the rest. Drawing the LCD and
     * face with one shader gives us a chance to do some nice filtering
     * on the edges of the LCD.
     */

    drawModel(cubeFace);

    /*
     * Clean up GL state.
     */

    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE1);
    glDisable(GL_TEXTURE_2D);
}

void GLRenderer::loadModel(const uint8_t *data, Model &model)
{
    model.data.init(data);

    glGenBuffers(1, &model.vb);
    glGenBuffers(1, &model.ib);

    glBindBuffer(GL_ARRAY_BUFFER, model.vb);
    glBufferData(GL_ARRAY_BUFFER, model.data.vertexDataSize(), model.data.vertexData(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.ib);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, model.data.indexDataSize(), model.data.indexData(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void GLRenderer::drawModel(Model &model)
{
    glBindBuffer(GL_ARRAY_BUFFER, model.vb);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, model.ib);
    glShadeModel(GL_SMOOTH);

    glInterleavedArrays(GL_N3F_V3F, 0, 0);
    glDrawElements(GL_TRIANGLES, (GLsizei) 3 * model.data.header().numTriangles, GL_UNSIGNED_SHORT, 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

GLuint GLRenderer::loadTexture(const uint8_t *pngData, GLenum wrap, GLenum filter)
{
    LodePNG::Decoder decoder;
    std::vector<unsigned char> pixels;

    // This is trusted data, no need for an explicit size limit.
    decoder.decode(pixels, pngData, 0x10000000);

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    if (filter == GL_LINEAR_MIPMAP_LINEAR) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 decoder.getWidth(),
                 decoder.getHeight(),
                 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 &pixels[0]);

    return texture;
}


void GLRenderer::saveTexturePNG(std::string name, unsigned width, unsigned height)
{
    std::vector<uint8_t> pixels(width * height * 4, 0);
    std::vector<uint8_t> png;
    LodePNG::Encoder encoder;
    
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, &pixels[0]);

    encoder.getInfoPng().color.colorType = LCT_RGB;
    encoder.encode(png, pixels, width, height);
    
    LodePNG::saveFile(png, name);
}

void GLRenderer::saveColorBufferPNG(std::string name)
{
    unsigned width = viewportWidth;
    unsigned height = viewportHeight;
    unsigned stride = width * 4;
    std::vector<uint8_t> pixels(stride * height, 0);
    std::vector<uint8_t> swappedPixels(stride * height, 0);
    std::vector<uint8_t> png;
    LodePNG::Encoder encoder;
    
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, &pixels[0]);
    
    // Swap lines, bottom-up to top-down       
    for (unsigned i = 0; i < height; i++)
        memcpy(&swappedPixels[stride * (height - i - 1)],
               &pixels[stride * i], stride);
    
    encoder.getInfoPng().color.colorType = LCT_RGB;
    encoder.encode(png, swappedPixels, width, height);
    
    LodePNG::saveFile(png, name);
}

void GLRenderer::overlayCubeFlash(unsigned id, int x, int y, int w, int h,
    const uint8_t *data, bool dataChanged)
{
    GLCube &cube = cubes[id];
    
    if (!cube.flashInitialized)
        glGenTextures(1, &cube.flashTex);

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, cube.flashTex);

    if (!cube.flashInitialized || dataChanged) {
        /*
         * Convert linear flash memory into a grid of tile images.
         */

        const unsigned tilesWide = 128;
        const unsigned tilesHigh = 256;
        const unsigned tileSize = 8;
        const unsigned pixelsWide = tilesWide * tileSize;
        const unsigned pixelsHigh = tilesHigh * tileSize;
        const unsigned pixelCount = pixelsWide * pixelsHigh;
        const unsigned byteCount = pixelCount * sizeof(uint16_t);

        STATIC_ASSERT(Cube::FlashModel::SIZE == byteCount);

        const uint16_t *src = reinterpret_cast<const uint16_t*>(data);
        static uint16_t dest[pixelCount];

        for (unsigned tileY = 0; tileY != tilesHigh; ++tileY)
            for (unsigned tileX = 0; tileX != tilesWide; ++tileX)
                for (unsigned pixelY = 0; pixelY != tileSize; ++pixelY)
                    for (unsigned pixelX = 0; pixelX != tileSize; ++pixelX) {
                        uint16_t color = *(src++);
                        color = (color >> 8) | (color << 8);
                        dest[ (tileX * tileSize) + pixelX  +
                             ((tileY * tileSize) + pixelY) * pixelsWide ] = color;
                    }

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);
        glTexImage2D(GL_TEXTURE_2D, 0, 3, pixelsWide, pixelsHigh,
                     0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5, dest);

        cube.flashInitialized = true;
    }
    
    static const float color[4] = { 1, 1, 1, 1 };
    overlayRect(x, y, w, h, color);
    glDisable(GL_TEXTURE_2D);
}

void GLRenderer::overlayAudioVisualizer(float alpha)
{
    /*
     * Draw an oscilloscope audio visualizer, using a fragment shader.
     * Our input to the shader is a texture containing audio samples.
     *
     * We use a single 2D texture, with sample index on the X axis
     * and channel number on the Y axis. Filtering is disabled.
     */

    if (alpha < 0.001)
        return;

    bool initializing = !scopeSampleTexture;
    if (initializing)
        glGenTextures(1, &scopeSampleTexture);

    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, scopeBackgroundTexture);

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, scopeSampleTexture);

    if (initializing) {
        /*
         * Allocate an empty texture
         *
         * Note that we really want linear on the X axis (sample) to avoid
         * jaggies when the scopes are large, but we ideally would want
         * NEAREST filtering on the Y axis, since that's used to pick a
         * channel. Instead we make do with bilinear, and we're careful
         * to sample exactly at the vertical texel center.
         *
         * If we fail to sample exactly at the center, we'll see "crosstalk"
         * between adjacent scope channels.
         */

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE16,
            MCAudioVisScope::SWEEP_LEN, MCAudioVisData::NUM_CHANNELS,
            0, GL_LUMINANCE, GL_UNSIGNED_SHORT, 0);
    }

    // Upload texture, one channel / row at a time
    for (unsigned channel = 0; channel < MCAudioVisData::NUM_CHANNELS; ++channel) {
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, channel,
            MCAudioVisScope::SWEEP_LEN, 1, GL_LUMINANCE, GL_UNSIGNED_SHORT,
            MCAudioVisData::instance.channels[channel].scope.getSweep());
    }

    // Make each channel's scope a square
    const unsigned height = viewportWidth / MCAudioVisData::NUM_CHANNELS;
    
    if (alpha > 0.999)
        glDisable(GL_BLEND);

    glUseProgramObjectARB(scopeProgram);
    glUniform1fARB(scopeAlphaAttr, alpha);
    overlayRect(0, viewportHeight - height, viewportWidth, height);

    glEnable(GL_BLEND);
    glActiveTexture(GL_TEXTURE1);
    glDisable(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_TEXTURE_2D);
}


