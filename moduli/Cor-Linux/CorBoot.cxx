#include "CorBoot.h"
#include "CorUnicodeEx.h"
#include <Cor/CorVirtualMemory.h>
#include <Cor/Tasks/CorProcesses.h>

#include <locale.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

namespace ESSE
{
	namespace Linux
	{
		uintptr			__esse_reference_count = 0;
		int				__argument_count = 0;
		const char **	__argument_volume = 0;
		pid_t			__this_process_pid;

		void ApplicationInit(int argc, const char ** argv) noexcept
		{
			if (argc > 2 && strcmp(argv[1], "--esse-detach") == 0) {
				char ** command_line = reinterpret_cast<char **>(malloc(sizeof(char *) * (argc - 1)));
				if (!command_line) _exit(1);
				for (int i = 2; i < argc; i++) command_line[i - 2] = const_cast<char *>(argv[i]);
				command_line[argc - 2] = 0;
				auto fork_status = fork();
				if (fork_status < 0) _exit(1);
				if (fork_status == 0) {
					execvp(command_line[0], command_line);
					_exit(0);
				} else _exit(0);
			}
			#ifdef ESSE_ELEVATIO_REQUISITA
			if (!IsProcessElevated() && argv) {
				try {
					CreateProcessDesc desc;
					desc.flags = CreateProcessSearchPathOnly | CreateProcessSearchPath;
					#ifdef ESSE_SUBSYSTEMA_CONSOLE
					desc.image = U"sudo";
					#else
					desc.image = U"pkexec";
					#endif
					for (int i = 0; i < argc; i++) desc.command_line.Append(argv[i]);
					auto proc = CreateProcess(desc);
					proc->Wait();
					ExitProcess(proc->GetExitCode());
				} catch (...) { ExitProcess(-1); }
			}
			#endif
			if (argv) { __argument_volume = argv; __argument_count = argc; }
			if (InterlockedIncrement(__esse_reference_count) == 1) {
				setlocale(LC_ALL, "");
				Unicode::Linux_UnicodeLibraryInitialize();
				__this_process_pid = getpid();
				signal(SIGPIPE, SIG_IGN);
			}
		}
		void ApplicationShutdown(void) noexcept {}
		void GetCommandLine(int * argc, const char *** argv) noexcept { *argc = __argument_count; *argv = __argument_volume; }
		void GetThisProcessPID(pid_t * pid) noexcept { *pid = __this_process_pid; }
	}
}

#if defined(ESSE_SUBSYSTEMA_LIBRARY)
ESSE_LIBRARY_INITIALIZE_ROUTINE;
ESSE_LIBRARY_SHUTDOWN_ROUTINE;
__attribute__((constructor)) static void __esse_lib_init(void) { ESSE::Linux::ApplicationInit(0, 0); LibraryInitialize(); }
__attribute__((destructor)) static void __esse_lib_shutdown(void) { LibraryShutdown(); ESSE::Linux::ApplicationShutdown(); }
#else
ESSE_MAIN_ROUTINE;
int main(int argc, char ** argv) { ESSE::Linux::ApplicationInit(argc, argv); int result = Main(); ESSE::Linux::ApplicationShutdown(); return result; }
#endif