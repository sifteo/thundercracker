/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the internal implementation of the Sifteo SDK.
 * Confidential, not for redistribution.
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

/*
 * Entry point program for simulation use, i.e. when compiling for a
 * desktop OS rather than for the actual master cube.
 */

#include <sifteo.h>
#include "radio.h"
#include "runtime.h"
#include "systime.h"
#include "audiooutdevice.h"
#include "audiomixer.h"
#include "flash.h"
#include "flashlayer.h"
#include "assetmanager.h"

static void installAssetsToMaster();

//#define SVM_TEST

#ifdef SVM_TEST
#include "svm.h"

static void installElfFile()
{
    FILE *elfFile = fopen("../../vm/program.elf", "rb");
    if (elfFile == NULL) {
        LOG(("couldn't open elf file, bail.\n"));
        return;
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
}
#endif // SVM_TEST

int main(int argc, char **argv)
{
    SysTime::init();

    Flash::init();
    FlashLayer::init();
    AssetManager::init();

#ifdef SVM_TEST
    installElfFile();
    SvmProgram program;
    program.run(111);
    return 0;
#endif

    installAssetsToMaster();    // normally this would happen over USB

    AudioMixer::instance.init();
    AudioOutDevice::init(AudioOutDevice::kHz16000, &AudioMixer::instance);
    AudioOutDevice::start();

    Radio::open();

    Runtime::run();

    return 0;
}

/*
    Simulate installing assets to the master over USB.
    For now, we just feed data to AssetManager.
*/
void installAssetsToMaster()
{
    FILE *file = fopen("asegment.bin", "rb");
    if (file == NULL) {
        LOG(("INFO: running without asegment.bin\n"));
        return;
    }

    fseek(file , 0 , SEEK_END);
    unsigned fsize = ftell(file);
    rewind(file);

    AssetManager::onData((const uint8_t*)&fsize, sizeof(fsize));

    uint8_t buf[64];    // 64 bytes == USB max packet size
    while (fsize) {
        unsigned chunk = MIN(fsize, sizeof(buf));
        unsigned rxed = fread(buf, 1, chunk, file);
        ASSERT(rxed == chunk);
        AssetManager::onData(buf, chunk);
        fsize -= chunk;
    }
    fclose(file);

    Flash::flush();
}
