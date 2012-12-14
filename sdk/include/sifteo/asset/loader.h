/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#pragma once
#ifdef NOT_USERSPACE
#   error This is a userspace-only header, not allowed by the current build.
#endif

#include <sifteo/asset/group.h>
#include <sifteo/math.h>
#include <sifteo/array.h>
#include <sifteo/abi.h>

namespace Sifteo {

/**
 * @addtogroup assets
 * @{
 */

class Volume;

/**
 * @brief One node in an AssetConfiguration list
 *
 * This node makes a single request to load one group into one slot. A list of
 * these nodes makes up an AssetConfiguration.
 */

struct AssetConfigurationNode {
    _SYSAssetConfiguration sys;

    /*
     * @brief Initialize this node with an AssetSlot and AssetGroup
     *
     * This initializes the AssetConfigurationNode with instructions for
     * the AssetLoader to make sure the AssetGroup 'group' is available in the AssetSlot
     * 'slot' by the time the load finishes.
     *
     * If the AssetGroup is part of a different Volume, you must provide that volume
     * handle as the 'volume' parameter, and the 'group' must be mapped via MappedVolume.
     * If you are loading assets packaged with your own game, you do not need to provide
     * a volume.
     */
    void init(_SYSAssetSlot slot, AssetGroup &group, _SYSVolumeHandle volume = 0)
    {
        sys.pGroup = reinterpret_cast<uintptr_t>(&group);
        sys.volume = volume;
        sys.dataSize = group.compressedSize();
        sys.numTiles = group.numTiles();
        sys.ordinal = group.sysHeader()->ordinal;
        sys.slot = slot;
    }

    /**
     * @brief Return the AssetSlot this node will be loaded into
     */
    AssetSlot slot() const {
        return AssetSlot(sys.slot);
    }

    /**
     * @brief Return the AssetGroup referenced by this node
     */
    AssetGroup *group() const {
        return reinterpret_cast<AssetGroup*>(sys.pGroup);
    }

    /** 
     * @brief Return the Volume handle referenced by this node.
     */
    _SYSVolumeHandle volume() const {
        return sys.volume;
    }

    /**
     * @brief Get the size of this asset group, in tiles.
     */
    unsigned numTiles() const {
        return sys.numTiles;
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
        return sys.dataSize;
    }
};


/**
 * @brief An AssetConfiguration represents an arrangement of AssetGroups to load
 *
 * This object provides instructions to an AssetLoader about which AssetGroups
 * to load into which slots. A single AssetConfiguration may be applied to
 * all cubes, or different AssetConfigurations may be used for different subsets
 * of cubes during a single load event.
 *
 * An AssetConfiguration is stored as an array of nodes, each of which associates
 * an AssetGroup with an AssetSlot. The AssetLoader is responsible for making this
 * entire configuration current on the cube, even if this requires erasing AssetSlots,
 * and even if new cubes arrive partway through the loading process.
 */

template <unsigned tCapacity>
class AssetConfiguration : public Array<AssetConfigurationNode, tCapacity, uint8_t> {
    typedef Array<AssetConfigurationNode, tCapacity, uint8_t> super;
public:

    /**
     * @brief Add an AssetGroup to this configuration
     *
     * This appends a new load instruction to the AssetConfiguration, instructing
     * the AssetLoader to make sure the AssetGroup 'group' is available in the AssetSlot
     * 'slot' by the time the load finishes.
     *
     * If the AssetGroup is part of a different Volume, you must provide that volume
     * handle as the 'volume' parameter, and the 'group' must be mapped via MappedVolume.
     * If you are loading assets packaged with your own game, you do not need to provide
     * a volume.
     */
    void append(_SYSAssetSlot slot, AssetGroup &group, _SYSVolumeHandle volume = 0)
    {
        super::append().init(slot, group, volume);
    }
};


/**
 * @brief An AssetLoader coordinates asset loading operations on one or more cubes.
 *
 * An AssetLoader may be a transient object; it only needs to exist when an
 * asset loading operation is taking place, and can be deleted or recycled
 * afterwards. AssetLoaders can be allocated on the stack, or as part of a union.
 *
 * A single AssetLoader can load any combination of AssetConfiguration objects concurrently
 * to any number of cubes, up to the defined CUBE_ALLOCATION. Each AssetConfiguration operates
 * on its own CubeSet, such that a different AssetConfiguration may be loaded on each cube,
 * or any combination of cubes.
 *
 * <b>Single AssetConfiguration Example</b>
 * \code
 * #include "assets.gen.h"
 * using namespace Sifteo;
 *
 * // allocate a slot into which our assets will be loaded
 * static AssetSlot MenuSlot = AssetSlot::allocate();
 *
 * // assume assets.gen.h provides a Sifteo::AssetGroup called MenuGroup
 *
 * AssetConfiguration<1> config;
 * config.append(MenuSlot, MenuGroup);
 *
 * // load a single configuration to the set of all connected cubes
 * AssetLoader loader;
 * loader.init();
 * loader.start(config, CubeSet::connected());
 *
 * // and wait until the load is complete
 * // NOTE - it is also possible to poll for completion using AssetLoader::isComplete()
 * loader.finish();
 * \endcode
 *
 * <b>Multiple AssetConfiguration Example</b>
 * \code
 * #include "assets.gen.h"
 * using namespace Sifteo;
 *
 * static AssetSlot MenuSlot = AssetSlot::allocate();
 * static AssetSlot AnimationSlot = AssetSlot::allocate();
 *
 * // assume assets.gen.h provides two Sifteo::AssetGroup objects
 * // called MenuGroup and AnimationGroup
 *
 * AssetConfiguration<1> menuConfig;
 * menuConfig.append(MenuSlot, MenuGroup);
 *
 * AssetConfiguration<1> animationConfig;
 * animationConfig.append(AnimationSlot, AnimationGroup);
 *
 * AssetLoader loader;
 * loader.init();
 *
 * // load menuConfig to cubes 0 and 1
 * CubeSet cubeset01(0, 2);
 * loader.start(menuConfig, cubeset01);
 *
 * // load animationConfig to cubes 2 and 3
 * CubeSet cubeset23(2, 4);
 * loader.start(animationConfig, cubeset23);
 *
 * loader.finish();
 * \endcode
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
        bzero(*this);
    }

    /**
     * @brief Ensure that the system is no longer using this AssetLoader object.
     *
     * If any loads are still in progress, this function blocks until they complete.
     * This must be called after an asset load is done, before the AssetLoader
     * object itself is deallocated or recycled.
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
     *
     * By default, this cancels all in-progress load operations. A CubeSet may
     * be specified, to cancel only the indicated cubes.
     *
     * If a load is cancelled, the group that was being loaded will not be available,
     * but pre-existing groups will still be usable. However, further load operations
     * on that AssetSlot will not be possible until the slot is erased. It is said
     * that a slot in this post-cancellation state is _indeterminate_ until it is
     * erased.
     */
    void cancel(_SYSCubeIDVector cubes = -1) {
        _SYS_asset_loadCancel(*this, cubes);
    }

    /**
     * @brief Start installing an AssetConfiguration
     *
     * This uses an AssetConfiguration object to describe a set of AssetGroups
     * which should be made available, and the slots to install each into.
     * The AssetLoader object will automatically erase any slots that need to be
     * erased before starting, and it will calculate progress.
     *
     * By default, the AssetConfiguration is installed on to all cubes that
     * are currently connected, and any cubes which connect prior to the
     * other cubes completing their loads. This will load the same AssetConfiguration
     * on all cubes.
     *
     * A CubeSet can be specified to restrict this load to only some of the
     * available cubes. This can also be used to load different AssetConfigurations
     * on different subsets of the available cubes.
     *
     * The provided AssetConfiguration must be valid, in that it must be possible
     * to load all indicated AssetGroups in the indicated slots. If space is
     * exhausted due to groups not mentioned in the AssetConfiguration, the
     * AssetLoader will automatically erase the slot and reload only necessary
     * groups. However, if space is exhausted solely due to groups mentioned in
     * the AssetConfiguration, this indicates that the AssetConfiguration is invalid.
     *
     * An AssetConfiguration which is invalid or which does not match the
     * corresponding AssetGroup header will cause a fault.
     *
     * The system does not copy the AssetConfiguration, it refers to it by address.
     * As a consequence, it is vital that your AssetConfiguration instance stays
     * in scope until you call AssetLoader::finish(), or until after your
     * ScopedAssetLoader goes out of scope.
     */
    template < typename T >
    void start(T& configuration, _SYSCubeIDVector cubes = -1)
    {
        // Limit to cubes that we've allocated _SYSAssetLoaderCubes for
        STATIC_ASSERT(CUBE_ALLOCATION <= _SYS_NUM_CUBE_SLOTS);
        cubes &= 0xFFFFFFFF << (32 - CUBE_ALLOCATION);
        _SYS_asset_loadStart(*this, &configuration.begin()->sys, configuration.count(), cubes);
    }

