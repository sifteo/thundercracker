/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _LOGGER_H
#define _LOGGER_H

class Logger {
 public:
    virtual ~Logger();
    virtual void taskBegin(const char *name) = 0;
    virtual void taskProgress(const char *fmt, ...) = 0;
    virtual void taskEnd() = 0;
};

class ConsoleLogger : public Logger {
 public:
    virtual ~ConsoleLogger();
    virtual void taskBegin(const char *name);
    virtual void taskProgress(const char *fmt, ...);
    virtual void taskEnd();
};

#endif

