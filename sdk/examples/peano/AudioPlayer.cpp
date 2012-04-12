#include "AudioPlayer.h"
#include "assets.gen.h"

#include "Game.h"

namespace TotalsGame
{
#if SFX_ON
    AudioChannel AudioPlayer::channelSfx[NumSfxChannels];
    const AssetAudio *AudioPlayer::whatsPlaying[NumSfxChannels];
#endif
#if MUSIC_ON
    AudioChannel AudioPlayer::channelMusic;
#endif

    void AudioPlayer::MuteMusic(bool mute)
    {
#if MUSIC_ON
        channelMusic.setVolume(mute?0:Audio::MAX_VOLUME);
#endif
    }

    void AudioPlayer::MuteSfx(bool mute)
    {
#if SFX_ON
        for(int i = 0; i < NumSfxChannels; i++)
        {
            channelSfx[i].setVolume(mute?0:255);
        }
#endif
    }

    bool AudioPlayer::MusicMuted()
    {
#if MUSIC_ON
        return channelMusic.volume() == 0;
#else
        return false;        
#endif
    }

    bool AudioPlayer::SfxMuted()
    {
#if SFX_ON
        return channelSfx[0].volume() == 0;
#else
        return false;
#endif
    }

    void AudioPlayer::HaltSfx(const AssetAudio& handle)
    {
#if SFX_ON
        for(int i = 0; i < NumSfxChannels; i++)
        {
            if(whatsPlaying[i] == &handle)
            {
                channelSfx[i].stop();
            }
        }
#endif
    }

    void AudioPlayer::PlaySfx(const AssetAudio& handle, bool preempt)
	{
#if SFX_ON
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
        whatsPlaying[index] = &handle;
#endif
	}

	void AudioPlayer::PlayMusic(const AssetAudio& music, bool loop)
    {
#if MUSIC_ON
		if (channelMusic.isPlaying())
		{
			channelMusic.stop();
		}
		channelMusic.play(music, loop ? LoopRepeat : LoopOnce);
#endif
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
		static const AssetAudio *sInGameMusic[3] = 
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
