/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#pragma once
#ifdef NOT_USERSPACE
#   error This is a userspace-only header, not allowed by the current build.
#endif

#include <sifteo/abi.h>
#include <sifteo/cube.h>
#include <sifteo/math.h>
#include <sifteo/video/color.h>
#include <sifteo/video/sprite.h>
#include <sifteo/video/framebuffer.h>
#include <sifteo/video/bg0rom.h>
#include <sifteo/video/bg0.h>
#include <sifteo/video/bg1.h>
#include <sifteo/video/bg2.h>
#include <sifteo/video/tilebuffer.h>

namespace Sifteo {

/**
 * @defgroup video Video
 *
 * @brief Video graphics subsystem
 *
 * @{
 */

static const unsigned LCD_width = 128;   ///< Height of the LCD screen, in pixels
static const unsigned LCD_height = 128;  ///< Width of the LCD screen, in pixels
static const unsigned TILE = 8;          ///< Size of one tile, in pixels

/// Size of the LCD screen, in pixels, as a vector
static const Int2 LCD_size = { LCD_width, LCD_height };

/// Center of the LCD screen, in pixels, as a vector
static const Int2 LCD_center = { LCD_width / 2, LCD_height / 2 };


/**
 * @brief Supported video modes.
 *
 * Each "video mode" is a separate way of interpreting the VideoBuffer
 * memory in order to compose a frame of graphics.
 *
 * Most of the video modes map directly to a collection of one or more
 * drawables, which may be accessed via this VideoBuffer class.
 * Some video modes are special-purpose, like POWERDOWN.
 *
 * A cube can only be in one video mode at a time, though it's possible
 * to use setWindow() to render portions of the screen in different
 * modes. If you do this, take notice of how different modes reuse the
 * same memory for different purposes.
 */
enum VideoMode {
    POWERDOWN_MODE = _SYS_VM_POWERDOWN,   ///< Power saving mode, LCD is off
    BG0_ROM        = _SYS_VM_BG0_ROM,     ///< BG0, with tile data from internal ROM
    SOLID_MODE     = _SYS_VM_SOLID,       ///< Solid color, from colormap[0]
    FB32           = _SYS_VM_FB32,        ///< 32x32 pixel 16-color framebuffer
    FB64           = _SYS_VM_FB64,        ///< 64x64 pixel 2-color framebuffer
    FB128          = _SYS_VM_FB128,       ///< 128x48 pixel 2-color framebuffer
    BG0            = _SYS_VM_BG0,         ///< BG0 background layer
    BG0_BG1        = _SYS_VM_BG0_BG1,     ///< BG0 background plus BG1 overlay
    BG0_SPR_BG1    = _SYS_VM_BG0_SPR_BG1, ///< BG0 background, 8 sprites, BG1 overlay
    BG2            = _SYS_VM_BG2,         ///< 16x16 tiled mode with affine transform
    STAMP          = _SYS_VM_STAMP,       ///< Reconfigurable 16-color framebuffer with transparency
};


/**
 * @brief Rotation and mirroring modes, for setRotation().
 *
 * In every video mode, the hardware can perform orthogonal rotation and
 * mirroring cheaply. This happens at render-time, not continuously. For
 * example, if you're doing partial screen drawing with setWindow() and
 * you change the current rotation, the new rotation mode will affect 
 * future drawing but not past drawing.
 */
enum Rotation {
    ROT_NORMAL              = 0,
    ROT_LEFT_90_MIRROR      = _SYS_VF_XY_SWAP,
    ROT_MIRROR              = _SYS_VF_X_FLIP,
    ROT_LEFT_90             = _SYS_VF_XY_SWAP | _SYS_VF_Y_FLIP,
    ROT_180_MIRROR          = _SYS_VF_Y_FLIP,
    ROT_RIGHT_90            = _SYS_VF_XY_SWAP | _SYS_VF_X_FLIP,
    ROT_180                 = _SYS_VF_X_FLIP | _SYS_VF_Y_FLIP,
    ROT_RIGHT_90_MIRROR     = _SYS_VF_XY_SWAP | _SYS_VF_X_FLIP | _SYS_VF_Y_FLIP
};


/**
 * @brief A memory buffer which holds graphics data.
 *
 * This is a mirror of the remote graphics memory in each cube's hardware.
 * By writing to this buffer, changes are enqueued for later transmission
 * to the physical video buffer.
 *
 * Cubes have a few basic video features that are always available:
 * rotation, windowing, and selecting a video mode. This class provides
 * access to those features, as well as low-level primitives for accessing
 * the underlying video buffer memory.
 *
 * The layout and meaning of video memory changes depending on what mode
 * you're in. This class provides a union of mode-specific accessors you
 * can use.
 *
 * VideoBuffers can be used with at most one CubeID at a time, though
 * this association between cubes and video buffers can change at runtime.
 * Most games will want to have one VideoBuffer per cube, but if you plan to
 * support very large numbers of cubes, it may be necessary to have fewer
 * VideoBuffers that are shared among a larger number of cubes.
 *
 * VideoBuffers must be explicitly attached to a cube.
 * See VideoBuffer::attach() and CubeID::detachVideoBuffer().
 *
 * If you're accessing the low-level VideoBuffer memory directly, it is
 * strongly recommended that you're aware of the synchronization protocol
 * used in this buffer, to keep it consistent between the system software
 * and your application.  See abi.h for details on this protocol.
 */
struct VideoBuffer {
    /// Anonymous union of various ways to interpret the VideoBuffer memory
    union {
        _SYSAttachedVideoBuffer sys;
        SpriteLayer             sprites;    ///< Drawable for the sprite layer in BG0_SPR_BG1 mode
        Colormap                colormap;   ///< Colormap accessor, for framebuffer modes
        FB32Drawable            fb32;       ///< Drawable for the FB32 framebuffer mode
        FB64Drawable            fb64;       ///< Drawable for the FB64 framebuffer mode
        FB128Drawable           fb128;      ///< Drawable for the FB128 framebuffer mode
        BG0ROMDrawable          bg0rom;     ///< Drawable for the BG0_ROM tiled mode
        BG0Drawable             bg0;        ///< Drawable for the BG0 layer, as used in BG0, BG0_BG1, and BG0_SPR_BG1 modes
        BG1Drawable             bg1;        ///< Drawable for the BG1 layer, as used in BG0_BG1 and BG0_SPR_BG1 modes
        BG2Drawable             bg2;        ///< Drawable for the BG2 tiled mode
        StampDrawable           stamp;      ///< Drawable for the STAMP framebuffer mode
    };

