#pragma once

#include <Cor/CorBasis.h>

namespace ESSE
{
	namespace Linux
	{
		namespace Classes
		{
			inline const void * X11_Screen			= reinterpret_cast<const void *>(0x101);
			inline const void * X11_Window			= reinterpret_cast<const void *>(0x102);
			inline const void * X11_WindowSystem	= reinterpret_cast<const void *>(0x103);
		}
	}
}