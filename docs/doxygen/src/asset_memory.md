Asset Memory Management      {#asset_memory}
=======================

@brief End to end details on graphics data management

# Overview

To make the most efficient use of memory and of radio bandwidth, Sifteo Cubes have a multi-level hierarchy of storage types, utilizing several special-purpose data compression codecs.

This is an advanced guide which describes the memory management architecture for graphics data on Sifteo Cubes. We assume familiarity with the @ref gfx and @ref asset_workflow.

![](@ref asset-mem.png)

## Main Flash

This is the largest storage device in the system, capable of storing all persistent data for several games. A game's entire .ELF binary image resides here. Main Flash is large, and it may be written to at runtime using the Sifteo::StoredObject class.

Main Flash is being used continuously by the system, to fetch music and executable code as well as graphics data. All of these accesses occur via a shared cache in RAM.

Main Flash stores asset data in exactly the same format it is encoded to by `stir`:

- AssetGroup data (tile pixels) are stored in a compressed format which can be decompressed autonomously by a Sifteo Cube into its own local Asset Flash.
- AssetImage data (tile indices) are stored in one of several supported formats, including a raw uncompressed (flat) format, or a block-oriented compression codec (DUB). Each AssetImage references tile data from exactly one AssetGroup.

Any time you draw an AssetImage, it is streamed from Main Flash, and decompressed either into a temporary Sifteo::TileBuffer, or into a Sifteo::VideoBuffer. TileBuffers may be used to cache decompressed AssetImages in RAM, in order to reduce the amount of time spent decompressing or loading data from Main Flash.

## Asset Flash

This is a medium-sized storage device located on each individual Sifteo Cube. Asset Flash stores _only_ uncompressed graphics data. In tandem with Video RAM, this memory drives the cube's graphics engine. Asset Flash is extremely fast for the graphics engine to read, but it's slow to write.

The only way to write to Asset Flash is by sending compressed pixel data over the radio. This pixel data is part of a Sifteo::AssetGroup. It has already been encoded by `stir`.

Asset Flash is persistent. After loading a Sifteo::AssetGroup, that group remains installed until it is removed or that memory is recycled by the same or a different game.

The allocation of Asset Flash to individual games takes place using _asset slots_. The Sifteo::AssetSlot object is the basic unit of allocation for Asset Flash. Each Sifteo::AssetSlot is equal to __4096 tiles__, or 512 kB of memory. Each Sifteo Cube has a total of 8 slots, or 4 MB. Due to hardware limitations, only half of this (4 slots) may be used at once. This, therefore, is the maximum number of Sifteo::AssetSlot objects which may exist in one game.

The binding between a game's _virtual_ asset slots and a per-cube _physical_ asset slot takes place when loading a game, just prior to installing any _bootstrap_ asset groups. The binding is arbitrary, and may be different for each cube. The system always reserves one physical asset slot per cube for each virtual asset slot in the game.

![](@ref asset-slots.png)

Within each Sifteo::AssetSlot, a game is responsible for managing its own Asset Flash memory. Each slot acts as a small append-only store for AssetGroup data. A persistent index stored in Main Flash keeps track of which groups are loaded in which slots, and where. A game can use Sifteo::AssetLoader to dynamically install a new AssetGroup into an AssetSlot. In fact, Asset Slots are very simple containers that only support a few operations:

1. Test whether an AssetGroup is already installed, and look up its address: Sifteo::AssetGroup::isInstalled()
2. Append a new AssetGroup: Sifteo::AssetLoader
3. Erase the entire slot: Sifteo::AssetSlot::erase()

An AssetSlot may hold at most 4096 tiles or 24 AssetGroups.

![](@ref asset-slots-detail.png)

## Bootstrapping

One of the goals of asset memory management should be to reduce or eliminate user-visible loading time. One way to accomplish this is to allow asset loading to begin before a game actually starts executing. Games may mark an AssetGroup as "bootstrapped", indicating that the game is expecting that group to be available in Asset Flash before the game begins executing.

For example, a game may contain the following code:

    static AssetSlot MainSlot = AssetSlot::allocate()
        .bootstrap(BootstrapGroup);

    static AssetSlot LevelSlot = AssetSlot::allocate();
    static AssetSlot CutsceneSlot = AssetSlot::allocate();

This instructs the system to take the following actions when preparing to run the game:

1. __Bind three virtual asset slots__ to physical asset slots. If possible the same physical slots will be reused, and any already-loaded AssetGroups will remain. If not, new empty physical slots will be assigned.
2. __Load BootstrapGroup into MainSlot__. There may be other assets in MainSlot already. These other assets are not removed unless the space is needed to install BootstrapGroup. This loading operation may happen concurrently with other animation or user interaction in the system launcher.
3. __Transfer control__ to the game's binary.

Bootstrapping is so named because it's intended to install a very minimal amount of asset data, just enough to display a title screen or main menu typically. For the best user experience, bootstrapped asset data should not take longer than a fraction of a second to install.

## Debugging

Since much of the asset memory management process occurs automatically, it can often be useful to have additional visibility into how your cube's asset slots have been assigned, or how a game is managing the groups loaded into its slots.

Right now the best way to do this is via the Inspector panel ("I" key) in Siftulator. This gives you a pictorial representation of the tiles in each cube's Asset Flash. This image corresponds directly with the layout of data in physical asset slots:

![](@ref asset-flash-inspector.png)
