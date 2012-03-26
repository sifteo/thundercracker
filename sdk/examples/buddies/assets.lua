UseSprites = false
GameAssets = group{quality = 10}

-- TOOD: Put each buddy in its own asset group

-- Buddy Backgrounds
BuddyBackground0 = image{"assets/bg1.png"}
BuddyBackground1 = image{"assets/bg2.png"}
BuddyBackground2 = image{"assets/bg3.png"}
BuddyBackground3 = image{"assets/bg4.png"}
BuddyBackground4 = image{"assets/bg5.png"}
BuddyBackground5 = image{"assets/bg6.png"}

-- Buddy Parts
if UseSprites then
    BuddyParts0 = image{"assets/parts1_sprite.png", height = 64, pinned = true}
    BuddyParts1 = image{"assets/parts2_sprite.png", height = 64, pinned = true}
    BuddyParts2 = image{"assets/parts3_sprite.png", height = 64, pinned = true}
    BuddyParts3 = image{"assets/parts4_sprite.png", height = 64, pinned = true}
    BuddyParts4 = image{"assets/parts5_sprite.png", height = 64, pinned = true}
    BuddyParts5 = image{"assets/parts6_sprite.png", height = 64, pinned = true}
else
    BuddyParts0 = image{"assets/parts1.png", height = 48}
    BuddyParts1 = image{"assets/parts2.png", height = 48}
    BuddyParts2 = image{"assets/parts3.png", height = 48}
    BuddyParts3 = image{"assets/parts4.png", height = 48}
    BuddyParts4 = image{"assets/parts5.png", height = 48}
    BuddyParts5 = image{"assets/parts6.png", height = 48}
end

-- Buddy Full Faces
BuddyFull0 = image{"assets/buddy_full_1.png"}
BuddyFull1 = image{"assets/buddy_full_2.png"}
BuddyFull2 = image{"assets/buddy_full_3.png"}
BuddyFull3 = image{"assets/buddy_full_4.png"}
BuddyFull4 = image{"assets/buddy_full_5.png"}
BuddyFull5 = image{"assets/buddy_full_6.png"}

-- Buddy Overlays
if UseSprites then
    BuddyPartFixed = image{"assets/fixed_sprite.png", pinned = UseSprites}
    BuddyPartHidden = image{"assets/hidden_sprite.png", pinned = UseSprites}
else
    BuddyPartFixed = image{"assets/fixed.png"}
    BuddyPartHidden = image{"assets/hidden.png"}
end

-- Buddy Sprites
BuddySpriteFrontGluv = image{"assets/sprite_front_gluv.png", pinned = true}
BuddySpriteFrontRike = image{"assets/sprite_front_rike.png", pinned = true}
BuddySpriteFrontZorg = image{"assets/sprite_front_zorg.png", pinned = true}
BuddySpriteCutsceneRike = image{"assets/cutscene_sprite_rike_left.png", pinned = true}
BuddySpriteCutsceneGluv = image{"assets/cutscene_sprite_gluv_right.png", pinned = true}

-- UI Fonts
UiFontWhite = image{"assets/fontstrip_content_white_nooutline.png", height = 16}
UiFontBlack = image{"assets/fontstrip_content_black_nooutline.png", height = 16}
UiFontOrange = image{"assets/fontstrip_content_orange.png", height = 16}
--UiFontBlue = image{"assets/fontstrip_content_blue.png", height = 16}
--UiFontHeadingBlue = image{"assets/fontstrip_heading_blue.png", height = 16}
UiFontHeadingOrange = image{"assets/fontstrip_heading_orange.png", height = 16}
--UiFontHeadingOrangeNoOutline = image{"assets/fontstrip_heading_orange_nooutline.png", height = 16}

-- Ui Misc
--UiBannerFaceCompleteBlue = image{"assets/ui_top_facecomplete_blue.png"}
UiBannerFaceCompleteOrange = image{"assets/ui_top_facecomplete_orange.png"}
UiBackground = image{"assets/ui_background.png"}
UiPanel = image{"assets/plain_panel.png"}
UiClueBlank = image{"assets/panel_clue_blank.png"}
UiResume = image {"assets/pausemenu_resume.png"}
UiRestart = image {"assets/pausemenu_restart_puzzle.png"}
UiEndGameNavExit = image{"assets/end_navigation_exit.png"}
UiCongratulations = image{"assets/panel_congratulations.png"}
UiRibbonGluv = image{"assets/ribbon_gluv.png"}

-- Shuffle Mode
ShuffleTitleScreen = image{"assets/titlescreen_shuffle.png"}
ShuffleShakeToShuffle = image{"assets/shake_to_shuffle.png"}
ShuffleTouchToSwap = image{"assets/touch_to_swap.png"}
ShuffleMemorizeFaces = image{"assets/panel_memorize_faces.png"}
ShuffleNeighbor = image{"assets/neighbor.png"}
ShuffleClueUnscramble = image{"assets/panel_clue_shufflemode.png"}
ShuffleBestTimes = image{"assets/panel_best_times.png"}
ShuffleBestTimesHighScore1 = image{"assets/panel_best_times_highscore_1.png"}
ShuffleBestTimesHighScore2 = image{"assets/panel_best_times_highscore_2.png"}
ShuffleBestTimesHighScore3 = image{"assets/panel_best_times_highscore_3.png"}
ShuffleEndGameNavReplay = image{"assets/end_navigation_replay_shuffle.png"}

-- Story Mode
StoryChapterTitle = image{"assets/ui_chapter_title.png"}
StoryChapterNext = image{"assets/panel_next_chapter.png"}
StoryChapterRetry = image{"assets/panel_retry_chapter.png"}
StoryCutsceneBackgroundLeft = image{"assets/cutscenebg_left.png"}
StoryCutsceneBackgroundRight = image{"assets/cutscenebg_right.png"}
StoryFaceComplete = image{"assets/panel_facecomplete_ribbon.png"}
StoryProgress = image{"assets/panel_progression.png"}
StoryRibbonNewCharacter = image{"assets/ribbon_new_character.png"}
StoryBookStartNext = image{"assets/panel_start_next_book.png"}

-- Sounds (use encode = "PCM" for PCM encoding)
SoundGems = sound{"assets/gems1_4A9.raw"}
