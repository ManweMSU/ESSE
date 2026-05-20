#include <Cor/Tasks/CorShell.h>
#include <Cor/Tasks/CorProcesses.h>
#include <Cor/IO/CorStreams.h>
#include "CorBoot.h"

namespace ESSE
{
	bool ShellExecute(const string & path, ShellExecuteMode mode) noexcept
	{
		try {
			if (mode == ShellExecuteMode::OpenFile || mode == ShellExecuteMode::ShowDirectory) {
				CreateProcessDesc desc;
				desc.flags = Linux::CreateProcessSearchPathOnly | CreateProcessSearchPath | CreateProcessOverrideIO | CreateProcessDetached;
				desc.image = U"xdg-open";
				desc.command_line << IO::ExpandPath(path);
				desc.standard_input = desc.standard_output = desc.standard_error = IO::InvalidHandle;
				auto opener = CreateProcess(desc);
				opener->Wait();
				return opener->GetExitCode() == 0;
			} else if (mode == ShellExecuteMode::ShowFile) {
				CreateProcessDesc desc;
				desc.flags = Linux::CreateProcessSearchPathOnly | CreateProcessSearchPath | CreateProcessOverrideIO | CreateProcessDetached;
				desc.image = U"xdg-open";
				desc.command_line << IO::Path::GetDirectory(IO::ExpandPath(path));
				desc.standard_input = desc.standard_output = desc.standard_error = IO::InvalidHandle;
				auto opener = CreateProcess(desc);
				opener->Wait();
				return opener->GetExitCode() == 0;
			} else if (mode == ShellExecuteMode::OpenConsole) {
				constexpr const unichar32 * known_termemu[] = { U"x-terminal-emulator", U"alacritty", U"konsole", U"xterm", 0 };
				CreateProcessDesc desc;
				desc.flags = Linux::CreateProcessSearchPathOnly | CreateProcessSearchPath | CreateProcessOverrideIO | CreateProcessDetached | CreateProcessOverrideWorkingDirectory;
				desc.working_directory = path;
				desc.standard_input = desc.standard_output = desc.standard_error = IO::InvalidHandle;
				ErrorContext ectx;
				uintptr i = 0;
				while (known_termemu[i]) {
					ErrorClear(ectx);
					desc.image = known_termemu[i++];
					auto terminal = CreateProcess(desc, ectx);
					if (!ErrorTest(ectx)) return true;
				}
				return false;
			}
		} catch (...) { return false; }
	}
}