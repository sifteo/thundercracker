#include "Common.h"

Math::Random gRandom;

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

#if SFX_ON
void PlaySfx(const AssetAudio& handle, bool preempt) {
  if (gChannelSfx.isPlaying()) {
    if (preempt) {
      gChannelSfx.stop();
    } else {
      return;
    }
  }
  gChannelSfx.play(handle);
}
#endif

#if MUSIC_ON
void PlayMusic(const AssetAudio& music, bool loop) {
  if (gChannelMusic.isPlaying()) {
    gChannelMusic.stop();
  }
  gChannelMusic.play(music, loop ? LoopRepeat : LoopOnce);
}
#endif
