GameAssets = group{quality=10}

function skin(name)
    local path = "skins/"..name.."/"
    local prefix = "Skin_"..name.."_"

    _G[prefix.."VaultDoor"] = image{path.."vault_door.png", width=128, height=128}
    _G[prefix.."Background"] = image{path.."background.png"}
    _G[prefix.."Background_Lit"] = image{path.."background_lit.png"}
    _G[prefix.."Accent"] = image{ path.."accent.png" }

    _G[prefix.."Lit_Bottom"] = image { path.."lit_bottom.png", pinned=true }
    _G[prefix.."Lit_Bottom_Add"] = image { path.."lit_bottom_add.png", pinned=true }
    _G[prefix.."Lit_Bottom_Div"] = image { path.."lit_bottom_div.png", pinned=true }
    _G[prefix.."Lit_Bottom_Mul"] = image { path.."lit_bottom_mul.png", pinned=true }
    _G[prefix.."Lit_Bottom_Sub"] = image { path.."lit_bottom_sub.png", pinned=true }
    _G[prefix.."Lit_Left"] = image { path.."lit_left.png", pinned=true }
    _G[prefix.."Lit_Right"] = image { path.."lit_right.png", pinned=true }
    _G[prefix.."Lit_Right_Add"] = image { path.."lit_right_add.png", pinned=true }
    _G[prefix.."Lit_Right_Div"] = image { path.."lit_right_div.png", pinned=true }
    _G[prefix.."Lit_Right_Mul"] = image { path.."lit_right_mul.png", pinned=true }
    _G[prefix.."Lit_Right_Sub"] = image { path.."lit_right_sub.png", pinned=true }
    _G[prefix.."Lit_Top"] = image { path.."lit_top.png", pinned=true }

    _G[prefix.."Unlit_Bottom"] = image { path.."unlit_bottom.png", pinned=true }
    _G[prefix.."Unlit_Bottom_Add"] = image { path.."unlit_bottom_add.png", pinned=true }
    _G[prefix.."Unlit_Bottom_Div"] = image { path.."unlit_bottom_div.png", pinned=true }
    _G[prefix.."Unlit_Bottom_Mul"] = image { path.."unlit_bottom_mul.png", pinned=true }
    _G[prefix.."Unlit_Bottom_Sub"] = image { path.."unlit_bottom_sub.png", pinned=true }
    _G[prefix.."Unlit_Left"] = image { path.."unlit_left.png", pinned=true }
    _G[prefix.."Unlit_Right"] = image { path.."unlit_right.png", pinned=true }
    _G[prefix.."Unlit_Right_Add"] = image { path.."unlit_right_add.png", pinned=true }
    _G[prefix.."Unlit_Right_Div"] = image { path.."unlit_right_div.png", pinned=true }
    _G[prefix.."Unlit_Right_Mul"] = image { path.."unlit_right_mul.png", pinned=true }
    _G[prefix.."Unlit_Right_Sub"] = image { path.."unlit_right_sub.png", pinned=true }
    _G[prefix.."Unlit_Top"] = image { path.."unlit_top.png", pinned=true }
end

skin("blue")
skin("Default")
skin("green")
skin("purple")
skin("red")
skin("turquoise")


Accent_Current = image{ "accent_current.png" }
Accent_Target = image{ "accent_target.png" }

Hint_0 = image { "hint_0.png", pinned=true }
Hint_1 = image { "hint_1.png", pinned=true }
Hint_2 = image { "hint_2.png", pinned=true }
Hint_3 = image { "hint_3.png", pinned=true }
Hint_4 = image { "hint_4.png", pinned=true }
Hint_5 = image { "hint_5.png", pinned=true }
Hint_6 = image { "hint_6.png", pinned=true }