    // Implicit conversions
    operator _SYSVideoBuffer* () { return &sys.vbuf; }
    operator const _SYSVideoBuffer* () const { return &sys.vbuf; }
    operator _SYSAttachedVideoBuffer* () { return &sys; }
    operator const _SYSAttachedVideoBuffer* () const { return &sys; }

    /**
     * @brief Implicit conversion to _SYSCubeID.
     *
     * This lets you pass a VideoBuffer
     * to the CubeID constructor, to easily get a CubeID instance for the
     * current cube that this buffer is attached to.
     */
    operator _SYSCubeID () const {
        return sys.cube;
    }

    /**
     * @brief Get the CubeID that this buffer is currently attached to.
     */
    CubeID cube() const {
        return sys.cube;
    }

    /**
     * @brief Change the current drawing window.
     *
     * The graphics processor generates one scanline at a time.
     * Windowed drawing allows the graphics processor to emit fewer scanlines
     * than a full screen, and to modify where on the LCD these scanlines
     * begin. In this way, you can do partial-screen rendering in the Y axis.
     *
     * This can be used for many kinds of animation effects as well as
     * effects that involve mixing different video modes.
     *
     * Windowing takes place prior to Rotation. So, if your screen is rotated
     * 90 degrees left, for example, setWindow would be operating on screen
     * columns rather than rows, and the "line" numbers would start at the
     * left edge.
     */
    void setWindow(uint8_t firstLine, uint8_t numLines) {
        poke(offsetof(_SYSVideoRAM, first_line) / 2, firstLine | (numLines << 8));
    }

