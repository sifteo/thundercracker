/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#pragma once

#include <sifteo/abi.h>

/*
 * This header needs to work in both userspace and non-userspace
 * builds, though the latter have a greatly reduced feature set.
 */
#ifndef NOT_USERSPACE
#   include <sifteo/limits.h>      // For CUBE_ALLOCATION
#   include <sifteo/math.h>        // For vector types
#   include <sifteo/macros.h>      // For ASSERT
#endif

namespace Sifteo {

/**
 * @defgroup assets Assets
 *
 * Assets overview blurb here...
 * @{
 */

#ifndef NOT_USERSPACE   // Begin userspace-only objects

/**
 * @brief A bundle of compressed tile data, for use by AssetImages
 *
 * At build time, STIR creates a statically initialized instance of the
 * AssetGroup class for every asset group in the game.
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
     * @brief Get a pointer to the read-only system data for this asset group.
     *
     * The resulting value is constant at compile-time, if possible. If
     * the optional 'requireConst' argument is true, this will result in a
     * link failure if the header cannot be determined to be constant at
     * compile time.
     */
    ALWAYS_INLINE const _SYSAssetGroupHeader *sysHeader(bool requireConst=false) const {
        // AssetGroups are typically in RAM, but we want the static
        // initializer data so that our return value is known at compile time.
        _SYSAssetGroup *G = (_SYSAssetGroup*)
            _SYS_lti_initializer(reinterpret_cast<const void*>(&sys), requireConst);
        return reinterpret_cast<const _SYSAssetGroupHeader*>(G->pHdr);
    }

    /**
     * @brief Get the size of this asset group, in tiles.
     */
    unsigned numTiles() const {
        return sysHeader()->numTiles;
    }

    /**
     * @brief How many tiles will this group use up in its AssetSlot?
     *
     * This is numTiles(), rounded up to the nearest AssetSlot allocation unit.
     */
    unsigned tileAllocation() const {
        return roundup<unsigned>(numTiles(), _SYS_ASSET_GROUP_SIZE_UNIT);
    }

    /**
     * @brief Get the compressed size of this asset group, in bytes
     */
    unsigned compressedSize() const {
        return sysHeader()->dataSize;
    }

    /**
     * @brief Return the base address of this asset group, as loaded onto the
     * specified cube.
     *
     * The result is only valid if the asset group is
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
     * @brief Is this AssetGroup installed on all cubes in the given vector?
     *
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
        return _SYS_asset_findInCache(*this, vec) == vec;
    }

    /**
     * @brief Is this AssetGroup installed on a particular cube?
     *
     * If 'true', we may update the asset's cached base address.
     */
    bool isInstalled(_SYSCubeID cube) {
        return isInstalled(_SYSCubeIDVector(0x80000000 >> cube));
    }
};


/**
 * @brief AssetSlots are numbered containers, in a cube's flash
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
 *     AssetSlot MySlot = AssetSlot::allocate()
 *          .bootstrap(Group1)
 *          .bootstrap(Group2);
 *
 * AssetSlots can only be created via allocate(), or copying another
 * AssetSlot. All ID allocation is performed in allocate().
 */

class AssetSlot {
public:
    _SYSAssetSlot sys;

    /// Explicit conversion from a system object
    explicit AssetSlot(_SYSAssetSlot sys) : sys(sys) {}

    /// Copy constructor
    AssetSlot(const AssetSlot &other) : sys(other.sys) {}

    /// Implicit conversion to system object
    operator const _SYSAssetSlot& () const { return sys; }
    operator const _SYSAssetSlot* () const { return &sys; }

    /**
     * @brief Statically allocate a new AssetSlot.
     *
     * This function returns a unique ID which is constant at link-time.
     */
    ALWAYS_INLINE static AssetSlot allocate() {
        return AssetSlot(_SYS_lti_counter("Sifteo.AssetGroupSlot", 0));
    }

