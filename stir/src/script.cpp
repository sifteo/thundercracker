/* -*- mode: C; c-basic-offset: 4; intent-tabs-mode: nil -*-
 *
 * STIR -- Sifteo Tiled Image Reducer
 * M. Elizabeth Scott <beth@sifteo.com>
 *
 * Copyright <c> 2011 Sifteo, Inc. All rights reserved.
 */

#include <ctype.h>
#include <string.h>

#include "sha.h"
#include "script.h"
#include "proof.h"
#include "cppwriter.h"
#include "audioencoder.h"
#include "dubencoder.h"

namespace Stir {

// Important global variables
#define GLOBAL_DEFGROUP         "_defaultGroup"
#define GLOBAL_QUALITY          "quality"

const char Group::className[] = "group";
const char Image::className[] = "image";
const char Sound::className[] = "sound";

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

Lunar<Sound>::RegType Sound::methods[] = {
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

    Lunar<Group>::Register(L);
    Lunar<Image>::Register(L);
    Lunar<Sound>::Register(L);
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

    collect();

    ProofWriter proof(log, outputProof);
    CPPHeaderWriter header(log, outputHeader);
    CPPSourceWriter source(log, outputSource);

    for (std::set<Group*>::iterator i = groups.begin(); i != groups.end(); i++) {
        Group *group = *i;
        TilePool &pool = group->getPool();

        log.heading(group->getName().c_str());

        pool.optimize(log);
        pool.encode(group->getLoadstream(), &log);

        proof.writeGroup(*group);
        header.writeGroup(*group);
        source.writeGroup(*group);
    }

    if (!sounds.empty()) {
        log.heading("Audio");
        log.infoBegin("Sound compression");

        for (std::set<Sound*>::iterator i = sounds.begin(); i != sounds.end(); i++) {
            Sound *sound = *i;
            header.writeSound(*sound);
            source.writeSound(*sound);
        }

        log.infoEnd();
    }

    proof.close();
    header.close();
    source.close();

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
        lua_pop(L, 2);          // Pop value and key-copy.
        success = false;
    }

    return success;
}

