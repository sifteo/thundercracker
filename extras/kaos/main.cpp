#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

AssetSlot slot = AssetSlot::allocate();
AssetLoader loader;
AssetConfiguration<1> config1;
AssetConfiguration<1> config2;
VideoBuffer g[3];

Metadata m = Metadata()
	.title("Kaos Monkey")
	.package("com.sifteo.extras.kaos", "0.1")
	.cubeRange(3);

void main() {
	loader.init();
	config1.append(slot, grp1);
	config2.append(slot, grp2);
	for(CubeID cid : CubeSet::connected()) {
		g[cid].attach(cid);
		g[cid].initMode(BG0_ROM);
	}
	TimeDelta time(0.f);
	for(;;) {
		auto startTime = SystemTime::now();
		loader.start(config1);
		for(CubeID cid : CubeSet::connected()) {
			g[cid].bg0rom.fill(vec(0,0), vec(16,16), BG0ROMDrawable::SOLID_BG);
		}
		while(!loader.isComplete()) {
			for(CubeID cid : CubeSet::connected()) {
				String<32> s;
				s << FixedFP(time.seconds(), 3, 3) << "s";
				g[cid].bg0rom.text(vec(1,1), s.c_str(), BG0ROMDrawable::BLUE);
				g[cid].bg0rom.text(vec(1,14), "Loading GRP1", BG0ROMDrawable::BLUE);
				g[cid].bg0rom.hBargraph(vec(0, 4), loader.cubeProgress(cid, 128), BG0ROMDrawable::ORANGE, 8);
			}
			System::paint();
		}
		loader.finish();
		time = SystemTime::now() - startTime;
		startTime = SystemTime::now();
		loader.start(config2);
		for(CubeID cid : CubeSet::connected()) {
			g[cid].bg0rom.fill(vec(0,0), vec(16,16), BG0ROMDrawable::SOLID_BG);
		}
		while(!loader.isComplete()) {
			for(CubeID cid : CubeSet::connected()) {
				String<32> s;
				s << FixedFP(time.seconds(), 3, 3) << "s";
				g[cid].bg0rom.text(vec(1,1), s.c_str(), BG0ROMDrawable::BLUE);
				g[cid].bg0rom.text(vec(1,14), "Loading GRP2", BG0ROMDrawable::BLUE);
				g[cid].bg0rom.hBargraph(vec(0, 4), loader.cubeProgress(cid, 128), BG0ROMDrawable::ORANGE, 8);
			}
			System::paint();
		}
		loader.finish();
		time = SystemTime::now() - startTime;
		startTime = SystemTime::now();
	}
}