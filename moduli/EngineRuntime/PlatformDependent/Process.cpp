#include "../Interfaces/Process.h"

namespace Engine
{
	class SystemProcess : public Process
	{
		ESSE::oref<ESSE::Process> _proc;
	public:
		SystemProcess(ESSE::Process * proc) : _proc(proc) {}
		virtual ~SystemProcess(void) override {}
		virtual bool Exited(void) noexcept override { return _proc->Exited(); }
		virtual int GetExitCode(void) noexcept override { return _proc->GetExitCode(); }
		virtual int GetPID(void) noexcept override { return _proc->GetPID(); }
		virtual bool IsGUI(void) noexcept override { return _proc->IsGUI(); }
		virtual bool Activate(void) noexcept override { return _proc->Activate(); }
		virtual void Wait(void) noexcept override { _proc->Wait(); }
		virtual void Terminate(void) noexcept override { _proc->Terminate(); }
	};

	Process * CreateProcess(const string & image, const Array<string> * command_line) noexcept
	{
		try {
			ESSE::ErrorContext ectx; ESSE::ErrorClear(ectx);
			ESSE::CreateProcessDesc desc;
			desc.flags = 0;
			desc.image = static_cast<const ESSE::unichar32 *>(image);
			if (command_line) for (auto & c : *command_line) desc.command_line.Append(static_cast<const ESSE::unichar32 *>(c));
			auto proc = ESSE::CreateProcess(desc, ectx);
			ESSE::ErrorThrow(ectx);
			return new SystemProcess(proc);
		} catch (...) { return 0; }
	}
	Process * CreateCommandProcess(const string & command_image, const Array<string> * command_line) noexcept
	{
		try {
			ESSE::ErrorContext ectx; ESSE::ErrorClear(ectx);
			ESSE::CreateProcessDesc desc;
			desc.flags = ESSE::CreateProcessSearchPath;
			desc.image = static_cast<const ESSE::unichar32 *>(command_image);
			if (command_line) for (auto & c : *command_line) desc.command_line.Append(static_cast<const ESSE::unichar32 *>(c));
			auto proc = ESSE::CreateProcess(desc, ectx);
			ESSE::ErrorThrow(ectx);
			return new SystemProcess(proc);
		} catch (...) { return 0; }
	}
	bool CreateProcessElevated(const string & image, const Array<string> * command_line) noexcept
	{
		try {
			ESSE::ErrorContext ectx; ESSE::ErrorClear(ectx);
			ESSE::CreateProcessDesc desc;
			desc.flags = ESSE::CreateProcessElevated;
			desc.image = static_cast<const ESSE::unichar32 *>(image);
			if (command_line) for (auto & c : *command_line) desc.command_line.Append(static_cast<const ESSE::unichar32 *>(c));
			auto proc = ESSE::CreateProcess(desc, ectx);
			if (ESSE::ErrorTest(ectx)) return false;
			return true;
		} catch (...) { return false; }
	}
	Array<string> * GetCommandLine(void)
	{
		SafePointer< Array<string> > result = new Array<string>(0x10);
		auto cmd = ESSE::GetCommandLine();
		for (auto & c : *cmd) result->Append(c.GetData());
		result->Retain();
		return result;
	}
	void Sleep(uint32 time) noexcept { ESSE::Sleep(time); }
	void ExitProcess(int exit_code) noexcept { ESSE::ExitProcess(exit_code); }
	bool IsProcessElevated(void) noexcept { return ESSE::IsProcessElevated(); }

	class SystemRunningProcess : public Engine::RunningProcess
	{
		ESSE::oref<ESSE::RunningProcess> _proc;
	public:
		SystemRunningProcess(ESSE::RunningProcess * proc) : _proc(proc) {}
		virtual ~SystemRunningProcess(void) override {}
		virtual bool Exited(void) noexcept override { return _proc->Exited(); }
		virtual int GetPID(void) noexcept override { return _proc->GetPID(); }
		virtual bool IsGUI(void) noexcept override { return _proc->IsGUI(); }
		virtual bool Activate(void) noexcept override { return _proc->Activate(); }
		virtual void Terminate(void) noexcept override { _proc->Terminate(); }
		virtual Time GetCreationTime(void) noexcept override { return _proc->GetCreationTime().Ticks; }
		virtual string GetExecutablePath(void) override { try { return _proc->GetExecutablePath().GetData(); } catch (...) { return U""; } }
		virtual string GetBundlePath(void) override { try { return _proc->GetBundlePath().GetData(); } catch (...) { return U""; } }
		virtual string GetBundleIdentifier(void) override { try { return _proc->GetBundleIdentifier().GetData(); } catch (...) { return U""; } }
	};
	RunningProcess * OpenProcess(int pid) noexcept { try { return new SystemRunningProcess(ESSE::OpenProcess(pid)); } catch (...) { return 0; } }
	int GetThisProcessPID(void) noexcept { return ESSE::GetThisProcessPID(); }
	Array<int> * EnumerateProcesses(void) noexcept
	{
		try {
			SafePointer< Array<int> > result = new Array<int>(0x100);
			auto proc = ESSE::EnumerateProcesses();
			for (auto & p : *proc) result->Append(p);
			result->Retain();
			return result;
		} catch (...) { return 0; }
	}
	Array<int> * EnumerateProcesses(const string & bundle) noexcept
	{
		try {
			SafePointer< Array<int> > result = new Array<int>(0x100);
			auto proc = ESSE::EnumerateProcesses(static_cast<const ESSE::unichar32 *>(bundle));
			for (auto & p : *proc) result->Append(p);
			result->Retain();
			return result;
		} catch (...) { return 0; }
	}
}