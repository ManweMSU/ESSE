#include "../Interfaces/SystemAudio.h"
#include <Fenestrae/Fenestrae.h>

namespace Engine
{
	namespace Audio
	{
		IAudioCodec * InitializeSystemCodec(void) { return 0; }
		IAudioDeviceFactory * CreateSystemAudioDeviceFactory(void) { return 0; }
		void SystemBeep(void) { auto ws = ESSE::Windows::GetWindowSystem(); if (ws) ws->Beep(); }
	}
}