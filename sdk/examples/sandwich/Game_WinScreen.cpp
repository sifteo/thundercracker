#include "Game.h"

void Game::WinScreen() {
	for(ViewSlot *p=ViewBegin(); p!=ViewEnd(); ++p) {
		p->HideSprites();
		p->Graphics().BG0_drawAsset(Vec2(0,0), Sting);//hax
	}
	System::paintSync();
	for(ViewSlot *p=ViewBegin(); p!=ViewEnd(); ++p) {
		p->GetCube()->vbuf.touch();
	}
	System::paintSync();
	for(;;){System::paint();}
	
}