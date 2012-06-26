Asset Memory Management      {#asset_memory}
=======================

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

![](@ref asset-slots.png)
