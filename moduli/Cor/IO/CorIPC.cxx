#include "CorIPC.h"

namespace ESSE
{
	namespace IPC
	{
		uintptr IConnection::SendData(const void * data, uintptr size)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = SendData(data, size, ectx);
			ErrorThrow(ectx);
			return result;
		}
		uintptr IConnection::ReceiveData(void * data, uintptr size)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = ReceiveData(data, size, ectx);
			ErrorThrow(ectx);
			return result;
		}
		void IConnection::SendHandle(handle file)
		{
			ErrorContext ectx; ErrorClear(ectx);
			SendHandle(file, ectx);
			ErrorThrow(ectx);
		}
		handle IConnection::ReceiveHandle(void)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = ReceiveHandle(ectx);
			ErrorThrow(ectx);
			return result;
		}
		void IConnection::SendMachPort(uint32 port)
		{
			ErrorContext ectx; ErrorClear(ectx);
			SendMachPort(port, ectx);
			ErrorThrow(ectx);
		}
		uint32 IConnection::ReceiveMachPort(void)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = ReceiveMachPort(ectx);
			ErrorThrow(ectx);
			return result;
		}

		oref<IConnection> IConnectionListener::Accept(void)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = Accept(ectx);
			ErrorThrow(ectx);
			return result;
		}
		string IConnectionListener::GetOSPath(void)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = GetOSPath(ectx);
			ErrorThrow(ectx);
			return result;
		}
		string IConnectionListener::GetListenerName(void)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = GetListenerName(ectx);
			ErrorThrow(ectx);
			return result;
		}

		string ISharedMemory::GetSegmentName(void)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = GetSegmentName(ectx);
			ErrorThrow(ectx);
			return result;
		}
		void * ISharedMemory::Map(uint vmem_attributes)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = Map(vmem_attributes, ectx);
			ErrorThrow(ectx);
			return result;
		}

		string ISharedLock::GetLockName(void)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = GetLockName(ectx);
			ErrorThrow(ectx);
			return result;
		}

		oref<IConnection> Connect(const string & listener_name, uint mode)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = Connect(listener_name, mode, ectx);
			ErrorThrow(ectx);
			return result;
		}
		oref<IConnectionListener> CreateConnectionListener(const string & listener_name, uint mode)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = CreateConnectionListener(listener_name, mode, ectx);
			ErrorThrow(ectx);
			return result;
		}
		oref<ISharedMemory> CreateSharedMemory(const string & segment_name, uintptr size, FileCreationMode mode)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = CreateSharedMemory(segment_name, size, mode, ectx);
			ErrorThrow(ectx);
			return result;
		}
		oref<ISharedLock> CreateSharedLock(const string & lock_name)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = CreateSharedLock(lock_name, ectx);
			ErrorThrow(ectx);
			return result;
		}
		oref<ISharedMemory> OpenSharedMemory(handle object, uintptr size)
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto result = OpenSharedMemory(object, size, ectx);
			ErrorThrow(ectx);
			return result;
		}
	}
}