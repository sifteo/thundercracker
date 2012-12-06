/*
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#pragma once
#ifdef NOT_USERSPACE
#   error This is a userspace-only header, not allowed by the current build.
#endif

#include <sifteo/array.h>
#include <sifteo/asset/loader.h>
#include <sifteo/abi.h>

namespace Sifteo {

/**
 * @defgroup filesystem Filesystem
 *
 * @brief Access to persistent storage, including saved game data
 * 
 * These objects serve as an interface to the simple filesystem which
 * stores saved game data, as well as storing games themselves.
 *
 * @{
 */


/**
 * @brief A lightweight ID for a persistently stored object.
 * 
 * Games may read and write such objects stored in a key-value
 * dictionary associated with their volume. The key space is limited.
 * Valid keys range from 0 to LIMIT. These keys may be allocated manually
 * or automatically.
 * 
 * If you're computing keys dynamically, cast your key to a StoredObject
 * in order to access the object data at that key. If you're allocating keys
 * statically, you can create global StoredObject instances which act as
 * a single place to associate IDs with names.
 * 
 * By default, objects are both read from and written to a store associated
 * with the current game's Volume. It is possible to read another game's
 * stored data (by specifying its Volume) but you may not write to another
 * game's object store.
 *
 * Objects are stored in a journaled filesystem, and writes are synchronous.
 * By design, if power fails or the system is otherwise interrupted during
 * a write, future reads will continue to return the last successfully-written
 * version of an object.
 *
 * Object sizes must be at least one byte, and no more than MAX_SIZE bytes.
 */

class StoredObject {
public:
    _SYSObjectKey sys;

    /// Maximum allowed value for a key (255)
    static const unsigned LIMIT = _SYS_FS_MAX_OBJECT_KEYS - 1;

    /// Maximum size of an object, in bytes (4080)
    static const unsigned MAX_SIZE = _SYS_FS_MAX_OBJECT_SIZE;

    /// Initialize from a system object
    StoredObject(_SYSObjectKey k) : sys(k) {}

    /// Initialize from an integer
    StoredObject(int k) : sys(k) {
        ASSERT(k >= 0 && k <= LIMIT);
    }

    /// Initialize from an unsigned integer
    StoredObject(unsigned k) : sys(k) {
        ASSERT(k <= LIMIT);
    }

    /// Implicit conversion to system object
    operator const _SYSObjectKey () const { return sys; }

    /**
     * @brief Allocate a unique StoredObject value at compile-time
     *
     * Every time this function is invoked, it is resolved by the linker
     * to a unique StoredObject. We start allocating at LIMIT, and each
     * successive call to allocate() returns a value decremented by one.
     *
     * This allocation order supports a strategy in which manually-assigned
     * or calculated IDs begin at 0 and count up, and automatically
     * allocated IDs begin at LIMIT and count down.
     */
    static StoredObject allocate() {
        return StoredObject(LIMIT - _SYS_lti_counter("Sifteo.StoredObject", 0));
    }

    /**
     * @brief Read a stored object into memory
     *
     * Reads into the provided buffer. Returns the actual number of bytes
     * read, which on success always equals either the size of the
     * stored data for this object or the size of the provided buffer,
     * whichever is smaller.
     *
     * The provided 'bufferSize' _must_ be large enough to hold the entire
     * stored object, excepting only any trailing 0xFF bytes. It is not,
     * in general, possible to perform partial reads of a stored object.
     *
     * If no data has been stored for this object yet, returns zero.
     * Note that the object store does not distinguish between zero-length
     * values and missing values.
     *
     * By default, this reads from the object store associated with the
     * current game. The optional 'volume' parameter can be set to
     * a specific Sifteo::Volume instance in order to read objects from
     * a different game's data store.
     *
     * @warning TO DO: Define error codes (< 0) as necessary.
     */
    int read(void *buffer, unsigned bufferSize, _SYSVolumeHandle volume = 0) const {
        return _SYS_fs_objectRead(sys, (uint8_t*)buffer, bufferSize, volume);
    }

    /**
     * @brief Save a new version of an object
     *
     * Replace this object's contents with a new block of data.
     * On success, always returns the number of bytes written,
     * which is always equal to dataSize.
     *
     * The data block will be padded to the next multiple of 16 bytes.
     *
     * Writing an empty data block is equivalent to erasing the
     * stored object.
     *
     * If power fails during a write, by design subsequent reads will
     * return the last successfully-saved version of that object.
     *
     * @warning TO DO: Define error codes (< 0) as necessary.
     */
    int write(const void *data, unsigned dataSize) const {
        return _SYS_fs_objectWrite(sys, (const uint8_t*)data, dataSize);
    }

