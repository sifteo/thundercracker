GameAssets = group{quality=10}
PlayerStand = image{"stand.png", width=32, height=32,pinned=true}
PlayerWalk = image{"walk.png", width=32, height=32,pinned=true}
PlayerPickup = image{"pickup.png", width=32, height=32, pinned=true}
PlayerIdle = image{"idle.png", width=32, height=32, pinned=true}
Sting = image{"sting.png",width=128,height=128}
Butterfly = image{"butterfly.png", width=8, height=8, pinned=true}
Items = image{"items.png", width=16, height=16, pinned=true}

Winscreen = image{"winscreen.png", width=128, height=128}
WinscreenBackground = image{"winscreen_background.png", width=128, height=128}
WinscreenAnim = image{"winscreen_anim.png", width=56, height=80, quality=2}

Sparkle = image{"sparkle.png", width=8, height=8, pinned=true}

Flash = image{"flash.png", width=8, height=8}
Black = image{"black.png", width=8, height=8}

ScrollLeft = image{"scroll_left.png", width=16, height=128}
ScrollLeftAccent = image{"scroll_left_accent.png", width=8, height=128}
ScrollMiddle = image{"scroll_middle.png", width=8, height=128}
ScrollRightAccent = image{"scroll_right_accent.png", width=8, height=128}
ScrollRight = image{"scroll_right.png", width=16, height=128}

ScrollBubble = image{"scroll_bubble.png", width=80, height=32}
ScrollThoughts = image{"scroll_thoughts.png", width=16, height=16}

dofile "gen_assets.lua"
