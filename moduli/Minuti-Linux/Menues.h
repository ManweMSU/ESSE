#pragma once

#include <Fenestrae/Fenestrae.h>

namespace ESSE
{
	namespace Linux
	{
		class IMenuService : public Object
		{
		public:
			virtual oref<Windows::IMenu> CreateMenu(void) noexcept = 0;
			virtual oref<Windows::IMenuItem> CreateMenuItem(void) noexcept = 0;
			virtual Object * GetVisuals(Windows::IScreen * screen, Windows::ITheme * theme) noexcept = 0;
			virtual Windows::IWindowSystem * GetWindowSystem(void) noexcept = 0;
			virtual bool IsRunningMenu(void) noexcept = 0;
			virtual void SetIsRunningMenu(bool set) noexcept = 0;
			static oref<IMenuService> CreateInstance(Windows::IWindowSystem * system) noexcept;
		};
	}
}