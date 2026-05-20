#include "CorProcesses.h"

namespace ESSE
{
	string RunningProcess::GetExecutablePath(void) { ErrorContext ectx; ErrorClear(ectx); auto result = GetExecutablePath(ectx); ErrorThrow(ectx); return result; }
	string RunningProcess::GetBundlePath(void) { ErrorContext ectx; ErrorClear(ectx); auto result = GetBundlePath(ectx); ErrorThrow(ectx); return result; }
	string RunningProcess::GetBundleIdentifier(void) { ErrorContext ectx; ErrorClear(ectx); auto result = GetBundleIdentifier(ectx); ErrorThrow(ectx); return result; }

	oref<Process> CreateProcess(const CreateProcessDesc & desc) { ErrorContext ectx; ErrorClear(ectx); auto result = CreateProcess(desc, ectx); ErrorThrow(ectx); return result; }
	oref<array<string>> GetCommandLine(void) { ErrorContext ectx; ErrorClear(ectx); auto result = GetCommandLine(ectx); ErrorThrow(ectx); return result; }
	oref<Dictionary<string, string>> GetEnvironment(void) { ErrorContext ectx; ErrorClear(ectx); auto result = GetEnvironment(ectx); ErrorThrow(ectx); return result; }
	oref<RunningProcess> OpenProcess(int pid) { ErrorContext ectx; ErrorClear(ectx); auto result = OpenProcess(pid, ectx); ErrorThrow(ectx); return result; }
	oref<array<int>> EnumerateProcesses(void) { ErrorContext ectx; ErrorClear(ectx); auto result = EnumerateProcesses(ectx); ErrorThrow(ectx); return result; }
	oref<array<int>> EnumerateProcesses(const string & bundle) { ErrorContext ectx; ErrorClear(ectx); auto result = EnumerateProcesses(bundle, ectx); ErrorThrow(ectx); return result; }
}