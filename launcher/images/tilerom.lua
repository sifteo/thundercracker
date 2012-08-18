--[[

 Note:

  Graphics are versioned using the cube's 8-bit version number,
  so we can tolerate some amount of change to the tile ROM contents.
  Graphics are labeled according to the first cube version they are
  compatible with. Unless a subsequent version exists, our convention
  is to assume a particular version of artwork will work on cubes of
  that version and later.

]]--

-----------------------------------------------------------------------
--------------------- Version 01 Tile ROM -----------------------------
-----------------------------------------------------------------------

v01_TileROM = group{ atlas="images/01-tilerom-atlas.png" }

v01_CubeConnected = image{ "images/01-cube-connected.png" }

-----------------------------------------------------------------------
--------------------- Version 02 Tile ROM -----------------------------
-----------------------------------------------------------------------

v02_TileROM = group{ atlas="../firmware/cube/tilerom/tilerom-atlas.png" }

v02_CubeConnected = image{ "images/02-cube-connected.png" }