    /**
     * @brief How much space is remaining in this slot, measured in tiles?
     */
    unsigned tilesFree() const {
        return _SYS_asset_slotTilesFree(*this);
    }

    /**
     * @brief Is there room in this slot to load a particular AssetGroup
     * without erasing the slot?
     *
     * This does not check whether the specified group has already been installed.
     */
    bool hasRoomFor(const AssetGroup &group) const {
        return tilesFree() >= group.tileAllocation();
    }

    /**
     * @brief Erase this slot
     *
     * Any existing AssetGroups loaded into this slot will no longer be
     * loaded, and the slot will be marked as fully empty.
     *
     * After this call, any AssetGroups that were installed here
     * will now return 'false' from isInstalled(). Any tiles rendered
     * from this AssetGroup will now have undefined contents on-screen.
     */
    void erase() const {
        _SYS_asset_slotErase(*this);
    }

    /**
     * @brief Mark a particular AssetGroup as a "bootstrap" asset for this slot.
     *
     * The launcher will ensure that this AssetGroup is installed before the
     * game starts.
     *
     * This function must be called before the AssetGroup is used for
     * rendering. This is where we update the asset's base address from
     * the system cache.
     *
     * Returns *this, so that bootstrap() can be chained, especially off
     * of allocate().
     */
    ALWAYS_INLINE AssetSlot bootstrap(AssetGroup &group) const {
        // _SYSMetadataBootAsset
        _SYS_lti_metadata(_SYS_METADATA_BOOT_ASSET, "IBBBB",
            group.sysHeader(true), sys, 0, 0, 0);

        // Update base address from the cache. Make sure it was successful.
        _SYSCubeIDVector vec = _SYS_asset_findInCache(group, -1);
        ASSERT(vec != 0);

        return *this;
    }
};


