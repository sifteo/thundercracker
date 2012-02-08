#include "AudioPlayer.h"
#include "assets.gen.h"

namespace TotalsGame
{
	AudioChannel AudioPlayer::channelSfx[NumSfxChannels];
	AudioChannel AudioPlayer::channelMusic;

	void AudioPlayer::Init()
	{
		for(int i = 0; i < NumSfxChannels; i++)
			channelSfx[i].init();
		channelMusic.init();
	}

	void AudioPlayer::PlaySfx(_SYSAudioModule& handle, bool preempt)
	{
		//find a nonplaying channel
		int index = 0;
		for(int i = 0; i < NumSfxChannels; i++)
		{
			if(!channelSfx[i].isPlaying())
			{
				index = i;
			}
		}


		if (channelSfx[index].isPlaying()) {
			if (preempt)
			{
				channelSfx[index].stop();
			} 
			else
			{
				return;
			}
		}
		channelSfx[index].play(handle);
	}

	void AudioPlayer::PlayMusic(_SYSAudioModule& music, bool loop)
	{
		if (channelMusic.isPlaying())
		{
			channelMusic.stop();
		}
		channelMusic.play(music, loop ? LoopRepeat : LoopOnce);
	}




    void AudioPlayer::PlayShutterOpen() 
	{
		static int parity = 0;
		parity = 1-parity;
		if(parity)
			PlaySfx(sfx_Slide_LessScrape_02);
		else
			PlaySfx(sfx_Slide_LessScrape_01);
    }

    void AudioPlayer::PlayShutterClose() 
	{ 
		PlaySfx(sfx_Slide_LessScrape_Close_01); 
	}
}