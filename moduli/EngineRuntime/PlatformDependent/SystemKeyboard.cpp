#include "../Interfaces/KeyCodes.h"

namespace Engine
{
	namespace Keyboard
	{
		bool IsKeyPressed(uint key_code) { return false; }
		bool IsKeyToggled(uint key_code) { return false; }
		int GetKeyboardDelay(void) { return 1000; }
		int GetKeyboardSpeed(void) { return 100; }
	}
}