/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _LOGGER_H
#define _LOGGER_H

namespace Stir {

class Logger {
 public:
    virtual ~Logger();

    virtual void taskBegin(const char *name) = 0;
    virtual void taskProgress(const char *fmt, ...) = 0;
    virtual void taskEnd() = 0;

    virtual void infoBegin(const char *name) = 0;
    virtual void infoLine(const char *fmt, ...) = 0;
    virtual void infoEnd() = 0;

    virtual void error(const char *fmt, ...) = 0;
};

class ConsoleLogger : public Logger {
 public:
    virtual ~ConsoleLogger();

    void setVerbose(bool verbose=true);

    virtual void taskBegin(const char *name);
    virtual void taskProgress(const char *fmt, ...);
    virtual void taskEnd();

    virtual void infoBegin(const char *name);
    virtual void infoLine(const char *fmt, ...);
    virtual void infoEnd();

    virtual void error(const char *fmt, ...);

 private:
    bool mVerbose;
};

};  // namespace Stir

#endif

