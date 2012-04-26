GameAssets = group{}

-- Test = image{"font_bigger_2.png", quality=10}
--TestGlow = image{"font_bigger_2_glow.png", quality=10}
-- Menu
BgTile = image{ "scroll_bgTile.png", pinned=true }
Tip0 = image{ "option_chooseAnOption.png" }
Tip1 = image{ "option_tiltToScroll.png" }
Tip2 = image{ "option_touchToSelect.png" }
Footer = image{"option_blank_bottom.png", quality=10}
LabelEmpty = image{"option_blank.png", quality=10}
LabelContinue = image{ "option_continue.png" }
LabelLocked = image{ "option_disabled.png" }
IconLondon = image{ "scroll_london.png" }
IconAthens = image{ "scroll_athens.png" }
IconLocked = image{ "scroll_locked.png" }
IconLockedRight = image{ "scroll_locked_right.png" }
MenuBlank = image{"menu_blank.png"}

Sparkle = image{"sparkle01.png", pinned=true, width=16, height=16, quality=10}
HintSprite = image{"hint_tilt.png", pinned=true, width=16, height=16, quality=10}
Slat1 = image{"woodslat_thin.png"}
Slat2 = image{"woodslat_thick.png"}
Slat3 = image{"woodslat_extra_thick.png"}
SparkleWipe = image{"sparklewipe.png", height=16, width=48, quality=10}
BorderBottom = image{"border.png", height=16, quality=10}
BorderTop = image{"border_top.png", quality=10}
BorderLeft = image{"border_left.png", quality=10}
BorderRight = image{"border_right.png", quality=10}
BorderGoldTop = image{"border_gold_top.png", quality=10}
BorderGoldBottom = image{"border_gold.png", height=16, quality=10}
BorderGoldLeft = image{"border_gold_left.png", quality=10}
BorderGoldRight = image{"border_gold_right.png", quality=10}
BorderRightNoNeighbor = image{"border_right_noneighbor.png", quality=10}
BorderLeftNoNeighbor = image{"border_left_noneighbor.png", quality=10}
BorderSlotBlank = image{"border_available.png", quality=10}
BorderSlotNormal = image{"border_created_basic.png", quality=10, width=16}
BorderSlotBonus = image{"border_created_bonus.png", quality=10, width=16}
BorderSlotHint = image{"hint_pulse_morph.png", quality=10, width=16}
--Tile1 = image{"tile_ivory_big_idle.png", quality=10}
--Tile1Glow = image{"tile_ivory_big_glow.png", width=96, quality=10}
CityProgressionBlank = image{"city_prog_empty.png", quality=10}
transparent = image{"transparent.png", quality=10, pinned=true}

-- TODO border_ambientGlow.png
-- TODO hint_pulse_morph_tilt.png
-- TODO icon_tilt.png
-- TODO timer.png
-- TODO title.png
IconPress = image{"icon_press.png", width=16, quality=10}
Tile2 = image{"tile_ivory_idle.png", quality=10}
Tile2Glow = image{"tile_ivory_glow.png", quality=10}
Tile2Blank = image{"tile_ivory_blank.png", quality=10}
Tile2Meta = image{"tile_metatile_idle.png", quality=10}
Tile2MetaGlow = image{"tile_metatile_glow.png", quality=10}
Tile3 = image{"tile_ivory_narrow_idle.png", quality=10}
Tile3Glow = image{"tile_ivory_narrow_glow.png", quality=10}
Tile3Blank = image{"tile_ivory_narrow_blank.png", quality=10}
Tile3Meta = image{"tile_metatile_narrow_idle.png", quality=10}
Tile3MetaGlow = image{"tile_metatile_narrow_glow.png", quality=10}
Font1Letter = image{"font_idle_big.png", width=80, height=64, quality=0}
Font1LetterGlow = image{"font_glow_big.png", width=80, height=64, quality=0}
Font2Letter = image{"font_idle_medium.png", width=48, height=32, quality=10}
Font2LetterGlow = image{"font_glow_medium.png", width=48, height=32, quality=10}
Font3Letter = image{"font_idle_small.png", width=32, height=24, quality=10}
Font3LetterGlow = image{"font_glow_small.png", width=32, height=24, quality=10}
TileBG = image{"bg_puzzlebox.png", quality=10}
TileBGGold = image{"bg_puzzlebox_gold.png", quality=10} --compression creates banding
StartPrompt = image{"wp2_gameselect_prompt.png", pinned=true, quality=10}
StartBG = image{"wp2_gameselect.png", quality=10}
StartLid = image{"wp2_gameselect_lid.png", quality=10}
Title = image{"wp2_gameselect.png", quality=10}
HighScores = image{"score_highscores.png", quality=10}
Score = image{"score_endscore.png", quality=10}
FontSmall = image{"score_font.png", width=8, height=16, quality=10}
FontBonus = image{"score_font_bonus.png", width=8, height=16, quality=10}

-- Cutscenes
-- TODO break into asset groups
Scene01a = image{"Scene01a.png", width=128, height=128, quality=10}
Scene01b = image{"Scene01b.png", width=128, height=128, quality=10}
Scene01c = image{"Scene01c.png", width=128, height=128, quality=10}
Scene02a = image{"Scene02a.png", width=128, height=128, quality=10}
Scene02b = image{"Scene02b.png", width=128, height=128, quality=10}
Scene02c = image{"Scene02c.png", width=128, height=128, quality=10}
Scene03a = image{"Scene03a.png", width=128, height=128, quality=10}
Scene03b = image{"Scene03b.png", width=128, height=128, quality=10}
Scene03c = image{"Scene03c.png", width=128, height=128, quality=10}
Scene04a = image{"Scene04a.png", width=128, height=128, quality=10}
Scene04b = image{"Scene04b.png", width=128, height=128, quality=10}
Scene04c = image{"Scene04c.png", width=128, height=128, quality=10}

blip = sound{"blip.raw"}
bonus = sound{"bonus.raw"}
fanfare_fire_laugh_01 = sound{"fanfare_fire_laugh_01.raw"}
fanfare_fire_laugh_02 = sound{"fanfare_fire_laugh_02.raw"}
fanfare_fire_laugh_03 = sound{"fanfare_fire_laugh_03.raw"}
fanfare_fire_laugh_04 = sound{"fanfare_fire_laugh_04.raw"}
flap_laugh_fireball = sound{"flap_laugh_fireball.raw"}
lip_snort = sound{"lip_snort.raw"}
neighbor = sound{"neighbor.raw"}
pause_off = sound{"pause_off.raw"}
pause_on = sound{"pause_on.raw"}
shake = sound{"shake.raw"}
teeth_close = sound{"teeth_close.raw"}
teeth_open = sound{"teeth_open.raw"}
timer_10sec = sound{"timer_10sec.raw"}
timer_1sec = sound{"timer_1sec.raw"}
timer_20sec = sound{"timer_20sec.raw"}
timer_2sec = sound{"timer_2sec.raw"}
timer_30sec = sound{"timer_30sec.raw"}
timer_3sec = sound{"timer_3sec_fadein_lowcut.raw"}
timeup_01 = sound{"timeup_01.raw"}
timeup_02 = sound{"timeup_02.raw"}
timeup_03 = sound{"timeup_03.raw"}
wordplay_music_sayonara = sound{"wordplay_music_sayonara.raw"}
wordplay_music_versus = sound{"wordplay_music_versus.raw"}