    /**
     * @brief Restore the default full-screen drawing window.
     */
    void setDefaultWindow() {
        setWindow(0, LCD_height);
    }

    /**
     * @brief Like setWindow(), but change only the first line.
     */
    void setWindowFirstLine(uint8_t firstLine) {
        pokeb(offsetof(_SYSVideoRAM, first_line), firstLine);
    }

    /**
     * @brief Like setWindow(), but change only the number of lines.
     */
    void setWindowNumLines(uint8_t numLines) {
        pokeb(offsetof(_SYSVideoRAM, num_lines), numLines);
    }
    
    /**
     * @brief Retrieve the most recent 'firstLine' value, set with setWindow()
     * or setWindowFirstLine()
     */
    uint8_t windowFirstLine() const {
        return peekb(offsetof(_SYSVideoRAM, first_line));
    }

    /**
     * @brief Retrieve the most recent 'numLines' value, set with setWindow()
     * or setWindowNumLines()
     */
    uint8_t windowNumLines() const {
        return peekb(offsetof(_SYSVideoRAM, num_lines));
    }

    /**
     * @brief Set the display rotation to use in subsequent rendering.
     */
    void setRotation(Rotation r) {
        const uint8_t mask = _SYS_VF_XY_SWAP | _SYS_VF_X_FLIP | _SYS_VF_Y_FLIP;
        uint8_t flags = peekb(offsetof(_SYSVideoRAM, flags));

        // Must do this atomically; the asynchronous paint controller can
        // modify other bits within this flags byte.
        xorb(offsetof(_SYSVideoRAM, flags), (r ^ flags) & mask);
    }

    /**
     * @brief Look up the last display rotation set by setRotation().
     */
    Rotation rotation() const {
        const uint8_t mask = _SYS_VF_XY_SWAP | _SYS_VF_X_FLIP | _SYS_VF_Y_FLIP;
        uint8_t flags = peekb(offsetof(_SYSVideoRAM, flags));
        return Rotation(mask & flags);
    }

    /**
     * @brief Map the LCD rotation mask to screen orientation.  This is the side
     * which maps to the physical "top" of the screen.
     */    
    Side orientation() const {
        switch (rotation()) {
            case ROT_NORMAL:    return TOP;
            case ROT_LEFT_90:   return LEFT;
            case ROT_180:       return BOTTOM;
            case ROT_RIGHT_90:  return RIGHT;
            default:            return NO_SIDE;
        }
    }

    /**
     * @brief Set the LCD rotation such that the top of the framebuffer is at
     * the physical side 'topSide'.
     *
     * This is the counterpart to the orientation() getter.
     */
    void setOrientation(Side topSide) {
        // Tiny lookup table in a uint32
        const uint32_t sideToRotation =
            (ROT_NORMAL     << 0)  |
            (ROT_LEFT_90    << 8)  |
            (ROT_180        << 16) |
            (ROT_RIGHT_90   << 24) ;

        ASSERT(topSide >= 0 && topSide < 4);
        uint8_t r = sideToRotation >> (topSide * 8);
        setRotation(Rotation(r));
    }

