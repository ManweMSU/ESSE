#pragma once

#include <Cor/Images/CorGraphics.h>

namespace ESSE
{
	namespace Graphica
	{
		oref<IDeviceFactory> CreateDeviceFactory(ErrorContext & ectx) noexcept;
		oref<IDeviceContextFactory2D> CreateDeviceContextFactory2D(ErrorContext & ectx) noexcept;
		oref<IDeviceFactory> CreateDeviceFactory(void);
		oref<IDeviceContextFactory2D> CreateDeviceContextFactory2D(void);
	}
}