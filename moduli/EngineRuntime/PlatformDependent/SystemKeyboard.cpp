#include "../Interfaces/KeyCodes.h"
#include <Fenestrae/Fenestrae.h>

namespace Engine
{
	namespace Keyboard
	{
		bool IsKeyPressed(uint key_code) { auto ws = ESSE::Windows::GetWindowSystem(); return ws ? ws->GetKeyboardManager()->IsKeyPressed(key_code) : false; }
		bool IsKeyToggled(uint key_code) { auto ws = ESSE::Windows::GetWindowSystem(); return ws ? ws->GetKeyboardManager()->IsKeyToggled(key_code) : false; }
		int GetKeyboardDelay(void) { auto ws = ESSE::Windows::GetWindowSystem(); return ws ? ws->GetKeyboardManager()->GetKeyboardDelay() : 1000; }
		int GetKeyboardSpeed(void) { auto ws = ESSE::Windows::GetWindowSystem(); return ws ? ws->GetKeyboardManager()->GetKeyboardSpeed() : 100; }
	}
}