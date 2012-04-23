/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef LOG_TRANSFORM_H
#define LOG_TRANSFORM_H

#include <stdint.h>

namespace llvm {
    
    class CallSite;

    void LogTransform(CallSite &CS, uint32_t flags = 0);

};

#endif // LOG_TRANSFORM_H
