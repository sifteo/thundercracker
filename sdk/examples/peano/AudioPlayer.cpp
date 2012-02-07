#include "AudioPlayer.h"

namespace TotalsGame
{
	void AudioPlayer::Init()
	{
		channelSfx.init();
		channelMusic.init();
	}

	void AudioPlayer::PlaySfx(_SYSAudioModule& handle, bool preempt)
	{
		if (channelSfx.isPlaying()) {
			if (preempt)
			{
				channelSfx.stop();
			} 
			else
			{
				return;
			}
		}
		channelSfx.play(handle);
	}

	void AudioPlayer::PlayMusic(_SYSAudioModule& music, bool loop)
	{
		if (channelMusic.isPlaying())
		{
			channelMusic.stop();
		}
		channelMusic.play(music, loop ? LoopRepeat : LoopOnce);
	}

	AudioChannel AudioPlayer::channelSfx;
	AudioChannel AudioPlayer::channelMusic;
}