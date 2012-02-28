#include "Common.h"

Math::Random gRandom;

const PinnedAssetImage* const kStorageTypeToIcon[3] = {
  &InventoryIcons, &KeyIcons, &EquipmentIcons
};

Cube::Side InferDirection(Vec2 u) {
	if (u.x > 0) {
		return SIDE_RIGHT;
	} else if (u.x < 0) {
		return SIDE_LEFT;
	} else if (u.y < 0) {
		return SIDE_TOP;
	} else {
		return SIDE_BOTTOM;
	}
}

//------------------------------------------------------------------------------
// Sfx Utilities
//------------------------------------------------------------------------------

void PlaySfx(_SYSAudioModule& handle, bool preempt) {
#if SFX_ON
  if (gChannelSfx.isPlaying()) {
    if (preempt) {
      gChannelSfx.stop();
    } else {
      return;
    }
  }
  gChannelSfx.play(handle);
#endif
}

void PlayMusic(_SYSAudioModule& music, bool loop) {
#if MUSIC_ON
  if (gChannelMusic.isPlaying()) {
    gChannelMusic.stop();
  }
  gChannelMusic.play(music, loop ? LoopRepeat : LoopOnce);
#endif
}
