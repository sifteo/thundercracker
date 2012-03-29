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

#include <vector>
#include <set>

#include "lunar.h"
#include "logger.h"
#include "tile.h"
#include "imagestack.h"

namespace Stir {

class Group;
class Image;
class Sound;

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

    bool addOutput(const char *filename);
    void setVariable(const char *key, const char *value);

 private:
    lua_State *L;
    Logger &log;

    bool anyOutputs;
    const char *outputHeader;
    const char *outputSource;
    const char *outputProof;

    std::set<Group*> groups;
    std::set<Sound*> sounds;

    friend class Group;
    friend class Image;
    friend class Sound;

    bool luaRunFile(const char *filename);
    void collect();

    static bool matchExtension(const char *filename, const char *ext);

    // Utilities for foolproof table argument unpacking
    static bool argBegin(lua_State *L, const char *className);
    static bool argMatch(lua_State *L, const char *arg);
    static bool argMatch(lua_State *L, lua_Integer arg);
    static bool argEnd(lua_State *L);
};


/*
 * Group --
 *
 *    Scriptable object for an asset group. Asset groups are
 *    independently-loadable sets of assets which are optimized and
 *    deployed as a single unit.
 */

class Group {
public:
    static const char className[];
    static Lunar<Group>::RegType methods[];

    Group(lua_State *L);

    lua_Number getQuality() const {
        return quality;
    }

    const TilePool &getPool() const {
        return pool;
    }

    TilePool &getPool() {
        return pool;
    }

    void setName(const char *s) {
        mName = s;
    }

    const std::string &getName() const {
        return mName;
    }

    void addImage(Image *i) {
        mImages.insert(i);
    }

    const std::set<Image*> &getImages() const {
        return mImages;
    }

    std::vector<uint8_t> &getLoadstream() {
        return mLoadstream;
    }

    const std::vector<uint8_t> &getLoadstream() const {
        return mLoadstream;
    }

    void setDefault(lua_State *L);
    static Group *getDefault(lua_State *L);
    uint64_t getSignature() const;

private:
    lua_Number quality;
    TilePool pool;
    std::string mName;
    std::set<Image*> mImages;
    std::vector<uint8_t> mLoadstream;
};


/*
 * Image --
 *
 *    Scriptable object for an image asset. Images are a very generic
 *    asset type, which can refer to a stack of one or more individual
 *    bitmaps which are converted to tiles either using a map or using
 *    linear assignment.
 */

class Image {
public:
    static const char className[];
    static Lunar<Image>::RegType methods[];

    Image(lua_State *L);

    void setName(const char *s) {
        mName = s;
    }

    const std::string &getName() const {
        return mName;
    }

    Group *getGroup() const {
        return mGroup;
    }

    const std::vector<TileGrid> &getGrids() const {
        return mGrids;
    }

    bool isPinned() const {
        return mTileOpt.pinned;
    }
    
    bool isFlat() const {
        return mIsFlat;
    }

    const char *getClassName() const;

    uint16_t encodePinned() const;
    void encodeFlat(std::vector<uint16_t> &data) const;
    bool encodeDUB(std::vector<uint16_t> &data, Logger &log) const;

 private:
    Group *mGroup;
    ImageStack mImages;
    TileOptions mTileOpt;
    std::vector<TileGrid> mGrids;
    std::string mName;
    bool mIsFlat;

    void createGrids();

    int width(lua_State *L);
    int height(lua_State *L);
    int frames(lua_State *L);
    int quality(lua_State *L);
    int group(lua_State *L);
};

class Sound {
public:
    static const char className[];
    static Lunar<Sound>::RegType methods[];

    Sound(lua_State *L);
    
    void setName(const char *s) {
        mName = s;
    }
    
    void setEncode(const std::string &encode)
    {
        mEncode = encode;
    }
    
    void setQuality(int quality)
    {
        mQuality = quality;
    }
    
    void setVBR(bool vbr)
    {
        mVBR = vbr;
    }

    const std::string &getName() const {
        return mName;
    }
    
    const std::string &getFile() const {
        return mFile;
    }
    
    const std::string &getEncode() const {
        return mEncode;
    }
    
    const int getQuality() const {
        return mQuality;
    }
    
    const bool getVBR() const {
        return mVBR;
    }
    
private:
    std::string mName;
    std::string mFile;
    std::string mEncode;
    int mQuality;
    bool mVBR;
};


};  // namespace Stir

#endif
