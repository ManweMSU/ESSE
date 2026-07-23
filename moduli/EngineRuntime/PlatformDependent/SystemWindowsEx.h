#pragma once

#include "../Interfaces/SystemWindows.h"
#include <Cor/IO/CorWindows.h>

namespace Engine
{
	namespace ESSEIO
	{
		class IRenderCallback
		{
		public:
			virtual void Render(Windows::ICoreWindow * window, const ESSE::Rectangle & margins, const ESSE::Color & clear_color, bool clear) noexcept = 0;
		};
		void SetWindowRenderCallback(Windows::ICoreWindow * window, IRenderCallback * callback) noexcept;
		void SetWindowUserRenderCallback(Windows::ICoreWindow * window) noexcept;
		void SetWindowNullRenderCallback(Windows::ICoreWindow * window) noexcept;
		void InvalidateWindow(Windows::ICoreWindow * window) noexcept;
		Windows::ICursor * GetCurrentlySetCursor(void) noexcept;
	}
}