#include "../Interfaces/SystemWindows.h"

namespace Engine
{
	namespace Windows
	{
		ObjectArray<IScreen> * GetActiveScreens(void) { return 0; }
		IScreen * GetDefaultScreen(void) { return 0; }
		ITheme * GetCurrentTheme(void) { return 0; }
		IWindowSystem * GetWindowSystem(void) { return 0; }
	}
}