/*
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#pragma once
#ifdef NOT_USERSPACE
#   error This is a userspace-only header, not allowed by the current build.
#endif

#include <sifteo/array.h>
#include <sifteo/abi.h>

namespace Sifteo {

/**
 * @defgroup filesystem Filesystem
 *
 * These objects serve as an interface to the simple filesystem which
 * stores saved game data, as well as storing games themselves.
 *
 * @{
 */


/**
 * A Volume is a coarse-grained region of external memory. Volumes are
 * used by the operating system for various purposes, including storing
 * games.
 *
 * Typical games will not need Volume objects. They are only useful if
 * you need to launch or retrieve data from other programs or other bundles
 * of data which were installed separately from your game.
 */

class Volume {
public:
    _SYSVolumeHandle sys;

    /// Enumeration of common volume types
    enum Type {
        T_GAME      = _SYS_FS_VOL_GAME,
        T_LAUNCHER  = _SYS_FS_VOL_LAUNCHER,
    };

    /// Default constructor, leaves the Volume with an invalid value of zero.
    Volume() : sys(0) {}

    /// Initialize from a system object
    Volume(_SYSVolumeHandle vh) : sys(vh) {}

    /// Implicit conversion to system object
    operator const _SYSVolumeHandle () const { return sys; }

    /**
     * List all volumes of the specified type. Populates an Array with
     * the results.
     */
    template <unsigned T>
    static void list(unsigned volType, Array<Volume, T> &volumes)
    {
        volumes.setCount(_SYS_fs_listVolumes(volType, &volumes.begin()->sys,
            volumes.capacity()));
    }

    /**
     * Transfer control to a new program, fully replacing the current
     * program in memory. Does not return.
     *
     * For Volumes containing a valid ELF binary only.
     */
    void exec() const
    {
        _SYS_elf_exec(sys);
    }
};


/**
 * A VolumeMapping represents a Volume that has been mapped into the
 * secondary flash memory region. This object can be used to access
 * the mapped data.
 *
 * For Volumes containing a valid ELF binary only.
 *
 * Only one thing may be mapped into the secondary flash memory region
 * at a time. This means that any time you create a MappedVolume, any
 * previously existing MappedVolume becomes no longer valid.
 *
 * Since only one Volume can be mapped at a time, it's important to copy
 * data out of the volume into RAM if you need to collect data from multiple
 * volumes. For example, a title can be copied to a Sifteo::String, or
 * an AssetImage can be copied to a Sifteo::TileBuffer.
 */

class MappedVolume {
    Volume vol;
    uint32_t offset;

    /*
     * On debug builds, this ensures that only one MappedVolume exists
     * at a time. On release builds, this has no effect and the static
     * variable optimizes out.
     */
    void debugInstanceCounter(int delta) {
        DEBUG_ONLY({
            static int counter = 0;
            counter += delta;
            ASSERT(counter >= 0);
            ASSERT(counter <= 1);
        });
    }

public:
    typedef _SYSUUID UUID;
    typedef _SYSMetadataCubeRange CubeRange;

    /**
     * Map a view of the provided Volume, which must contain an ELF binary.
     * No other MappedVolume instances may exist at this time.
     */
    explicit MappedVolume(Volume vol)
        : vol(vol), offset(_SYS_elf_map(vol)) {
        debugInstanceCounter(1);
    }

    ~MappedVolume() {
        debugInstanceCounter(-1);

        // Reclaim a little bit of memory by unmapping
        _SYS_elf_map(0);
    }

    /// Returns the Volume associated with this mapping
    Volume volume() const {
        return vol;
    }

    /**
     * Look up a metadata value from the mapped Volume. On success, returns
     * a pointer to the value, and (if actualSize is not NULL) writes the
     * actual size of the metadata value to *actualSize.
     *
     * If the metadata value is not found, or it's smaller than minSize,
     * returns NULL.
     *
     * Note that the 'actualSize' parameter here is required, in order to
     * reliably differentiate this from the 1- or 2-argument form of
     * metadata().
     */
    void *metadata(unsigned key, unsigned minSize, unsigned *actualSize) const {
        return _SYS_elf_metadata(vol, key, minSize, actualSize);
    }

    /**
     * Look up a metadata value from the mapped Volume. The result is
     * cast to a constant pointer of the indicated type, and we require that
     * the metadata value is at least as large as sizeof(T).
     *
     * On success, returns a pointer to the value, and
     * (if actualSize is not NULL) writes the actual size of the metadata
     * value to *actualSize.
     *
     * If the metadata value is not found, or it's smaller than sizeof(T),
     * returns NULL.
     *
     * Use this form of metadata() in the common case when minSize equals
     * the size of the object you're retrieving a pointer to.
     */
    template <typename T>
    const T* metadata(unsigned key, unsigned *actualSize = NULL) const {
        return reinterpret_cast<const T*>(metadata(key, sizeof(T), actualSize));
    }

    /**
     * Translate a virtual address in this ELF volume's read-only data segment,
     * into a virtual address in the mapped view of the same program.
     *
     * This accounts for the difference between the primary and secondary
     * flash address spaces, as well as the offset between the RODATA segment
     * and the beginning of the ELF file.
     */
    template <typename T>
    T* translate(T* va) const {
        return reinterpret_cast<T*>(translate(reinterpret_cast<uint32_t>(va)));
    }

    /**
     * A variant of translate() for pointers that have already been
     * cast to uint32_t, such as pointers that are found in system asset
     * objects.
     */
    uint32_t translate(uint32_t va) const {
        return va + offset;
    }

    /**
     * Retrieve a mapped, NUL-terminated string with the volume's title.
     * Always returns a valid pointer. If the game has no title, returns
     * a placeholder string.
     */
    const char *title() const {
        const char *p = metadata<char>(_SYS_METADATA_TITLE_STR);
        return p ? p : "(untitled)";
    }

    /**
     * Retrieve the volume's UUID. Always returns a valid pointer. If there
     * is no UUID, returns an all-zero dummy UUID.
     */
    const UUID *uuid() const {
        const UUID *p = metadata<UUID>(_SYS_METADATA_UUID);
        static UUID zero;
        return p ? p : &zero;
    }

    void writeAssetGroup(const _SYSMetadataBootAsset &meta, AssetGroup &group)
    {
        bzero(group);
        group.sys.pHdr = translate(meta.pHdr);
    }
};


/**
 * @} endgroup Filesystem
 */

} // namespace Sifteo
