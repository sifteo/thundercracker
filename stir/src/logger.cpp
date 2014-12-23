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

#ifndef WIN32
#   include <unistd.h>
#endif

#include <stdio.h>
#include <stdarg.h>
#include <algorithm>

#include "logger.h"

namespace Stir {

Logger::~Logger() {}

ConsoleLogger::~ConsoleLogger() {}

ConsoleLogger::ConsoleLogger()
    : mVerbose(false), mNeedNewline(false), mIsTTY(true), mLabelWidth(10)
{
#ifndef WIN32
    mIsTTY = isatty(2);
#endif
}

void ConsoleLogger::setVerbose(bool verbose)
{
    mVerbose = verbose;
}

void ConsoleLogger::heading(const char *name)
{
    if (mVerbose)
        fprintf(stderr, "======== %s ========\n", name);
}

void ConsoleLogger::taskBegin(const char *name)
{
    if (mVerbose)
        fprintf(stderr, "%s...\n", name);
}

void ConsoleLogger::taskProgress(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);

    if (mVerbose) {
        char line[1024];
        line[0] = '\t';
        vsnprintf(line + 1, sizeof line - 2, fmt, ap);
        line[sizeof line - 1] = 0;

        if (mIsTTY) {
            // Show progress incrementally, erasing the line with \r.
            fprintf(stderr, "\r%s    ", line);
            mNeedNewline = true;
        } else {
            // Save only the last progress line
            mLastProgressLine = line;
        }
    }

    va_end(ap);
}

void ConsoleLogger::taskEnd()
{
    if (!mLastProgressLine.empty()) {
        fprintf(stderr, "%s\n", mLastProgressLine.c_str());
        mLastProgressLine = "";
    }

    if (mNeedNewline) {
        fprintf(stderr, "\n");
        mNeedNewline = false;
    }
}

void ConsoleLogger::infoBegin(const char *name)
{
    if (mVerbose)
        fprintf(stderr, "%s:\n", name);
}

void ConsoleLogger::infoLine(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);

    if (mVerbose) {
        fprintf(stderr, "  ");
        vfprintf(stderr, fmt, ap);
        fprintf(stderr, "\n");
    }

    va_end(ap);
}

void ConsoleLogger::infoLineWithLabel(const char *label, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);

    if (mVerbose) {
        fprintf(stderr, " %*s: ", mLabelWidth, label);
        vfprintf(stderr, fmt, ap);
        fprintf(stderr, "\n");
    }

    va_end(ap);
}

void ConsoleLogger::infoEnd()
{}

void ConsoleLogger::error(const char *fmt, ...)
{
    va_list ap;

    if (mNeedNewline) {
        fprintf(stderr, "\n");
        mNeedNewline = false;
    }

    va_start(ap, fmt);
    fprintf(stderr, "-!- ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
}

void ConsoleLogger::setMinLabelWidth(unsigned width)
{
    mLabelWidth = std::max(mLabelWidth, width);
}

};  // namespace Stir
