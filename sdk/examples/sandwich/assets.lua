GameAssets = group{quality=10}
PlayerStand = image{"stand.png", width=32, height=32,pinned=true}
PlayerWalk = image{"walk.png", width=32, height=32,pinned=true}
PlayerPickup = image{"pickup.png", width=32, height=32, pinned=true}
PlayerIdle = image{"idle.png", width=32, height=32, pinned=true}
Sting = image{"sting.png",width=128,height=128}
Skeleton = image{"skeleton.png", width=32, height=32,pinned=true}
Items = image{"items.png", width=16, height=16, pinned=true}

Winscreen = image{"winscreen.png", width=128, height=128}
WinscreenBackground = image{"winscreen_background.png", width=128, height=128}
WinscreenAnim = image{"winscreen_anim.png", width=56, height=80, quality=3}

dofile "gen_assets.lua"
