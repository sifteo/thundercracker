/*
 * Unit tests for the firmware's flash filesystem.
 */

#include <sifteo/macros.h>
#include <sifteo/abi.h>


void checkTestVolumes()
{
    _SYSVolumeHandle volumes[16];
    unsigned numVolumes = _SYS_fs_listVolumes(0x8765, volumes, arraysize(volumes));

    ASSERT(numVolumes >= 1);
    ASSERT(volumes[0] != 0);
    ASSERT((volumes[0] & 0xFF) != 0);
}

void checkSelf()
{
    _SYSVolumeHandle volumes[64];
    unsigned numVolumes = _SYS_fs_listVolumes(_SYS_FS_VOL_GAME, volumes, arraysize(volumes));

    for (unsigned i = 0; i < numVolumes; ++i) {
        _SYSVolumeHandle vol = volumes[i];

        LOG("Visiting volume 0x%08x\n", vol);
    }
}

void main()
{
    // Initialization
    SCRIPT(LUA,
        package.path = package.path .. ";../../lib/?.lua"
        require('test-filesystem')
    );

    // Do some tests on our own binary
    checkSelf();

    // Run all of the pure Lua tests (no API exercise needed)
    SCRIPT(LUA, testFilesystem());

    // This leaves a bunch of test volumes (Type 0x8765) in the filesystem.
    // We're guaranteed to see at least one of these.
    checkTestVolumes();
}