    /// Erase this object. Equivalent to a zero-length write()
    int erase() const {
        return write(0, 0);
    }

    /**
     * @brief Template wrapper for read() of fixed-size objects.
     *
     * @deprecated Use readObject() instead. This will be removed in a
     * future version update.
     */
    template <typename T>
    int read(T &buffer, _SYSVolumeHandle volume = 0) const {
        return read((void*) &buffer, sizeof buffer, volume);
    }

    /// Template wrapper for read() of fixed-size objects.
    template <typename T>
    int readObject(T &buffer, _SYSVolumeHandle volume = 0) const {
        return read((void*) &buffer, sizeof buffer, volume);
    }

    /**
     * @brief Template wrapper for write() of fixed-size objects.
     *
     * @deprecated Use writeObject() instead. This will be removed in a
     * future version update.
     */
    template <typename T>
    int write(const T &buffer) const {
        return write((const void*) &buffer, sizeof buffer);
    }

    /// Template wrapper for write() of fixed-size objects.
    template <typename T>
    int writeObject(const T &buffer) const {
        return write((const void*) &buffer, sizeof buffer);
    }

    /// Equality comparison operator
    bool operator== (_SYSObjectKey other) const {
        return sys == other;
    }

    /// Inequality comparison operator
    bool operator!= (_SYSObjectKey other) const {
        return sys != other;
    }
};


/**
 * @brief A coarse-grained region of external memory
 *
 * Volumes are used by the operating system for various purposes,
 * including storing games.
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
        T_GAME      = _SYS_FS_VOL_GAME,         ///< Volume type for normal games
        T_LAUNCHER  = _SYS_FS_VOL_LAUNCHER,     ///< Volume type for the system launcher
    };

    /// Default constructor, leaves the Volume with an invalid value of zero.
    Volume() : sys(0) {}

    /// Initialize from a system object
    Volume(_SYSVolumeHandle vh) : sys(vh) {}

    /// Implicit conversion to system object
    operator const _SYSVolumeHandle () const { return sys; }

    /**
     * @brief List all volumes of the specified type
     *
     * Populates an Array with the results.
     */
    template <unsigned T>
    static void list(unsigned volType, Array<Volume, T> &volumes)
    {
        volumes.setCount(_SYS_fs_listVolumes(volType, &volumes.begin()->sys,
            volumes.capacity()));
    }

    /**
     * @brief Transfer control to a new program
     *
     * Fully replaces the current program in memory. Does not return.
     * For Volumes containing a valid ELF binary only.
     *
     * This does _not_ automatically call System::setCubeRange() on behalf
     * of the new program, nor does it load any bootstrap assets. Both of
     * these tasks are typically done by the Launcher app, before calling
     * exec(), by reading the volume metadata with MappedVolume::metadata().
     *
     * @warning While it is possible to call Volume::current().exec() to restart
     * the current program, there is an edge case to be aware of: Programs may be
     * deleted or upgraded while they are running, in which case Volume::current()
     * refers to a volume which has been deleted. Deleted volumes will not be
     * recycled as long as they are mapped, so programs normally continue running
     * correctly in this case. But a deleted volume cannot be executed or mapped.
     * If this edge case is important to avoid, one approach would be to search
     * for the new version of your own application, if any, using Volume::list().
     */
    void exec() const {
        _SYS_elf_exec(sys);
    }

    /**
     * @brief Return the Volume corresponding to the currently running program.
     *
     * This can be used in various ways, such as skipping the current game
     * in a listing of other games, reading your own metadata, or accessing
     * saved objects that belong to you.
     */
    static Volume running() {
        return (_SYSVolumeHandle) _SYS_fs_runningVolume();
    }

    /**
     * @brief Return the Volume corresponding to the previously running program.
     *
     * This refers to the program that was running before the last exec().
     * For normal games, this will always be the Volume associated with the
     * system launcher.
     *
     * If there was no previously running program (the system booted directly
     * into the current Volume) this returns a volume with the invalid value
     * of zero.
     */
    static Volume previous() {
        return (_SYSVolumeHandle) _SYS_fs_previousVolume();
    }

    /// Equality comparison operator
    bool operator== (_SYSVolumeHandle other) const {
        return sys == other;
    }

    /// Inequality comparison operator
    bool operator!= (_SYSVolumeHandle other) const {
        return sys != other;
    }
};


