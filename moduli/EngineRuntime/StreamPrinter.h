#pragma once

#include "SystemPrinting.h"
#include "PortableEncoder.h"

namespace Engine
{
	namespace Portable
	{
		enum StreamPrinterFlags : uint {
			StreamPrinterCloseContext	= 0x800000
		};

		Graphics::IPrintingContext * CreateStreamPrinter(IEncoderContext * ctx, const Graphics::PrinterModeDesc & desc, uint pdf_flags);
	}
}