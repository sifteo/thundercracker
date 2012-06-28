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
    LOG("Testing object store record / replay\n");

    static StoredObject foo = StoredObject::allocate();
    static StoredObject bar = StoredObject::allocate();
    static StoredObject wub = StoredObject::allocate();
    static StoredObject old = StoredObject::allocate();

    ASSERT(foo != bar);
    ASSERT(foo != wub);
    ASSERT(bar != wub);
    ASSERT(old != foo);
    ASSERT(old != bar);
    ASSERT(old != wub);

    // Write a large object, to use up space faster
    static struct {
        int value;
        uint8_t pad[4000];
    } obj;
    memset(obj.pad, 0xff, sizeof obj.pad);

    // Keep a single old value in the LFS, which we'll expect to remain there.
    // (This tests relocation of non-obsolete keys during garbage collection)
    obj.value = 0x12345678;
    old.write(obj);

    SCRIPT(LUA, logger = FlashLogger:start(fs, "flash.log"));

    /*
     * Write some interleaved values, in random order,
     * being sure to use enough space that we'll hit various
     * garbage collection and index overflow edge cases.
     */

    Random r(123);
    const int lastValue = 5000;

    for (int value = 0; value <= lastValue; value++) {
        obj.value = value;
        SCRIPT_FMT(LUA, "writeTotal = writeTotal + %d", sizeof obj);

        switch (r.randrange(3)) {
            case 0: foo.write(obj); break;
            case 1: bar.write(obj); break;
            case 2: wub.write(obj); break;
        }
    }

    /*
     * All writes that happen during logging are reverted
     * in logger:stop(). Make sure the objects are no longer found.
     */

    ASSERT(sizeof obj == foo.read(obj));
    ASSERT(sizeof obj == bar.read(obj));
    ASSERT(sizeof obj == wub.read(obj));

    SCRIPT(LUA, logger:stop());

    ASSERT(0 == foo.read(obj));
    ASSERT(0 == bar.read(obj));
    ASSERT(0 == wub.read(obj));
    
    /*
     * Now check that as we replay the log, we see the events occur
     * in monotonically increasing order.
     */

    SCRIPT(LUA, player = FlashReplay:start(fs, "flash.log"));

    int lastFoo = -1;
    int lastBar = -1;
    int lastWub = -1;

    for (unsigned i = 0;; ++i) {
        SCRIPT(LUA,
            if player:play(1) < 1 then
                error("Finished replaying the log without finding our last object!")
            end
        );
        
        obj.value = -1;
        old.read(obj);
        ASSERT(obj.value == 0x12345678);

        obj.value = -1;
        foo.read(obj);
        ASSERT(obj.value >= lastFoo);
        if (lastValue == (lastFoo = obj.value))
            break;

        obj.value = -1;
        bar.read(obj);
        ASSERT(obj.value >= lastBar);
        if (lastValue == (lastBar = obj.value))
            break;

        obj.value = -1;
        wub.read(obj);
        ASSERT(obj.value >= lastWub);
        if (lastValue == (lastWub = obj.value))
            break;
    }

    SCRIPT(LUA, player:stop());
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

    // Now start flooding the FS with object writes
    createObjects();

    // Run all of the pure Lua tests (no API exercise needed)
    SCRIPT(LUA, testFilesystem());

    // This leaves a bunch of test volumes (Type 0x8765) in the filesystem.
    // We're guaranteed to see at least one of these.
    checkTestVolumes();

    // Back to Lua, let it check whether our wear levelling has been working
    SCRIPT(LUA, dumpAndCheckFilesystem());

    LOG("Success.\n");
}
