-- Asset groups are collections assets. Each asset group is
-- independently loadable. An asset can't be used unless its group is
-- loaded.
--
-- Every asset is by default assigned to the most recently created
-- group.  You can override this by explicitly specifying a 'group'
-- parameter on the asset.
--
-- Here, we'll just define all the assets within a single group.

MainAssets = group{}

-- Image assets are used for sprites, backgrounds, maps,
-- items. Really, anything graphical in your game will be an image
-- asset. A simple image asset can be defined just by loading a PNG
-- file, and giving it a name:

Title = image{"samples/penguin-title.png"}

-- Every image asset can have one or more frames. These can actually
-- be animation frames, or perhaps just multiple identically-sized
-- images that are used in tandem, such as the glyphs in a font.
--
-- A multi-frame asset can be defined using multiple filenames, by
-- passing in a list of PNG files. Or if you use larger PNG files
-- and specify a smaller width and/or height, the large image
-- will be automatically segmented into frames. Frames are ordered
-- left to right, top to bottom.

Disapproval = image{"samples/zspriteb.png", width=32, height=48}

-- You can also adjust the quality separately for each asset,
-- as well as globally, or per-group.

Owlbear = image{"samples/owlbear.png", width=128, height=128, quality=6}

-- A font is a good example of an image with a very small per-frame
-- size, but many many frames. This is a basic 8x16 pixel font.
--
-- "Pinning" an asset means to preserve the uniqueness and order
-- of every single tile in the image. Normaly this would be a bad
-- idea, since it negates much of the compression that STIR can
-- perform. But on an image that isn't very compressible anyway,
-- it can be a win because we don't need to include a separate map.
-- The map is implied by the order in which the tiles occur in memory.

Font = image{"samples/font-8x16.png", width=8, height=16, pinned=true, quality=10}