Coin_8 = image { "coin_8.png", pinned=true } 
Coin_16 = image { "coin_16.png", pinned=true } 
Coin_24 = image { "coin_24.png", pinned=true } 
Diamond_8 = image { "diamond_8.png", pinned=true } 
Diamond_16 = image { "diamond_16.png", pinned=true } 
Diamond_24 = image { "diamond_24.png", pinned=true } 
Ruby_8 = image { "ruby_8.png", pinned=true } 
Ruby_16 = image { "ruby_16.png", pinned=true } 
Ruby_24 = image { "ruby_24.png", pinned=true } 
Emerald_8 = image { "emerald_8.png", pinned=true } 
Emerald_16 = image { "emerald_16.png", pinned=true } 
Emerald_24 = image { "emerald_24.png", pinned=true } 





Narrator_Balloon = image { "narrator_balloon.png" }
Narrator_Base = image { "narrator_base.png" }
Narrator_Coin = image { "narrator_coin.png" }
Narrator_Diamond = image { "narrator_diamond.png" }
Narrator_Emerald = image { "narrator_emerald.png" }
Narrator_GetReady = image { "narrator_getready.png" }
Narrator_Ruby = image { "narrator_ruby.png" }
Narrator_Sad = image { "narrator_sad.png" }
Narrator_Wave = image { "narrator_wave.png" }
Narrator_Yay = image { "narrator_yay.png" }
Narrator_Mix01 = image { "narrator_mix01.png" }
Narrator_Mix02 = image { "narrator_mix02.png" }

Icon_Back = image { "icon_back.png"}
Icon_Clear_Data = image { "icon_clear_data.png"}
Icon_Expert = image { "icon_expert.png" }
Icon_Locked = image { "icon_locked.png"}
Icon_Main_Menu = image { "icon_main_menu.png"}
Icon_Music_Off = image { "icon_music_off.png"}
Icon_Music_On = image { "icon_music_on.png"}
Icon_No = image { "icon_no.png"}
Icon_Novice = image { "icon_novice.png" }
Icon_Resume = image { "icon_resume.png"}
Icon_Sfx_Off = image { "icon_sfx_off.png"}
Icon_Sfx_On = image { "icon_sfx_on.png"}
Icon_Yes = image { "icon_yes.png"}
Icon_Continue = image { "icon_continue.png"}
Icon_Random = image { "icon_random.png"}
Icon_Howtoplay = image { "icon_howtoplay.png"}
Icon_Level_Select = image { "icon_level_select.png"}
Icon_Setup = image { "icon_setup.png"}

icon_starting_simple = image { "icon_starting_simple.png"}
icon_two_tuples = image { "icon_two_tuples.png"}
icon_bend_it = image { "icon_bend_it.png"}
icon_crosses = image { "icon_crosses.png"}
icon_experts_only = image { "icon_experts_only.png"}
icon_squares = image { "icon_squares.png"}
icon_stacking_up = image { "icon_stacking_up.png"}

Press_To_Select = image { "press_to_select.png"}
Tilt_For_More = image { "tilt_for_more.png"}
Neighbor_For_Details = image { "neighbor_for_details.png", pinned=true }

Dark_Purple = image { "dark_purple.png" }
Tutorial_Groups = image { "tutorial_groups.png" }
Nan = image { "nan.png" }

Digits = image { "digits.png", width=16, height=16, pinned=true }

NormalDigit_0 = image { "0w.png", pinned=true }
NormalDigit_1 = image { "1w.png", pinned=true }
NormalDigit_2 = image { "2w.png", pinned=true }
NormalDigit_3 = image { "3w.png", pinned=true }
NormalDigit_4 = image { "4w.png", pinned=true }
NormalDigit_5 = image { "5w.png", pinned=true }
NormalDigit_6 = image { "6w.png", pinned=true }
NormalDigit_7 = image { "7w.png", pinned=true }
NormalDigit_8 = image { "8w.png", pinned=true }
NormalDigit_9 = image { "9w.png", pinned=true }

AccentDigit_0 = image { "0.png", pinned=true }
AccentDigit_1 = image { "1.png", pinned=true }
AccentDigit_2 = image { "2.png", pinned=true }
AccentDigit_3 = image { "3.png", pinned=true }
AccentDigit_4 = image { "4.png", pinned=true }
AccentDigit_5 = image { "5.png", pinned=true }
AccentDigit_6 = image { "6.png", pinned=true }
AccentDigit_7 = image { "7.png", pinned=true }
AccentDigit_8 = image { "8.png", pinned=true }
AccentDigit_9 = image { "9.png", pinned=true }


