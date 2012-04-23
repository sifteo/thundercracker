/*
 * Thundercracker Firmware -- Confidential, not for redistribution.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef SVMVALIDATOR_H
#define SVMVALIDATOR_H

#include <stdint.h>

class SvmValidator
{
public:
    SvmValidator();  // Do not implement

    static unsigned validBytes(void *block, unsigned lenInBytes);

private:
    static bool isValid16(uint16_t instr);
    static bool isValid32(uint32_t instr);
};

#endif // SVMVALIDATOR_H
