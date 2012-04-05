/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef _SIFTEO_ASSET_H
#define _SIFTEO_ASSET_H

#ifdef NO_USERSPACE_HEADERS
#   error This is a userspace-only header, not allowed by the current build.
#endif

#include <sifteo/abi.h>
#include <sifteo/limits.h>      // For CUBE_ALLOCATION
#include <sifteo/math.h>        // For vector types
#include <sifteo/macros.h>      // For ASSERT

namespace Sifteo {


/**
 * An asset group. At build time, STIR creates a statically
 * initialized instance of the AssetGroup class for every asset group
 * in the game.
 *
 * At runtime, AssetGroup objects track the load state of a particular
 * group across all cubes. AssetGroups must be loaded at runtime using
 * an AssetLoader object.
 */

struct AssetGroup {
    _SYSAssetGroup sys;
    _SYSAssetGroupCube cubes[CUBE_ALLOCATION];

    /// Implicit conversion to system object
    operator const _SYSAssetGroup& () const { return sys; }
    operator _SYSAssetGroup& () { return sys; }
    operator const _SYSAssetGroup* () const { return &sys; }
    operator _SYSAssetGroup* () { return &sys; }

    /**
     * Get a pointer to the read-only system data for this asset group.
     * The resulting value is constant at compile-time.
     */
    const _SYSAssetGroupHeader *sysHeader() const {
        // AssetGroups are typically in RAM, but we want the static
        // initializer data so that our return value is known at compile time.
        _SYSAssetGroup *G = (_SYSAssetGroup*)
            _SYS_lti_initializer(reinterpret_cast<const void*>(&sys));
        return reinterpret_cast<const _SYSAssetGroupHeader*>(G->pHdr);
    }

    /**
     * Get the size of this asset group, in tiles.
     */
    unsigned numTiles() const {
        return sysHeader()->numTiles;
    }

    /**
     * Get the compressed size of this asset group, in bytes
     */
    unsigned compressedSize() const {
        return sysHeader()->dataSize;
    }

    /**
     * Return the base address of this asset group, as loaded onto the
     * specified cube. The result is only valid if the asset group is
     * already installed on that cube.
     *
     * It is an error to rely on the value of baseAddress() before
     * calling AssetGroup::isInstalled(), AssetSlot::bootstrap(),
     * or using AssetLoader to load the group.
     */
    uint16_t baseAddress(_SYSCubeID cube) const {
        ASSERT(cube < CUBE_ALLOCATION);
        return cubes[cube].baseAddr;
    }

    /**
     * Is this AssetGroup installed on all cubes in the given vector?
     * Cube vectors are sets of CubeID::bit() values bitwise-OR'ed together.
     *
     * Assets can be installed either explicitly using an AssetLoader, or
     * they may be already marked as installed on game entry due to
     * asset bootstrapping, or due to a cached AssetSlot from a previous
     * invocation of the game.
     *
     * If a game doesn't explicitly install an AssetGroup, it must at least
     * call isInstalled() to check whether the group is already installed
     * in a cached AssetSlot. Because of this, we might update the asset's
     * cached base address on success.
     */
    bool isInstalled(_SYSCubeIDVector vec) {
        return _SYS_asset_findInCache(*this);
    }

    /**
     * Is this AssetGroup installed on a particular cube?
     *
     * If 'true', we may update the asset's cached base address.
     */
    bool isInstalled(_SYSCubeID cube) {
        return isInstalled(_SYSCubeIDVector(0x80000000 >> cube));
    }
};


/**
 * An asset slot. Slots are numbered containers, in a cube's flash
 * memory, which can hold AssetGroups.
 *
 * Slots are allocated statically. Any given game has a fixed number of
 * slots that it requires. At launch or at cube connect-time, those slots
 * are mapped to physical regions of memory on each cube, by the system.
 *
 * Assets cannot be freely replaced within a slot. You can append a new
 * AssetGroup to a particular slot, and you can erase a slot. Individual
 * groups cannot be removed without erasing the whole slot.
 *
 * Assets within an AssetSlot are cached between invocations of the game.
 * This means that a "new" AssetSlot may start out with not all tiles free,
 * and any assets already resident in that slot will be marked as installed.
 *
 * To define a new AssetSlot, use AssetSlot::allocate().
 *
 * To ensure that AssetSlots used in metadata are constant at compile-time,
 * the preferred allocation pattern is to declare a global variable such as:
 *
 *    AssetSlot MySlot = AssetSlot::allocate()
 *          .bootstrap(Group1)
 *          .bootstrap(Group2);
 */

struct AssetSlot {
    _SYSAssetSlot sys;
    AssetSlot(_SYSAssetSlot sys) : sys(sys) {}

