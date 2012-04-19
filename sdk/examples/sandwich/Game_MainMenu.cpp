#include "Game.h"
#include <sifteo/menu.h>

// Can't we make these const?

static MenuAssets kMenuAssets = { 
	&MenuBackground, &MenuFooter, &MenuHeader, { 0 }
};
static MenuItem kMenuItems[] = { 
	{&MenuIconPearl, &MenuHeaderContinue}, 
	{&MenuIconSprout, &MenuHeaderReplayCavern},
	{&MenuIconHuh, &MenuHeaderLocked},
	{&MenuIconHuh, &MenuHeaderLocked},
	{&MenuIconHuh, &MenuHeaderLocked},
	{&MenuIconHuh, &MenuHeaderLocked},
	{&MenuIconHuh, &MenuHeaderLocked},
	{&MenuIconBurgher, &MenuHeaderClearData},



	{ 0, 0}
};

// don't forget credits!

Viewport* Game::MainMenu(Viewport* keyViewport) {

	// slide out
	auto startTime = SystemTime::now();
	auto deadline = startTime + 1.333f;
	auto& g = keyViewport->Canvas();
	unsigned rowsPlotted = 0;
	while(deadline.inFuture()) {
		float u = clamp(float(SystemTime::now() - startTime) / (1.333f-0.0001f), 0.f, 1.f);
		u = 1.f - (1.f-u)*(1.f-u)*(1.f-u)*(1.f-u);
		unsigned rowsNeeded = unsigned(16.f * u)+1;
		while(rowsNeeded > rowsPlotted) {
			rowsPlotted++;
			if (rowsPlotted <= 3) {
				g.bg0.image(vec<unsigned>(0,18-rowsPlotted), vec<unsigned>(16,1), MenuFooter, vec<unsigned>(0, 3-rowsPlotted));
			} else if (rowsPlotted >= 14) {
				g.bg0.image(vec<unsigned>(0,18-rowsPlotted), vec<unsigned>(16,1), MenuHeader, vec<unsigned>(0, 16-rowsPlotted));
			} else {
				g.bg0.span(vec<unsigned>(0, 18-rowsPlotted), 16, MenuBackground);
			}
		}
		g.bg0.setPanning(vec<unsigned>(0, -int(128 * u)));
		DoPaint();
	}

	System::finish();
	g.bg0.setPanning(vec(0,0));
	g.bg0.image(vec(0,0), MenuHeader);
	g.bg0.fill(vec(0, 3), vec(16, 16-3-3), MenuBackground);
	g.bg0.image(vec(0,16-3), MenuFooter);
	DoPaint();
	System::finish();

	Menu mainMenu(keyViewport->Canvas(), &kMenuAssets, kMenuItems);
	mainMenu.setIconYOffset(24);
	MenuEvent ev;
	unsigned scroll = 0;
	while(mainMenu.pollEvent(&ev)) {
		switch(ev.type) {
			case MENU_ITEM_PRESS:
				goto BreakOut;
				break;
			case MENU_PREPAINT:
			case MENU_UNEVENTFUL:
			case MENU_NEIGHBOR_ADD:
			case MENU_NEIGHBOR_REMOVE:
			case MENU_ITEM_ARRIVE:
			case MENU_ITEM_DEPART:
			case MENU_EXIT:
				break;
		}
	}
	BreakOut:
	keyViewport->Canvas().bg1.erase();
	DoWait(1.f);
	return keyViewport;
}