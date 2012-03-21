#pragma once

#include <sifteo.h>

namespace TotalsGame
{
	class AudioPlayer
	{
	public:
		static void Init();

		static void PlaySfx(const AssetAudio& handle, bool preempt=true);
		static void PlayMusic(const AssetAudio& music, bool loop=true);

        static void MuteMusic(bool mute);
        static void MuteSfx(bool mute);
        static bool MusicMuted();
        static bool SfxMuted();

		static void PlayShutterOpen();
		static void PlayShutterClose();
		static void PlayInGameMusic();
        static void PlayNeighborAdd();
        static void PlayNeighborRemove();

	private:
        static const int NumSfxChannels = 1;

		static AudioChannel channelSfx[NumSfxChannels];
		static AudioChannel channelMusic;
	};

}
