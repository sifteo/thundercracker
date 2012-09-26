#include <sifteo.h>
#include "assets.gen.h"
using namespace Sifteo;

// METADATA

static Metadata M = Metadata()
    .title("Pairing Example")
    .package("com.sifteo.sdk.pairing", "0.1")
    .icon(Icon)
    .cubeRange(1, CUBE_ALLOCATION);


AssetSlot gMainSlot = AssetSlot::allocate()
	.bootstrap(BootstrapAssets);

// GLOBALS

static VideoBuffer vbuf[CUBE_ALLOCATION];
static CubeSet newCubes;
static CubeSet lostCubes;
static CubeSet reconnectedCubes;
static CubeSet dirtyCubes;
static CubeSet activeCubes;

static AssetLoader loader;
static AssetConfiguration<1> config;

// FUNCTIONS

static void repaintActiveCube(CubeID cid) {
	Int2 restPositions[4] = {
		vec(64 - Bars[0].pixelWidth()/2,0),
		vec(0, 64 - Bars[1].pixelHeight()/2),
		vec(64 - Bars[2].pixelWidth()/2, 128-Bars[2].pixelHeight()),
		vec(128-Bars[3].pixelWidth(), 64 - Bars[3].pixelHeight()/2)
	};
	auto& g = vbuf[cid];
	
	auto neighbors = g.physicalNeighbors();
	auto anyNeighbors = 0;
	for(int side=0; side<4; ++side) {
		if (neighbors.hasNeighborAt(Side(side))) {
			anyNeighbors = 1;
			g.sprites[side].setImage(Bars[side]);
			g.sprites[side].move(restPositions[side]);
		} else {
			g.sprites[side].hide();
		}
	}
	g.bg0.image(vec(0,0), Backgrounds, anyNeighbors);
}

static void initActiveCube(CubeID cid) {
	activeCubes.mark(cid);
	vbuf[cid].initMode(BG0_SPR_BG1);
	repaintActiveCube(cid);
}

static void paintWrapper() {
	// clear the palette
	newCubes.clear();
	lostCubes.clear();
	reconnectedCubes.clear();
	dirtyCubes.clear();

	// paint and collect events
	System::paint();

	// dynamically load assets just-in-time
	if (!(newCubes | reconnectedCubes).empty()) {
		if (loader.isComplete()) {
			loader.start(config);
		}
		while(!loader.isComplete()) {
			for(CubeID cid : (newCubes | reconnectedCubes)) {
				vbuf[cid].bg0rom.hBargraph(
					vec(0, 4), loader.cubeProgress(cid, 128), BG0ROMDrawable::ORANGE, 8
				);
			}
			System::paint();
		}
		loader.finish();
	}

	// repaint cubes
	for(CubeID cid : (newCubes|dirtyCubes)) {
		initActiveCube(cid);
	}
}

static void onCubeConnect(void* ctxt, unsigned cid) {
	if (lostCubes.test(cid)) {
		lostCubes.clear(cid);
		reconnectedCubes.mark(cid);
		dirtyCubes.mark(cid);
	} else {
		newCubes.mark(cid);
	}
	auto& g = vbuf[cid];
	g.attach(cid);
	g.initMode(BG0_ROM);
	g.bg0rom.fill(vec(0,0), vec(16,16), BG0ROMDrawable::SOLID_BG);
	g.bg0rom.text(vec(1,1), "Hold on!", BG0ROMDrawable::BLUE);
	g.bg0rom.text(vec(1,14), "Adding Cube...", BG0ROMDrawable::BLUE);
}

static void onCubeDisconnect(void* ctxt, unsigned cid) {
	lostCubes.mark(cid);
	newCubes.clear(cid);
	reconnectedCubes.clear(cid);
	dirtyCubes.clear(cid);
	activeCubes.clear(cid);
}

static void onCubeRefresh(void* ctxt, unsigned cid) {
	dirtyCubes.mark(cid);
}

static bool isActive(NeighborID nid) {
	return nid.isCube() && activeCubes.test(nid);
}

static void onNeighborAdd(void* ctxt, unsigned cube0, unsigned side0, unsigned cube1, unsigned side1) {
	if (isActive(cube0)) { repaintActiveCube(cube0); }
	if (isActive(cube1)) { repaintActiveCube(cube1); }
}

static void onNeighborRemove(void* ctxt, unsigned cube0, unsigned side0, unsigned cube1, unsigned side1) {
	if (isActive(cube0)) { repaintActiveCube(cube0); }
	if (isActive(cube1)) { repaintActiveCube(cube1); }
}

void main() {
	config.append(gMainSlot, BootstrapAssets);
	loader.init();

	Events::cubeConnect.set(onCubeConnect);
	Events::cubeDisconnect.set(onCubeDisconnect);
	Events::cubeRefresh.set(onCubeRefresh);

	Events::neighborAdd.set(onNeighborAdd);
	Events::neighborRemove.set(onNeighborRemove);
	
	for(CubeID cid : CubeSet::connected()) {
		vbuf[cid].attach(cid);
		initActiveCube(cid);
	}
	for(;;) {
		paintWrapper();
	}
}
