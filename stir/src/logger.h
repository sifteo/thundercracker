/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * M. Elizabeth Scott <beth@sifteo.com>
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

#ifndef _LOGGER_H
#define _LOGGER_H

#include <string>

namespace Stir {

class Logger {
 public:
    virtual ~Logger();

    virtual void heading(const char *name) = 0;

    virtual void taskBegin(const char *name) = 0;
    virtual void taskProgress(const char *fmt, ...) = 0;
    virtual void taskEnd() = 0;

    virtual void infoBegin(const char *name) = 0;
    virtual void infoLine(const char *fmt, ...) = 0;
    virtual void infoLineWithLabel(const char *label, const char *fmt, ...) = 0;
    virtual void infoEnd() = 0;

    virtual void error(const char *fmt, ...) = 0;

    virtual void setMinLabelWidth(unsigned width) = 0;
};

class ConsoleLogger : public Logger {
 public:
    ConsoleLogger();
    virtual ~ConsoleLogger();

    void setVerbose(bool verbose=true);

    virtual void heading(const char *name);

    virtual void taskBegin(const char *name);
    virtual void taskProgress(const char *fmt, ...);
    virtual void taskEnd();

    virtual void infoBegin(const char *name);
    virtual void infoLine(const char *fmt, ...);
    virtual void infoLineWithLabel(const char *label, const char *fmt, ...);
    virtual void infoEnd();

    virtual void error(const char *fmt, ...);

    virtual void setMinLabelWidth(unsigned width);

 private:
    bool mVerbose;
    bool mNeedNewline;
    bool mIsTTY;
    unsigned mLabelWidth;
    std::string mLastProgressLine;
};

};  // namespace Stir

#endif

