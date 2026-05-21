#include "../Interfaces/Shell.h"
#include "../Interfaces/SystemIO.h"
#include "../Interfaces/Process.h"

namespace Engine
{
	namespace Shell
	{
		bool OpenFile(const string & file)
		{
			return ESSE::ShellExecute(static_cast<const ESSE::unichar32 *>(file), ESSE::ShellExecuteMode::OpenFile);
		}
		void ShowInBrowser(const string & path, bool directory)
		{
			if (directory) ESSE::ShellExecute(static_cast<const ESSE::unichar32 *>(path), ESSE::ShellExecuteMode::ShowDirectory);
			else ESSE::ShellExecute(static_cast<const ESSE::unichar32 *>(path), ESSE::ShellExecuteMode::ShowFile);
		}
		void OpenCommandPrompt(const string & working_directory)
		{
			ESSE::ShellExecute(static_cast<const ESSE::unichar32 *>(working_directory), ESSE::ShellExecuteMode::OpenConsole);
		}
	}
}