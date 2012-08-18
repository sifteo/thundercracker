#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

static AssetSlot MainSlot = AssetSlot::allocate();

static Metadata M = Metadata()
    .title("BG0_ROM test")
    .cubeRange(1);

static VideoBuffer vid;
const CubeID cube(0);

void testText()
{
    vid.initMode(BG0_ROM);

    for (char c = ' '; c <= '~'; c++) {
        char str[2] = { c, 0 };
        vid.bg0rom.text(vec(c & 15, c >> 4), str);
    }

    System::paint();
    System::finish();

    SCRIPT(LUA, util:assertScreenshot(cube, 'text'));
}

void testBargraph()
{
    vid.initMode(BG0_ROM);

    for (unsigned i = 0; i < 16; i++) {
        vid.bg0rom.hBargraph(vec(4U,i), i, vid.bg0rom.ORANGE, 1);
    }

    System::paint();
    System::finish();
    SCRIPT(LUA, util:assertScreenshot(cube, 'bargraph'));
}

void testColors()
{
    vid.initMode(BG0_ROM);

    static const struct {
        const char *name;
        BG0ROMDrawable::Palette value;
    } colors[] = {
        { "black_on_white",    BG0ROMDrawable::BLACK_ON_WHITE },
        { "blue_on_white",     BG0ROMDrawable::BLUE_ON_WHITE },
        { "orange_on_white",   BG0ROMDrawable::ORANGE_ON_WHITE },
        { "yellow_on_blue",    BG0ROMDrawable::YELLOW_ON_BLUE },
        { "red_on_white",      BG0ROMDrawable::RED_ON_WHITE },
        { "gray_on_white",     BG0ROMDrawable::GRAY_ON_WHITE },
        { "white_on_black",    BG0ROMDrawable::WHITE_ON_BLACK },
        { "white_on_blue",     BG0ROMDrawable::WHITE_ON_BLUE },
        { "white_on_teal",     BG0ROMDrawable::WHITE_ON_TEAL },
        { "black_on_yellow",   BG0ROMDrawable::BLACK_ON_YELLOW },
        { "dkgray_on_ltgray",  BG0ROMDrawable::DKGRAY_ON_LTGRAY },
        { "green_on_white",    BG0ROMDrawable::GREEN_ON_WHITE },
        { "white_on_green",    BG0ROMDrawable::WHITE_ON_GREEN },
        { "purple_on_white",   BG0ROMDrawable::PURPLE_ON_WHITE },
        { "ltblue_on_dkblue",  BG0ROMDrawable::LTBLUE_ON_DKBLUE },
        { "gold_on_white",     BG0ROMDrawable::GOLD_ON_WHITE },
    };
    
    for (unsigned i = 0; i < arraysize(colors); ++i) {
        vid.bg0rom.text(vec(0U, i), colors[i].name, colors[i].value);
    }

    System::paint();
    System::finish();
    SCRIPT(LUA, util:assertScreenshot(cube, 'colors'));
}

void testMap()
{
    vid.initMode(BG0_ROM);
    vid.bg0.image(vec(0,0), Map);
    System::paint();
    System::finish();
    SCRIPT(LUA, util:assertScreenshot(cube, 'map'));
}

void main()
{
    while (!CubeSet::connected().test(cube))
        System::yield();

    SCRIPT(LUA,
        package.path = package.path .. ";../../lib/?.lua"
        require('siftulator')
        cube = Cube(0)
    );

    vid.attach(cube);

    testText();
    testBargraph();
    testColors();
    testMap();

    LOG("Success.\n");
}