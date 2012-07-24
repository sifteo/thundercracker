#include <sifteo.h>
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
        { "black",      BG0ROMDrawable::BLACK, },
        { "blue",       BG0ROMDrawable::BLUE, },
        { "orange",     BG0ROMDrawable::ORANGE, },
        { "invorange",  BG0ROMDrawable::INVORANGE, },
        { "red",        BG0ROMDrawable::RED, },
        { "gray",       BG0ROMDrawable::GRAY, },
        { "inv",        BG0ROMDrawable::INV, },
        { "invgray",    BG0ROMDrawable::INVGRAY, },
        { "ltblue",     BG0ROMDrawable::LTBLUE, },
        { "ltorange",   BG0ROMDrawable::LTORANGE, },
    };
    
    for (unsigned i = 0; i < arraysize(colors); ++i) {
        vid.bg0rom.span(vec(0U, i), 2,
            colors[i].value ^ BG0ROMDrawable::SOLID_FG);

        vid.bg0rom.span(vec(2U, i), 14,
            colors[i].value ^ BG0ROMDrawable::SOLID_BG);

        vid.bg0rom.text(vec(4U, i), colors[i].name, colors[i].value);
        vid.bg0rom.hBargraph(vec(14U, i), 12, colors[i].value);
    }

    System::paint();
    System::finish();
    SCRIPT(LUA, util:assertScreenshot(cube, 'colors'));
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

    LOG("Success.\n");
}