Asset Workflow      {#asset_workflow}
==============

@b stir is a tool that formats your visual and audio assets to the compact formats used on Sifteo Cubes. It accepts configuration files written in [Lua](http://www.lua.org), and generates C++ code that should be built into your project.

Proofs of your image assets are also generated, formatted as HTML documents, so you can easily inspect them in your browser after the formatting process.

# Asset Images

Let's look at a basic assets.lua config dealing with images only:

~~~~~~~~~~~~~
GameAssets = group{}

Background = image{"stars-bg.png", quality=9.85}
Star = image{"star-8.png", pinned=true, width=8, height=8}
Font = image{"font-8x16.png", pinned=true, width=8, height=16}
~~~~~~~~~~~~~

The @b group element specifies that a Sifteo::AssetGroup named `GameAssets` should be created. This group contains all the `image` entries that follow it, until another `group` is declared. You can also explicitly specify the group that an `image` belongs to by adding `group=MyGroupName` within the `image` element.

All images in a single _group_ are compressed together, and form a bundle of compressed asset data which is loaded to the Sifteo Cubes in one piece. For more information on AssetGroups, see @ref asset_memory.

Each @b image element specifies that an AssetImage should be generated. Given no other details, a plain Sifteo::AssetImage will be generated, as in the case of @b Background above. By adding `pinned=true`, a Sifteo::PinnedAssetImage is created. Pinned assets are required for sprites, since the Sifteo platform requires all tiles in a sprite to be sequential.

By default, images are compressed to save room in your game's binary. However, if you need random access to your image data from within your game, you can specify `flat=1` within your `image` element, and @b stir will create a Sifteo::FlatAssetImage instead - a larger, uncompressed version of your image. For example: `Background = image{"stars-bg.png", flat=true}`

## Color

No guarantees are made about the specific RGB color space for source images, since the LCD on each Sifteo Cube is not color-calibrated and colors can vary significantly with changes in viewing angle.

Stir does not perform any colorspace conversion other than rounding each pixel to the nearest 16-bit RGB565 color. However, the compression process needs to evaluate the perceptibility of every individual lossy operation that could be performed, to decide which transforms would be too lossy given the current quality level. These perceptual comparisons are made in the linear CIELAB color space, after converting from an approximated sRGB colorspace with a single gamma of 2.2.

## Quality

Images are compressed more or less aggressively based on their `quality` parameter. Quality can be any real number from 1 to 10, with 10 meaning lossless, and lower values giving @b stir more freedom to compress.

All images within a Sifteo::AssetGroup can share tiles, and if @b stir determines that tiles within different images are similar enough, it can create a single tile to be shared by the 2 images, reducing the overall size of your application's installation. Of course, if the quality setting is too high, @b stir cannot take these liberties and your application will be larger as a result.

Compression is a very important part of keeping your asset budget under control. Dial down the quality for your images as much as you can stand and applications will be smaller, asset installation times will be shorter, and the world will be a better place.

Quality is continuously variable. Don't be shy about using a quality value like 9.998 if that's what looks best for your assets. Avoid using a quality of 10 unless it's vital that the assets are completely lossless. Lossless compression will often result in many tiles which have no visible differences.

## Frames

If you omit `width` and `height` attributes for your image elements, @b stir will default to creating AssetImages with the native width and height of your source image, in a single frame.

Animations consisting of multiple frames should lay out each frame contiguously in the source image. You can then specify `width` or `height` to indicate the size of a frame within your image. In this case, @b stir will divide your image into a grid of appropriately sized frames, and its `frames` attribute will be calculated accordingly from left to right, top to bottom.

@note If you're creating a long animation which may have many duplicated frames in it, stir will automatically factor out these duplicated frames assuming the asset is not marked as `flat` or `pinned`. The _Dictionary Uniform Block (DUB)_ codec operates on blocks of up to 8x8 tiles within each frame. If any 8x8 tile blocks are identical, they will all be represented by the same data in memory.

## Image Option Summary

Option              | Meaning
-------             | -------------
`quality=N`         | Set the quality of this image to `N`, where N is a number between 1 and 10
`pinned=true`       | Create a Sifteo::PinnedAssetImage
`flat=true`         | Create a Sifteo::FlatAssetImage
`width=8`           | Specify the width of a frame in your image, defaults to source image's native width.
`height=16`         | Specify the height of a frame of your image, defaults to source image's native height.
`group=MyGroup`     | Specify that this image is a member of the `group` element `MyGroup`

## Asset Image Lists

It is occasionally desirable to export images as an array.

~~~~~~~~~~~~~
Environments = group{}

TileSets = { image{"desert.png"}, image{"ocean.png"}, image{"castle.png"} }
~~~~~~~~~~~~~

Image Lists must contain only images, or else they are ignored.  Furthermore, the images must be homogeneous: you may not interleave pinned, flat and ordinary assets in the same list.

Asset Image Lists are just a way of grouping together several distinct AssetImage instances. This can be handy when you have several images which are related, but may have different dimensions or compression formats, or when each image already contains multiple frames.

Note that this syntax is distinct from the following, which creates a single Sifteo::AssetImage with two frames by passing an array of PNG filenames to the image constructor:

~~~~~~~~~~~~~
Sprites = group{}

PlayerSprite = image{ { "player-sitting.png", "player-standing.png"}, pinned=true }
~~~~~~~~~~~~~

@note If you have the choice between using a single Asset Image with multiple frames, instead of an Asset Image List, the single Asset Image can be more efficient.

# Audio     {#asset_audio}

The Sifteo audio system supports both sample and tracker module playback. Tracker audio is great for longer, sequenced audio, like background music. Samples are typically used for shorter assets, like sound effects, or for audio that is not easily sequenced, like speech.

If you haven't already, see @ref audio for details of the Sifteo audio system.

## Samples

Here's an assets.lua snippet that creates two samples:

~~~~~~~~~~~~~
Click = sound{ "click.wav" }
Bloop = sound{ "bloop.wav", encode="pcm" }
~~~~~~~~~~~~~

Each @b sound element specifies that a Sifteo::AssetAudio object should be generated, which can be played by a Sifteo::AudioChannel.

`stir` treats `sound` files whose name ends in `.wav` as WAV files, which are widely supported in most audio editing and authoring environments. When processing WAV files, `stir` ensures that the sample rate and sample format of your audio is correct - 16kHz sample rate and signed 16-bit samples. Any other `sound` files are treated as headerless raw PCM16 data.

The default encoding for audio data in external storage is ADPCM, which compresses your source audio to 25% of its original size. To specify that your sample be stored uncompressed, add the `encode="pcm"` option to your `sound` object, as in the `Bloop` entry above.

## Tracker

Here's an assets.lua containing two modules:

~~~~~~~~~~~~~
Bubbles = tracker{"bubbles.xm"}
Slumberjack = tracker{"slumberjack.xm", loop=false}
~~~~~~~~~~~~~

Each @b tracker element specifies that a Sifteo::AssetTracker module should be generated, which can be played by a Sifteo::AudioTracker.

By default, tracker modules loop using the restart point defined in the XM file. Modules play only once if the restart point is not valid. You may specify the `loop=false` option to forcibly disable looping.

# stir Options
@b stir provides several options to configure its execution. These options are integrated into the default Makefiles that ship with the SDK, but you may wish to integrate @b stir into your workflow in other ways.

Option                  | Meaning
-------                 | -------------
`-h`                    | Show a help message and exit
`-v`                    | Enable verbose output
`-o <myproof>.html`     | Writes an HTML image proof to `<myproof>.html` - open this up in your web browser
`-o <assets.gen>.cpp`   | Generates C++ source data for your assets to `<assets.gen>.cpp` - include this file in your build
`-o <assets.gen>.h`     | Generates C++ header data for your assets to `<assets.gen>.h` - include this file in your build
`VAR=VALUE`             | Define a Lua script variable, prior to parsing the script