    /// Implicit conversion to system object
    operator const _SYSAssetSlot& () const { return sys; }
    operator _SYSAssetSlot& () { return sys; }
    operator const _SYSAssetSlot* () const { return &sys; }
    operator _SYSAssetSlot* () { return &sys; }

    /**
     * Create a new AssetSlot. This function returns
     * a unique ID which is constant at link-time.
     */
    static AssetSlot allocate() {
        return AssetSlot(_SYS_lti_counter("Sifteo.AssetGroupSlot", 0));
    }

    /**
     * How much space is remaining in this slot, measured in tiles?
     */
    unsigned tilesFree() const {
        return _SYS_asset_slotTilesFree(*this);
    }

    /**
     * Is there room in this slot to load a particular AssetGroup
     * without erasing the slot? This does not check whether the
     * specified group has already been installed.
     */
    bool hasRoomFor(const AssetGroup &group) const {
        return tilesFree() >= group.numTiles();
    }

    /**
     * Erase this slot. Any existing AssetGroups loaded into this
     * slot will no longer be loaded, and the slot will be marked
     * as fully empty.
     *
     * After this call, any AssetGroups that were installed here
     * will now return 'false' from isInstalled(). Any tiles rendered
     * from this AssetGroup will now have undefined contents on-screen.
     */
    void erase() const {
        _SYS_asset_slotErase(*this);
    }

    /**
     * Mark a particular AssetGroup as a "bootstrap" asset for this slot.
     * The loader will ensure that this AssetGroup is installed before the
     * game starts.
     *
     * This function must be called before the AssetGroup is used for
     * rendering. This is where we update the asset's base address from
     * the system cache.
     *
     * Returns *this, so that bootstrap() can be chained, especially off
     * of allocate().
     */
    AssetSlot bootstrap(AssetGroup &group) const {
        // _SYSMetadataBootAsset
        _SYS_lti_metadata(_SYS_METADATA_BOOT_ASSET, "IBBBB",
            group.sysHeader(), sys, 0, 0, 0);

        // Update base address from the cache. Make sure it was successful.
        bool success = _SYS_asset_findInCache(group);
        ASSERT(success);

        return *this;
    }
};


/**
 * An AssetLoader object is responsible for installing an AssetGroup into
 * an AssetSlot on one or more cubes. The AssetLoader is a transient object;
 * it can exist only when an actual asset loading operation is taking
 * place, and be deleted or recycled afterwards. Your game can allocate
 * AssetLoaders on the stack, or as part of a union.
 *
 * Assets can be installed onto any number of cubes concurrently, and each
 * cube can be loading a different AssetGroup.
 */

struct AssetLoader {
    _SYSAssetLoader sys;
    _SYSAssetLoaderCube cubes[CUBE_ALLOCATION];

    /// Implicit conversion to system object
    operator const _SYSAssetLoader& () const { return sys; }
    operator _SYSAssetLoader& () { return sys; }
    operator const _SYSAssetLoader* () const { return &sys; }
    operator _SYSAssetLoader* () { return &sys; }

    /**
     * Initialize this object. This is made explicit, rather than using
     * a constructor, so that games can keep AssetLoader in a union if
     * necessary to save memory.
     *
     * Must be called once, prior to any calls to start().
     */
    void init() {
        sys.cubeVec = 0;
    }

    /**
     * Ensure that the system is no longer using this AssetLoader object.
     * If all asset downloads complete successfully, there is no need to
     * call finish(), but doing so is harmless.
     *
     * If any asset downloads are still in progress, this cancels them.
     * A partially downloaded asset group will use up memory in its
     * AssetSlot, but the asset group will not be marked as 'installed'.
     */
    void finish() {
        _SYS_asset_loadFinish(*this);
    }