Title = image{"title.png", width=128, height=128}


Center = image{"assets_center.png", width=8, height=8}

Horizontal_Left = image{"assets_horizontal_left.png", width=16, height=8}
Horizontal_Center = image{"assets_horizontal_center.png", width=32, height=8}
Horizontal_Right = image{"assets_horizontal_right.png", width=16, height=8}

Vertical_Top = image{"assets_vertical_top.png", width=8, height=16}
Vertical_Center = image{"assets_vertical_center.png", width=8, height=32}
Vertical_Bottom = image{"assets_vertical_bottom.png", width=8, height=16}

MajorN_Frame2_AccentDigit = image{"assets_jointN_frame2_accentdigit.png"}
MajorS_Frame109_AccentDigit = image{"assets_jointS_frame109_accentdigit.png"}
Transparent_4x1 = image {"transparent_4x1.png"}

MajorN = image{"assets_jointN.png", width=64, height=24}
MajorW = image{"assets_jointW.png", width=24, height=64}
MajorS = image{"assets_jointS.png", width=64, height=24}
MajorE = image{"assets_jointE.png", width=24, height=64}
MajorNW = image{"assets_jointNW.png", width=8, height=8}
MajorSW = image{"assets_jointSW.png", width=8, height=8}
MajorSE = image{"assets_jointSE.png", width=8, height=8}
MajorNE = image{"assets_jointNE.png", width=8, height=8}
MinorNW = image{"assets_minorNW.png", width=8, height=8}
MinorSW = image{"assets_minorSW.png", width=8, height=8}
MinorSE = image{"assets_minorSE.png", width=8, height=8}
MinorNE = image{"assets_minorNE.png", width=8, height=8}


sfx_Stinger_02 = sound{ "PV_Stinger_02.raw", encode="ADPCM" }
sfx_Slide_LessScrape_01 = sound{ "PV_slide_lessScrape_01.raw", encode="ADPCM" }
sfx_Slide_LessScrape_02 = sound{ "PV_slide_lessScrape_02.raw", encode="ADPCM" }
sfx_Slide_LessScrape_Close_01 = sound{ "PV_slide_lessScrape_close_01.raw", encode="ADPCM" }
sfx_Hide_Overlay = sound { "PV_hide_overlay.raw", encode="ADPCM" }
sfx_Hint_Deny_01 = sound { "PV_hint_deny_01.raw", encode="ADPCM" }
sfx_Hint_Deny_02 = sound { "PV_hint_deny_02.raw", encode="ADPCM" }
sfx_Hint_Stinger_01 = sound { "PV_hint_stinger_01.raw", encode="ADPCM" }
sfx_Hint_Stinger_02 = sound { "PV_hint_stinger_02.raw", encode="ADPCM" }
sfx_Target_Overlay = sound { "PV_target_overlay.raw", encode="ADPCM" }
sfx_Tutorial_Mix_Nums = sound {"PV_tutorial_mix_nums.raw", encode="ADPCM" }
sfx_Level_Clear = sound {"PV_level_clear.raw", encode="ADPCM" }
sfx_Connect = sound { "PV_connect.raw", encode="ADPCM" }
sfx_Fast_Tick = sound{ "PV_fast_tick.raw", encode="ADPCM" }
sfx_Menu_Tilt_Stop = sound { "PV_menu_tilt_stop.raw", encode="ADPCM" }
sfx_Menu_Tilt = sound { "PV_menu_tilt.raw", encode="ADPCM" }
sfx_Tutorial_Correct = sound { "PV_tutorial_correct.raw", encode="ADPCM" }
sfx_Tutorial_Stinger_01 = sound { "PV_tutorial_stinger_01.raw", encode="ADPCM" }
sfx_Dialogue_Balloon = sound { "PV_dialogue_balloon.raw", encode="ADPCM" }
sfx_Tutorial_Oops = sound { "PV_tutorial_oops.raw", encode="ADPCM" }
sfx_Chapter_Victory = sound { "PV_chapter_victory.raw", encode="ADPCM" }