/**
 * @brief A Volume that has been mapped into the secondary flash memory region
 *
 * This object can be used to access the mapped data.
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

    static const unsigned MAX_BOOTSTRAP_GROUPS = _SYS_MAX_METADATA_ITEM_BYTES / sizeof(_SYSMetadataBootAsset);

    /// An AssetConfiguration large enough to hold all bootstrap groups
    typedef AssetConfiguration<MAX_BOOTSTRAP_GROUPS> BootstrapAssetConfiguration;

    /// An array large enough to hold all bototstrap AssetGroups
    typedef Array<AssetGroup, MAX_BOOTSTRAP_GROUPS> BootstrapAssetGroups;

    /**
     * @brief Construct a detached MappedVolume
     *
     * You must call attach() to connect this to a Volume.
     */
    MappedVolume()
        : vol(0), offset(0) {
        debugInstanceCounter(1);
    }

    /**
     * @brief Map a view of the provided Volume
     *
     * The volume must contain an ELF binary.
     * No other MappedVolume instances may exist at this time.
     */
    explicit MappedVolume(Volume vol) {
        debugInstanceCounter(1);
        attach(vol);
    }

    ~MappedVolume()
    {
        debugInstanceCounter(-1);

        // Reclaim a little bit of memory by unmapping
        _SYS_elf_map(0);
    }

    /**
     * @brief Attach this MappedVolume to a new volume
     *
     * Pointers obtained prior to this reattachment,
     * if any, will no longer be valid.
     */
    void attach(Volume vol)
    {
        if (this->vol != vol) {
            this->vol = vol;
            offset = _SYS_elf_map(vol);
        }
    }

    /// Returns the Volume associated with this mapping
    Volume volume() const {
        return vol;
    }

    /**
     * @brief Look up a metadata value from the mapped Volume
     *
     * On success, returns
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
     * @brief Look up a metadata value from the mapped Volume. The result is
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
     * @brief Translate a virtual address in this ELF volume's read-only data segment,
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
     * @brief A variant of translate() for pointers that have already been
     * cast to uint32_t, such as pointers that are found in system asset
     * objects.
     */
    uint32_t translate(uint32_t va) const {
        return va + offset;
    }

    /**
     * @brief Retrieve a mapped, NUL-terminated string with the volume's title.
     *
     * Always returns a valid pointer. If the game has no title, returns
     * a placeholder string.
     */
    const char *title(const char *placeholder = "(untitled)") const {
        const char *p = metadata<char>(_SYS_METADATA_TITLE_STR);
        return p ? p : placeholder;
    }

    /**
     * @brief Retrieve a mapped, NUL-terminated string with the volume's package string.
     *
     * Always returns a valid pointer. If the game has no package, returns
     * a placeholder string.
     */
    const char *package(const char *placeholder = "(none)") const {
        const char *p = metadata<char>(_SYS_METADATA_PACKAGE_STR);
        return p ? p : placeholder;
    }

    /**
     * @brief Retrieve a mapped, NUL-terminated string with the volume's version string.
     *
     * Always returns a valid pointer. If the game has no package, returns
     * a placeholder string.
     */
    const char *version(const char *placeholder = "(none)") const {
        const char *p = metadata<char>(_SYS_METADATA_VERSION_STR);
        return p ? p : placeholder;
    }

    /**
     * @brief Retrieve the volume's UUID.
     *
     * Always returns a valid pointer. If there is no UUID, returns an all-zero dummy UUID.
     */
    const UUID *uuid() const {
        const UUID *p = metadata<UUID>(_SYS_METADATA_UUID);
        static UUID zero;
        return p ? p : &zero;
    }

    /**
     * @brief Translate a bootstrap asset metadata record to an AssetGroup
     *
     * Using this object's memory mapping, convert a _SYSMetadataBootAsset
     * record into an AssetGroup which can be used to load the corresponding
     * assets.
     *
     * This is only useful for programs which must bootstrap another
     * program's assets.
     */
    void translate(const _SYSMetadataBootAsset &meta, AssetGroup &group)
    {
        bzero(group);
        group.sys.pHdr = translate(meta.pHdr);
    }

    /**
     * @brief Translate an image metadata record to an AssetImage
     *
     * Using this object's memory mapping, convert a _SYSMetadataImage
     * record into an AssetImage which references the image data mapped
     * in from another volume.
     *
     * Since the _SYSMetadataImage only stores information about the
     * read-only data associated with an AssetGroup, you must also
     * supply an AssetGroup object to be used for the read-write
     * storage of state, such as load addresses for the group on each
     * cube
     *
     * This function can be used to load graphics assets from another game
     * or other Volume on the system, as long as those assets have been
     * annotated in their game's metadata.
     */
    void translate(const _SYSMetadataImage *meta, AssetImage &image, AssetGroup &group)
    {
        bzero(group);
        group.sys.pHdr = translate(meta->groupHdr);

        bzero(image);
        image.sys.pAssetGroup = reinterpret_cast<uint32_t>(&group);
        image.sys.width = meta->width;
        image.sys.height = meta->height;
        image.sys.frames = meta->frames;
        image.sys.format = meta->format;
        image.sys.pData = translate(meta->pData);
    }

    /**
     * @brief Get the bootstrap assets for this Volume, returning the result in
     * the provided BootstrapAssetConfiguration array.
     */
    void getBootstrap(BootstrapAssetGroups &groups, BootstrapAssetConfiguration &config)
    {
        uint32_t actual;
        auto vec = metadata<_SYSMetadataBootAsset>(_SYS_METADATA_BOOT_ASSET, &actual);
        if (!vec)
            return;

        unsigned count = actual / sizeof *vec;
        for (unsigned i = 0; i != count; ++i) {

            AssetGroup &group = groups.append();
            translate(vec[i], group);

            config.append(vec[i].slot, group, volume());
        }
    }
};


