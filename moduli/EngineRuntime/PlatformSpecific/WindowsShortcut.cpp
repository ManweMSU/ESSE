#include "WindowsShortcut.h"

namespace Engine
{
	namespace WindowsSpecific
	{
		void CreateShellLink(const string & link, const string & path, const string & description) {}
		string GetShellFolderPath(ShellFolder folder) { return U""; }
	}
}