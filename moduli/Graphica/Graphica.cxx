#include "Graphica.h"

namespace ESSE
{
	namespace Graphica
	{
		oref<IDeviceFactory> CreateDeviceFactory(void) { ErrorContext ectx; ErrorClear(ectx); auto result = CreateDeviceFactory(ectx); ErrorThrow(ectx); return result; }
		oref<IDeviceContextFactory2D> CreateDeviceContextFactory2D(void) { ErrorContext ectx; ErrorClear(ectx); auto result = CreateDeviceContextFactory2D(ectx); ErrorThrow(ectx); return result; }
	}
}