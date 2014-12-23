/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * Micah Elizabeth Scott <micah@misc.name>
 *
 * Copyright <c> 2011 Sifteo, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
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
    bool writeGroup(const Group &group);
    bool writeSound(const Sound &sound);
    void writeTrackerShared(const Tracker &tracker);
    void writeTracker(const Tracker &tracker);
    void writeImageList(const ImageList& images);

 private:
    void writeImage(const Image &image, bool writeDecl=true, bool writeAsset=true, bool writeData=true);

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
    void writeTracker(const Tracker &tracker);
    void writeImageList(const ImageList &list);

 protected:
    void head();
};


};  // namespace Stir

#endif
