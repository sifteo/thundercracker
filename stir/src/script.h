/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#ifndef _SCRIPT_H
#define _SCRIPT_H

extern "C" {
#   include "lua/lua.h"
#   include "lua/lauxlib.h"
#   include "lua/lualib.h"
}

#include "logger.h"

namespace Stir {

/*
 * Script --
 *
 *    STIR is configured via a script file that defines all assets.
 *    This object manages the execution of a STIR script using an
 *    embedded Lua interpreter.
 */

class Script {
public:
    Script(Logger &l);
    ~Script();

    bool run(const char *filename);
    void setVariable(const char *key, const char *value);

 private:
    lua_State *L;
    Logger &log;
};

};  // namespace Stir

#endif
