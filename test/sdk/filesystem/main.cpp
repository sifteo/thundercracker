/*
 * Unit tests for the flash filesystem implementation and APIs.
 */

#include <sifteo.h>
using namespace Sifteo;

static Metadata M = Metadata::Metadata()
    .title("Filesystem Test");


void checkTestVolumes()
{
    Array<Volume, 16> volumes;
    Volume::list(0x8765, volumes);

    ASSERT(volumes.count() >= 1);
    ASSERT(volumes[0].sys != 0);
    ASSERT((volumes[0].sys & 0xFF) != 0);
}

void checkSelf()
{
    Array<Volume, 16> games;
    Volume::list(Volume::T_GAME, games);

    Array<Volume, 16> launchers;
    Volume::list(Volume::T_LAUNCHER, launchers);

    ASSERT(games.count() == 0);
    ASSERT(launchers.count() == 1);
    ASSERT(launchers[0] == Volume::running());

    // Test our metadata, and some string ops
    MappedVolume self(Volume::running());
    String<11> buf;
    buf << self.title();
    ASSERT(buf == "Filesystem");
    ASSERT(buf < "Filfsystem");
    ASSERT(buf >= "Filesyste");
}

void createObjects()
{
    static StoredObject foo = StoredObject::allocate();
    static StoredObject bar = StoredObject::allocate();
    static StoredObject wub = StoredObject::allocate();

    ASSERT(foo != bar);
    ASSERT(foo != wub);
    ASSERT(bar != wub);
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

    // Now start flooding the FS with object writes
    createObjects();

    // Back to Lua, let it check whether our wear levelling has been working
    SCRIPT(LUA, dumpAndCheckFilesystem());

    LOG("Success.\n");
}
