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
#include <sifteo/macros.h>
#include <sifteo/math.h>

namespace Sifteo {

/**
 * @addtogroup video
 * @{
 */

/**
 * @brief SpriteRefs refer to a single sprite on a single cube
 *
 * Typically these are transient objects which let you access one sprite's
 * state inside a VideoBuffer, but if necessary you can store SpriteRefs.
 *
 * A SpriteRef acts as an accessor for memory which is part of your
 * VideoBuffer. When you change sprite parameters using a SpriteRef, you're
 * modifying VRAM which is interpreted by the BG0_SPR_BG1 mode.
 *
 * The sprite layer exists between BG0 and BG1, and it can have up to
 * SpriteLayer::NUM_SPRITES (8) sprites on it. Sprites are Z-ordered
 * according to their ID. Their dimensions must be a power of two on
 * each axis, and their image must be a 'pinned' asset, i.e. an asset
 * where every tile is stored sequentially in memory with no deduplication.
 */
struct SpriteRef {
    _SYSAttachedVideoBuffer *sys;
    unsigned id;

    /**
     * @brief Set the sprite's image, given the physical address of the first
     * tile in a pinned asset.
     *
     * This is a lower-level method that  you typically won't use
     * directly. Does not resize the sprite.
     */
    void setImage(uint16_t tile) const {
        uint16_t word = _SYS_TILE77(tile);
        uint16_t addr = ( offsetof(_SYSVideoRAM, spr[0].tile)/2 +
                          sizeof(_SYSSpriteInfo)/2 * id );
        _SYS_vbuf_poke(&sys->vbuf, addr, word);
    }

    /**
     * @brief Set a sprite's image and size, given a
     * PinnedAssetImage and optionally a frame number.
     */
    void setImage(const PinnedAssetImage &asset, int frame = 0) const {
        setImage(asset.tile(sys->cube, vec(0,0), frame));
        resize(asset.pixelSize());
    }

    /**
     * @brief Get this sprite's current image width, in pixels.
     */
    unsigned width() const {
        uint16_t addr = ( offsetof(_SYSVideoRAM, spr[0].mask_x) +
                          sizeof(_SYSSpriteInfo) * id );
        return -(int8_t)_SYS_vbuf_peekb(&sys->vbuf, addr);
    }

    /**
     * @brief Get this sprite's current image height, in pixels.
     */
    unsigned height() const {
        uint16_t addr = ( offsetof(_SYSVideoRAM, spr[0].mask_y) +
                          sizeof(_SYSSpriteInfo) * id );
        return -(int8_t)_SYS_vbuf_peekb(&sys->vbuf, addr);
    }

    /**
     * @brief Get this sprite's current image size as a vector, in pixels.
     */
    UByte2 size() const {
        uint16_t addr = ( offsetof(_SYSVideoRAM, spr[0].mask_y)/2 +
                          sizeof(_SYSSpriteInfo)/2 * id );
        uint16_t word = _SYS_vbuf_peek(&sys->vbuf, addr);
        return vec<uint8_t>(-(int8_t)(word >> 8), -(int8_t)word);
    }

    /**
     * @brief Set this sprite's width, in pixels.
     */
    void setWidth(unsigned pixels) const {
        uint16_t addr = ( offsetof(_SYSVideoRAM, spr[0].mask_x) +
                          sizeof(_SYSSpriteInfo) * id );
        _SYS_vbuf_pokeb(&sys->vbuf, addr, -(int8_t)pixels);
    }

    /**
     * @brief Set this sprite's height, in pixels.
     */
    void setHeight(unsigned pixels) const {
        uint16_t addr = ( offsetof(_SYSVideoRAM, spr[0].mask_y) +
                          sizeof(_SYSSpriteInfo) * id );
        _SYS_vbuf_pokeb(&sys->vbuf, addr, -(int8_t)pixels);
    }

    /**
     * @brief Set this sprite's size, in pixels.
     */
    void resize(unsigned x, unsigned y) const {
        // Size must be a power of two in current firmwares.
        ASSERT((x & (x - 1)) == 0 && (y & (y - 1)) == 0);

        _SYS_vbuf_spr_resize(&sys->vbuf, id, x, y);
    }

    /**
     * @brief Set this sprite's size, in pixels, from a UInt2.
     */
    void resize(UInt2 size) const {
        resize(size.x, size.y);
    }

    /**
     * @brief Is this sprite hidden?
     *
     * Returns true if he sprite was hidden by
     * hide(). Note that, internally, sprites are hidden by giving
     * them a height of zero. Resizing a sprite, including resizing
     * via setImage, will show the sprite again.
     */
    bool isHidden() const {
        return height() == 0;
    }

