/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_VIDEO_H
#define _SIFTEO_VIDEO_H

#ifdef NO_USERSPACE_HEADERS
#   error This is a userspace-only header, not allowed by the current build.
#endif

#include <sifteo/abi.h>
#include <sifteo/drawable.h>

namespace Sifteo {

static const unsigned LCD_width = 128;   /// Height of the LCD screen, in pixels
static const unsigned LCD_height = 128;  /// Width of the LCD screen, in pixels
static const unsigned TILE = 8;          /// Size of one tile, in pixels


/**
 * A memory buffer which holds graphics data. This is a mirror of the
 * remote graphics memory in each cube's hardware. By writing to this
 * buffer, changes are enqueued for later transmission to the physical
 * video buffer.
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
    union {
        _SYSAttachedVideoBuffer sys;
    };

    /**
     * Supported video modes. Each "video mode" is a separate way of
     * interpreting the VideoBuffer memory in order to compose a frame
     * of graphics.
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
    enum Mode {
        Powerdown       = _SYS_VM_POWERDOWN,   // Power saving mode, LCD is off
        BG0_ROM         = _SYS_VM_BG0_ROM,     // BG0, with tile data from internal ROM
        Solid           = _SYS_VM_SOLID,       // Solid color, from colormap[0]
        FB32            = _SYS_VM_FB32,        // 32x32 pixel 16-color framebuffer
        FB64            = _SYS_VM_FB64,        // 64x64 pixel 2-color framebuffer
        FB128           = _SYS_VM_FB128,       // 128x48 pixel 2-color framebuffer
        BG0             = _SYS_VM_BG0,         // BG0 background layer
        BG0_BG1         = _SYS_VM_BG0_BG1,     // BG0 background plus BG1 overlay
        BG0_SPR_BG1     = _SYS_VM_BG0_SPR_BG1, // BG0 background, 8 sprites, BG1 overlay
        BG2             = _SYS_VM_BG2,         // 16x16 tiled mode with affine transform
    };
    
    /**
     * Rotation and mirroring modes, for setRotation().
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
        ROT_LEFT_90             = _SYS_VF_XY_SWAP | _SYS_VF_X_FLIP,
        ROT_180_MIRROR          = _SYS_VF_Y_FLIP,
        ROT_RIGHT_90            = _SYS_VF_XY_SWAP | _SYS_VF_Y_FLIP,
        ROT_180                 = _SYS_VF_X_FLIP | _SYS_VF_Y_FLIP,
        ROT_RIGHT_90_MIRROR     = _SYS_VF_XY_SWAP | _SYS_VF_X_FLIP | _SYS_VF_Y_FLIP
    };

    /**
     * Implicit conversion to _SYSVideoBuffer*,
     * for use in low-level system calls.
     */
    operator _SYSVideoBuffer* () {
        return &sys.vbuf;
    }

    /**
     * Constant implicit conversion to _SYSVideoBuffer*,
     * for use in low-level system calls.
     */
    operator const _SYSVideoBuffer* () const {
        return &sys.vbuf;
    }

    /**
     * Implicit conversion to _SYSCubeID. This lets you pass a VideoBuffer
     * to the CubeID constructor, to easily get a CubeID instance for the
     * current cube that this buffer is attached to.
     */
    operator _SYSCubeID () const {
        return sys.cube;
    }

    /**
     * Change the current drawing window.
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
     * Like setWindow(), but change only the first line.
     */
    void setWindowFirstLine(uint8_t firstLine) {
        pokeb(offsetof(_SYSVideoRAM, first_line), firstLine);
    }

    /**
     * Like setWindow(), but change only the number of lines.
     */
    void setWindowNumLines(uint8_t numLines) {
        pokeb(offsetof(_SYSVideoRAM, num_lines), numLines);
    }
    
    /**
     * Retrieve the most recent 'firstLine' value, set with setWindow()
     * or setWindowFirstLine()
     */
    uint8_t getWindowFirstLine() const {
        return peekb(offsetof(_SYSVideoRAM, first_line));
    }