/**
 * @brief AssetLoaders install an AssetGroup into an AssetSlot on one or more cubes.
 *
 * The AssetLoader is a transient object; it can exist only when an actual
 * asset loading operation is taking place, and be deleted or recycled
 * afterwards. Your game can allocate AssetLoaders on the stack, or as part of a union.
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
     * @brief Initialize this object.
     *
     * This is made explicit, rather than using a constructor, so that games
     * can keep AssetLoader in a union if necessary to save memory.
     *
     * Must be called once, prior to any calls to start().
     */
    void init() {
        sys.cubeVec = 0;
        sys.complete = 0;
    }

    /**
     * @brief Ensure that the system is no longer using this AssetLoader object.
     *
     * If any loads are still in progress, this function blocks until they complete.
     * After finish() returns, the loaded AssetGroup has been finalized, and it will
     * be ready for use.
     *
     * This must be called after an asset load is done, before the AssetLoader
     * object itself is deallocated or recycled. If the asset loading process
     * is interrupted between start() and finish(), the state of the remote
     * flash memory associated with this Asset Slot may be indeterminate,
     * requiring the entire slot to be erased and reloaded.
     *
     * If finish() has already been called, has no effect.
     */
    void finish() {
        _SYS_asset_loadFinish(*this);
    }

    /**
     * @brief End any in-progress asset loading operations without finishing them.
     *
     * This function is similar to finish(), except that any in-progress asset loading
     * operations do not finish. Instead, any asset slots written to since the last
     * finish() will be in an indeterminate state, and the slot must be erased before
     * any further loading may be attempted.
     */
    void cancel() {
        _SYS_asset_loadCancel(*this);
    }

    /**
     * @brief Start installing group "group" into slot "slot" on some set of cubes,
     * identified by the "cubes" bitvector.
     * 
     * See the single-cube version of start() for a detailed explanation.
     * This function is identical, except that the asset is broadcast to
     * multiple cubes simultaneously.
     *
     * We return 'true' if the asset download started and/or we found a cached
     * asset for every one of the specified cubes. If one or more of the
     * specified cubes has no room in "slot", we return false.
     *
     * If another asynchronous asset installation is already in progress,
     * this call will automatically block until that load has finished.
     */
    bool start(AssetGroup &group, AssetSlot slot, _SYSCubeIDVector cubes) {
        if (!_SYS_asset_loadStart(*this, group, slot, cubes))
            return false;

        return true;
    }

    /**
     * @brief Start intalling group "group" into slot "slot" on a single cube.
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
     * 'false' immediately, without starting an asset download.
     *
     * This typically would indicate a logic error in the game's asset
     * management. When start() fails, the Asset Slot data for this cube
     * will be left in a "loading in progress" state, which can only be cleared
     * by erasing the asset slot.
     */
    bool start(AssetGroup &group, AssetSlot slot, _SYSCubeID cube) {
        return start(group, slot, _SYSCubeIDVector(0x80000000 >> cube));
    }

    /**
     * @brief Which AssetGroup is being installed on the given cube during
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
     * @brief How much progress has been made on the indicated cube's
     * asset install, in bytes?
     *
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
     * @brief How much progress has been made on the indicated cube's
     * asset install, in arbitrary units?
     *
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
    int progress(_SYSCubeID cubeID, int max = 100) const {
        return progressBytes(cubeID) * max / group(cubeID).compressedSize();
    }

    /**
     * @brief How much progress has been made? This is a version of progress()
     * that returns a value scaled between 'min' and 'max'.
     */
    int progress(_SYSCubeID cubeID, int min, int max) const {
        return min + progress(cubeID, max - min);
    }

    /**
     * @brief Is the asset install finished for all cubes in the specified vector?
     *
     * This only returns 'true' when the install is completely done on all
     * of these cubes, and the AssetGroup is fully available for use in drawing.
     *
     * Note that this is more efficient than calling AssetGroup::isInstalled.
     * Whereas isInstalled() needs to look for the AssetGroup in the system's
     * cache, this function is simply checking whether the current installation
     * session has completed.
     */
    bool isComplete(_SYSCubeIDVector vec) const {
        return (sys.complete & vec) == vec;
    }
    
    /**
     * @brief Is the asset install for "cubeID" finished?
     * 
     * This will return true only when the install is completely done,
     * and the AssetGroup is fully available for use in drawing.
     */
    bool isComplete(_SYSCubeID cubeID) const {
        return isComplete(_SYSCubeIDVector(0x80000000 >> cubeID));
    }

    /**
     * @brief Are all asset installations from this asset loading session complete?
     */
    bool isComplete() const {
        return isComplete(sys.cubeVec);
    }
};


/**
 * @brief An AssetLoader subclass which automatically calls init() and
 * finish() in the constructor and destructor, respectively.
 *
 * The base AssetLoader class does not do this, so that it can be a POD
 * type compatible with unions. But if you're creating an AssetLoader on the
 * stack or in another object without using unions, this class may be
 * easier to use.
 */
class ScopedAssetLoader : public AssetLoader {
public:
    ScopedAssetLoader() { init(); }
    ~ScopedAssetLoader() { finish(); }
};


#endif   // End userspace-only objects


