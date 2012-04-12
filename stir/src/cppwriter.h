/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _CPPWRITER_H
#define _CPPWRITER_H

#include <stdint.h>
#include <fstream>

#include "tile.h"
#include "script.h"
#include "logger.h"

namespace Stir {


/*
 * CPPWriter --
 *
 *     Base class for a C++ source generator.
 */

class CPPWriter {
 public:
    CPPWriter(Logger &log, const char *filename);

    void close();

 protected:
    static const char *indent;
    Logger &mLog;
    std::ofstream mStream;

    void head();
    virtual void foot();

    void writeArray(const std::vector<uint8_t> &data);
    void writeArray(const std::vector<uint16_t> &data);
    void writeString(const std::vector<uint8_t> &data);
};


/*
 * CPPSourceWriter --
 *
 *     Writer for C++ source files containing data definitions
 *     to be linked into the final game binary.
 */

class CPPSourceWriter : public CPPWriter {
 public:
    CPPSourceWriter(Logger &log, const char *filename);
    void writeGroup(const Group &group);
    void writeSound(const Sound &sound);

 private:
    void writeImage(const Image &image);
    unsigned nextGroupOrdinal;
};


/*
 * CPPHeaderWriter --
 *
 *     Writer for C++ header files containing metadata and inline
 *     values to be used while compiling a game.
 */

class CPPHeaderWriter : public CPPWriter {
 public:
    CPPHeaderWriter(Logger &log, const char *filename);
    void writeGroup(const Group &group);
    void writeSound(const Sound &sound);

 protected:
    void head();
    virtual void foot();

 private:
    std::string guardName;

    void createGuardName(const char *filename);
};


};  // namespace Stir

#endif
