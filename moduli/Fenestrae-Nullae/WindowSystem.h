#pragma once

#include <Cor/IO/CorWindows.h>

namespace ESSE
{
	namespace Windows
	{
		oref<IWindowSystem> AllocateWindowSystem(ErrorContext & ectx) noexcept;
	}
}