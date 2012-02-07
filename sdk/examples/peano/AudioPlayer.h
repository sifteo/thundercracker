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

	private:
		static AudioChannel channelSfx;
		static AudioChannel channelMusic;
	};

}