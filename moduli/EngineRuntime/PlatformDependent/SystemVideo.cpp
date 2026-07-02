#include "../Interfaces/SystemVideo.h"

namespace Engine
{
	namespace Video
	{
		IVideoCodec * InitializeSystemCodec(void) { return 0; }
		IVideoFactory * CreateSystemVideoFactory(void) { return 0; }
	}
}