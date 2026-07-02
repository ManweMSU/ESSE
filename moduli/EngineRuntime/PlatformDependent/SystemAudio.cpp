#include "../Interfaces/SystemAudio.h"

namespace Engine
{
	namespace Audio
	{
		IAudioCodec * InitializeSystemCodec(void) { return 0; }
		IAudioDeviceFactory * CreateSystemAudioDeviceFactory(void) { return 0; }
		void SystemBeep(void) {}
	}
}