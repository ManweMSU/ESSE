#include <Cor/Tasks/CorThreads.h>

#include <semaphore.h>
#include <pthread.h>
#include <errno.h>
#include <new>

namespace ESSE
{
	class PosixSignal : public Signal
	{
		volatile bool _state;
		pthread_cond_t _cond;
		pthread_mutex_t _mutex;
	public:
		PosixSignal(bool initial) : _state(initial)
		{
			if (pthread_cond_init(&_cond, 0) == -1) throw Exception();
			if (pthread_mutex_init(&_mutex, 0) == -1) { pthread_cond_destroy(&_cond); throw Exception(); }
		}
		virtual ~PosixSignal(void) override { pthread_cond_destroy(&_cond); pthread_mutex_destroy(&_mutex); }
		virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"Signal"; ESSE_TRY_OUTRO(string()) }
		virtual void Wait(void) noexcept override
		{
			if (pthread_mutex_lock(&_mutex) != 0) abort();
			while (!_state) {
				auto error = pthread_cond_wait(&_cond, &_mutex);
				if (error == 0) break;
				if (error != EINTR) abort();
			}
			if (pthread_mutex_unlock(&_mutex) != 0) abort();
		}
		virtual bool WaitFor(uint32 ms) noexcept override
		{
			if (ms) {
				struct timespec date;
				if (clock_gettime(CLOCK_REALTIME, &date) == -1) abort();
				uint64 new_ns = date.tv_nsec + uint64(ms) * 1000000UL;
				uint64 sec_carry = new_ns / 1000000000UL;
				date.tv_nsec = new_ns % 1000000000UL;
				date.tv_sec += sec_carry;
				if (pthread_mutex_lock(&_mutex) != 0) abort();
				int result = 0;
				while (!_state && result != ETIMEDOUT) {
					result = pthread_cond_timedwait(&_cond, &_mutex, &date);
					if (result == 0 || result == ETIMEDOUT) break; else if (result != EINTR) abort();
				}
				if (result == 0 && !_state) result = ETIMEDOUT;
				if (pthread_mutex_unlock(&_mutex) != 0) abort();
				return result == 0;
			} else {
				if (pthread_mutex_lock(&_mutex) != 0) abort();
				int result = _state ? 0 : ETIMEDOUT;
				if (pthread_mutex_unlock(&_mutex) != 0) abort();
				return result == 0;
			}
		}
		virtual void Raise(void) noexcept override
		{
			if (pthread_mutex_lock(&_mutex) != 0) abort();
			_state = true;
			if (pthread_cond_broadcast(&_cond) != 0) abort();
			if (pthread_mutex_unlock(&_mutex) != 0) abort();
		}
		virtual void Reset(void) noexcept override
		{
			if (pthread_mutex_lock(&_mutex) != 0) abort();
			_state = false;
			if (pthread_mutex_unlock(&_mutex) != 0) abort();
		}
	};
	class PosixSemaphore : public Semaphore
	{
		sem_t _sem;
	public:
		PosixSemaphore(uint32 initial) { if (sem_init(&_sem, 0, initial) == -1) throw Exception(); }
		virtual ~PosixSemaphore(void) override { sem_destroy(&_sem); }
		virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"Semaphore"; ESSE_TRY_OUTRO(string()) }
		virtual void Wait(void) noexcept override { while (true) { if (sem_wait(&_sem) == 0) return; if (errno != EINTR) abort(); } }
		virtual bool WaitFor(uint32 ms) noexcept override
		{
			if (ms) {
				struct timespec date;
				if (clock_gettime(CLOCK_REALTIME, &date) == -1) abort();
				uint64 new_ns = date.tv_nsec + uint64(ms) * 1000000UL;
				uint64 sec_carry = new_ns / 1000000000UL;
				date.tv_nsec = new_ns % 1000000000UL;
				date.tv_sec += sec_carry;
				while (true) {
					auto status = sem_timedwait(&_sem, &date);
					if (status == 0) return true;
					else if (errno == EAGAIN || errno == ETIMEDOUT) return false;
					else if (errno == EINTR) continue;
					else abort();
				}
			} else while (true) {
				auto status = sem_trywait(&_sem);
				if (status == 0) return true;
				else if (errno == EAGAIN) return false;
				else if (errno == EINTR) continue;
				else abort();
			}
		}
		virtual void Open(void) noexcept override { if (sem_post(&_sem) == -1) abort(); }
	};
	class PosixThread : public Thread
	{
		volatile int _retval;
		oref<Signal> _wait_sync;
		ThreadRoutine _routine;
		void * _argument;
	private:
		static void * _thread_proc(void * _self) noexcept
		{
			auto self = reinterpret_cast<PosixThread *>(_self);
			self->_retval = self->_routine(self->_argument);
			self->_wait_sync->Raise();
			self->Release();
			return 0;
		}
		PosixThread(void) noexcept : _retval(-1) {}
	public:
		virtual ~PosixThread(void) override {}
		virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"Thread"; ESSE_TRY_OUTRO(string()) }
		virtual void Wait(void) noexcept override { _wait_sync->Wait(); }
		virtual bool WaitFor(uint32 ms) noexcept override { return _wait_sync->WaitFor(ms); }
		virtual bool Exited(void) noexcept override { return _wait_sync->WaitFor(0); }
		virtual int GetExitCode(void) noexcept override { if (Exited()) return _retval; else return -1; }
		static oref<Thread> CreateInstance(ThreadRoutine routine, void * argument, uint32 stack_size) noexcept
		{
			auto thread = owrap(new (std::nothrow) PosixThread);
			if (!thread) return 0;
			thread->_routine = routine;
			thread->_argument = argument;
			thread->_wait_sync = CreateSignal(false);
			if (!thread->_wait_sync) return 0;
			pthread_t thread_handle;
			pthread_attr_t thread_attr;
			if (pthread_attr_init(&thread_attr)) return 0;
			if (pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED)) { pthread_attr_destroy(&thread_attr); return 0; }
			if (pthread_attr_setstacksize(&thread_attr, max(stack_size, uint32(PTHREAD_STACK_MIN)))) { pthread_attr_destroy(&thread_attr); return 0; }
			thread->Retain();
			auto status = pthread_create(&thread_handle, &thread_attr, _thread_proc, thread.Inner());
			pthread_attr_destroy(&thread_attr);
			if (status) { thread->Release(); return 0; }
			return oref<Thread>(thread.Inner());
		}
	};

	oref<Thread> CreateThread(ThreadRoutine routine, void * argument, uint32 stack_size) noexcept { return PosixThread::CreateInstance(routine, argument, stack_size); }
	oref<Semaphore> CreateSemaphore(uint32 initial) noexcept { try { oref<Semaphore> result; result.SetOwned(new PosixSemaphore(initial)); return result; } catch (...) { return 0; } }
	oref<Signal> CreateSignal(bool set) noexcept { try { oref<Signal> result; result.SetOwned(new PosixSignal(set)); return result; } catch (...) { return 0; } }
	
	namespace Memory {
		bool CreateThreadLocal(handle & index) noexcept
		{
			pthread_key_t result;
			if (pthread_key_create(&result, 0) == 0) {
				index = reinterpret_cast<handle>(uintptr(result));
				return true;
			} else return false;
		}
		void ReleaseThreadLocal(handle index) noexcept { pthread_key_delete(pthread_key_t(reinterpret_cast<uintptr>(index))); }
		void SetThreadLocal(handle index, handle value) noexcept { pthread_setspecific(pthread_key_t(reinterpret_cast<uintptr>(index)), value); }
		handle GetThreadLocal(handle index) noexcept { return pthread_getspecific(pthread_key_t(reinterpret_cast<uintptr>(index))); }
	}
}