/**
 * @brief Any kind of asset image, as defined in your `stir` script.
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

#ifndef NOT_USERSPACE   // Begin userspace-only members

    /// Access the AssetGroup instance associated with this AssetImage
    AssetGroup &assetGroup() const { return *reinterpret_cast<AssetGroup*>(sys.pAssetGroup); }

    /// The (width, height) vector of this image, in tiles
    Int2 tileSize() const { return vec<int>(sys.width, sys.height); }

    /// Half the size of this image, in tiles
    Int2 tileExtent() const { return tileSize() / 2; }

    /// The (width, height) vector of this image, in pixels
    Int2 pixelSize() const { return vec<int>(sys.width << 3, sys.height << 3); }

    /// Half the size of this image, in pixels
    Int2 pixelExtent() const { return pixelSize() / 2; }

#endif   // End userspace-only members

    /// The width of this image, in tiles
    int tileWidth() const { return sys.width; }

    /// The height of this image, in tiles
    int tileHeight() const { return sys.height; }

    /// The width of this image, in pixels
    int pixelWidth() const { return sys.width << 3; }

    /// The height of this image, in pixels
    int pixelHeight() const { return sys.height << 3; }

    /// Access the number of 'frames' in this image
    int numFrames() const { return sys.frames; }

    /// Compute the total number of tiles per frame (tileWidth * tileHeight)
    int numTilesPerFrame() const { return tileWidth() * tileHeight(); }

    /// Compute the total number of tiles in the image
    int numTiles() const { return numFrames() * numTilesPerFrame(); }
    
    /// Implicit conversion to system object
    operator const _SYSAssetImage& () const { return sys; }
    operator _SYSAssetImage& () { return sys; }
    operator const _SYSAssetImage* () const { return &sys; }
    operator _SYSAssetImage* () { return &sys; }
};


/**
 * @brief An AssetImage in which all tiles are stored sequentially in memory.
 *
 * This doesn't store any separate tilemap information, just the index for
 * the first tile in the sequence.
 *
 * Generate a PinnedAssetImage by passing the `pinned=1` option to image{}
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

#ifndef NOT_USERSPACE   // Begin userspace-only members

    /// Access the AssetGroup instance associated with this AssetImage
    AssetGroup &assetGroup() const { return *reinterpret_cast<AssetGroup*>(sys.pAssetGroup); }

    /// The (width, height) vector of this image, in tiles
    Int2 tileSize() const { return vec<int>(sys.width, sys.height); }

    /// Half the size of this image, in tiles
    Int2 tileExtent() const { return tileSize() / 2; }

    /// The (width, height) vector of this image, in pixels
    Int2 pixelSize() const { return vec<int>(sys.width << 3, sys.height << 3); }

    /// Half the size of this image, in pixels
    Int2 pixelExtent() const { return pixelSize() / 2; }

    /**
     * @brief Returns the index of the tile at linear position 'i' in the image.
     *
     * The returned index is unrelocated; it is relative to the base address
     * of the image's AssetGroup.
     */
    uint16_t tile(unsigned i) const {
        ASSERT(i < numTiles());
        return sys.pData + i;
    }

    /**
     * @brief Return the index of the tile at the specified (x, y) tile coordinates.
     *
     * The returned index is unrelocated; it is relative to the base address
     * of the image's AssetGroup.
     */
    uint16_t tile(Int2 pos, unsigned frame = 0) const {
        ASSERT(pos.x < tileWidth() && pos.y < tileHeight() && frame < numFrames());
        return sys.pData + pos.x + pos.y * tileHeight() + frame * numTilesPerFrame();
    }

    /**
     * @brief Returns the index of the tile at linear position 'i' in the image.
     *
     * The returned index is relocated to an absolute address for the
     * specified cube. This image's assets must be installed on that cube.
     */
    uint16_t tile(_SYSCubeID cube, unsigned i) const {
        ASSERT(i < numTiles());
        return assetGroup().baseAddress(cube) + sys.pData + i;
    }

    /**
     * @brief Return the index of the tile at the specified (x, y) tile coordinates.
     *
     * The returned index is relocated to an absolute address for the
     * specified cube. This image's assets must be installed on that cube.
     */
    uint16_t tile(_SYSCubeID cube, Int2 pos, unsigned frame = 0) const {
        ASSERT(pos.x < tileWidth() && pos.y < tileHeight() && frame < numFrames());
        return assetGroup().baseAddress(cube) + sys.pData
            + pos.x + pos.y * tileHeight() + frame * numTilesPerFrame();
    }

