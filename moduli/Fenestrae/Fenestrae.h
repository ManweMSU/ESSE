#pragma once

#include <Cor/IO/CorWindows.h>

namespace ESSE
{
	namespace Windows
	{
		IWindowSystem * GetWindowSystem(void) noexcept;
		IWindowSystem * CreateWindowSystem(ErrorContext & ectx) noexcept;
		IWindowSystem * CreateWindowSystem(void);
	}
}