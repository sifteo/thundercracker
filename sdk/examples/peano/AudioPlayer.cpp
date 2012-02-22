#include "AudioPlayer.h"
#include "assets.gen.h"

#include "Game.h"

#define ENABLE_AUDIO 01

namespace TotalsGame
{
#if ENABLE_AUDIO
    AudioChannel AudioPlayer::channelSfx[NumSfxChannels];
    AudioChannel AudioPlayer::channelMusic;
#endif

	void AudioPlayer::Init()
	{
#if ENABLE_AUDIO
		for(int i = 0; i < NumSfxChannels; i++)
			channelSfx[i].init();
		channelMusic.init();
#endif
	}

    void AudioPlayer::MuteMusic(bool mute)
    {
#if ENABLE_AUDIO
        channelMusic.setVolume(mute?0:Audio::MAX_VOLUME);
#endif
    }

    void AudioPlayer::MuteSfx(bool mute)
    {
#if ENABLE_AUDIO
        for(int i = 0; i < NumSfxChannels; i++)
        {
            channelSfx[i].setVolume(mute?0:Audio::MAX_VOLUME);
        }
#endif
    }

    bool AudioPlayer::MusicMuted()
    {
#if ENABLE_AUDIO
        return channelMusic.volume() == 0;
#else
        return false;
#endif
    }

    bool AudioPlayer::SfxMuted()
    {
#if ENABLE_AUDIO
        return channelSfx[0].volume() == 0;
#else
        return false;
#endif
    }

	void AudioPlayer::PlaySfx(_SYSAudioModule& handle, bool preempt)
	{
#if ENABLE_AUDIO
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
#endif
	}

	void AudioPlayer::PlayMusic(_SYSAudioModule& music, bool loop)
    {
#if ENABLE_AUDIO
		if (channelMusic.isPlaying())
		{
			channelMusic.stop();
		}
		channelMusic.play(music, loop ? LoopRepeat : LoopOnce);
#endif
	}




    void AudioPlayer::PlayShutterOpen() 
    {
#if ENABLE_AUDIO
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
#if ENABLE_AUDIO
		PlaySfx(sfx_Slide_LessScrape_Close_01); 
#endif
	}

	void AudioPlayer::PlayInGameMusic() 
	{
#if ENABLE_AUDIO
		static const int musicCount = 3;
		static _SYSAudioModule *sInGameMusic[3] = 
		{
			&sfx_PeanosVaultCrimeWave,
			&sfx_PeanosVaultInsideJob,
			&sfx_PeanosVaultSneakers
		};

		if (Game::GetInstance().currentPuzzle == NULL) 
		{
			PlayMusic(*sInGameMusic[Game::rand.randrange(musicCount)]);
		} 
		else
        {
			int i = Game::GetInstance().database.IndexOfChapter(Game::GetInstance().currentPuzzle->chapter);
			PlayMusic(*sInGameMusic[i % musicCount]);
		}
#endif
	}

    void AudioPlayer::PlayNeighborAdd()
    {
#if ENABLE_AUDIO
        PlaySfx(sfx_Connect);
#endif
    }

    void AudioPlayer::PlayNeighborRemove()
    {
#if ENABLE_AUDIO
        PlaySfx(sfx_Fast_Tick);
#endif
    }

}
