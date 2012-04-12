#pragma once

#include "sifteo.h"
#include "config.h"

#if MUSIC_ON
#define PLAY_MUSIC(a) do{AudioPlayer::PlayMusic(a);}while(0)
#else
#define PLAY_MUSIC(a) do{}while(0)
#endif

#if SFX_ON
#define PLAY_SFX(a) do{AudioPlayer::PlaySfx(a);}while(0)
#define PLAY_SFX2(a,b) do{AudioPlayer::PlaySfx(a,b);}while(0)
#else
#define PLAY_SFX(a) do{}while(0)
#define PLAY_SFX2(a,b) do{}while(0)
#endif

using namespace Sifteo;

namespace TotalsGame
{
	class AudioPlayer
	{
	public:
		static void PlaySfx(const AssetAudio& handle, bool preempt=true);
		static void PlayMusic(const AssetAudio& music, bool loop=true);

        static void MuteMusic(bool mute);
        static void MuteSfx(bool mute);
        static bool MusicMuted();
        static bool SfxMuted();
        static void HaltSfx(const AssetAudio &handle);

		static void PlayShutterOpen();
		static void PlayShutterClose();
		static void PlayInGameMusic();
        static void PlayNeighborAdd();
        static void PlayNeighborRemove();

	private:
        static const int NumSfxChannels = 3;
#if SFX_ON
		static AudioChannel channelSfx[NumSfxChannels];
        static const AssetAudio *whatsPlaying[NumSfxChannels];
#endif
#if MUSIC_ON
		static AudioChannel channelMusic;
#endif
	};

}
