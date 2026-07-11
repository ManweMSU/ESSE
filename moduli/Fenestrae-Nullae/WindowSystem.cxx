#include "WindowSystem.h"

namespace ESSE
{
	namespace Windows
	{
		oref<IWindowSystem> AllocateWindowSystem(ErrorContext & ectx) noexcept { ErrorSet(ectx, Errores::ErrorNotImplemented); return 0; }
	}
}