#include "AudioPlayer.h"
#include "assets.gen.h"

#include "Game.h"

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

    void AudioPlayer::MuteMusic(bool mute)
    {
        channelMusic.setVolume(mute?0:Audio::MAX_VOLUME);
    }

    void AudioPlayer::MuteSfx(bool mute)
    {
        for(int i = 0; i < NumSfxChannels; i++)
        {
            channelSfx[i].setVolume(mute?0:Audio::MAX_VOLUME);
        }
    }

    bool AudioPlayer::MusicMuted()
    {
        return channelMusic.volume() == 0;
        return false;
    }

    bool AudioPlayer::SfxMuted()
    {
        return channelSfx[0].volume() == 0;
        return false;
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
#if SFX_ON
		static int parity = 0;
		parity = 1-parity;
		if(parity)
			PlaySfx(sfx_Slide_LessScrape_02);
		else
			PlaySfx(sfx_Slide_LessScrape_01);
#endif
    }

    void AudioPlayer::PlayShutterClose() 
	{ 
#if SFX_ON
		PlaySfx(sfx_Slide_LessScrape_Close_01); 
#endif
	}

	void AudioPlayer::PlayInGameMusic() 
	{
#if MUSIC_ON
		static const int musicCount = 3;
		static _SYSAudioModule *sInGameMusic[3] = 
		{
			&sfx_PeanosVaultCrimeWave,
			&sfx_PeanosVaultInsideJob,
			&sfx_PeanosVaultSneakers
		};

		if (Game::currentPuzzle == NULL) 
		{
			PlayMusic(*sInGameMusic[Game::rand.randrange(musicCount)]);
		} 
		else
        {
            int i = Game::currentPuzzle->chapterIndex;
			PlayMusic(*sInGameMusic[i % musicCount]);
		}
#endif
	}

    void AudioPlayer::PlayNeighborAdd()
    {
#if SFX_ON
        PlaySfx(sfx_Connect);
#endif
    }

    void AudioPlayer::PlayNeighborRemove()
    {
#if SFX_ON
        PlaySfx(sfx_Fast_Tick);
#endif
    }

}
