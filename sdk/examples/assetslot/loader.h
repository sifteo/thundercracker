/*
 * Sifteo SDK Example.
 */

#pragma once
#include <sifteo.h>


/**
 * An asynchronous asset loader, with a progress display UI.
 */

class MyLoader
{
public:
    /**
     * Initialize a new loader. Display graphics on all indicated cubes,
     * and use 'loaderSlot' for all of the loader's own assets.
     */
    MyLoader(Sifteo::CubeSet cubes, Sifteo::AssetSlot loaderSlot, Sifteo::VideoBuffer *vid);

    /**
     * Synchronously load an AssetGroup on all cubes.
     * If the group is already installed, returns quickly without
     * drawing anything.
     */
    void load(Sifteo::AssetGroup &group, Sifteo::AssetSlot slot);

private:
    Sifteo::CubeSet cubes;
    Sifteo::AssetSlot loaderSlot;
    Sifteo::VideoBuffer *vid;
    Sifteo::ScopedAssetLoader assetLoader;
};
