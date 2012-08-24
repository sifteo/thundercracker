/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVMVALIDATOR_H
#define SVMVALIDATOR_H

#include <stdint.h>

namespace SvmValidator {
    unsigned findValidBundles(const uint32_t *block);
}

#endif // SVMVALIDATOR_H
