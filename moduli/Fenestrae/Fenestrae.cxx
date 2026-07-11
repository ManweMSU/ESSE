#include "Fenestrae.h"

namespace ESSE
{
	namespace Windows
	{
		oref<IWindowSystem> _window_system;
		oref<IWindowSystem> AllocateWindowSystem(ErrorContext & ectx) noexcept;
		IWindowSystem * GetWindowSystem(void) noexcept { return _window_system; }
		IWindowSystem * CreateWindowSystem(ErrorContext & ectx) noexcept
		{
			if (_window_system) return _window_system;
			_window_system = AllocateWindowSystem(ectx);
			if (ErrorTest(ectx)) return 0;
			return _window_system;
		}
		IWindowSystem * CreateWindowSystem(void)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = CreateWindowSystem(ectx);
			ErrorThrow(ectx);
			return result;
		}
	}
}