/*
 * This file is part of the Sifteo VM (SVM) Target for LLVM
 *
 * M. Elizabeth Scott <beth@sifteo.com>
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

#include "SVMTargetMachine.h"
using namespace llvm;

VMSelectionDAGInfo::SVMSelectionDAGInfo(const SVMTargetMachine &TM)
    : TargetSelectionDAGInfo(TM) {}

SVMSelectionDAGInfo::~SVMSelectionDAGInfo() {}
