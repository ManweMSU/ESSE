#pragma once

#include "CorIO.h"

namespace ESSE
{
	namespace IPC
	{
		namespace ConnectionMode { enum ConnectionModeFlags : uint {
			ConnectionModeRegular		= 0x00,
			ConnectionModeThreadSafe	= 0x01,
		}; };

		class IConnection : public Object
		{
		public:
			virtual uintptr SendData(const void * data, uintptr size, ErrorContext & ectx) noexcept = 0;
			virtual uintptr ReceiveData(void * data, uintptr size, ErrorContext & ectx) noexcept = 0;
			virtual void SendHandle(handle file, ErrorContext & ectx) noexcept = 0;
			virtual handle ReceiveHandle(ErrorContext & ectx) noexcept = 0;
			virtual void SendMachPort(uint32 port, ErrorContext & ectx) noexcept = 0;
			virtual uint32 ReceiveMachPort(ErrorContext & ectx) noexcept = 0;
			virtual handle GetIOHandle(void) noexcept = 0;

			uintptr SendData(const void * data, uintptr size);
			uintptr ReceiveData(void * data, uintptr size);
			void SendHandle(handle file);
			handle ReceiveHandle(void);
			void SendMachPort(uint32 port);
			uint32 ReceiveMachPort(void);
		};
		class IConnectionListener : public Object
		{
		public:
			virtual oref<IConnection> Accept(ErrorContext & ectx) noexcept = 0;
			virtual string GetOSPath(ErrorContext & ectx) noexcept = 0;
			virtual string GetListenerName(ErrorContext & ectx) noexcept = 0;

			oref<IConnection> Accept(void);
			string GetOSPath(void);
			string GetListenerName(void);
		};
		class ISharedMemory : public Object
		{
		public:
			virtual string GetSegmentName(ErrorContext & ectx) noexcept = 0;
			virtual uintptr GetLength(void) noexcept = 0;
			virtual void * Map(uint vmem_attributes, ErrorContext & ectx) noexcept = 0;
			virtual void Unmap(void) noexcept = 0;
			virtual handle GetIOHandle(void) noexcept = 0;

			string GetSegmentName(void);
			void * Map(uint vmem_attributes);
		};
		class ISharedLock : public Object
		{
		public:
			virtual string GetLockName(ErrorContext & ectx) noexcept = 0;
			virtual bool TryLock(void) noexcept = 0;
			virtual void Unlock(void) noexcept = 0;

			string GetLockName(void);
		};

		oref<IConnection> Connect(const string & listener_name, uint mode, ErrorContext & ectx) noexcept;
		oref<IConnectionListener> CreateConnectionListener(const string & listener_name, uint mode, ErrorContext & ectx) noexcept;
		oref<ISharedMemory> CreateSharedMemory(const string & segment_name, uintptr size, FileCreationMode mode, ErrorContext & ectx) noexcept;
		oref<ISharedLock> CreateSharedLock(const string & lock_name, ErrorContext & ectx) noexcept;
		oref<ISharedMemory> OpenSharedMemory(handle object, uintptr size, ErrorContext & ectx) noexcept;

		oref<IConnection> Connect(const string & listener_name, uint mode);
		oref<IConnectionListener> CreateConnectionListener(const string & listener_name, uint mode);
		oref<ISharedMemory> CreateSharedMemory(const string & segment_name, uintptr size, FileCreationMode mode);
		oref<ISharedLock> CreateSharedLock(const string & lock_name);
		oref<ISharedMemory> OpenSharedMemory(handle object, uintptr size);

		void PurifyConnectionListener(const string & name) noexcept;
		void PurifySharedMemory(const string & name) noexcept;
		void PurifySharedLock(const string & name) noexcept;
	}
}