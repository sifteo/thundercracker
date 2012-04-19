SandwichAssets = group{}

Blank = image{"Blank.png", width=128, height=128}
Sting = image{"sting.png",width=128,height=128}
SandwichTitle = image{"title.png"}
TitleBalloon = image{"title_bubble.png"}
TitleThoughts = image{"title_thought_dots.png"}

PlayerStand = image{"stand.png", width=32, height=32,pinned=true}
PlayerWalk = image{"walk.png", width=32, height=32,pinned=true}
PlayerPickup = image{"pickup.png", width=32, height=32, pinned=true}
PlayerIdle = image{"idle.png", width=32, height=32, pinned=true}

Butterfly = image{"butterfly.png", width=8, height=8, pinned=true}
BombAccent = image{"BombAccent.png", width=16, height=16, pinned=true}
Explosion = image{"explosion.png", width=48, height=48 }


Items = image{"items.png", width=16, height=16, pinned=true}

InventoryReticle = image{"inventory_reticle.png", width=32, height=32}

WhiteTile = image{"flash.png", pinned=true}
BlackTile = image{"black.png", pinned=true}
DialogBox = image{"dialog_box.png"}

InventoryBackground = image{"inventory_background.png"}
MinimapBasic = image{"minimap_basic.png", width=8, height=8}
MinimapDot = image{"minimap_dot.png", width=8, height=8, pinned=true}
Edge = image{ "edge.png" }
IconPress = image{ "icon_press.png" }

FrameLeft = image{ "frame_left.png" }
FrameRight = image{ "frame_right.png" }
FrameBottom = image{ "frame_bottom.png" }
FrameTop = image{ "frame_top.png" }

MenuBackground = image{ "menu_background.png", pinned=true }
MenuHeader = image{ "menu_header_blank.png" }
MenuHeaderContinue = image{ "menu_header_continue.png" }
MenuHeaderReplayCavern = image{ "menu_header_replay_caverns.png" }
MenuHeaderLocked = image{ "menu_header_locked.png" }
MenuHeaderClearData = image{ "menu_header_clear_data.png" }
MenuFooter = image{ "menu_footer.png" }
MenuIconPearl = image{ "menu_icon_pearl.png" }
MenuIconSprout = image{ "menu_icon_sprout.png" }
MenuIconBurgher = image{ "menu_icon_burgher.png" }
MenuIconHuh = image{ "menu_icon_huh.png" }

dofile "content.gen.lua"

music_castle = sound{ "music_castle.raw"}
music_dungeon = sound{ "music_dungeon.raw" }
music_sting = sound{ "music_sting.raw" }
music_winscreen = sound{ "music_winscreen.raw" }
sfx_deNeighbor = sound{ "sfx_deNeighbor.raw" }
sfx_doorBlock = sound{ "sfx_doorBlock.raw" }
sfx_doorOpen = sound{ "sfx_doorOpen.raw" }
sfx_neighbor = sound{ "sfx_neighbor.raw" }
sfx_pickup = sound{ "sfx_pickup.raw" }
sfx_running = sound{ "sfx_running.raw" }
sfx_walkStart = sound{ "sfx_walkStart.raw" }
sfx_zoomIn = sound{ "sfx_zoomIn.raw" }
sfx_zoomOut = sound{ "sfx_zoomOut.raw" }
