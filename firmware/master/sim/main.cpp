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

int main(int argc, char **argv)
{
    SysTime::init();

    AudioMixer::instance.init();
    AudioOutDevice::init(AudioOutDevice::kHz16000, &AudioMixer::instance);
    AudioOutDevice::start();

    Flash::init();
    AssetManager::init();
    installAssetsToMaster();    // normally this would happen over USB

    FlashLayer::init();

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
    while (!feof(file)) {
        unsigned rxed = fread(buf, 1, sizeof(buf), file);
        AssetManager::onData(buf, rxed);
    }
    fclose(file);

    Flash::flush();
}
