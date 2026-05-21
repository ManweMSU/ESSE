#include "../Interfaces/Threading.h"

namespace Engine
{
	class SystemThread : public Thread
	{
		ESSE::oref<ESSE::Thread> _thread;
	public:
		SystemThread(ESSE::Thread * thread) : _thread(thread) {}
		virtual ~SystemThread(void) override {}
		virtual bool Exited(void) noexcept override { return _thread->Exited(); }
		virtual int GetExitCode(void) noexcept override { return _thread->GetExitCode(); }
		virtual void Wait(void) noexcept override { _thread->Wait(); }
	};
	class SystemSemaphore : public Semaphore
	{
		ESSE::oref<ESSE::Semaphore> _sem;
	public:
		SystemSemaphore(uint32 initial) { _sem = ESSE::CreateSemaphore(initial); if (!_sem) throw Exception(); }
		virtual ~SystemSemaphore(void) override {}
		virtual void Wait(void) noexcept override { _sem->Wait(); }
		virtual bool TryWait(void) noexcept override { return _sem->WaitFor(0); }
		virtual bool WaitFor(uint time) noexcept override { return _sem->WaitFor(time); }
		virtual void Open(void) noexcept override { _sem->Open(); }
	};
	class SystemSignal : public Signal
	{
		ESSE::oref<ESSE::Signal> _signal;
	public:
		SystemSignal(bool value) { _signal = ESSE::CreateSignal(value); if (!_signal) throw Exception(); }
		virtual ~SystemSignal(void) override {}
		virtual void Wait(void) noexcept override { _signal->Wait(); }
		virtual bool TryWait(void) noexcept override { return _signal->WaitFor(0); }
		virtual bool WaitFor(uint time) noexcept override { return _signal->WaitFor(time); }
		virtual void Raise(void) noexcept override { _signal->Raise(); }
		virtual void Reset(void) noexcept override { _signal->Reset(); }
	};

	Thread * CreateThread(ThreadRoutine routine, void * argument, uint32 stack_size) noexcept
	{
		try {
			auto thread = ESSE::CreateThread(reinterpret_cast<ESSE::ThreadRoutine>(routine), argument, stack_size);
			if (!thread) return 0;
			return new SystemThread(thread);
		} catch (...) { return 0; }
	}
	Semaphore * CreateSemaphore(uint32 initial) noexcept { try { return new SystemSemaphore(initial); } catch (...) { return 0; } }
	Signal * CreateSignal(bool set) noexcept { try { return new SystemSignal(set); } catch (...) { return 0; } }

	bool CreateThreadLocal(handle & index) noexcept { return ESSE::Memory::CreateThreadLocal(index); }
	void ReleaseThreadLocal(handle index) noexcept { ESSE::Memory::ReleaseThreadLocal(index); }
	void SetThreadLocal(handle index, handle value) noexcept { ESSE::Memory::SetThreadLocal(index, value); }
	handle GetThreadLocal(handle index) noexcept { return ESSE::Memory::GetThreadLocal(index); }
}
