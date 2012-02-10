#pragma once

#include <sifteo.h>

namespace TotalsGame
{
	class AudioPlayer
	{
	public:
		static void Init();

		static void PlaySfx(_SYSAudioModule& handle, bool preempt=true);
		static void PlayMusic(_SYSAudioModule& music, bool loop=true);

		static void PlayShutterOpen();
		static void PlayShutterClose();
		static void PlayInGameMusic();
        static void PlayNeighborAdd();
        static void PlayNeighborRemove();

	private:
		static const int NumSfxChannels = 3;

		static AudioChannel channelSfx[NumSfxChannels];
		static AudioChannel channelMusic;
	};

}
