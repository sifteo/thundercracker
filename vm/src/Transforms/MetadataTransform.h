/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#ifndef METADATA_TRANSFORM_H
#define METADATA_TRANSFORM_H

#include <stdint.h>

namespace llvm {
    
    class CallSite;
    class TargetData;

    void MetadataTransform(CallSite &CS, const TargetData *TD);

};

#endif // METADATA_TRANSFORM_H
