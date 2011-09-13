-- Asset groups are collections assets. Each asset group is
-- independently loadable. An asset can't be used unless its group is
-- loaded.
--
-- Every asset is by default assigned to the most recently created
-- group.  You can override this by explicitly specifying a 'group'
-- parameter on the asset.
--
-- Here, we'll just define all the assets within a single group.
--
-- Groups can have parameters which affect all items within that
-- group.  For example, we can set the default compression quality for
-- all assets in a group. You can override this for individual assets
-- if you want.

MainAssets = group{}

-- Image assets are used for sprites, backgrounds, maps,
-- items. Really, anything graphical in your game will be an image
-- asset. A simple image asset can be defined just by loading a PNG
-- file, and giving it a name:

Owlbear = image{"samples/sunset512.png"}