void Script::collect()
{
    /*
     * Scan through the global variables leftover after running a
     * script, and collect groups and images.
     *
     * We don't add images to their respective groups until this
     * point, since we want to be sure we only process assets that
     * remain in the global namespace.
     */

    lua_pushnil(L);
    while (lua_next(L, LUA_GLOBALSINDEX)) {
        lua_pushvalue(L, -2);   // Make a copy of the key object
        const char *name = lua_tostring(L, -1);
        Group *group = Lunar<Group>::cast(L, -2);
        Image *image = Lunar<Image>::cast(L, -2);
        Sound *sound = Lunar<Sound>::cast(L, -2);

        if (name && name[0] != '_') {
            if (group || image || sound)
                log.setMinLabelWidth(strlen(name));

            if (group) {
                group->setName(name);
                groups.insert(group);
            }
            if (image) {
                image->setName(name);
                image->getGroup()->addImage(image);
            }
            if (sound) {
                sound->setName(name);
                sounds.insert(sound);
            }
        }

        lua_pop(L, 2);
    };
    lua_pop(L, 1);
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

uint64_t Group::getSignature() const
{
    /*
     * Signatures are calculated automatically, as a portion of the
     * SHA1 hash of the loadstream.
     */

    SHA_CTX ctx;
    sha1_byte digest[SHA1_DIGEST_LENGTH];
    std::vector<uint8_t> ls = getLoadstream();

    SHA1_Init(&ctx);
    SHA1_Update(&ctx, &ls[0], ls.size());
    SHA1_Final(digest, &ctx);

    uint64_t sig = 0;
    for (unsigned i = 0; i < sizeof sig; i++) {
        sig <<= 8;
        sig |= digest[i];
    }

    return sig;
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
        mTileOpt.quality = lua_tonumber(L, -1);
    } else {
        mTileOpt.quality = mGroup->getQuality();
    }

    if (Script::argMatch(L, "frames"))
        mImages.setFrames(lua_tointeger(L, -1));
    if (Script::argMatch(L, "width"))
        mImages.setWidth(lua_tointeger(L, -1));
    if (Script::argMatch(L, "height"))
        mImages.setHeight(lua_tointeger(L, -1));
    if (Script::argMatch(L, "pinned"))
        mTileOpt.pinned = lua_toboolean(L, -1);

    if (Script::argMatch(L, "flat"))
        mIsFlat = lua_toboolean(L, -1);
    else
        mIsFlat = false;

    if (isFlat() && isPinned()) {
        luaL_error(L, "Image formats 'flat' and 'pinned' are mutually exclusive");
        return;
    }

    if (Script::argMatch(L, 1)) {
        /*
         * The positional argument can be a single string, or a
         * table which is interpreted as an array of strings.
         *
         * To handle these cases uniformly, we just trivially wrap a
         * single string in a table if needbe.
         */

        if (!lua_istable(L, -1)) {
            lua_newtable(L);            // New wrapper table
            lua_pushinteger(L, 1);      // Index for sole element (1)
            lua_pushvalue(L, -3);       // argMatch() result
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

    createGrids();
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
    lua_pushnumber(L, mTileOpt.quality);
    return 1;
}

int Image::group(lua_State *L)
{
    Lunar<Group>::push(L, mGroup, false);
    return 1;
}

void Image::createGrids()
{
    mGrids.clear();

    for (unsigned frame = 0; frame < mImages.getFrames(); frame++) {
        mGrids.push_back(TileGrid(&mGroup->getPool()));
        mImages.storeFrame(frame, mGrids.back(), mTileOpt);
    }
}

const char *Image::getClassName() const
{
    if (isPinned())
        return "PinnedAssetImage";
    else if (isFlat())
        return "FlatAssetImage";
    else
        return "AssetImage";
}

uint16_t Image::encodePinned() const
{
    // Pinned assets are just represented by a single index

    const TileGrid &firstGrid = mGrids[0];
    const TilePool &pool = firstGrid.getPool();  
    return pool.index(firstGrid.tile(0, 0));
}

void Image::encodeFlat(std::vector<uint16_t> &data) const
{
    // Flat assets are an uncompressed array of indices

    for (unsigned f = 0; f < mGrids.size(); f++) {
        const TileGrid &grid = mGrids[f];
        const TilePool &pool = grid.getPool();

        for (unsigned y = 0; y < grid.height(); y++)
            for (unsigned x = 0; x < grid.width(); x++)
                data.push_back(pool.index(grid.tile(x, y)));
    }
}

bool Image::encodeDUB(std::vector<uint16_t> &data, Logger &log) const
{
    // Compressed image, encoded using the DUB codec.
    
    DUBEncoder encoder( mImages.getWidth() / Tile::SIZE,
                        mImages.getHeight() / Tile::SIZE,
                        mImages.getFrames() );

    std::vector<uint16_t> tiles;
    encodeFlat(tiles);

    encoder.encodeTiles(tiles);
    
    // Too large to encode correctly?
    if (encoder.isTooLarge())
        return false;

    // Not compressible enough to bother?
    if (encoder.getRatio() < 10.0f)
        return false;
    
    encoder.logStats(getName(), log);
    data = encoder.getResult();

    return true;
}

Sound::Sound(lua_State *L)
{
    if (!Script::argBegin(L, className))
        return;
        
    if (Script::argMatch(L, 1)) {
        const char *filename = lua_tostring(L, -1);
        mFile = filename;
    }
    
    if (Script::argMatch(L, "encode")) {
        setEncode(lua_tostring(L, -1));
    } else {
        setEncode("");  // Default
    }
    
    if (Script::argMatch(L, "quality")) {
        setQuality(lua_tonumber(L, -1));
    } else {
        lua_getglobal(L, GLOBAL_QUALITY);
        setQuality(lua_tonumber(L, -1));
    }

    if (Script::argMatch(L, "vbr")) {
        setVBR(lua_toboolean(L, -1));
    } else {
        setVBR(false);
    }

    if (!Script::argEnd(L))
        return;

    // Validate the encoder parameters immediately, so we can raise an error
    // during script execution rather than during actual audio compression.
    AudioEncoder *enc = AudioEncoder::create(getEncode(), getQuality(), getVBR());
    if (enc)
        delete enc;
    else
        luaL_error(L, "Invalid audio encoding parameters");
}


};  // namespace Stir
