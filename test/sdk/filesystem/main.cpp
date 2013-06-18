/*
 * Unit tests for the flash filesystem implementation and APIs.
 */

#include <sifteo.h>
using namespace Sifteo;

static Metadata M = Metadata::Metadata()
    .title("Filesystem Test");

static Random rand(1234);

static struct {
    int value;
    uint8_t pad[1024];
} objBuffer;


struct ObjectFlavor
{
    ObjectFlavor(float probability)
        : key(StoredObject::allocate()), probability(probability), writeValue(-1), readValue(-1) {}

    void write()
    {
        if (writeValue < 0 || rand.chance(probability)) {
            objBuffer.value = ++writeValue;
            key.write(objBuffer);
            SCRIPT_FMT(LUA, "writeTotal = writeTotal + %d", sizeof objBuffer);

            key.read(objBuffer);
            if (objBuffer.value != writeValue) {
                LOG("--- Failed to read immediately after write to key 0x%02x. Expected %d, read %d\n",
                    key.sys, writeValue, objBuffer.value);
                ASSERT(0);
            }
        }
    }

    void read(int logLine)
    {
        objBuffer.value = -1;
        key.read(objBuffer);
        if (objBuffer.value != readValue && objBuffer.value != readValue + 1) {
            LOG("--- Replay mismatch on key 0x%02x, log line %d. Expected %d, read %d\n",
                key.sys, logLine, readValue, objBuffer.value);
            ASSERT(0);
        }
        readValue = objBuffer.value;
    }

    bool done()
    {
        return readValue == writeValue;
    }

    bool found()
    {
        return key.read(objBuffer) == sizeof objBuffer;
    }

    StoredObject key;
    float probability;
    int writeValue;
    int readValue;
};


// Write a large object, to use up space faster
static struct {
    int value;
    uint8_t pad[1024];
} obj;


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
    ASSERT(Volume(0) == Volume::previous());

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

    // To effectively test LFS, we need objects with varying frequencies
    ObjectFlavor foo(1.00);
    ObjectFlavor bar(0.10);
    ObjectFlavor wub(0.01);
    ObjectFlavor qux(0.001);

    SCRIPT(LUA,
        saveFlashSnapshot(fs, "flash.snapshot")
        logger = FlashLogger:start(fs, "flash.log")
    );

    /*
     * Write some interleaved values, in random order,
     * being sure to use enough space that we'll hit various
     * garbage collection and index overflow edge cases.
     */

    LOG("Recording values...\n");

    for (unsigned i = 0; i < 10000; i++) {
        foo.write();
        bar.write();
        wub.write();
        qux.write();

        System::keepAwake();
    }

    /*
     * All writes that happen during logging are reverted
     * in logger:stop(). Make sure the objects are no longer found.
     */

    ASSERT(foo.found());
    ASSERT(bar.found());
    ASSERT(wub.found());
    ASSERT(qux.found());

    SCRIPT(LUA,
        logger:stop()
        loadFlashSnapshot(fs, "flash.snapshot")
    );

    ASSERT(!foo.found());
    ASSERT(!bar.found());
    ASSERT(!wub.found());
    ASSERT(!qux.found());

    /*
     * Now check that as we replay the log, we see the events occur
     * in monotonically increasing order.
     */

    SCRIPT(LUA,
        player = FlashReplay:start(fs, "flash.log")
    );

    LOG("Replaying log...\n");

    for (unsigned i = 1; !(foo.done() && bar.done() && wub.done() && qux.done()); ++i) {

        SCRIPT(LUA,
            if player:play(1) < 1 then
                error("Finished replaying the log without finding our last object!")
            end
        );

        foo.read(i);
        bar.read(i);
        wub.read(i);
        qux.read(i);

        if ((i % 500) == 0) {
            LOG("%5d: %02x:%d/%d %02x:%d/%d %02x:%d/%d %02x:%d/%d\n", i,
                foo.key.sys, foo.readValue, foo.writeValue,
                bar.key.sys, bar.readValue, bar.writeValue,
                wub.key.sys, wub.readValue, wub.writeValue,
                qux.key.sys, qux.readValue, qux.writeValue);
        }

        System::keepAwake();
    }

    SCRIPT(LUA, player:stop());
}

void testFsInfo()
{  
    // Short reads
    static _SYSFilesystemInfo info;
    info.totalUnits = 0xABCDEF;
    ASSERT(5 == _SYS_fs_info(&info, 5));
    ASSERT(info.unitSize == 128*1024);
    ASSERT(info.totalUnits == 0xABCDFE);

    // Long reads
    ASSERT(sizeof info == _SYS_fs_info(&info, 0x800));

    // Dump out contents
    LOG_INT(info.unitSize);
    LOG_INT(info.totalUnits);
    LOG_INT(info.freeUnits);
    LOG_INT(info.systemUnits);
    LOG_INT(info.launcherElfUnits);
    LOG_INT(info.launcherObjUnits);
    LOG_INT(info.gameElfUnits);
    LOG_INT(info.gameObjUnits);
    LOG_INT(info.selfElfUnits);
    LOG_INT(info.selfObjUnits);

    FilesystemInfo fi;
    fi.gather();

    ASSERT(info.unitSize == fi.allocationUnitSize());
    ASSERT(info.selfElfUnits == fi.selfElfUnits());
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

    // Test _SYS_fs_info() a bit
    testFsInfo();

    LOG("Success.\n");
}