    /**
     * Start installing group "group" into slot "slot" on some set of cubes,
     * identified by the "cubes" bitvector.
     * 
     * See the single-cube version of start() for a detailed explanation.
     * This function is identical, except that the asset is broadcast to
     * multiple cubes simultaneously.
     *
     * We return 'true' if the asset download started and/or we found a cached
     * asset for every one of the specified cubes. If one or more of the
     * specified cubes has no room in "slot", we return false.     
     */
    bool start(AssetGroup &group, AssetSlot slot, _SYSCubeIDVector cubes) {
        if (!_SYS_asset_loadStart(*this, group, slot, cubes))
            return false;

        // Make sure the download actually started. If the system detected
        // something was wrong, the cubeVec bit will not be set.
        ASSERT((sys.cubeVec & cubes) == cubes);
    }

    /**
     * Start intalling group "group" into slot "slot" on a single cube.
     *
     * The asset group may already be cached in "slot" by the system. If
     * so, this function returns 'true' and the asset will immediately be
     * marked as "installed". Note that, even though no installation actually
     * takes place over-the-air in this case, games must still call start()
     * in order to establish a base address for the AssetGroup, and the
     * AssetLoader will still be necessary in the event that the group was
     * not cached.
     *
     * If there is room to install this AssetGroup in "slot", we return 'true'
     * and start asynchronously installing the asset. This AssetLoader object
     * must remain in existence until the download is complete. Progress can
     * be monitored with this object's other methods.
     *
     * If there is no remaining space in AssetSlot, this function returns
     * 'false' immediately, without starting an asset download. The game must
     * decide how to proceed at this point. Depending on the game's asset
     * management strategy, this may indicate a logic error, or it may be
     * a cue that the AssetSlot needs to be erased.
     */
    bool start(AssetGroup &group, AssetSlot slot, _SYSCubeID cube) {
        return start(group, slot, _SYSCubeIDVector(0x80000000 >> cube));
    }

    /**
     * Which AssetGroup is being installed on the given cube during
     * this load session?
     *
     * Requires that start() was called at some point on this AssetLoader
     * and with this 'cube'.
     */
    AssetGroup &group(_SYSCubeID cubeID) const {
        ASSERT((0x80000000 >> cubeID) & sys.cubeVec);
        const _SYSAssetLoaderCube &cube = cubes[cubeID];
        return *reinterpret_cast<AssetGroup*>(cube.pAssetGroup);
    }

    /**
     * How much progress has been made on the indicated cube's asset install?
     * Returns the number of bytes of compressed data that have been sent.
     *
     * Note that this measures the number of compressed bytes sent to the cube.
     * There will be some lag between when the bytes are sent, and when they
     * are actually acknowledged as written. Also, the progress may be
     * a bit nonlinear due to the difference between compressed and
     * decompressed size.
     */
    unsigned progressBytes(_SYSCubeID cubeID) const {
        ASSERT((0x80000000 >> cubeID) & sys.cubeVec);
        const _SYSAssetLoaderCube &cube = cubes[cubeID];
        return cube.progress;
    }

    /**
     * How much progress has been made on the indicated cube's asset install?
     * Returns a value between 0 and 'max'. The default value of 100 for
     * 'max' will have us return percent completion.
     *
     * Note that this measures the number of compressed bytes sent to the cube.
     * There will be some lag between when the bytes are sent, and when they
     * are actually acknowledged as written. Also, the progress may be
     * a bit nonlinear due to the difference between compressed and
     * decompressed size.
     *
     * Callers should not interpret progress() == 100 as an indication that
     * the asset group is complete and ready to use. Callers must use
     * AssetGroup::isInstalled() or AssetLoader::isComplete() to determine
     * this.
     */
    unsigned progress(_SYSCubeID cubeID, unsigned max = 100) const {
        return progressBytes(cubeID) * max / group(cubeID).compressedSize();
    }

    /**
     * Is the asset install finished for all cubes in the specified vector?
     * This only returns 'true' when the install is completely done on all
     * of these cubes, and the AssetGroup is fully available for use in drawing.
     *
     * Note that this is more efficient than calling AssetGroup::isInstalled.
     * Whereas isInstalled() needs to look for the AssetGroup in the system's
     * cache, this function is simply checking whether the current installation
     * session has completed.
     */
    bool isComplete(_SYSCubeIDVector vec) const {
        ASSERT((sys.cubeVec & vec) == vec);
        return (sys.complete & vec) == vec;
    }
    
    /**
     * Is the asset install for "cubeID" finished? This will return true
     * only when the install is completely done, and the AssetGroup is fully
     * available for use in drawing.
     */
    bool isComplete(_SYSCubeID cubeID) const {
        return isComplete(_SYSCubeIDVector(0x80000000 >> cubeID));
    }

