#include "CorTasks.h"
#include <Cor/CorSystemInformation.h>

namespace ESSE
{
	void IDispatchQueue::SubmitTask(IDispatchTask * task) { ErrorContext ectx; ErrorClear(ectx); SubmitTaskE(task, ectx); ErrorThrow(ectx); }
	void IDispatchQueue::SubmitTasks(IDispatchTask ** tasks, uintptr count) { ErrorContext ectx; ErrorClear(ectx); SubmitTasksE(tasks, count, ectx); ErrorThrow(ectx); }

	int TaskThreadRoutine(void * argument) noexcept
	{
		auto task = reinterpret_cast<IDispatchTask *>(argument);
		task->DoTask(0);
		task->Release();
		return 0;
	}
	oref<Thread> CreateThread(IDispatchTask * task, uint32 stack_size) noexcept
	{
		if (!task) return 0;
		task->Retain();
		auto result = CreateThread(TaskThreadRoutine, task, stack_size);
		if (!result) task->Release();
		return result;
	}

	int TaskQueue::_thread_proc(void * argument) noexcept
	{
		auto self = reinterpret_cast<TaskQueue *>(argument);
		self->Process();
		self->Release();
		return 0;
	}
	TaskQueue::TaskQueue(void)
	{
		_sync = CreateSemaphore(1);
		_task_count = CreateSemaphore(0);
		if (!_sync || !_task_count) throw OutOfMemoryException();
	}
	TaskQueue::~TaskQueue(void) {}
	string TaskQueue::ToStringE(ErrorContext & ectx) const noexcept
	{
		string result;
		_sync->Wait();
		try { result = U"Task Queue: " + string(_num_tasks) + U" tasks pending"; }
		catch (...) { _sync->Open(); ErrorSet(ectx, Errores::ErrorOutOfMemory); return string(); }
		_sync->Open();
		return result;
	}
	void TaskQueue::SubmitTaskE(IDispatchTask * task, ErrorContext & ectx) noexcept { SubmitTasksE(&task, 1, ectx); }
	void TaskQueue::SubmitTasksE(IDispatchTask ** tasks, uintptr count, ErrorContext & ectx) noexcept
	{
		if (!tasks) { ErrorSet(ectx, Errores::ErrorInvalidArgument); return; }
		_sync->Wait();
		try {
			for (uintptr i = 0; i < count; i++) if (tasks[i]) {
				_tasks.Push(tasks[i]);
				_task_count->Open();
				_num_tasks++;
			}
		} catch (...) { ErrorSet(ectx, Errores::ErrorOutOfMemory); }
		_sync->Open();
	}
	int TaskQueue::GetTaskQueueLength(void) const noexcept { _sync->Wait(); auto count = _num_tasks; _sync->Open(); return count; }
	void TaskQueue::Process(void) noexcept
	{
		bool loop = true;
		while (loop) {
			_task_count->Wait();
			_sync->Wait();
			_num_tasks--;
			auto task = _tasks.Pop();
			_sync->Open();
			if (task) task->DoTask(this);
			else loop = false;
		}
	}
	bool TaskQueue::ProcessOnce(void) noexcept
	{
		if (_task_count->WaitFor(0)) {
			_sync->Wait();
			_num_tasks--;
			auto task = _tasks.Pop();
			_sync->Open();
			if (task) task->DoTask(this);
			return true;
		} else return false;
	}
	void TaskQueue::Quit(ErrorContext & ectx) noexcept
	{
		_sync->Wait();
		try { _tasks.Push(0); } catch (...) { ErrorSet(ectx, Errores::ErrorOutOfMemory); _sync->Open(); return; }
		_task_count->Open();
		_num_tasks++;
		_sync->Open();
	}
	void TaskQueue::Break(ErrorContext & ectx) noexcept
	{
		_sync->Wait();
		try { _tasks.InsertFirst(0); } catch (...) { ErrorSet(ectx, Errores::ErrorOutOfMemory); _sync->Open(); return; }
		_task_count->Open();
		_num_tasks++;
		_sync->Open();
	}
	void TaskQueue::Quit(void) { ErrorContext ectx; ErrorClear(ectx); Quit(ectx); ErrorThrow(ectx); }
	void TaskQueue::Break(void) { ErrorContext ectx; ErrorClear(ectx); Break(ectx); ErrorThrow(ectx); }
	oref<Thread> TaskQueue::ProcessAsSeparateThread(uint32 stack_size) noexcept
	{
		Retain();
		auto thread = CreateThread(_thread_proc, this, stack_size);
		if (!thread) Release();
		return thread;
	}

