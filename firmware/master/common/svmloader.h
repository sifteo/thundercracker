/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVM_LOADER_H
#define SVM_LOADER_H

#include <stdint.h>
#include <inttypes.h>


class SvmLoader {
public:
    SvmLoader();  // Do not implement

    static void run(uint16_t appId);
    static void exit();
};


#endif // SVM_LOADER_H