#endif   // End userspace-only members

    /// The width of this image, in tiles
    int tileWidth() const { return sys.width; }

    /// The height of this image, in tiles
    int tileHeight() const { return sys.height; }

    /// The width of this image, in pixels
    int pixelWidth() const { return sys.width << 3; }

    /// The height of this image, in pixels
    int pixelHeight() const { return sys.height << 3; }

    /// Access the number of 'frames' in this image
    int numFrames() const { return sys.frames; }

    /// Compute the total number of tiles per frame (tileWidth * tileHeight)
    int numTilesPerFrame() const { return tileWidth() * tileHeight(); }

    /// Compute the total number of tiles in the image
    int numTiles() const { return numFrames() * numTilesPerFrame(); }

    /// Implicit conversion to AssetImage base class
    operator const AssetImage& () const { return *reinterpret_cast<const AssetImage*>(this); }
    operator AssetImage& () { return *reinterpret_cast<AssetImage*>(this); }
    operator const AssetImage* () const { return reinterpret_cast<const AssetImage*>(this); }
    operator AssetImage* () { return reinterpret_cast<AssetImage*>(this); }

    /// Implicit conversion to system object
    operator const _SYSAssetImage& () const { return sys; }
    operator _SYSAssetImage& () { return sys; }
    operator const _SYSAssetImage* () const { return &sys; }
    operator _SYSAssetImage* () { return &sys; }
};


/**
 * @brief An AssetImage in which all tile indices are stored in a flat array,
 * without any additional compression.
 *
 * Flat assets are usually less efficient than compressed AssetImages,
 * but they allow you to have cheap random access to any tile in the image.
 *
 * Generate a FlatAssetImage by passing the `flat=1` option to image{}
 * in your STIR script.
 */
 
struct FlatAssetImage {
    _SYSAssetImage sys;

#ifndef NOT_USERSPACE   // Begin userspace-only members

    /// Access the AssetGroup instance associated with this AssetImage
    AssetGroup &assetGroup() const { return *reinterpret_cast<AssetGroup*>(sys.pAssetGroup); }

    /// The (width, height) vector of this image, in tiles
    Int2 tileSize() const { return vec<int>(sys.width, sys.height); }

    /// Half the size of this image, in tiles
    Int2 tileExtent() const { return tileSize() / 2; }

    /// The (width, height) vector of this image, in pixels
    Int2 pixelSize() const { return vec<int>(sys.width << 3, sys.height << 3); }

    /// Half the size of this image, in pixels
    Int2 pixelExtent() const { return pixelSize() / 2; }

    /**
     * @brief Get a pointer to the raw tile data.
     *
     * Flat asset images represent their tile data as an array of uint16_t
     * un-relocated tile indices. This returns a pointer to that array.
     */
    const uint16_t *tileArray() const {
        return reinterpret_cast<const uint16_t *>(sys.pData);
    }

    /**
     * @brief Returns the index of the tile at linear position 'i' in the image.
     *
     * The returned index is unrelocated; it is relative to the base address
     * of the image's AssetGroup.
     */
    uint16_t tile(unsigned i) const {
        ASSERT(i < numTiles());
        return tileArray()[i];
    }

    /**
     * @brief Return the index of the tile at the specified (x, y) tile coordinates.
     *
     * The returned index is unrelocated; it is relative to the base address
     * of the image's AssetGroup.
     */
    uint16_t tile(Int2 pos, unsigned frame = 0) const {
        ASSERT(pos.x < tileWidth() && pos.y < tileHeight() && frame < numFrames());
        return tileArray()[pos.x + pos.y * tileHeight() + frame * numTilesPerFrame()];
    }

    /**
     * @brief Returns the index of the tile at linear position 'i' in the image.
     *
     * The returned index is relocated to an absolute address for the
     * specified cube. This image's assets must be installed on that cube.
     */
    uint16_t tile(_SYSCubeID cube, unsigned i) const {
        ASSERT(i < numTiles());
        return assetGroup().baseAddress(cube) + tileArray()[i];
    }