	int ThreadPool::_thread_proc(void * argument) noexcept
	{
		auto self = reinterpret_cast<ThreadPool *>(argument);
		do {
			oref<IDispatchTask> task;
			if (self->_task_count->WaitFor(0)) {
				self->_sync->Wait();
				if (!self->_shutdown) task = self->_tasks.Pop();
				self->_num_tasks--;
				self->_sync->Open();
			} else {
				self->_sync->Wait();
				self->_active_threads--;
				if (self->_active_threads == 0) self->_idle->Raise();
				self->_sync->Open();
				self->_task_count->Wait();
				self->_sync->Wait();
				if (!self->_shutdown) task = self->_tasks.Pop();
				self->_num_tasks--;
				if (self->_active_threads == 0) self->_idle->Reset();
				self->_active_threads++;
				self->_sync->Open();
			}
			if (task) {
				task->DoTask(self);
				task.Clear();
			} else {
				self->_sync->Wait();
				self->_active_threads--;
				if (self->_active_threads == 0) self->_idle->Raise();
				self->_sync->Open();
				return 0;
			}
		} while (true);
		return 0;
	}
	ThreadPool::ThreadPool(void) : ThreadPool(0) {}
	ThreadPool::ThreadPool(int num_threads) : ThreadPool(num_threads, NormalStackSize) {}
	ThreadPool::ThreadPool(int num_threads, uint32 stack_size) : _workers(1), _num_tasks(0), _active_threads(0), _shutdown(true)
	{
		if (num_threads <= 0) num_threads = System::GetProcessorCores() + num_threads;
		if (num_threads <= 0) num_threads = 1;
		_sync = CreateSemaphore(0);
		_task_count = CreateSemaphore(0);
		_idle = CreateSignal(false);
		if (!_sync || !_task_count || !_idle) throw OutOfMemoryException();
		_workers.SetLength(num_threads);
		for (uintptr i = 0; i < _workers.GetLength(); i++) {
			auto wthread = CreateThread(_thread_proc, this, stack_size);
			if (wthread) {
				_workers.SetElement(wthread, i);
				_active_threads++;
			} else {
				if (i) {
					for (uintptr j = 0; j < i; j++) { _task_count->Open(); _num_tasks++; }
					_sync->Open();
					for (uintptr j = 0; j < i; j++) _workers[j].Wait();
				}
				throw OutOfMemoryException();
			}
		}
		_shutdown = false;
		_sync->Open();
	}
	ThreadPool::~ThreadPool(void)
	{
		Wait();
		_sync->Wait();
		_shutdown = true;
		for (auto & w : _workers) { _task_count->Open(); _num_tasks++; }
		_sync->Open();
		for (auto & w : _workers) w.Wait();
	}
	string ThreadPool::ToStringE(ErrorContext & ectx) const noexcept
	{
		string result;
		_sync->Wait();
		try { result = U"Thread Pool: " + string(_workers.GetLength()) + U" threads, " + string(_num_tasks) + U" tasks pending, " + string(_active_threads) + U" threads active"; }
		catch (...) { _sync->Open(); ErrorSet(ectx, Errores::ErrorOutOfMemory); return string(); }
		_sync->Open();
		return result;
	}
	void ThreadPool::SubmitTaskE(IDispatchTask * task, ErrorContext & ectx) noexcept { SubmitTasksE(&task, 1, ectx); }
	void ThreadPool::SubmitTasksE(IDispatchTask ** tasks, uintptr count, ErrorContext & ectx) noexcept
	{
		if (!tasks) { ErrorSet(ectx, Errores::ErrorInvalidArgument); return; }
		ESSE_TRY_INTRO
		_sync->Wait();
		try {
			for (uintptr i = 0; i < count; i++) if (tasks[i]) {
				_tasks.Push(tasks[i]);
				_task_count->Open();
				_num_tasks++;
			}
		} catch (...) { _sync->Open(); throw; }
		_sync->Open();
		ESSE_TRY_OUTRO()
	}
	uintptr ThreadPool::GetThreadCount(void) const noexcept { _sync->Wait(); auto result = _workers.GetLength(); _sync->Open(); return result; }
	uintptr ThreadPool::GetActiveThreads(void) const noexcept { _sync->Wait(); auto result = _active_threads; _sync->Open(); return result; }
	uintptr ThreadPool::GetTaskQueueLength(void) const noexcept { _sync->Wait(); auto result = _num_tasks; _sync->Open(); return result; }
	void ThreadPool::Wait(void) noexcept
	{
		bool idle = false;
		do {
			_idle->Wait();
			_sync->Wait();
			if (!_num_tasks && !_active_threads) idle = true;
			_sync->Open();
		} while (!idle);
	}
	bool ThreadPool::Active(void) const noexcept
	{
		if (_idle->WaitFor(0)) {
			_sync->Wait();
			bool idle = (!_num_tasks && !_active_threads);
			_sync->Open();
			return !idle;
		} else return true;
	}
}