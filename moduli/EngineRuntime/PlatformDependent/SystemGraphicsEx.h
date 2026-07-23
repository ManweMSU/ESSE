#pragma once

#include "../Interfaces/SystemGraphics.h"
#include <Cor/Images/CorGraphicsExtensions.h>

namespace Engine
{
	namespace ESSEIO
	{
		Graphics::I2DDeviceContext * WrapContext(ESSE::Graphica::IDeviceContext2D * context, Graphics::IDevice * controlling_device) noexcept;
		Graphics::I2DDeviceContext * WrapContext(ESSE::Graphica::IDeviceContext2D * context, ESSE::Graphica::DeviceCache * cache) noexcept;
		Graphics::IDevice * WrapDevice(ESSE::Graphica::IDevice * device) noexcept;
		ESSE::Graphica::IDevice * UnwrapDevice(Graphics::IDevice * device) noexcept;
		ESSE::Graphica::IDeviceContext2D * UnwrapContext(Graphics::I2DDeviceContext * context) noexcept;
	}
}