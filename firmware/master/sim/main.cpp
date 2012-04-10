/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Entry point program for simulation use, i.e. when compiling for a
 * desktop OS rather than for the actual master cube.
 */

#include "radio.h"
#include "systime.h"
#include "audiooutdevice.h"
#include "audiomixer.h"
#include "flash.h"
#include "flashlayer.h"
#include "assetmanager.h"
#include "svmloader.h"
#include "svmcpu.h"
#include "gdbserver.h"


// XXX: Hack, for testing SVM only
static bool installElfFile(const char *path)
{
    if (!path) {
        LOG(("usage: master-sim program.elf\n"));
        return false;
    }

    FILE *elfFile = fopen(path, "rb");
    if (elfFile == NULL) {
        LOG(("couldn't open elf file, bail.\n"));
        return false;
    }

    // write the file to external flash
    uint8_t buf[512];
    Flash::chipErase();

    unsigned addr = 0;
    while (!feof(elfFile)) {
        unsigned rxed = fread(buf, 1, sizeof(buf), elfFile);
        if (rxed > 0) {
            Flash::write(addr, buf, rxed);
            addr += rxed;
        }
    }
    fclose(elfFile);
    Flash::flush();
    
    return true;
}

int main(int argc, char **argv)
{
    // handle cmd line args - arg 1 is always the elf binary to run
    for (int c = 2; c < argc; c++) {

        if (!strcmp(argv[c], "--flash_stats")) {
            LOG(("INFO: running with flash stats enabled.\n"));
            FlashBlock::enableStats();
        }

        else if (!strcmp(argv[c], "--trace")) {
            LOG(("INFO: running with SVM trace enabled.\n"));
            SvmCpu::enableTracing();
        }

        else {
            LOG(("unrecognized option, ignoring: %s.\n", argv[c]));
        }

    }

    SysTime::init();

    Flash::init();
    FlashBlock::init();
    AssetManager::init();

    if (!installElfFile(argv[1]))
        return 1;

    AudioMixer::instance.init();
    AudioOutDevice::init(AudioOutDevice::kHz16000, &AudioMixer::instance);
    AudioOutDevice::start();

    Radio::open();
    GDBServer::start(2345);

    SvmLoader::run(111);

    return 0;
}
