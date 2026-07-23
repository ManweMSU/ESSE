#include <Cor/Tasks/CorProcesses.h>
#include <Cor/Tasks/CorThreads.h>
#include <Cor/IO/CorIO.h>
#include <Cor/IO/CorStreams.h>
#include "CorBoot.h"
#include "CorIOEx.h"

#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <wait.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/resource.h>

#ifdef ESSE_MODULUS_FENESTRARUM_LINUX_X11
#include <Fenestrae-Linux-X11/X11WindowSystem.h>
#endif

extern char ** environ;

namespace ESSE
{
	namespace Linux
	{
		// TODO: ADD WAYLAND BASED IMPLEMENTATION
		#ifdef ESSE_MODULUS_FENESTRARUM_LINUX_X11
		bool IsProcessGUI(pid_t pid) noexcept { return X11::IsGraphicalProcess(pid); }
		bool ActivateProcessGUI(pid_t pid) noexcept { return X11::ActivateProcess(pid); }
		#else
		bool IsProcessGUI(pid_t pid) noexcept { return false; }
		bool ActivateProcessGUI(pid_t pid) noexcept { return false; }
		#endif
	}
	class PosixProcess : public Process
	{
		static oref<ObjectDictionary<pid_t, PosixProcess>> _process_list;
	private:
		oref<Semaphore> _local_sync;
		oref<Signal> _wait;
		pid_t _pid;
		int _exit_code;
		bool _exited;
	private:
		static int _process_waiter_thread(void * plist) noexcept
		{
			oref<ObjectDictionary<pid_t, PosixProcess>> list = owrap(reinterpret_cast<ObjectDictionary<pid_t, PosixProcess> *>(plist));
			while (true) {
				int status;
				auto pid = wait(&status);
				if (pid < 0) {
					if (errno == EINTR) continue; else {
						Memory::AcquireRootLock();
						for (auto & proc : *list) {
							proc.value->_local_sync->Wait();
							proc.value->_exit_code = -1;
							proc.value->_exited = true;
							proc.value->_pid = -1;
							proc.value->_wait->Raise();
							proc.value->_local_sync->Open();
						}
						list->Clear();
						Memory::ReleaseRootLock();
					}
				} else {
					int set_exit_code = -1;
					if (WIFEXITED(status)) set_exit_code = WEXITSTATUS(status);
					Memory::AcquireRootLock();
					auto proc = (*list)[pid];
					if (proc) {
						proc->_local_sync->Wait();
						proc->_exit_code = set_exit_code;
						proc->_exited = true;
						proc->_pid = -1;
						proc->_wait->Raise();
						proc->_local_sync->Open();
						list->Remove(pid);
					}
					auto exit_thread = list->IsEmpty();
					Memory::ReleaseRootLock();
					if (exit_thread) return 0;
				}
			}
			return 0;
		}
		PosixProcess(pid_t pid) : _pid(pid), _exit_code(-1), _exited(false)
		{
			_local_sync = CreateSemaphore(1);
			_wait = CreateSignal(false);
			if (!_local_sync || !_wait) throw OutOfMemoryException();
		}
	public:
		virtual ~PosixProcess(void) override {}
		virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"Process"; ESSE_TRY_OUTRO(string()) }
		virtual void Wait(void) noexcept override { _wait->Wait(); }
		virtual bool WaitFor(uint32 ms) noexcept override { return _wait->WaitFor(ms); }
		virtual bool Terminate(void) noexcept override { int status = -1; _local_sync->Wait(); if (_pid >= 0) status = kill(_pid, SIGKILL); _local_sync->Open(); return status >= 0; }
		virtual bool Exited(void) noexcept override { _local_sync->Wait(); auto result = _exited; _local_sync->Open(); return result; }
		virtual int GetExitCode(void) noexcept override { _local_sync->Wait(); auto result = _exit_code; _local_sync->Open(); return result; }
		virtual int GetPID(void) noexcept override { _local_sync->Wait(); auto result = _pid; _local_sync->Open(); return result; }
		virtual bool IsGUI(void) noexcept override { return Linux::IsProcessGUI(GetPID()); }
		virtual bool Activate(void) noexcept override { return Linux::ActivateProcessGUI(GetPID()); }
		static oref<Process> CreateInstance(const CreateProcessDesc & desc, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				if (desc.flags & CreateProcessElevated) {
					if (desc.flags & CreateProcessSearchPath) { ErrorSet(ectx, Errores::ErrorInvalidArgument); return 0; }
					if (desc.flags & CreateProcessOverrideIO) { ErrorSet(ectx, Errores::ErrorInvalidArgument); return 0; }
					if (desc.flags & CreateProcessOverrideEnvironment) { ErrorSet(ectx, Errores::ErrorInvalidArgument); return 0; }
					if (desc.flags & CreateProcessOverrideWorkingDirectory) { ErrorSet(ectx, Errores::ErrorInvalidArgument); return 0; }
					CreateProcessDesc subdesc;
					subdesc.flags = Linux::CreateProcessSearchPathOnly | CreateProcessSearchPath | CreateProcessOverrideIO;
					subdesc.image = U"pkexec";
					subdesc.standard_input = subdesc.standard_output = subdesc.standard_error = IO::InvalidHandle;
					subdesc.command_line << IO::GetExecutablePath();
					subdesc.command_line << U"--esse-detach";
					subdesc.command_line << IO::ExpandPath(desc.image);
					subdesc.command_line.Append(desc.command_line);
					auto agent = CreateInstance(subdesc, ectx);
					if (ErrorTest(ectx)) return 0;
					agent->Wait();
					if (agent->GetExitCode() != 0) { ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::AccessDenied); return 0; }
					return 0;
				} else {
					int argc = desc.command_line.GetLength() + 1;
					array<ucs1_string> try_images(0x20);
					array<ucs1_string> argv_mem(desc.command_line.GetLength()), env_mem(0x20);
					array<char *> argv(1), env(0x20);
					argv.SetLength(argc + 1);
					for (auto & c : desc.command_line) argv_mem.Append(ucs1_string(c));
					for (uintptr i = 0; i < argv_mem.GetLength(); i++) argv[i + 1] = const_cast<char *>(argv_mem[i].GetData());
					argv[argc] = 0;
					if (desc.image[0] != U'/' && (desc.flags & CreateProcessSearchPath)) {
						if (!(desc.flags & Linux::CreateProcessSearchPathOnly)) {
							try_images << IO::GetCurrentDirectory() + U"/" + desc.image;
							try_images << IO::Path::GetDirectory(IO::GetExecutablePath()) + U"/" + desc.image;
						}
						uintptr i = 0;
						while (environ[i]) {
							auto ev = environ[i++];
							if (ev[0] == 'P' && ev[1] == 'A' && ev[2] == 'T' && ev[3] == 'H' && ev[4] == '=') {
								auto paths = SplitString(string(ev + 5), EnvironmentArraySeparator);
								for (auto & p : paths) try_images << p + U"/" + desc.image;
								break;
							}
						}
					} else try_images.Append(IO::ExpandPath(desc.image));
					if (desc.flags & CreateProcessOverrideEnvironment) {
						for (auto & e : desc.environment) env_mem.Append(e.key + U"=" + e.value);
						for (auto & e : env_mem) env.Append(const_cast<char *>(e.GetData()));
						env.Append(0);
					}
					int inter_io[2];
					if (pipe(inter_io) < 0) { Linux::ErrorSetPosix(ectx); return 0; }
					if (fcntl(inter_io[0], F_SETFD, FD_CLOEXEC) < 0) { Linux::ErrorSetPosix(ectx); close(inter_io[0]); close(inter_io[1]); return 0; }
					if (fcntl(inter_io[1], F_SETFD, FD_CLOEXEC) < 0) { Linux::ErrorSetPosix(ectx); close(inter_io[0]); close(inter_io[1]); return 0; }
					Memory::AcquireRootLock();
					auto pid = fork();
					if (pid < 0) { Linux::ErrorSetPosix(ectx); Memory::ReleaseRootLock(); close(inter_io[0]); close(inter_io[1]); return 0; }
					if (pid > 0) {
						close(inter_io[1]);
						char exec_failed_status = 0; // 0 - success, 1 - internal error, 2--15 - basic error, 16+ - IO error
						while (true) {
							auto status = read(inter_io[0], &exec_failed_status, 1);
							if (status < 0 && errno != EINTR) { exec_failed_status = 1; break; }
							else if (status == 0) { exec_failed_status = 0; break; }
							else break;
						}
						close(inter_io[0]);
						if (exec_failed_status) {
							while (true) {
								auto status = waitpid(pid, 0, 0);
								if (status >= 0 || errno != EINTR) break;
							}
							Memory::ReleaseRootLock();
							if (exec_failed_status == 1) ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::ReadFailure);
							else if (exec_failed_status < 0x10) ErrorSet(ectx, exec_failed_status - 1);
							else ErrorSet(ectx, Errores::ErrorIO, exec_failed_status - 0x10);
							return 0;
						}
						oref<PosixProcess> process;
						try {
							process = owrap(new PosixProcess(pid));
							if (!_process_list) _process_list = owrap(new ObjectDictionary<pid_t, PosixProcess>());
						} catch (...) { Memory::ReleaseRootLock(); ErrorSet(ectx, Errores::ErrorOutOfMemory); return 0; }
						bool was_empty = _process_list->IsEmpty();
						try { _process_list->Append(pid, process); }
						catch (...) { Memory::ReleaseRootLock(); ErrorSet(ectx, Errores::ErrorOutOfMemory); return 0; }
						if (was_empty) {
							_process_list->Retain();
							auto waiter = CreateThread(_process_waiter_thread, _process_list.Inner(), 0x10000);
							if (!waiter) {
								_process_list->Remove(pid);
								_process_list->Release();
								Memory::ReleaseRootLock();
								ErrorSet(ectx, Errores::ErrorOutOfMemory);
								return 0;
							}
						}
						Memory::ReleaseRootLock();
						return oref<Process>(process.Inner());
					} else {
						Memory::ReleaseRootLock();
						close(inter_io[0]);
						ErrorContext local; ErrorClear(local);
						if (desc.flags & CreateProcessDetached) {
							if (setsid() < 0) Linux::ErrorSetPosix(local);
						}
						if (desc.flags & CreateProcessOverrideIO) {
							if (!ErrorTest(local)) IO::SetStandardHandle(IO::StandardHandleType::Input, desc.standard_input, local);
							if (!ErrorTest(local)) IO::SetStandardHandle(IO::StandardHandleType::Output, desc.standard_output, local);
							if (!ErrorTest(local)) IO::SetStandardHandle(IO::StandardHandleType::Error, desc.standard_error, local);
						}
						if (desc.flags & CreateProcessOverrideWorkingDirectory) {
							if (!ErrorTest(local)) IO::SetCurrentDirectory(desc.working_directory, local);
						}
						uint64 limit = 0;
						struct rlimit rl;
						if (getrlimit(RLIMIT_NOFILE, &rl) == 0) limit = min(rl.rlim_max, rl.rlim_cur);
						auto conf_limit = sysconf(_SC_OPEN_MAX);
						if (conf_limit >= 0 && uint64(conf_limit) < limit) limit = conf_limit;
						while (limit) {
							limit--;
							if (limit != inter_io[1] && limit != STDIN_FILENO && limit != STDOUT_FILENO && limit != STDERR_FILENO) close(limit);
						}
						sigset_t set;
						sigemptyset(&set);
						signal(SIGPIPE, SIG_DFL);
						signal(SIGWINCH, SIG_DFL);
						signal(SIGINT, SIG_DFL);
						signal(SIGQUIT, SIG_DFL);
						signal(SIGHUP, SIG_DFL);
						signal(SIGTERM, SIG_DFL);
						sigprocmask(SIG_SETMASK, &set, 0);
						if (!ErrorTest(local)) for (auto & t : try_images) {
							argv[0] = const_cast<char *>(t.GetData());
							if (desc.flags & CreateProcessOverrideEnvironment) execve(argv[0], argv, env);
							else execve(argv[0], argv, environ);
							if (!ErrorTest(local)) {
								if (errno == E2BIG) ErrorSet(local, Errores::ErrorInvalidArgument);
								else if (errno == EACCES || errno == EPERM || errno == ETXTBSY) ErrorSet(local, Errores::ErrorIO, Errores::SuberrorIO::AccessDenied);
								else if (errno == EAGAIN || errno == EMFILE || errno == ENFILE || errno == ENOMEM) ErrorSet(local, Errores::ErrorIO, Errores::SuberrorIO::NotEnoughMemory);
								else if (errno == EFAULT) ErrorSet(local, Errores::ErrorInvalidState);
								else if (errno == EINVAL || errno == EISDIR || errno == ELIBBAD || errno == ENOEXEC) ErrorSet(local, Errores::ErrorInvalidFormat);
								else if (errno == ELOOP) ErrorSet(local, Errores::ErrorIO, Errores::SuberrorIO::BadPathName);
								else if (errno == ENAMETOOLONG) ErrorSet(local, Errores::ErrorIO, Errores::SuberrorIO::FileNameTooLong);
								else if (errno == ENOENT) ErrorSet(local, Errores::ErrorIO, Errores::SuberrorIO::FileNotFound);
								else if (errno == ENOTDIR) ErrorSet(local, Errores::ErrorIO, Errores::SuberrorIO::PathNotFound);
								else ErrorSet(local, Errores::ErrorIO, Errores::SuberrorIO::Unknown);
							}
						}
						int fail_number;
						if (local.error_code == Errores::ErrorIO) fail_number = 0x10 + local.error_subcode;
						else fail_number = 1 + local.error_code;
						write(inter_io[1], &fail_number, 1);
						_exit(0);
					}
				}
			ESSE_TRY_OUTRO(0)
		}
	};
	class PosixRunningProcess : public RunningProcess
	{
		pid_t _pid;
		Time _date_created;
	private:
		static Time _read_creation_time(pid_t pid) noexcept
		{
			try {
				auto system_time_unit = sysconf(_SC_CLK_TCK);
				auto proc_stat_stream = FileStream::Create(U"/proc/" + string(pid) + U"/stat", FileAccess::AccessRead, FileCreationMode::OpenExisting);
				auto proc_stat_decoder = new TextDecoder(proc_stat_stream, Unicode::Encoding::UTF8);
				auto word = SplitString(proc_stat_decoder->ReadAll(), U' ');
				long time_started = word.GetLength() > 21 ? word[21].ToUInt64() : 0;
				long os_time_started = 0;
				proc_stat_stream = FileStream::Create(U"/proc/stat", FileAccess::AccessRead, FileCreationMode::OpenExisting);
				proc_stat_decoder = new TextDecoder(proc_stat_stream, Unicode::Encoding::UTF8);
				while (!proc_stat_decoder->IsEOF()) {
					auto line = proc_stat_decoder->ReadLine();
					if (line.Substring(0, 6) == U"btime ") {
						os_time_started = line.Substring(6, -1).ToUInt64();
						break;
					}
				}
				long unix_time_started = os_time_started * 1000 + time_started * 1000 / system_time_unit;
				return Time::FromUnixTime(unix_time_started);
			} catch (...) { return Time(0); }
		}
	public:
		PosixRunningProcess(pid_t pid) : _pid(pid) { _date_created = _read_creation_time(_pid); }
		virtual ~PosixRunningProcess(void) override {}
		virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"Running Process"; ESSE_TRY_OUTRO(string()) }
		virtual bool Terminate(void) noexcept override
		{
			if (_read_creation_time(_pid) != _date_created) return false;
			if (kill(_pid, SIGKILL) < 0) return false;
			return true;
		}
		virtual bool Exited(void) noexcept override { return _read_creation_time(_pid) == _date_created; }
		virtual int GetPID(void) noexcept override { return _pid; }
		virtual bool IsGUI(void) noexcept override { return Linux::IsProcessGUI(_pid); }
		virtual bool Activate(void) noexcept override { return Linux::ActivateProcessGUI(_pid); }
		virtual Time GetCreationTime(void) noexcept override { return _date_created; }
		virtual string GetExecutablePath(ErrorContext & ectx) noexcept override
		{
			ESSE_TRY_INTRO
				ucs1_string lnf(U"/proc/" + string(_pid) + U"/exe");
				array<unichar8> link_utf8(1);
				link_utf8.SetLength(PATH_MAX);
				auto length = readlink(lnf, link_utf8, link_utf8.GetLength());
				if (length < 0) { Linux::ErrorSetPosix(ectx); return string(); }
				return string(link_utf8);
			ESSE_TRY_OUTRO(string())
		}
		virtual string GetBundlePath(ErrorContext & ectx) noexcept override { ErrorSet(ectx, Errores::ErrorNotImplemented); return string(); }
		virtual string GetBundleIdentifier(ErrorContext & ectx) noexcept override { ErrorSet(ectx, Errores::ErrorNotImplemented); return string(); }
	};
	oref<ObjectDictionary<pid_t, PosixProcess>> PosixProcess::_process_list;

	oref<Process> CreateProcess(const CreateProcessDesc & desc, ErrorContext & ectx) noexcept { return PosixProcess::CreateInstance(desc, ectx); }
	oref<array<string>> GetCommandLine(ErrorContext & ectx) noexcept
	{
		ESSE_TRY_INTRO
			int argc;
			const char ** argv;
			Linux::GetCommandLine(&argc, &argv);
			auto result = owrap(new array<string>(argc));
			for (int i = 0; i < argc; i++) *result << string(argv[i]);
			return result;
		ESSE_TRY_OUTRO(0)
	}
	oref<Dictionary<string, string>> GetEnvironment(ErrorContext & ectx) noexcept
	{
		ESSE_TRY_INTRO
			auto result = owrap(new Dictionary<string, string>);
			uintptr i = 0;
			while (environ[i]) {
				string decl(environ[i++]);
				auto del = decl.FindFirst(U'=');
				if (del < 0) continue;
				result->Append(decl.Substring(0, del), decl.Substring(del + 1, -1));
			}
			return result;
		ESSE_TRY_OUTRO(0)
	}
	void Sleep(uint32 time) noexcept
	{
		struct timespec req, elasped;
		req.tv_nsec = (time % 1000) * 1000000;
		req.tv_sec = time / 1000;
		do {
			int result = nanosleep(&req, &elasped);
			if (result == -1) { if (errno == EINTR) req = elasped; else return; } else return;
		} while (true);
	}
	void ExitProcess(int exit_code) noexcept { _exit(exit_code); }
	bool IsProcessElevated(void) noexcept { return geteuid() == 0; }
	oref<RunningProcess> OpenProcess(int pid, ErrorContext & ectx) noexcept { ESSE_TRY_INTRO oref<RunningProcess> result; result.SetOwned(new PosixRunningProcess(pid)); return result; ESSE_TRY_OUTRO(0) }
	int GetThisProcessPID(void) noexcept { pid_t pid; Linux::GetThisProcessPID(&pid); return pid; }
	oref<array<int>> EnumerateProcesses(ErrorContext & ectx) noexcept
	{
		ESSE_TRY_INTRO
			auto result = owrap(new array<int>(0x100));
			auto proc = IO::EnumerateFiles(U"/proc", U"", IO::FileSearch::FileSearchDirectories, ectx);
			if (ErrorTest(ectx)) return 0;
			for (auto & p : *proc) if (p[0] >= U'0' && p[0] <= U'9') try { result->Append(p.ToInt32()); } catch (...) {}
			return result;
		ESSE_TRY_OUTRO(0)
	}
	oref<array<int>> EnumerateProcesses(const string & bundle, ErrorContext & ectx) noexcept { ErrorSet(ectx, Errores::ErrorNotImplemented); return 0; }
}