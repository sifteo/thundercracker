-- ROM Group
TileROM = group{ atlas="../firmware/cube/tilerom/tilerom-atlas.png" }
Logo = image{ "images/img-logo-white-on-blue.png" }

-- Menu Group
MenuGroup = group{ quality=9.8 }

-- Main menu graphics
Menu_BgTile = image{ "images/bg.png", pinned=1 }
Menu_StripeTile = image{ "images/stripes.png", pinned=1 }

Menu_Tip0 = image{ "images/Tip0.png" }
Menu_Tip1 = image{ "images/Tip1.png" }
Menu_Tip2 = image{ "images/Tip2.png" }

Menu_Footer = image{ "images/Footer.png" }

Menu_LabelEmpty = image{ "images/LabelEmpty.png" }

-- Font
Font = image{ "images/font.png", width=8, height=8 }

-- NineBlockPattern, for representing icon-less games

NineBlock_Center = image{ "images/nineblock-center.png" }
NineBlock_Shapes = image{ "images/nineblock-shapes.png", width=32, height=32 }

-- Applet icons

Icon_GetGames = image{ "images/icon-getgames.png" }
Icon_Status = image{ "images/icon-status.png" }
Icon_StatusOther = image{ "images/icon-status-other.png" }
Icon_MoreCubes = image{ "images/icon-more-cubes.png" }

-- System Status
BatteryBars = image{ {
    "images/img-battery-bars-0.png",
    "images/img-battery-bars-1.png",
    "images/img-battery-bars-2.png",
    "images/img-battery-bars-3.png",
    "images/img-battery-bars-4.png"
} }
