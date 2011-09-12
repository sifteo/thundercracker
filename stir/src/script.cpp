/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <ctype.h>
#include <string.h>

#include "script.h"

namespace Stir {

// Lua registry keys
#define REG_SCRIPT		"Stir::Script"

// Important global variables
#define GLOBAL_DEFGROUP		"_defaultGroup"
#define GLOBAL_QUALITY		"quality"

const char Group::className[] = "group";
const char Image::className[] = "image";

Lunar<Group>::RegType Group::methods[] = {
    {0,0}
};

Lunar<Image>::RegType Image::methods[] = {
    LUNAR_DECLARE_METHOD(Image, width),
    LUNAR_DECLARE_METHOD(Image, height),
    LUNAR_DECLARE_METHOD(Image, frames),
    LUNAR_DECLARE_METHOD(Image, quality),
    LUNAR_DECLARE_METHOD(Image, group),
    {0,0}
};


Script::Script(Logger &l)
    : log(l), anyOutputs(false), outputHeader(NULL),
      outputSource(NULL), outputProof(NULL)
{    
    L = lua_open();
    luaL_openlibs(L);

    // Default globals
    lua_pushinteger(L, 9);
    lua_setglobal(L, GLOBAL_QUALITY);

    // Store 'this' for use by getInstance()
    lua_pushliteral(L, REG_SCRIPT);
    lua_pushlightuserdata(L, this);
    lua_settable(L, LUA_REGISTRYINDEX);

    Lunar<Group>::Register(L);
    Lunar<Image>::Register(L);
}

Script *Script::getInstance(lua_State *L)
{
    lua_pushliteral(L, REG_SCRIPT);
    lua_gettable(L, LUA_REGISTRYINDEX);
    Script *obj = static_cast<Script*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return obj;
}

Script::~Script()
{
    lua_close(L);
}

bool Script::run(const char *filename)
{
    if (!anyOutputs)
	log.error("Warning, no output files given!");

    if (!luaRunFile(filename))
	return false;

    return true;
}

bool Script::luaRunFile(const char *filename)
{
    int s = luaL_loadfile(L, filename);

    if (!s) {
	s = lua_pcall(L, 0, LUA_MULTRET, 0);
    }

    if (s) {
	log.error("Script error: %s", lua_tostring(L, -1));
	lua_pop(L, 1);
	return false;
    }

    return true;
}

bool Script::addOutput(const char *filename)
{
    if (outputHeader == NULL && (matchExtension(filename, "h") ||
				 matchExtension(filename, "hpp"))) {
	outputHeader = filename;
	anyOutputs = true;
	return true;
    }

    if (outputSource == NULL && (matchExtension(filename, "cpp") ||
				 matchExtension(filename, "cc"))) {
	outputSource = filename;
	anyOutputs = true;
	return true;
    }

    if (outputProof == NULL && (matchExtension(filename, "html") ||
				matchExtension(filename, "htm"))) {
	outputProof = filename;
	anyOutputs = true;
	return true;
    }

    return false;
}

void Script::setVariable(const char *key, const char *value)
{
    lua_pushstring(L, value);
    lua_setglobal(L, key);
}

bool Script::matchExtension(const char *filename, const char *ext)
{
    const char *p = strrchr(filename, '.');

    if (!p)
	return false;

    for (;;) {
	p++;
	if (tolower(*p) != tolower(*ext))
	    return false;
	if (!*ext)
	    return true;
	ext++;
    }
}

bool Script::argBegin(lua_State *L, const char *className)
{
    /*
     * Before iterating over table arguments, see if the table is even valid
     */

    if (!lua_istable(L, 1)) {
	luaL_error(L, "%s{} argument is not a table. Did you use () instead of {}?",
		   className);
	return false;
    }

    return true;
}

bool Script::argMatch(lua_State *L, const char *arg)
{
    /*
     * See if we can extract an argument named 'arg' from the table.
     * If we can, returns 'true' with the named argument at the top of
     * the stack. If not, returns 'false'.
     */

    lua_pushstring(L, arg);
    lua_gettable(L, 1);

    if (lua_isnil(L, -1)) {
	lua_pop(L, 1);
	return false;
    }

    lua_pushstring(L, arg);
    lua_pushnil(L);
    lua_settable(L, 1);

    return true;
}
 
bool Script::argMatch(lua_State *L, lua_Integer arg)
{
    lua_pushinteger(L, arg);
    lua_gettable(L, 1);

    if (lua_isnil(L, -1)) {
	lua_pop(L, 1);
	return false;
    }

    lua_pushinteger(L, arg);
    lua_pushnil(L);
    lua_settable(L, 1);

    return true;
}
  
bool Script::argEnd(lua_State *L)
{
    /*
     * Finish iterating over an argument table.
     * If there are any unused arguments, turn them into errors.
     */

    bool success = true;

    lua_pushnil(L);
    while (lua_next(L, 1)) {
	lua_pushvalue(L, -2);   // Make a copy of the key object
	luaL_error(L, "Unrecognized parameter (%s)", lua_tostring(L, -1));
	lua_pop(L, 2);		// Pop value and key-copy.
	success = false;
    }

    return success;
}


Group::Group(lua_State *L)
{
    if (!Script::argBegin(L, className))
	return;

    if (Script::argMatch(L, "quality")) {
	quality = lua_tonumber(L, -1);
    } else {
	lua_getglobal(L, GLOBAL_QUALITY);
	quality = lua_tonumber(L, -1);
    }

    if (!Script::argEnd(L))
	return;

    setDefault(L);
}

void Group::setDefault(lua_State *L)
{
    /*
     * Make this the default group for now, and mark it as non-garbage
     * collected. Groups will persist for the lifetime of the Script
     * environment.
     */
    
    Lunar<Group>::push(L, this, false);
    lua_setglobal(L, GLOBAL_DEFGROUP);
}

Group *Group::getDefault(lua_State *L)
{
    lua_getglobal(L, GLOBAL_DEFGROUP);
    Group *obj = Lunar<Group>::cast(L, -1);
    lua_pop(L, 1);
    return obj;
}


Image::Image(lua_State *L)
{
    if (!Script::argBegin(L, className))
	return;

    if (Script::argMatch(L, "group")) {
	mGroup = Lunar<Group>::cast(L, -1);
	if (!mGroup) {
	    luaL_error(L, "Invalid asset group. Groups must be defined using group{}.");
	    return;
	}
    } else {
	mGroup = Group::getDefault(L);
	if (!mGroup) {
	    luaL_error(L, "Image asset defined with no group. Define a group first: MyGroup = group{}");
	    return;
	}
    }

    if (Script::argMatch(L, "quality")) {
	mQuality = lua_tonumber(L, -1);
    } else {
	mQuality = mGroup->getQuality();
    }

    if (Script::argMatch(L, "frames"))
	mImages.setFrames(lua_tointeger(L, -1));
    if (Script::argMatch(L, "width"))
	mImages.setWidth(lua_tointeger(L, -1));
    if (Script::argMatch(L, "height"))
	mImages.setHeight(lua_tointeger(L, -1));

    if (Script::argMatch(L, 1)) {
	/*
	 * The positional argument can be a single string, or a
	 * table which is interpreted as an array of strings.
	 *
	 * To handle these cases uniformly, we just trivially wrap a
	 * single string in a table if needbe.
	 */

	if (!lua_istable(L, -1)) {
	    lua_newtable(L);		// New wrapper table
	    lua_pushinteger(L, 1);	// Index for sole element (1)
	    lua_pushvalue(L, -3);	// argMatch() result
	    lua_settable(L, -3);
	}

	unsigned len = lua_objlen(L, -1);
	for (unsigned i = 1; i <= len; i++) {
	    lua_rawgeti(L, -1, i);
	    const char *filename = lua_tostring(L, -1);
	    if (!mImages.load(filename)) {	
		luaL_error(L, "Not a valid PNG image file: '%s'", filename);
		return;
	    }
	    lua_pop(L, 1);
	}

    } else {
	luaL_error(L, "%s{} requires either a single filename or a table of filenames",
		   className);
	return;
    }

    if (!Script::argEnd(L))
	return;

    mImages.finishLoading();

    if (!mImages.isConsistent()) {
	luaL_error(L, "Image dimensions are inconsistent");
	return;
    }

    if (!mImages.divisibleBy(Tile::SIZE)) {
	luaL_error(L, "Image size %dx%d is not divisible by %d-pixel tile size",
		   mImages.getWidth(), mImages.getHeight(), Tile::SIZE);
	return;
    }	
}

int Image::width(lua_State *L)
{
    lua_pushinteger(L, mImages.getWidth());
    return 1;
}

int Image::height(lua_State *L)
{
    lua_pushinteger(L, mImages.getHeight());
    return 1;
}

int Image::frames(lua_State *L)
{
    lua_pushinteger(L, mImages.getFrames());
    return 1;
}

int Image::quality(lua_State *L)
{
    lua_pushnumber(L, mQuality);
    return 1;
}

int Image::group(lua_State *L)
{
    Lunar<Group>::push(L, mGroup, false);
    return 1;
}


};  // namespace Stir
