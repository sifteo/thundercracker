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
--------------------- Sound Effects -----------------------------------
-----------------------------------------------------------------------

SFX_PauseMenu = sound{ "pauseMenu.wav" }

-----------------------------------------------------------------------
--------------------- Version 01 Tile ROM -----------------------------
-----------------------------------------------------------------------

v01_TileROM = group{ atlas="01-tilerom-atlas.png" }

--- Common

v01_LogoWhiteOnBlue = image{ "01-img-logo-white-on-blue.png", flat=1 }

--- UIMenu

v01_MenuBackground = image{ "01-menu-background.png", flat=1 }
v01_IconQuit = image{ "01-icon-quit.png", flat=1 }
v01_IconBack = image{ "01-icon-back.png", flat=1 }
v01_IconResume = image{ "01-icon-resume.png", flat=1 }

--- UIShutdown

v01_ShutdownBackground = image{ "01-shutdown-background.png", flat=1 }

-----------------------------------------------------------------------
--------------------- Version 02 Tile ROM -----------------------------
-----------------------------------------------------------------------

v02_TileROM = group{ atlas="../../cube/tilerom/tilerom-atlas.png" }

--- Common

v02_LogoWhiteOnBlue = image{ "02-img-logo-white-on-blue.png", flat=1 }

--- UIFault

v02_FaultMessage = image{ "02-fault-message.png", flat=1 }

v02_CubeRangePause = image{ "02-cube-range-pause.png", flat=1 }

--- UIMenu

v02_MenuBackground = image{ "02-menu-background.png", flat=1 }
v02_IconQuit = image{ "02-icon-quit.png", flat=1 }
v02_IconBack = image{ "02-icon-back.png", flat=1 }
v02_IconResume = image{ "02-icon-resume.png", flat=1 }

--- UIShutdown

v02_BigDigits = image{ "01-big-digits.png", flat=1, width=32, height=40 }
