#include "Game.h"
#include "Dialog.h"

static AssetSlot MainSlot = AssetSlot::allocate()
	.bootstrap(SandwichAssets);

static Metadata M = Metadata()
	.title("Sandwich Kingdom")
	.cubeRange(NUM_CUBES);

#if SFX_ON
AudioChannel gChannelSfx;
#endif
#if MUSIC_ON
AudioChannel gChannelMusic;
#endif

void main() {
	while(1) {
		gGame.MainLoop();
	}
}