    /**
     * Retrieve the most recent 'numLines' value, set with setWindow()
     * or setWindowNumLines()
     */
    uint8_t getWindowNumLines() const {
        return peekb(offsetof(_SYSVideoRAM, num_lines));
    }

    /**
     * Set the display rotation to use in subsequent rendering.
     */
    void setRotation(Rotation r) {
        const uint8_t mask = _SYS_VF_XY_SWAP | _SYS_VF_X_FLIP | _SYS_VF_Y_FLIP;
        uint8_t flags = peekb(offsetof(_SYSVideoRAM, flags));
        flags &= ~mask;
        flags |= r & mask;
        pokeb(offsetof(_SYSVideoRAM, flags), flags);
    }

    /**
     * Look up the last display rotation set by setRotation().
     */
    Rotation getRotation() const {
        const uint8_t mask = _SYS_VF_XY_SWAP | _SYS_VF_X_FLIP | _SYS_VF_Y_FLIP;
        uint8_t flags = peekb(offsetof(_SYSVideoRAM, flags));
        return Rotation(mask & flags);
    }

    /**
     * Change the video mode. This affects subsequent rendering only.
     * Note that this may change the way hardware interprets the contents
     * of video memory, so it's important to synchronize any mode changes
     * such that you know the hardware has finished rendering with the old
     * mode before you start rendering with the new mode.
     */
    void setMode(Mode m) {
        pokeb(offsetof(_SYSVideoRAM, mode), m);
    }

    /**
     * Retrieve the last video mode set by setMode()
     */
    Mode getMode() const {
        return Mode(peekb(offsetof(_SYSVideoRAM, mode)));
    }

    /**
     * Attach this VideoBuffer to a cube. When this cube
     * is enabled and connected, the system will asynchronously
     * stream video data from this VideoBuffer to that cube.
     *
     * This call automatically reinitializes the change bitmap in this
     * buffer, so that we'll resend its contents to the ube on next paint.
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
     */
    void attach(_SYSCubeID id) {
        sys.cube = id;
        _SYS_vbuf_init(*this);
        _SYS_setVideoBuffer(*this, *this);
    }

    /**
     * Prepare to modify a particular address. Addresses must be
     * locked prior to the buffer memory being modified. Multiple
     * calls to lock may be made in a row, without an intervening
     * unlock().
     */
    void lock(uint16_t addr) {
        _SYS_vbuf_lock(*this, addr);
    }

    /**
     * End a sequence of modifications to VRAM. If the system is not
     * already busy flushing updates to the cube, this allows it to begin.
     */
    void unlock() {
        _SYS_vbuf_unlock(*this);
    }

    /**
     * Mark the VideoBuffer as having changed, without actually modifying
     * any memory. This will force the next System::paint() to actually
     * redraw this cube, even if it seems like nothing has changed.
     */
    void touch() {
        sys.vbuf.needPaint = (uint32_t)-1;
    }

    /**
     * Modify a word of VRAM, automatically locking it and marking the
     * change only if that word has actually been modified. After a
     * sequence of poke() calls, the caller is responsible for issuing
     * one unlock().
     */
    void poke(uint16_t addr, uint16_t word) {
        _SYS_vbuf_poke(*this, addr, word);
    }

    /**
     * Like poke(), but modifies a single byte. Less efficient, but
     * sometimes you really do just want to modify one byte.
     */
    void pokeb(uint16_t addr, uint8_t byte) {
        _SYS_vbuf_pokeb(*this, addr, byte);
    }

    /**
     * Poke a 14-bit tile index into a particular VRAM word.
     */
    void pokei(uint16_t addr, uint16_t index) {
        _SYS_vbuf_poke(*this, addr, _SYS_TILE77(index));
    }

    /**
     * Read one word of VRAM
     */
    uint16_t peek(uint16_t addr) const {
        return _SYS_vbuf_peek(*this, addr);
    }

    /**
     * Read one byte of VRAM
     */
    uint8_t peekb(uint16_t addr) const {
        return _SYS_vbuf_peekb(*this, addr);
    }
};


};  // namespace Sifteo

#endif
