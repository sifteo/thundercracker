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
AddBuddy(6, "amor")
AddBuddy(7, "droo")
AddBuddy(8, "erol")
AddBuddy(9, "flur")
AddBuddy(10, "veax")
AddBuddy(11, "yawp")

-- Buddy Overlays
if UseSprites then
    BuddyPartFixed = image{"assets/fixed_sprite.png", pinned = UseSprites}
    BuddyPartHidden = image{"assets/hidden_sprite.png", pinned = UseSprites}
else
    BuddyPartFixed = image{"assets/fixed.png"}
    BuddyPartHidden = image{"assets/hidden.png"}
end
    
-- Menu
BgTile = image{"assets/menu/bg_blue_8x8.png"}
IconFreePlay = image{"assets/menu/IconFreePlay.png"}
IconStory = image{"assets/menu/IconStory.png"}
IconShuffle = image{"assets/menu/IconShuffle.png"}
IconOptions = image{"assets/menu/IconOptions.png"}
IconContinue = image{"assets/menu/IconContinue.png"}
IconExit = image{"assets/menu/IconExit.png"}
IconLocked = image{"assets/menu/IconLocked.png"}
TipSelectMode = image{"assets/menu/selectmode.png"}
TipSelectBook = image{"assets/menu/selectabook.png"}
TipTiltToScroll = image{"assets/menu/tilt.png"}
TipPressToSelect = image{"assets/menu/press.png"}
Footer = image{"assets/menu/Footer.png"}
LabelEmpty = image{"assets/menu/LabelEmpty.png"}
LabelFreePlay = image{"assets/menu/LabelFreePlay.png"}
LabelStory = image{"assets/menu/LabelStory.png"}
LabelShuffle = image{"assets/menu/LabelShuffle.png"}
LabelOptions = image{"assets/menu/LabelOptions.png"}
LabelContinue = image{"assets/menu/LabelContinue.png"}
LabelExit = image{"assets/menu/LabelExit.png"}
for i = 0, 2, 1 do
    _G["IconBook" .. i] = image{"assets/menu/IconBook" .. (i + 1) .. ".png"}
    _G["LabelBook" .. i] = image{"assets/menu/LabelBook" .. (i + 1) .. ".png"}
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
UiTitleScreen = image{"assets/titlescreen.png"}
--UiBannerFaceCompleteBlue = image{"assets/ui_top_facecomplete_blue.png"}
UiBannerFaceCompleteOrange = image{"assets/ui_top_facecomplete_orange.png"}
UiBackground = image{"assets/ui_background.png"}
UiPanel = image{"assets/plain_panel.png"}
UiClueBlank = image{"assets/panel_clue_blank.png"}
UiResume = image {"assets/pausemenu_resume.png"}
UiRestart = image {"assets/pausemenu_restart_puzzle.png"}
UiEndGameNavExit = image{"assets/end_navigation_exit.png"}
UiCongratulations = image{"assets/panel_congratulations.png"}
--UiGoBlue = image{"assets/go_blue.png", pinned = true}
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
StoryRibbonComplete = image{"assets/ribbon_story_complete.png"}
StoryBookStartNext = image{"assets/panel_start_next_book.png"}
StoryCongratulationsUnlock = image{"assets/panel_congratulations_unlock.png"}

-- Sounds
SoundCutsceneChatter = sound{"assets/sound/CutsceneChatter.raw"}
SoundFaceComplete = sound{"assets/sound/FaceComplete.raw"}
SoundFaceCompleteAll = sound{"assets/sound/FaceCompleteAll.raw"}
SoundFreePlayReset = sound{"assets/sound/FreePlayReset.raw"}
SoundGameStart = sound{"assets/sound/GameStart.raw"}
SoundHintBlink = sound{"assets/sound/HintBlink.raw"}
SoundMenuConfirm = sound{"assets/sound/MenuConfirm.raw"}
SoundMenuDeneighbor = sound{"assets/sound/MenuDeneighbor.raw"}
SoundMenuNeighbor = sound{"assets/sound/MenuNeighbor.raw"}
SoundMenuStart = sound{"assets/sound/MenuStart.raw"}
SoundMenuTiltStart = sound{"assets/sound/MenuTiltStart.raw"}
SoundMenuTiltStop = sound{"assets/sound/MenuTiltStop.raw"}
SoundPause = sound{"assets/sound/Pause.raw"}
SoundPieceLand = sound{"assets/sound/PieceLand.raw"}
SoundPieceNudge = sound{"assets/sound/PieceNudge.raw"}
SoundPiecePinch = sound{"assets/sound/PiecePinch.raw"}
SoundPieceSlide = sound{"assets/sound/PieceSlide.raw"}
SoundShuffleBegin = sound{"assets/sound/ShuffleBegin.raw"}
SoundShuffleEnd = sound{"assets/sound/ShuffleEnd.raw"}
SoundStoryChapterTitle = sound{"assets/sound/StoryChapterTitle.raw"}
SoundSwapFail = sound{"assets/sound/SwapFail.raw"}
SoundTransition = sound{"assets/sound/Transition.raw"}
SoundUnpause = sound{"assets/sound/Unpause.raw"}
