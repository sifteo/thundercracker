/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * This file is part of the public interface to the Sifteo SDK.
 * Copyright <c> 2012 Sifteo, Inc. All rights reserved.
 */

/*
 * Definition of the Application Binary Interface for Sifteo games.
 *
 * Whereas the rest of the SDK is effectively a layer of malleable
 * syntactic sugar, this defines the rigid boundary between a game and
 * its execution environment. Everything in this file posesses a
 * binary compatibility guarantee.
 *
 * The ABI is defined in plain C, and all symbols are namespaced with
 * '_SYS' so that it's clear they aren't meant to be used directly by
 * game code.
 */

#ifndef _SIFTEO_ABI_H
#define _SIFTEO_ABI_H

#include <sifteo/abi/syscall.h>
#include <sifteo/abi/types.h>
#include <sifteo/abi/vram.h>
#include <sifteo/abi/audio.h>
#include <sifteo/abi/asset.h>
#include <sifteo/abi/events.h>
#include <sifteo/abi/elf.h>

#endif

