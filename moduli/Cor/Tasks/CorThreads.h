#pragma once

#include "../Classes/CorObject.h"

namespace ESSE
{
	constexpr uint32 NormalStackSize = 0x200000;
	typedef int (* ThreadRoutine) (void * argument) noexcept;
	class Signal : public WaitableObject
	{
	public:
		virtual void Raise(void) noexcept = 0;
		virtual void Reset(void) noexcept = 0;
	};
	class Semaphore : public WaitableObject
	{
	public:
		virtual void Open(void) noexcept = 0;
	};
	class Thread : public WaitableObject
	{
	public:
		virtual bool Exited(void) noexcept = 0;
		virtual int GetExitCode(void) noexcept = 0;
	};

	oref<Thread> CreateThread(ThreadRoutine routine, void * argument = 0, uint32 stack_size = NormalStackSize) noexcept;
	oref<Semaphore> CreateSemaphore(uint32 initial) noexcept;
	oref<Signal> CreateSignal(bool set) noexcept;

	namespace Memory {
		bool CreateThreadLocal(handle & index) noexcept;
		void ReleaseThreadLocal(handle index) noexcept;
		void SetThreadLocal(handle index, handle value) noexcept;
		handle GetThreadLocal(handle index) noexcept;
	}
}