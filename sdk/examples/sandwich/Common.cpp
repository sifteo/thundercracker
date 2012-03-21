#include "Common.h"

Math::Random gRandom;

Cube::Side InferDirection(Int2 u) {
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

int AdvanceTowards(int curr, int targ, int mag) {
  if (curr > targ) {
    int x = curr - targ;
    if (x > mag) {
      return curr - mag;
    }
  } else if (curr < targ) {
    int x = targ - curr;
    if (x > mag) {
      return curr + mag;
    }
  }
  return targ;
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
