Asset Workflow      {#asset_workflow}
==============

@b stir is a tool that formats your visual and audio assets to the compact formats used on Sifteo Cubes. It accepts configuration files written in [Lua](http://www.lua.org), and generates C++ code that should be built into your project.

Proofs of your image assets are also generated, so you can easily inspect them after the formatting process.

# Overview

@b stir is driven by a config file within your project written in [Lua](http://www.lua.org), typically named `assets.lua`. Don't worry if you're not familiar with Lua - the simplest stir configurations are essentially declarative lists of assets, and you can easily adapt the SDK examples to fit your needs.

# Asset Images

Let's look at a basic assets.lua config dealing with images only:

~~~~~~~~~~~~~{.lua}
GameAssets = group{}

Background = image{"stars-bg.png", quality=10}
Star = image{"star-8.png", pinned=true, width=8, height=8}
Font = image{"font-8x16.png", pinned=true, width=8, height=16}
~~~~~~~~~~~~~

The @b group element specifies that a Sifteo::AssetGroup named `GameAssets` should be created. This group contains all the `image` entries that follow it, until another `group` is declared. You can also explicitly specify the group that an `image` belongs to by adding `group=MyGroupName` within the `image` element. An AssetGroup is the unit of get installed to Sifteo Cubes at the same time, so you should typically keep related images in the same group.

Each @b image element specifies that an AssetImage should be generated. Given no other details, a plain Sifteo::AssetImage will be generated, as in the case of @b Background above. By adding `pinned=true`, a Sifteo::PinnedAssetImage is created. Pinned assets are required for sprites, since the Sifteo platform requires all tiles in a sprite to be sequential.

By default, images are compressed to save room in your game's binary. However, if you need random access to your image data from within your game, you can specify `flat=1` within your `image` element, and @b stir will create a Sifteo::FlatAssetImage instead - a larger, uncompressed version of your image. For example: `Background = image{"stars-bg.png", flat=true}`

## Quality

Images are compressed more or less aggressively based on their `quality` parameter. Quality can be a number from 1 to 10, with 10 meaning lossless, and lower values giving @b stir more freedom to compress.

All images within a Sifteo::AssetGroup can share tiles, and if @b stir determines that tiles within different images are similar enough, it can create a single tile to be shared by the 2 images, reducing the overall size of your application's installation. Of course, if the quality setting is too high, @b stir cannot take these liberties and your application will be larger as a result.

Compression is a very important part of keeping your asset budget under control. Dial down the quality for your images as much as you can stand and applications will be smaller, asset installation times will be shorter, and the world will be a better place.

## Frames

If you omit `width` and `height` attributes for your image elements, @b stir will default to creating AssetImages with the native width and height of your source image, in a single frame.

Animations consisting of multiple frames should lay out each frame contiguously in the source image. You can then specify `width` or `height` to indicate the size of a frame within your image. In this case, @b stir will divide your image into a grid of appropriately sized frames, and its `frames` attribute will be calculated accordingly from left to right, top to bottom.

## Image Option Summary

Option              | Meaning
-------             | -------------
`quality=N`         | Set the quality of this image to `N`, where N is a number between 1 and 10
`pinned=true`       | Create a Sifteo::PinnedAssetImage
`flat=true`         | Create a Sifteo::FlatAssetImage
`width=8`           | Specify the width of a frame in your image, defaults to source image's native width.
`height=16`         | Specify the height of a frame of your image, defaults to source image's native height.
`group=MyGroup`     | Specify that this image is a member of the `group` element `MyGroup`

# Asset Audio

## Samples

## Tracker

# stir Options
@b stir provides several options to configure its execution. These options are integrated into the default Makefiles that ship with the SDK, but you may wish to integrate @b stir into your workflow in additional ways.

Option                  | Meaning
-------                 | -------------
`-h`                    | Show a help message and exit
`-v`                    | Enable verbose output
`-o <myproof>.html`     | Writes an HTML image proof to `myproof.html` - open this up in your web browser
`-o <assets.gen>.cpp`   | Generates C++ source data for your assets to `<assets.gen>.cpp` - include this file in your build
`-o <assets.gen>.h`     | Generates C++ header data for your assets to `<assets.gen>.h` - include this file in your build
`VAR=VALUE`             | Define a Lua script variable, prior to parsing the script