    /**
     * @brief Measures progress on a single cube, as an integer
     *
     * This measures just the loading progress for a single cube, returning
     * a number between 0 and 'max'. If the cube is done loading, this returns
     * 'max'. If the cube hasn't started loading or has no need to load assets,
     * returns zero.
     *
     * This calculates using 32-bit integer math, and 'max' multiplied
     * by the number of bytes of uncompressed asset data must not overflow
     * the 32-bit type.
     */
    unsigned cubeProgress(_SYSCubeID cubeID, unsigned max) const
    {
        ASSERT(cubeID < CUBE_ALLOCATION);
        // NB: Division by zero on ARM (udiv) yields zero.
        return cubes[cubeID].progress * max / cubes[cubeID].total;
    }

    /**
     * @brief Measures progress on a single cube, as a floating point value
     *
     * This measures just the loading progress for a single cube, returning
     * a number between 0.0f and 1.0f. If the cube is done loading, this returns
     * 1.0f. If the cube hasn't started loading or has no need to load assets,
     * returns 0.0f.
     */
    float cubeProgress(_SYSCubeID cubeID) const
    {
        ASSERT(cubeID < CUBE_ALLOCATION);
        unsigned progress = cubes[cubeID].progress;
        unsigned total = cubes[cubeID].total;
        return total ? (progress / float(total)) : 0.0f;
    }

    /**
     * @brief Measures total progress on all cubes, as an integer
     *
     * This measures loading progress for all cubes, returning
     * a number between 0 and 'max'. If all cubes are done loading, this returns
     * 'max'. If the loading has not yet made any progress or no loading
     * was necessary, returns zero.
     *
     * This calculates using 32-bit integer math, and 'max' multiplied
     * by the number of bytes of uncompressed asset data must not overflow
     * the 32-bit type.
     */
    unsigned averageProgress(unsigned max) const
    {
        unsigned progress = 0, total = 0;
        for (unsigned i = 0; i < CUBE_ALLOCATION; ++i) {
            progress += cubes[i].progress;
            total += cubes[i].total;
        }
        return progress * max / total;
    }

    /**
     * @brief Measures total progress on all cubes, as a floating point value
     *
     * This measures loading progress for all cubes, returning
     * a number between 0.0f and 1.0f. If all cubes are done loading, this returns
     * 1.0f. If the loading has not yet made any progress, returns 0.0f. If no
     * loading was necessary, returns NaN.
     */
    float averageProgress() const
    {
        unsigned progress = 0, total = 0;
        for (unsigned i = 0; i < CUBE_ALLOCATION; ++i) {
            progress += cubes[i].progress;
            total += cubes[i].total;
        }
        return total ? (progress / float(total)) : 0.0f;
    }

    /**
     * @brief Which cubes are still busy loading?
     */
    _SYSCubeIDVector busyCubes() const {
        return sys.busyCubes;
    }

    /**
     * @brief Is the asset install finished for all cubes in the specified vector?
     *
     * This only returns 'true' when the install is completely done on all
     * of these cubes.
     *
     * Note that this is more efficient than calling AssetGroup::isInstalled.
     * Whereas isInstalled() needs to look for the AssetGroup in the system's
     * cache, this function is simply checking whether the current installation
     * session has completed.
     */
    bool isComplete(_SYSCubeIDVector vec) const {
        return (sys.busyCubes & vec) == 0;
    }
    
    /**
     * @brief Is the asset install for "cubeID" finished?
     * 
     * Returns true only when the install is completely done on this cube.
     */
    bool isComplete(_SYSCubeID cubeID) const {
        return isComplete(_SYSCubeIDVector(0x80000000 >> cubeID));
    }

    /**
     * @brief Are all asset installations from this asset loading session complete?
     */
    bool isComplete() const {
        return sys.busyCubes == 0;
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

/**
 * @} end addtogroup assets
 */

};  // namespace Sifteo