    /**
     * @brief Set this VideoBuffer's cube orientation to be consistent
     * with the orientation of another "source" VideoBuffer whose cube is
     * neighbored to this one.
     *
     * This version requires the caller to supply Neighborhood instances
     * corresponding to both our own cube and the 'src' cube. This does
     * not require that the cubes are currently neighbored, just that they
     * were when the Neighborhood objects were created.
     *
     * Note that these Neighborhoods must be in physical orientation, not
     * transformed to virtual.
     */
    void orientTo(const Neighborhood &thisN, const VideoBuffer &src, const Neighborhood &srcN) {
        int srcSide = srcN.sideOf(cube());
        int dstSide = thisN.sideOf(src.cube());
        if(srcSide != NO_SIDE && dstSide != NO_SIDE) {
            setOrientation(Side(umod(2 + dstSide - srcSide + src.orientation(), NUM_SIDES)));
        }
        
    }

    /**
     * @brief Variant of orientTo() without explicitly specified Neighborhoods
     *
     * Uses the current system neighbor state for each of the two cubes.
     *
     * Note that the caller is responsible for ensuring that this cube and
     * the source cube are actually neighbored. The set of neighbored cubes
     * may change at any event dispatch point, such as System::paint().
     */
    void orientTo(const VideoBuffer &src) {
        orientTo(Neighborhood(cube()), src, Neighborhood(src.cube()));
    }

    /**
     * @brief Convert a physical side (relative to the hardware itself) to a virtual
     * side (relative to the specified screen orientation).
     */
    static Side physicalToVirtual(Side side, Side rot) {
        if (side == NO_SIDE) { return NO_SIDE; }
        ASSERT(side >= 0 && side < 4 && rot != NO_SIDE);
        return Side(umod(side - rot, NUM_SIDES));
    }

    /**
     * @brief Convert a virtual side (relative to the specified screen orientation)
     * to a physical side (relative to the hardware itself).
     */
    static Side virtualToPhysical(Side side, Side rot) {
        if (side == NO_SIDE) { return NO_SIDE; }
        ASSERT(side >= 0 && side < 4 && rot != NO_SIDE);
        return Side(umod(side + rot, NUM_SIDES));
    }

    /**
     * @brief Convert a physical side (relative to the hardware itself) to a virtual
     * side (relative to the current screen orientation).
     */
    Side physicalToVirtual(Side side) const {
        return physicalToVirtual(side, orientation());
    }

    /**
     * @brief Convert a virtual side (relative to the current screen orientation)
     * to a physical side (relative to the hardware itself).
     */
    Side virtualToPhysical(Side side) const {
        return virtualToPhysical(side, orientation());
    }

    /**
     * @brief Convert a Neighborhood from physical to virtual orientation.
     *
     * These two expressions produce the same result:
     *
     *     vbuf.physicalToVirtual(N).neighborAt(S)
     *     N.neighborAt(vbuf.physicalToVirtual(S))
     *
     * This version uses the specified cube orientation.
     */
    static Neighborhood physicalToVirtual(Neighborhood nb, Side rot) {
        Neighborhood result;
        result.sys.sides[0] = nb.sys.sides[virtualToPhysical(Side(0), rot)];
        result.sys.sides[1] = nb.sys.sides[virtualToPhysical(Side(1), rot)];
        result.sys.sides[2] = nb.sys.sides[virtualToPhysical(Side(2), rot)];
        result.sys.sides[3] = nb.sys.sides[virtualToPhysical(Side(3), rot)];
        return result;
    }

    /**
     * @brief Convert a Neighborhood from virtual to physical orientation.
     *
     * These two expressions produce the same result:
     *
     *     vbuf.virtualToPhysical(N).neighborAt(S)
     *     N.neighborAt(vbuf.virtualToPhysical(S))
     *
     * This version uses the specified cube orientation.
     */
    static Neighborhood virtualToPhysical(Neighborhood nb, Side rot) {
        Neighborhood result;
        result.sys.sides[0] = nb.sys.sides[physicalToVirtual(Side(0), rot)];
        result.sys.sides[1] = nb.sys.sides[physicalToVirtual(Side(1), rot)];
        result.sys.sides[2] = nb.sys.sides[physicalToVirtual(Side(2), rot)];
        result.sys.sides[3] = nb.sys.sides[physicalToVirtual(Side(3), rot)];
        return result;
    }

