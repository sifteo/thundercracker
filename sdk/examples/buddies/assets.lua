UseSprites = false
GameAssets = group{quality = 10}

-- TOOD: Put each buddy in its own asset group
function AddBuddy (i, name)
    _G["BuddyFull"        .. i] = image{"assets/buddy_full_"  .. (i + 1) .. ".png"}
    _G["BuddyBackground"  .. i] = image{"assets/bg"           .. (i + 1) .. ".png"}
    _G["BuddySpriteFront" .. i] = image{"assets/sprite_"      .. name    .. "_front.png", pinned = true}
    _G["BuddySpriteRight" .. i] = image{"assets/sprite_"      .. name    .. "_right.png", pinned = true}
    _G["BuddySpriteLeft"  .. i] = image{"assets/sprite_"      .. name    .. "_left.png", pinned = true}
    _G["BuddyRibbon"      .. i] = image{"assets/ribbon_"      .. name    .. ".png"}
    _G["BuddySmall"       .. i] = image{"assets/buddy_small_" .. name    .. ".png"}
    if UseSprites then
       _G["BuddyParts"    .. i] = image{"assets/parts"       .. (i + 1) .. "_sprite.png", height = 64, pinned = true}
    else
       _G["BuddyParts"    .. i] = image{"assets/parts"       .. (i + 1) .. ".png", height = 48}
    end
end

AddBuddy(0, "gluv")
AddBuddy(1, "suli")
AddBuddy(2, "rike")
AddBuddy(3, "boff")
AddBuddy(4, "zorg")
AddBuddy(5, "maro")

-- Buddy Overlays
if UseSprites then
    BuddyPartFixed = image{"assets/fixed_sprite.png", pinned = UseSprites}
    BuddyPartHidden = image{"assets/hidden_sprite.png", pinned = UseSprites}
else
    BuddyPartFixed = image{"assets/fixed.png"}
    BuddyPartHidden = image{"assets/hidden.png"}
end

-- UI Fonts
UiFontWhite = image{"assets/fontstrip_content_white_nooutline.png", height = 16}
UiFontBlack = image{"assets/fontstrip_content_black_nooutline.png", height = 16}
UiFontOrange = image{"assets/fontstrip_content_orange.png", height = 16}
--UiFontBlue = image{"assets/fontstrip_content_blue.png", height = 16}
--UiFontHeadingBlue = image{"assets/fontstrip_heading_blue.png", height = 16}
UiFontHeadingOrange = image{"assets/fontstrip_heading_orange.png", height = 16}
UiFontHeadingOrangeNoOutline = image{"assets/fontstrip_heading_orange_nooutline.png", height = 16}

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
UiGoBlue = image{"assets/go_blue.png", pinned = true}
UiGoOrange = image{"assets/go_orange.png", pinned = true}
UiFaceHolder = image{"assets/faceholder.png"};

-- Free Play Mode
FreePlayTitle = image{"assets/titlescreen_free.png"}
FreePlayDescription = image{"assets/panel_free_play_description.png"}

-- Shuffle Mode
ShuffleTitleScreen = image{"assets/titlescreen_shuffle.png"}
ShuffleShakeToShuffle = image{"assets/shake_to_shuffle.png"}
ShuffleTouchToSwap = image{"assets/touch_to_swap.png"}
ShuffleMemorizeFaces = image{"assets/panel_memorize_faces.png"}
ShuffleNeighbor = image{"assets/neighbor_shufflegoal.png"}
ShuffleClueUnscramble = image{"assets/panel_clue_shufflemode.png"}
ShuffleBestTimes = image{"assets/panel_best_times.png"}
ShuffleBestTimesHighScore1 = image{"assets/panel_best_times_highscore_1.png"}
ShuffleBestTimesHighScore2 = image{"assets/panel_best_times_highscore_2.png"}
ShuffleBestTimesHighScore3 = image{"assets/panel_best_times_highscore_3.png"}
ShuffleEndGameNavReplay = image{"assets/end_navigation_replay_shuffle.png"}

-- Story Mode
for i = 0, 14, 1 do
    _G["StoryBookTitle" .. i] = image{"assets/book" .. (i + 1) .. ".png"}
end
StoryChapterTitle = image{"assets/ui_chapter_title.png"}
StoryChapterNext = image{"assets/panel_next_chapter.png"}
StoryChapterRetry = image{"assets/panel_retry_chapter.png"}
StoryCutsceneBackgroundLeft = image{"assets/cutscenebg_left.png"}
StoryCutsceneBackgroundRight = image{"assets/cutscenebg_right.png"}
StoryFaceComplete = image{"assets/panel_facecomplete_ribbon.png"}
StoryNeighbor = image{"assets/neighbor.png"}
StoryProgress = image{"assets/panel_storymode_puzzle_progression.png"}
StoryProgressNumbers = image{"assets/progress_numbers.png", height = 16}
StoryRibbonNewCharacter = image{"assets/ribbon_new_character.png"}
StoryBookStartNext = image{"assets/panel_start_next_book.png"}

-- Sounds (use encode = "PCM" for PCM encoding)
SoundGems = sound{"assets/gems1_4A9.raw"}