    /**
     * @brief Return the index of the tile at the specified (x, y) tile coordinates.
     *
     * The returned index is relocated to an absolute address for the
     * specified cube. This image's assets must be installed on that cube.
     */
    uint16_t tile(_SYSCubeID cube, Int2 pos, unsigned frame = 0) const {
        ASSERT(pos.x < tileWidth() && pos.y < tileHeight() && frame < numFrames());
        return assetGroup().baseAddress(cube) + tileArray()[
            pos.x + pos.y * tileHeight() + frame * numTilesPerFrame()];
    }

#endif   // End userspace-only members

    /// The width of this image, in tiles
    int tileWidth() const { return sys.width; }

    /// The height of this image, in tiles
    int tileHeight() const { return sys.height; }

    /// The width of this image, in pixels
    int pixelWidth() const { return sys.width << 3; }

    /// The height of this image, in pixels
    int pixelHeight() const { return sys.height << 3; }

    /// Access the number of 'frames' in this image
    int numFrames() const { return sys.frames; }

    /// Compute the total number of tiles per frame (tileWidth * tileHeight)
    int numTilesPerFrame() const { return tileWidth() * tileHeight(); }

    /// Compute the total number of tiles in the image
    int numTiles() const { return numFrames() * numTilesPerFrame(); }

    /// Implicit conversion to AssetImage base class
    operator const AssetImage& () const { return *reinterpret_cast<const AssetImage*>(this); }
    operator AssetImage& () { return *reinterpret_cast<AssetImage*>(this); }
    operator const AssetImage* () const { return reinterpret_cast<const AssetImage*>(this); }
    operator AssetImage* () { return reinterpret_cast<AssetImage*>(this); }

    /// Implicit conversion to system object
    operator const _SYSAssetImage& () const { return sys; }
    operator _SYSAssetImage& () { return sys; }
    operator const _SYSAssetImage* () const { return &sys; }
    operator _SYSAssetImage* () { return &sys; }
};


/**
 * @brief An audio asset, using any supported compression codec.
 */

struct AssetAudio {
    _SYSAudioModule sys;

    /**
     * @brief Return the default speed for this audio asset, in samples per second.
     *
     * This can be used to calculate a new speed for AudioChannel::setSpeed().
     */
    unsigned speed() {
        return sys.sampleRate;
    }

    /**
     * @brief Create an AssetAudio object programmatically, from uncompressed PCM data
     *
     * This creates an AssetAudio instance which references the specified
     * buffer of 16-bit uncompressed PCM samples. You can use this to wrap
     * dynamically synthesized audio data for playback on an AudioChannel.
     *
     * This is intended mostly for creating software "instruments" for
     * real-time audio synthesis, so we default to looping mode, and
     * there is no default sample rate or volume. These parameters
     * can all be set via AudioChannel at runtime.
     */
    static AssetAudio fromPCM(const int16_t *samples, unsigned numSamples)
    {
        const AssetAudio result = {{
            /* sampleRate  */  0,
            /* loopStart   */  0,
            /* loopEnd     */  numSamples,
            /* loopType    */  _SYS_LOOP_REPEAT,
            /* type        */  _SYS_PCM,
            /* volume      */  _SYS_AUDIO_DEFAULT_VOLUME,
            /* dataSize    */  numSamples * sizeof samples[0],
            /* pData       */  reinterpret_cast<uintptr_t>(samples),
        }};
        return result;
    }

    /// Templatized version of fromPCM(), for fixed-size sample arrays.
    template <typename T>
    static AssetAudio fromPCM(const T &samples) {
        return fromPCM(&samples[0], arraysize(samples));
    }
};

/**
 * @brief A Tracker module, converted from XM format by `stir`
 */

struct AssetTracker {
    _SYSXMSong song;
};

/**
 * @} end addtogroup assets
 */

};  // namespace Sifteo