    /**
     * Are all asset installations from this asset loading session complete?
     */
    bool isComplete() const {
        return isComplete(sys.cubeVec);
    }
};


/**
 * Any kind of asset image, as defined in your STIR script.
 *
 * AssetImage acts as the base class for all tiled assets. It does not
 * imply any particular data representation: an AssetImage may be pinned,
 * flat, or compressed.
 *
 * We don't use actual C++ subclassing here, since we need to be able to
 * statically initialize all kinds of AssetImages. So, each subclass is POD,
 * and we supply implicit conversions from those classes back to AssetImage.
 *
 * STIR will generate all assets as AssetImage instances unless they
 * were specifically marked as pinned or flat assets.
 *
 * In a plain AssetImage, it's possible to access the asset's geometry
 * and to draw it with system support. The underlying data, however, should
 * be treated as opaque when working with plain AssetImages.
 */

struct AssetImage {
    _SYSAssetImage sys;

    // Common accessors for all AssetImage flavors
    AssetGroup &assetGroup() const { return *reinterpret_cast<AssetGroup*>(sys.pAssetGroup); }
    int tileWidth() const { return sys.width; }
    int tileHeight() const { return sys.height; }
    Int2 tileSize() const { return Vec2<int>(sys.width, sys.height); }
    int pixelWidth() const { return sys.width << 3; }
    int pixelHeight() const { return sys.height << 3; }
    Int2 pixelSize() const { return Vec2<int>(sys.width << 3, sys.height << 3); }
    int numFrames() const { return sys.frames; }
    int numTilesPerFrame() const { return tileWidth() * tileHeight(); }
    int numTiles() const { return numFrames() * numTilesPerFrame(); }
    
    /// Implicit conversion to system object
    operator const _SYSAssetImage& () const { return sys; }
    operator _SYSAssetImage& () { return sys; }
    operator const _SYSAssetImage* () const { return &sys; }
    operator _SYSAssetImage* () { return &sys; }
};


/**
 * An asset image in which all tiles are stored sequentially in memory.
 * This doesn't store any separate tilemap information, just the index for
 * the first tile in the sequence.
 *
 * Generate a PinnedAssetImage by passing the "pinned=1" option to image{}
 * in your STIR script.
 *
 * Pinned assets are required for sprites, since the hardware requires all
 * tiles in a sprite to be sequential.
 *
 * Pinned assets may also be efficient to use in other cases when there's
 * very little potential for tile deduplication.
 */
 
struct PinnedAssetImage {
    _SYSAssetImage sys;

    // Common accessors for all AssetImage flavors
    AssetGroup &assetGroup() const { return *reinterpret_cast<AssetGroup*>(sys.pAssetGroup); }
    int tileWidth() const { return sys.width; }
    int tileHeight() const { return sys.height; }
    Int2 tileSize() const { return Vec2<int>(sys.width, sys.height); }
    int pixelWidth() const { return sys.width << 3; }
    int pixelHeight() const { return sys.height << 3; }
    Int2 pixelSize() const { return Vec2<int>(sys.width << 3, sys.height << 3); }
    int numFrames() const { return sys.frames; }
    int numTilesPerFrame() const { return tileWidth() * tileHeight(); }
    int numTiles() const { return numFrames() * numTilesPerFrame(); }

    /// Implicit conversion to AssetImage base class
    operator const AssetImage& () const { return *reinterpret_cast<const AssetImage*>(this); }
    operator AssetImage& () { return *reinterpret_cast<AssetImage*>(this); }

    /**
     * Returns the index of the tile at linear position 'i' in the image.
     *
     * The returned index is unrelocated; it is relative to the base address
     * of the image's AssetGroup.
     */
    uint16_t tile(unsigned i) const {
        ASSERT(i < numTiles());
        return sys.data + i;
    };

    /**
     * Return the index of the tile at the specified (x, y) tile coordinates,
     * and optionally on the specified frame number.
     *
     * The returned index is unrelocated; it is relative to the base address
     * of the image's AssetGroup.
     */
    uint16_t tile(Int2 pos, unsigned frame = 0) const {
        ASSERT(pos.x < tileWidth() && pos.y < tileHeight() && frame < numFrames());
        return sys.data + pos.x + pos.y * tileHeight() + frame * numTilesPerFrame();
    }