    /**
     * @brief Hide a sprite.
     *
     * This effectively disables the sprite, causing the
     * cube to skip it when rendering the next frame.
     */
    void hide() const {
        setHeight(0);
    }

    /**
     * @brief Move this sprite to a new location, in pixels.
     *
     * This specifies the
     * location of the sprite's top-left corner, measured relative to the
     * top-left corner of the screen. Sprite locations may be negative.
     */
    void move(int x, int y) const {
        _SYS_vbuf_spr_move(&sys->vbuf, id, x, y);
    }

    /**
     * @brief Move this sprite to a new location, in pixels, passed as an Int2.
     *
     * This specifies the location of the sprite's top-left corner,
     * measured relative to the top-left corner of the screen.
     * Sprite locations may be negative.
     */
    void move(Int2 pos) const {
        move(pos.x, pos.y);
    }

    /**
     * @brief Move this sprite to a new location, in pixels, passed as a Float2.
     *
     * This specifies the location of the sprite's top-left corner,
     * measured relative to the top-left corner of the screen.
     * Sprite locations may be negative.
     * Coordinates are rounded to the nearest integer pixel.
     */
    void move(Float2 pos) const {
        Int2 i = pos.round();
        move(i.x, i.y);
    }

    /**
     * @brief Get this sprite's current X position, in pixels.
     */
    unsigned x() const {
        uint16_t addr = ( offsetof(_SYSVideoRAM, spr[0].pos_x) +
                          sizeof(_SYSSpriteInfo) * id );
        return -(int8_t)_SYS_vbuf_peekb(&sys->vbuf, addr);
    }

    /**
     * @brief Get this sprite's current Y position, in pixels.
     */
    unsigned y() const {
        uint16_t addr = ( offsetof(_SYSVideoRAM, spr[0].pos_y) +
                          sizeof(_SYSSpriteInfo) * id );
        return -(int8_t)_SYS_vbuf_peekb(&sys->vbuf, addr);
    }

    /**
     * @brief Get this sprite's current position as a vector, in pixels.
     */
    Byte2 position() const {
        uint16_t addr = ( offsetof(_SYSVideoRAM, spr[0].pos_y)/2 +
                          sizeof(_SYSSpriteInfo)/2 * id );
        uint16_t word = _SYS_vbuf_peek(&sys->vbuf, addr);
        return vec<int8_t>(-(int8_t)(word >> 8), -(int8_t)word);
    }

    // Next sprite ID
    SpriteRef operator++ () {
        ++id;
        return *this;
    }

    // Next sprite ID
    SpriteRef operator++ (int) {
        SpriteRef result = *this;
        ++id;
        return result;
    }

    // Previous sprite ID
    SpriteRef operator-- () {
        --id;
        return *this;
    }

    // Previous sprite ID
    SpriteRef operator-- (int) {
        SpriteRef result = *this;
        --id;
        return result;
    }

    // Calculate a new SpriteRef with an ID relative to this one
    SpriteRef operator[] (int index) {
        SpriteRef result = *this;
        result.id += index;
        return result;
    }
};


/**
 * @brief A SpriteLayer represents the VRAM attributes for the sprite rendering
 * layer in BG0_SPR_BG1 mode.
 *
 * Normally, you'll access SpriteLayer as the
 * 'sprite' member in a VideoBuffer.
 *
 * This class acts as a container for sprites. Individual sprites can
 * be accessed using the indexing operator [].
 */
struct SpriteLayer {
    _SYSAttachedVideoBuffer sys;

    static const unsigned NUM_SPRITES = _SYS_VRAM_SPRITES;

    /**
     * @brief Return a SpriteRef which references a single sprite on a single
     * VideoBuffer.
     *
     * You don't need to store this reference typically;
     * for example:
     *
     *     vbuf.sprites[2].setImage(MyAsset);
     */
    SpriteRef operator[](unsigned id) {
        ASSERT(id < NUM_SPRITES);
        SpriteRef result = { &sys, id };
        return result;
    }

    /**
     * @brief Reset all sprites to their default hidden state.
     */
    void erase() {
        _SYS_vbuf_fill(&sys.vbuf, _SYS_VA_SPR / 2, 0,
            sizeof(_SYSSpriteInfo) / 2 * NUM_SPRITES);
    }
    
    /**
     * @brief Return the VideoBuffer associated with this drawable.
     */
    _SYSVideoBuffer &videoBuffer() {
        return sys.vbuf;
    }

    /**
     * @brief Return the CubeID associated with this drawable.
     */
    CubeID cube() const {
        return sys.cube;
    }
};

/**
 * @} end addtogroup video
 */

};  // namespace Sifteo
