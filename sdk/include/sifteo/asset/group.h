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
#include <sifteo/limits.h>
#include <sifteo/macros.h>
#include <sifteo/math.h>

namespace Sifteo {

/**
 * @addtogroup assets
 * @{
 */

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
    ALWAYS_INLINE const _SYSAssetGroupHeader *sysHeader(bool requireConst=false) const
    {
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
     * It is an error to rely on the value of baseAddress() before using
     * either AssetSlot::bootstrap() or AssetLoader to load a group.
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
     * This function should be used only as a hint. Don't use it to make
     * decisions about whether or not to include this group in an AssetConfiguration!
     * If you will need to use a group, it belongs in the AssetConfiguration
     * regardless of whether or not it's already loaded.
     */
    bool isInstalled(_SYSCubeIDVector vec) {
        return _SYS_asset_findInCache(*this, vec) == vec;
    }

    /**
     * @brief Is this AssetGroup installed on a particular cube?
     *
     * This function should be used only as a hint. Don't use it to make
     * decisions about whether or not to include this group in an AssetConfiguration!
     * If you will need to use a group, it belongs in the AssetConfiguration
     * regardless of whether or not it's already loaded.
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
     *
     * By default, this measures the space remaining on all cubes, and
     * returns the smallest of all results. A CubeSet may be explicitly
     * specified, to limit the measurement to those cubes.
     */
    unsigned tilesFree(_SYSCubeIDVector cubes = -1) const {
        return _SYS_asset_slotTilesFree(*this, cubes);
    }

    /**
     * @brief Is there room in this slot to load a particular AssetGroup
     * without erasing the slot?
     *
     * This does not check whether the specified group has already been installed.
     *
     * By default, this checks whether _all_ cubes have room for the group.
     * A CubeSet may be explicitly specified, to limit the check to those cubes. 
     */
    bool hasRoomFor(const AssetGroup &group, _SYSCubeIDVector cubes = -1) const {
        return tilesFree(cubes) >= group.tileAllocation();
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
     *
     * By default, the slot will be erased on all cubes. A CubeSet may be
     * explicitly specified, to limit the erasure to those cubes.
     */
    void erase(_SYSCubeIDVector cubes = -1) const {
        _SYS_asset_slotErase(*this, cubes);
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
 * @} end addtogroup assets
 */

};  // namespace Sifteo