/**
 * @brief Information about the composition of the filesystem
 *
 * This object contains global information about our filesystem,
 * including its total size and amount of free space, as well as
 * space usage totals separated out by category.
 *
 * The default constructor leaves FilesystemInfo uninitialized.
 * Fill it in with up-to-date information by calling gather().
 */

class FilesystemInfo
{
public:
    _SYSFilesystemInfo sys;

    /**
     * @brief Inspects the current state of the filesystem, and fills in the
     * FilesystemInfo accordingly.
     *
     * This is a relatively heavyweight operation, similar to Volume::list()
     */
    void gather() {
        _SYS_fs_info(&sys, sizeof sys);
    }

    /**
     * @brief Return the size of one allocation unit.
     *
     * The filesystem allocates space to Volumes in relatively large units.
     * This returns the size of that unit, in bytes.
     */
    uint32_t allocationUnitSize() {
        return sys.unitSize;
    }

    /// Free space, in allocation units
    uint32_t freeUnits() {
        return sys.freeUnits;
    }

    /// Free space, in bytes
    uint32_t freeBytes() {
        return sys.freeUnits * sys.unitSize;
    }

    /// Total space, in allocation units
    uint32_t totalUnits() {
        return sys.totalUnits;
    }

    /// Total space, in bytes
    uint32_t totalBytes() {
        return sys.totalUnits * sys.unitSize;
    }

    /**
     * Space used by system data, in allocation units
     *
     * System data includes cube pairing information, asset caching data, and
     * other internal storage used to implement basic system functionality.
     * This is the same data which appears as 'system' space in `swiss manifest`.
     */
    uint32_t systemUnits() {
        return sys.systemUnits;
    }

    /// Space used by system data, in bytes
    uint32_t systemBytes() {
        return sys.systemUnits * sys.unitSize;
    }

    /// Space used by the launcher's ELF binary, in allocation units
    uint32_t launcherElfUnits() {
        return sys.launcherElfUnits;
    }

    /// Space used by the launcher's ELF binary, in bytes
    uint32_t launcherElfBytes() {
        return sys.launcherElfUnits * sys.unitSize;
    }

    /// Space used by the launcher's StoredObjects, in allocation units
    uint32_t launcherObjUnits() {
        return sys.launcherObjUnits;
    }

    /// Space used by the launcher's StoredObjects, in bytes
    uint32_t launcherObjBytes() {
        return sys.launcherObjUnits * sys.unitSize;
    }

    /// Space used by all game ELF binaries, in allocation units
    uint32_t gameElfUnits() {
        return sys.gameElfUnits;
    }

    /// Space used by all game ELF binaries, in bytes
    uint32_t gameElfBytes() {
        return sys.gameElfUnits * sys.unitSize;
    }

    /// Space used by all game StoredObjects, in allocation units
    uint32_t gameObjUnits() {
        return sys.gameObjUnits;
    }

    /// Space used by all game StoredObjects, in bytes
    uint32_t gameObjBytes() {
        return sys.gameObjUnits * sys.unitSize;
    }

    /// Space used by the current volume's ELF binary, in allocation units
    uint32_t selfElfUnits() {
        return sys.selfElfUnits;
    }

    /// Space used by the current volume's ELF binary, in bytes
    uint32_t selfElfBytes() {
        return sys.selfElfUnits * sys.unitSize;
    }

    /// Space used by the current volume's StoredObjects, in allocation units
    uint32_t selfObjUnits() {
        return sys.selfObjUnits;
    }

    /// Space used by the current volume's StoredObjects, in bytes
    uint32_t selfObjBytes() {
        return sys.selfObjUnits * sys.unitSize;
    }
};


/**
 * @} endgroup Filesystem
 */

} // namespace Sifteo
