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
#include "sifteo/abi.h"
#include "tracker.h"

#include <iostream>

namespace Stir {

class Group;
class Image;
class ImageList;
class Sound;
class Tracker;


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
    std::set<Tracker*> trackers;
    std::set<Sound*> sounds;
    std::vector<ImageList> imageLists;


    friend class Group;
    friend class Image;
    friend class Sound;
    friend class Tracker;

    bool luaRunFile(const char *filename);
    bool collect();
    bool collectList(const char* name, int tableStackIndex);

    static bool matchExtension(const char *filename, const char *ext);

    // Utilities for foolproof table argument unpacking
    static bool argBegin(lua_State *L, const char *className);
    static bool argMatch(lua_State *L, const char *arg);
    static bool argMatch(lua_State *L, lua_Integer arg);
    static bool argEnd(lua_State *L);

    static _SYSAudioLoopType toLoopType(lua_State *L, int index);
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
    
    bool isFixed() const {
        return fixed;
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

private:
    lua_Number quality;
    TilePool pool;
    bool fixed;
    std::string mName;
    std::set<Image*> mImages;
    std::vector<uint8_t> mLoadstream;

    static const uint8_t gf84[];
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

    void setName(std::string s) {
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

    bool inList() const {
        return mInList;
    }

    void setInList(bool flag) {
        mInList = flag;
    }

    const char *getClassName() const;

    uint16_t encodePinned() const;
    void encodeFlat(std::vector<uint16_t> &data) const;
    bool encodeDUB(std::vector<uint16_t> &data, Logger &log, std::string &format) const;

 private:
    Group *mGroup;
    ImageStack mImages;
    TileOptions mTileOpt;
    std::vector<TileGrid> mGrids;
    std::string mName;
    bool mIsFlat;
    bool mInList;

    void createGrids();

    int width(lua_State *L);
    int height(lua_State *L);
    int frames(lua_State *L);
    int quality(lua_State *L);
    int group(lua_State *L);
};


/*
 * ImageList --
 * 
 *      A wrapper for a list of images.  ImagesLists are created
 *      from global variables bound to homogeneous tables of image
 *      instances (that is, it contains no non-image elements, and each
 *      element has the same C++ result type).
 */

class ImageList {
public:
    typedef std::vector<Image*>::const_iterator const_iterator;

public:
    ImageList(std::string name, std::vector<Image*> images) : 
        mName(name), mImages(images) {}

    const std::string &getName() const {
        return mName;
    }

    const_iterator begin() const {
        return mImages.begin();
    }

    const_iterator end() const {
        return mImages.end();
    }

    unsigned size() const {
        return mImages.size();
    }

    const char* getImageClassName() const {
        return mImages[0]->getClassName();
    }

private:
    std::string mName;
    std::vector<Image*> mImages;
};


class Sound {
public:
    static const char className[];
    static Lunar<Sound>::RegType methods[];
    static const uint32_t UNSPECIFIED_SAMPLE_RATE = 0;
    static const uint32_t STANDARD_SAMPLE_RATE = 16000;

    Sound(lua_State *L);
    
    void setName(const char *s) {
        mName = s;
    }
    
    void setEncode(const std::string &encode)
    {
        mEncode = encode;
    }
    
    void setSampleRate(uint32_t sample_rate)
    {
        mSampleRate = sample_rate;
    }

    void setLoopStart(uint32_t loop_start)
    {
        mLoopStart = loop_start;
    }

    void setLoopLength(uint32_t loop_length)
    {
        mLoopLength = loop_length;
    }

    void setLoopType(_SYSAudioLoopType loop_type)
    {
        mLoopType = loop_type;
    }

    void setVolume(uint16_t volume)
    {
        mVolume = volume;
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

    const uint32_t getSampleRate() const {
        return mSampleRate;
    }

    const uint32_t getLoopStart() const {
        return mLoopStart;
    }
    
    const uint32_t getLoopLength() const {
        return mLoopLength;
    }

    const _SYSAudioLoopType getLoopType() const {
        return mLoopType;
    }

    const uint16_t getVolume() const {
        return mVolume;
    }

private:
    std::string mName;
    std::string mFile;
    std::string mEncode;
    uint32_t mSampleRate;
    uint32_t mLoopStart;
    uint32_t mLoopLength;
    uint16_t mVolume;
    _SYSAudioLoopType mLoopType;
};

class Tracker {
public:
    static const char className[];
    static Lunar<Tracker>::RegType methods[];

    Tracker(lua_State *L);

    void setName(const char *s) {
        mName = s;
    }

    const std::string &getName() const {
        return mName;
    }

    const std::string &getFile() const {
        return mFile;
    }

    const _SYSXMSong &getSong() const {
        assert(loader.song.nPatterns);
        return loader.song;
    }
    const _SYSXMPattern &getPattern(uint8_t i) const {
        assert(i < loader.song.nPatterns);
        return loader.patterns[i];
    }
    const std::vector<uint8_t> &getPatternData(uint8_t i) const {
        assert(i < loader.patternDatas.size());
        return loader.patternDatas[i];
    }
    // const uint32_t numPatternDatas() const {
    //     return loader.globalSampleDatas.size();
    // }

    const std::vector<uint8_t> &getPatternTable() const {
        return loader.patternTable;
    }
    const _SYSXMInstrument &getInstrument(uint8_t i) const {
        assert(i < loader.song.nInstruments);
        return loader.instruments[i];
    }
    const size_t numInstruments() const {
        return loader.song.nInstruments;
    }
    const std::vector<uint8_t> &getEnvelope(uint8_t i) const {
        assert(i < loader.envelopes.size());
        return loader.envelopes[i];
    }
    // const uint32_t numEnvelopes() const {
    //     return loader.globalEnvelopes.size();
    // }
    const std::vector<uint8_t> &getSample(uint8_t i) const {
        assert(i < loader.globalSampleDatas.size());
        return loader.globalSampleDatas[i];
    }
    const uint32_t numSamples() const {
        return loader.globalSampleDatas.size();
    }

    const uint32_t getFileSize() const {
        return loader.fileSize;
    }
    const uint32_t getSize() const {
        return loader.size;
    }

    // Returns the type of looping, as a _SYS_LOOP_* constant
    _SYSAudioLoopType getLoopType() const {
        return loopType;
    }
    
    // Get the song's restartPosition, modified by our current loopType.
    int getRestartPosition() const {
        if (getLoopType() == _SYS_LOOP_ONCE)
            return -1;
        else
            return (unsigned) loader.song.restartPosition;
    }

private:
    friend class Script;
    friend class XmTrackerLoader;

    std::string mName;
    std::string mFile;
    XmTrackerLoader loader;

    _SYSAudioLoopType loopType;
};

};  // namespace Stir

#endif
