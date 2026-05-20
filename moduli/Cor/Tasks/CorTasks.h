#pragma once

#include "CorThreads.h"
#include "../Classes/CorVolume.hxx"

namespace ESSE
{
	class IDispatchQueue;
	class IDispatchTask;

	class IDispatchQueue : public Object
	{
	public:
		virtual void SubmitTaskE(IDispatchTask * task, ErrorContext & ectx) noexcept = 0;
		virtual void SubmitTasksE(IDispatchTask ** tasks, uintptr count, ErrorContext & ectx) noexcept = 0;
		void SubmitTask(IDispatchTask * task);
		void SubmitTasks(IDispatchTask ** tasks, uintptr count);
	};
	class IDispatchTask : public Object
	{
	public:
		virtual void DoTask(IDispatchQueue * queue) noexcept = 0;
	};

	template<class F> class FunctionalTask final : public IDispatchTask
	{
		F f;
	public:
		FunctionalTask(const F & func) : f(func) {}
		virtual ~FunctionalTask(void) override {}
		virtual void DoTask(IDispatchQueue * queue) noexcept override { f(); }
	};
	template<class T1, class F> class StructuredFunctionalTask final : public IDispatchTask
	{
		F f;
	public:
		T1 Value1;
		StructuredFunctionalTask(const F & func) : f(func) {}
		virtual ~StructuredFunctionalTask(void) override {}
		virtual void DoTask(IDispatchQueue * queue) noexcept override { f(Value1); }
	};
	template<class T1, class T2, class F> class StructuredFunctionalTask2 final : public IDispatchTask
	{
		F f;
	public:
		T1 Value1;
		T2 Value2;
		StructuredFunctionalTask2(const F & func) : f(func) {}
		virtual ~StructuredFunctionalTask2(void) override {}
		virtual void DoTask(IDispatchQueue * queue) noexcept override { f(Value1, Value2); }
	};
	template<class F> oref< FunctionalTask<F> > CreateFunctionalTask(const F & func) { return owrap(new FunctionalTask<F>(func)); }
	template<class T1, class F> oref< StructuredFunctionalTask<T1, F> > CreateStructuredTask(const F & func) { return owrap(new StructuredFunctionalTask<T1, F>(func)); }
	template<class T1, class T2, class F> oref< StructuredFunctionalTask2<T1, T2, F> > CreateStructuredTask(const F & func) { return owrap(new StructuredFunctionalTask2<T1, T2, F>(func)); }

	oref<Thread> CreateThread(IDispatchTask * task, uint32 stack_size = NormalStackSize) noexcept;

	class TaskQueue : public IDispatchQueue
	{
		oref<Semaphore> _sync, _task_count;
		Queue<oref<IDispatchTask>> _tasks;
		uintptr _num_tasks;
	private:
		static int _thread_proc(void * argument) noexcept;
	public:
		TaskQueue(void);
		virtual ~TaskQueue(void) override;
		virtual string ToStringE(ErrorContext & ectx) const noexcept override;
		virtual void SubmitTaskE(IDispatchTask * task, ErrorContext & ectx) noexcept override;
		virtual void SubmitTasksE(IDispatchTask ** tasks, uintptr count, ErrorContext & ectx) noexcept override;
		int GetTaskQueueLength(void) const noexcept;
		void Process(void) noexcept;
		bool ProcessOnce(void) noexcept;
		void Quit(ErrorContext & ectx) noexcept;
		void Break(ErrorContext & ectx) noexcept;
		void Quit(void);
		void Break(void);
		oref<Thread> ProcessAsSeparateThread(uint32 stack_size = NormalStackSize) noexcept;
	};
	class ThreadPool : public IDispatchQueue
	{
		object_array<Thread> _workers;
		oref<Semaphore> _sync, _task_count;
		oref<Signal> _idle;
		Queue<oref<IDispatchTask>> _tasks;
		uintptr _num_tasks, _active_threads;
		bool _shutdown;
	private:
		static int _thread_proc(void * argument) noexcept;
	public:
		ThreadPool(void);
		ThreadPool(int num_threads);
		ThreadPool(int num_threads, uint32 stack_size);
		virtual ~ThreadPool(void) override;
		virtual string ToStringE(ErrorContext & ectx) const noexcept override;
		virtual void SubmitTaskE(IDispatchTask * task, ErrorContext & ectx) noexcept override;
		virtual void SubmitTasksE(IDispatchTask ** tasks, uintptr count, ErrorContext & ectx) noexcept override;
		uintptr GetThreadCount(void) const noexcept;
		uintptr GetActiveThreads(void) const noexcept;
		uintptr GetTaskQueueLength(void) const noexcept;
		void Wait(void) noexcept;
		bool Active(void) const noexcept;
	};
}