    /**
     * @brief Convert a Neighborhood from physical to virtual orientation.
     *
     * These two expressions produce the same result:
     *
     *     vbuf.physicalToVirtual(N).neighborAt(S)
     *     N.neighborAt(vbuf.physicalToVirtual(S))
     *
     * Uses the current orientation of this VideoBuffer.
     */
    Neighborhood physicalToVirtual(Neighborhood nb) const {
        return physicalToVirtual(nb, orientation());
    }

    /**
     * @brief Convert a Neighborhood from virtual to physical orientation.
     *
     * These two expressions produce the same result:
     *
     *     vbuf.virtualToPhysical(N).neighborAt(S)
     *     N.neighborAt(vbuf.virtualToPhysical(S))
     *
     * Uses the current orientation of this VideoBuffer.
     */
    Neighborhood virtualToPhysical(Neighborhood nb) const {
        return virtualToPhysical(nb, orientation());
    }

    /**
     * @brief Return the current physical neighbors for the cube attached to
     * this VideoBuffer.
     *
     * This is equivalent to creating a new Neighborhood
     * instance from cube(), but it's provided by analogy with
     * virtualNeighbors().
     */
    Neighborhood physicalNeighbors() const {
        return Neighborhood(cube());
    }

    /**
     * @brief Return the current virtual neighbors for the cube attached to
     * this VideoBuffer.
     *
     * This is equivalent to creating a new Neighborhood
     * instance from cube(), and transforming it with physicalToVirtual().
     */
    Neighborhood virtualNeighbors() const {
        return physicalToVirtual(Neighborhood(cube()));
    }

    /**
     * @brief Return the physical accelerometer reading for this cube.
     *
     * The resulting vector is oriented with respect to the cube hardware.
     *
     * This is equivalent to calling accel() on the cube() object.
     */
    Byte3 physicalAccel() const {
        return cube().accel();
    }

    /**
     * @brief Return the virtual accelerometer reading for this cube.
     *
     * The resulting vector is oriented with respect to the current
     * LCD rotation.
     */
    Byte3 virtualAccel() const {
        return cube().accel().zRotateI(orientation());
    }

    /**
     * @brief Change the video mode.
     *
     8 This affects subsequent rendering only.
     * Note that this may change the way hardware interprets the contents
     * of video memory, so it's important to synchronize any mode changes
     * such that you know the hardware has finished rendering with the old
     * mode before you start rendering with the new mode.
     */
    void setMode(VideoMode m) {
        pokeb(offsetof(_SYSVideoRAM, mode), m);
    }

    /**
     * @brief Retrieve the last video mode set by setMode()
     */
    VideoMode mode() const {
        return VideoMode(peekb(offsetof(_SYSVideoRAM, mode)));
    }
    
    /**
     * @brief Zero all mode-specific video memory.
     *
     * This is typically part of initMode(), but it may be necessary to do
     * this at other times. For example, when doing a multi-mode scene,
     * you may need to ensure that unused portions of VRAM are in a known blank state.
     */
    void erase() {
        _SYS_vbuf_fill(*this, 0, 0, _SYS_VA_FIRST_LINE / 2);
    }

    /**
     * @brief Initialize the video buffer and change modes.
     *
     * This is a shorthand for calling System::finish() to finish existing
     * rendering before changing the mode, actually changing the mode,
     * restoring the default window and rotation, and finally zero'ing
     * all mode-specific video memory.
     *
     * Most programs should use initMode() to set up the initial mode for
     * a VideoBuffer, prior to attaching it to a cube. This is because
     * the process of attaching a video buffer causes the system to assume
     * all video memory is 'dirty', and re-send it to the cube. It is important
     * to have video memory in the state you want it to be in before this
     * happens.
     *
     * You can optionally specify a non-default window size. This
     * is helpful, since often when doing multi-mode rendering you
     * need to change modes and change windows at the same time.
     */
    void initMode(VideoMode m, unsigned firstLine = 0, unsigned numLines = LCD_height) {
        _SYS_finish();
        erase();
        setWindow(firstLine, numLines);
        setRotation(ROT_NORMAL);
        setMode(m);
    }

