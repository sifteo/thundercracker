#include "Game.h"
#include <sifteo/menu.h>

// Can't we make these const?

static MenuAssets kMenuAssets = { 
	&MenuBackground, &MenuFooter, &MenuHeader, { 0 }
};
static MenuItem kMenuItems[] = { 
	{&MenuIconPearl, &MenuHeaderContinue}, 
	{&MenuIconSprout, &MenuHeaderReplayCavern},
	{&MenuIconBurgher, &MenuHeaderClearData},
	{ 0, 0}
};

Viewport* Game::MainMenu(Viewport* keyViewport) {
	Menu mainMenu(keyViewport->Canvas(), &kMenuAssets, kMenuItems);
	System::finish();
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