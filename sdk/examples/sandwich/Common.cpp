#include "Common.h"

Random gRandom;

Side InferDirection(Int2 u) {
	if (u.x > 0) {
		return RIGHT;
	} else if (u.x < 0) {
		return LEFT;
	} else if (u.y < 0) {
		return TOP;
	} else {
		return BOTTOM;
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