    /**
     * @brief Attach this VideoBuffer to a cube.
     *
     * When this cube is enabled and connected, the system will asynchronously
     * stream video data from this VideoBuffer to that cube.
     *
     * This call automatically reinitializes the change bitmap in this
     * buffer, so that we'll resend its contents to the cube on next paint.
     *
     * If this VideoBuffer was previously attached to a different cube,
     * you must manually attach() a different video buffer to the old
     * cube first, or call CubeID::detachVideoBuffer() on the old cube.
     * A VideoBuffer must never be attached to multiple cubes at once.
     *
     * In general, a VideoBuffer should be attach()'ed prior to performing
     * any tile rendering. This is because the relocated tile addresses of
     * your assets may be different for each cube, so we need to know the
     * attached cube's ID at draw time.
     *
     * Waits for all cubes to finish rendering before (re)attaching this buffer.
     */
    void attach(_SYSCubeID id) {
        _SYS_finish();
        sys.cube = id;
        _SYS_vbuf_init(*this);
        _SYS_setVideoBuffer(*this, *this);
    }

    /**
     * @brief Prepare to modify a particular address
     *
     * Addresses must be locked prior to the buffer memory being modified.
     * Multiple calls to lock may be made in a row, without an intervening
     * unlock().
     *
     * Typically this operation is performed automatically as part of any
     * other write operation, like a poke().
     */
    void lock(uint16_t addr) {
        _SYS_vbuf_lock(*this, addr);
    }

    /**
     * @brief End a sequence of modifications to VRAM
     *
     * If the system is not already busy flushing updates to the cube,
     * this allows it to begin. This operation is performed implicitly
     * during a System::paint().
     */
    void unlock() {
        _SYS_vbuf_unlock(*this);
    }

    /**
     * @brief Mark the VideoBuffer as having changed, without actually modifying
     * any memory.
     *
     * This will force the next System::paint() to actually
     * redraw this cube, even if it seems like nothing has changed.
     */
    void touch() {
        lock(_SYS_VA_FLAGS);
    }

    /**
     * @brief Modify a word of VRAM, automatically locking it and marking the
     * change only if that word has actually been modified.
     *
     * After a sequence of poke() calls, the caller is responsible for issuing
     * one unlock(). This happens automatically at the next System::paint().
     */
    void poke(uint16_t addr, uint16_t word) {
        _SYS_vbuf_poke(*this, addr, word);
    }

    /**
     * @brief Like poke(), but modifies a single byte. Less efficient, but
     * sometimes you really do just want to modify one byte.
     */
    void pokeb(uint16_t addr, uint8_t byte) {
        _SYS_vbuf_pokeb(*this, addr, byte);
    }

    /**
     * @brief Poke a 14-bit tile index into a particular VRAM word.
     */
    void pokei(uint16_t addr, uint16_t index) {
        _SYS_vbuf_poke(*this, addr, _SYS_TILE77(index));
    }

    /**
     * @brief Like pokeb(), but atomically XORs a value with the byte.
     * This is a no-op if and only if byte==0.
     */
    void xorb(uint16_t addr, uint8_t byte) {
        _SYS_vbuf_xorb(*this, addr, byte);
    }

    /**
     * @brief Read one word of VRAM
     */
    uint16_t peek(uint16_t addr) const {
        return _SYS_vbuf_peek(*this, addr);
    }

    /**
     * @brief Read one byte of VRAM
     */
    uint8_t peekb(uint16_t addr) const {
        return _SYS_vbuf_peekb(*this, addr);
    }
};

/**
 * @} endgroup video
*/

};  // namespace Sifteo
