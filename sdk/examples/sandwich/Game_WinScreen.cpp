#include "Game.h"
#include "Dialog.h"

static void ShowDialog(ViewSlot* pView, const AssetImage& detail, const char* msg) {
	System::paintSync();
	pView->GetCube()->vbuf.touch();
	System::paintSync();
	pView->HideSprites();
	BG1Helper overlay(*pView->GetCube());
	overlay.DrawAsset(Vec2(&detail == &NPC_Detail_pearl_detail ? 1 : 2, 0), detail);
	overlay.Flush();
	System::paintSync();
	pView->GetCube()->vbuf.touch();
	System::paintSync();
	pView->GetCube()->vbuf.touch();
	System::paintSync();
	pView->Graphics().setWindow(80, 48);
	Dialog diag;
	diag.Init(pView->GetCube());
	diag.Erase();
	diag.ShowAll(msg);
	diag.SetAlpha(0);
	System::paintSync();
	pView->GetCube()->vbuf.touch();
	System::paintSync();
	PlaySfx(sfx_neighbor);
	for(int i=0; i<16; ++i) {
		diag.SetAlpha(i<<4);
		System::paint();
	}
	diag.SetAlpha(255);
}

void Game::WinScreen() {
	ViewSlot* views[3] = { mPlayer.View(), ViewBegin(), ViewBegin() };

	ShowDialog(views[0], NPC_Detail_pearl_detail, "At last!\nThe Sandwich Eternis\nis complete!");
	
	SystemTime t = SystemTime::now();
	do { System::paint(); } while(SystemTime::now() - t < 2.f);
	if (views[1] == mPlayer.View()) { views[1]++; }
	
	ShowDialog(views[1], NPC_Detail_sprout_detail, "The kingdom is saved!");
	
	t = SystemTime::now();
	do { System::paint(); } while(SystemTime::now() - t < 2.f);
	while (views[2] == views[0] || views[2] == views[1]) { views[2]++; }
	
	ShowDialog(views[2], NPC_Detail_burgher_detail, "Noooo!!!");
	
	t = SystemTime::now();
	do { System::paint(); } while(SystemTime::now() - t < 5.f);

	System::paintSync();
	for(int i=0; i<3; ++i) {
		Dialog d;
		d.Init(views[i]->GetCube());
		d.Erase();
		d.Show("Thanks for Playing!");
	}
	PlaySfx(sfx_neighbor);
	System::paintSync();
	for(int i=0; i<3; ++i) {
		views[i]->GetCube()->vbuf.touch();
	}
	System::paintSync();

	t = SystemTime::now();
	do { System::paint(); } while(SystemTime::now() - t < 5.f);
}

