#pragma once

#include "../Classes/CorArray.hxx"
#include "../Classes/CorVolume.hxx"
#include "../Classes/CorTime.h"

namespace ESSE
{
	#if defined(ESSE_SYSTEMA_WINDOWS)
		constexpr unichar32 EnvironmentArraySeparator = U';';
	#elif defined(ESSE_SYSTEMA_UNIX)
		constexpr unichar32 EnvironmentArraySeparator = U':';
	#else
		constexpr unichar32 EnvironmentArraySeparator = U':';
	#endif

	class Process : public WaitableObject
	{
	public:
		virtual bool Terminate(void) noexcept = 0;
		virtual bool Exited(void) noexcept = 0;
		virtual int GetExitCode(void) noexcept = 0;
		virtual int GetPID(void) noexcept = 0;
		virtual bool IsGUI(void) noexcept = 0;
		virtual bool Activate(void) noexcept = 0;
	};
	class RunningProcess : public Object
	{
	public:
		virtual bool Terminate(void) noexcept = 0;
		virtual bool Exited(void) noexcept = 0;
		virtual int GetPID(void) noexcept = 0;
		virtual bool IsGUI(void) noexcept = 0;
		virtual bool Activate(void) noexcept = 0;
		virtual Time GetCreationTime(void) noexcept = 0;
		virtual string GetExecutablePath(ErrorContext & ectx) noexcept = 0;
		virtual string GetBundlePath(ErrorContext & ectx) noexcept = 0;
		virtual string GetBundleIdentifier(ErrorContext & ectx) noexcept = 0;

		string GetExecutablePath(void);
		string GetBundlePath(void);
		string GetBundleIdentifier(void);
	};

	enum CreateProcessFlags : uint {
		CreateProcessSearchPath					= 0x01,
		CreateProcessElevated					= 0x02,
		CreateProcessOverrideIO					= 0x04,
		CreateProcessOverrideWorkingDirectory	= 0x08,
		CreateProcessOverrideEnvironment		= 0x10,
		CreateProcessDetached					= 0x20,
	};
	struct CreateProcessDesc
	{
		uint flags;
		string image;
		string working_directory;
		array<string> command_line;
		Dictionary<string, string> environment;
		handle standard_input;
		handle standard_output;
		handle standard_error;
	};
	oref<Process> CreateProcess(const CreateProcessDesc & desc, ErrorContext & ectx) noexcept;
	oref<array<string>> GetCommandLine(ErrorContext & ectx) noexcept;
	oref<Dictionary<string, string>> GetEnvironment(ErrorContext & ectx) noexcept;

	void Sleep(uint32 time) noexcept;
	void ExitProcess(int exit_code) noexcept;
	bool IsProcessElevated(void) noexcept;

	oref<RunningProcess> OpenProcess(int pid, ErrorContext & ectx) noexcept;
	int GetThisProcessPID(void) noexcept;
	oref<array<int>> EnumerateProcesses(ErrorContext & ectx) noexcept;
	oref<array<int>> EnumerateProcesses(const string & bundle, ErrorContext & ectx) noexcept;

	oref<Process> CreateProcess(const CreateProcessDesc & desc);
	oref<array<string>> GetCommandLine(void);
	oref<Dictionary<string, string>> GetEnvironment(void);
	oref<RunningProcess> OpenProcess(int pid);
	oref<array<int>> EnumerateProcesses(void);
	oref<array<int>> EnumerateProcesses(const string & bundle);
}