    /**
     * Returns the index of the tile at linear position 'i' in the image.
     *
     * The returned index is relocated to an absolute address for the
     * specified cube. This image's assets must be installed on that cube.
     */
    uint16_t tile(_SYSCubeID cube, unsigned i) const {
        ASSERT(i < numTiles());
        return assetGroup().baseAddress(cube) + sys.data + i;
    };

    /**
     * Return the index of the tile at the specified (x, y) tile coordinates,
     * and optionally on the specified frame number.
     *
     * The returned index is relocated to an absolute address for the
     * specified cube. This image's assets must be installed on that cube.
     */
    uint16_t tile(_SYSCubeID cube, Int2 pos, unsigned frame = 0) const {
        ASSERT(pos.x < tileWidth() && pos.y < tileHeight() && frame < numFrames());
        return assetGroup().baseAddress(cube) + sys.data
            + pos.x + pos.y * tileHeight() + frame * numTilesPerFrame();
    }
};


/**
 * An asset image in which all tile indices are stored in a flat array,
 * without any additional compression. Flat assets are usually less efficient
 * than compressed AssetImages, but they allow you to have cheap random
 * access to any tile in the image.
 *
 * Generate a FlatAssetImage by passing the "flat=1" option to image{}
 * in your STIR script.
 */
 
struct FlatAssetImage {
    _SYSAssetImage sys;

    // Common accessors for all AssetImage flavors
    AssetGroup &assetGroup() const { return *reinterpret_cast<AssetGroup*>(sys.pAssetGroup); }
    int tileWidth() const { return sys.width; }
    int tileHeight() const { return sys.height; }
    Int2 tileSize() const { return Vec2<int>(sys.width, sys.height); }
    int pixelWidth() const { return sys.width << 3; }
    int pixelHeight() const { return sys.height << 3; }
    Int2 pixelSize() const { return Vec2<int>(sys.width << 3, sys.height << 3); }
    int numFrames() const { return sys.frames; }
    int numTilesPerFrame() const { return tileWidth() * tileHeight(); }
    int numTiles() const { return numFrames() * numTilesPerFrame(); }

    /// Implicit conversion to AssetImage base class
    operator const AssetImage& () const { return *reinterpret_cast<const AssetImage*>(this); }
    operator AssetImage& () { return *reinterpret_cast<AssetImage*>(this); }

    /**
     * Flat asset images represent their tile data as an array of uint16_t
     * un-relocated tile indices. This returns a pointer to that array.
     */
    const uint16_t *tileArray() const {
        return reinterpret_cast<const uint16_t *>(sys.data);
    }

    /**
     * Returns the index of the tile at linear position 'i' in the image.
     *
     * The returned index is unrelocated; it is relative to the base address
     * of the image's AssetGroup.
     */
    uint16_t tile(unsigned i) const {
        ASSERT(i < numTiles());
        return tileArray()[i];
    };

    /**
     * Return the index of the tile at the specified (x, y) tile coordinates,
     * and optionally on the specified frame number.
     *
     * The returned index is unrelocated; it is relative to the base address
     * of the image's AssetGroup.
     */
    uint16_t tile(Int2 pos, unsigned frame = 0) const {
        ASSERT(pos.x < tileWidth() && pos.y < tileHeight() && frame < numFrames());
        return tileArray()[pos.x + pos.y * tileHeight() + frame * numTilesPerFrame()];
    }

    /**
     * Returns the index of the tile at linear position 'i' in the image.
     *
     * The returned index is relocated to an absolute address for the
     * specified cube. This image's assets must be installed on that cube.
     */
    uint16_t tile(_SYSCubeID cube, unsigned i) const {
        ASSERT(i < numTiles());
        return assetGroup().baseAddress(cube) + tileArray()[i];
    };

    /**
     * Return the index of the tile at the specified (x, y) tile coordinates,
     * and optionally on the specified frame number.
     *
     * The returned index is relocated to an absolute address for the
     * specified cube. This image's assets must be installed on that cube.
     */
    uint16_t tile(_SYSCubeID cube, Int2 pos, unsigned frame = 0) const {
        ASSERT(pos.x < tileWidth() && pos.y < tileHeight() && frame < numFrames());
        return assetGroup().baseAddress(cube) + tileArray()[
            pos.x + pos.y * tileHeight() + frame * numTilesPerFrame()];
    }
};


/**
 * An audio asset, using any supported compression codec.
 */

struct AssetAudio {
    _SYSAudioModule sys;
};


};  // namespace Sifteo

#endif
