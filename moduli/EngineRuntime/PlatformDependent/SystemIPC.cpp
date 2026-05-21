#include "../Interfaces/SystemIPC.h"

namespace Engine
{
	namespace IPC
	{
		uint ErrorContextToIPC(const ESSE::ErrorContext & ectx) noexcept
		{
			if (ectx.error_code == ESSE::Errores::ErrorInvalidArgument) return ErrorInvalidArgument;
			if (ectx.error_code == ESSE::Errores::ErrorIO) {
				if (ectx.error_subcode == ESSE::Errores::SuberrorIO::FileExists) return ErrorAlreadyExists;
				if (ectx.error_subcode == ESSE::Errores::SuberrorIO::FileNotFound || ectx.error_subcode == ESSE::Errores::SuberrorIO::PathNotFound) return ErrorDoesNotExist;
				if (ectx.error_subcode == ESSE::Errores::SuberrorIO::BadPathName || ectx.error_subcode == ESSE::Errores::SuberrorIO::FileNameTooLong) return ErrorBadFileName;
			}
			return ErrorAllocation;
		}
		class SystemConnection : public IConnection
		{
			ESSE::oref<ESSE::IPC::IConnection> _con;
		public:
			SystemConnection(const string & listener_name, uint flags, uint * error)
			{
				ESSE::ErrorContext ectx; ESSE::ErrorClear(ectx);
				uint mode = 0;
				if (flags & CreateConnectionMultiThreaded) mode |= ESSE::IPC::ConnectionMode::ConnectionModeThreadSafe;
				_con = ESSE::IPC::Connect(static_cast<const ESSE::unichar32 *>(listener_name), mode, ectx);
				if (ESSE::ErrorTest(ectx)) {
					if (error) *error = ErrorContextToIPC(ectx);
					throw Exception();
				} else if (error) *error = ErrorSuccess;
			}
			SystemConnection(ESSE::IPC::IConnection * con) : _con(con) {}
			virtual ~SystemConnection(void) override {}
			virtual void Read(void * buffer, uint32 length) override
			{
				auto read = _con->ReceiveData(buffer, length);
				if (read != length) throw IO::FileReadEndOfFileException(read);
			}
			virtual void Write(const void * data, uint32 length) override
			{
				auto written = _con->SendData(data, length);
				if (written != length) throw ESSE::InputOutputException(ESSE::Errores::SuberrorIO::WriteFailure);
			}
			virtual int64 Seek(int64 position, Streaming::SeekOrigin origin) override { throw Exception(); }
			virtual uint64 Length(void) override { throw Exception(); }
			virtual void SetLength(uint64 length) override { throw Exception(); }
			virtual void Flush(void) override {}
			virtual handle GetIOHandle(void) noexcept override { return _con->GetIOHandle(); }
		};
		class SystemConnectionListener : public IConnectionListener
		{
			ESSE::oref<ESSE::IPC::IConnectionListener> _con;
		public:
			SystemConnectionListener(const string & listener_name, uint flags, uint * error)
			{
				ESSE::ErrorContext ectx; ESSE::ErrorClear(ectx);
				uint mode = 0;
				if (flags & CreateConnectionMultiThreaded) mode |= ESSE::IPC::ConnectionMode::ConnectionModeThreadSafe;
				_con = ESSE::IPC::CreateConnectionListener(static_cast<const ESSE::unichar32 *>(listener_name), mode, ectx);
				if (ESSE::ErrorTest(ectx)) {
					if (error) *error = ErrorContextToIPC(ectx);
					throw Exception();
				} else if (error) *error = ErrorSuccess;
			}
			virtual ~SystemConnectionListener(void) override {}
			virtual IConnection * Accept(void) noexcept override
			{
				ESSE::ErrorContext ectx; ESSE::ErrorClear(ectx);
				auto con = _con->Accept(ectx);
				if (ESSE::ErrorTest(ectx)) return 0; else {
					IConnection * result = new (std::nothrow) SystemConnection(con);
					if (result) result->Retain();
					return result;
				}
			}
			virtual string GetOSPath(void) override { return _con->GetOSPath().GetData(); }
			virtual string GetListenerName(void) override { return _con->GetListenerName().GetData(); }
		};
		class SystemSharedMemory : public ISharedMemory
		{
			ESSE::oref<ESSE::IPC::ISharedMemory> _mem;
		public:
			SystemSharedMemory(const string & segment_name, uint length, uint flags, uint * error)
			{
				ESSE::ErrorContext ectx; ESSE::ErrorClear(ectx);
				ESSE::FileCreationMode mode;
				if (flags & SharedMemoryCreateNew) mode = ESSE::FileCreationMode::CreateNew;
				else mode = ESSE::FileCreationMode::OpenExisting;
				_mem = ESSE::IPC::CreateSharedMemory(static_cast<const ESSE::unichar32 *>(segment_name), length, mode, ectx);
				if (ESSE::ErrorTest(ectx)) {
					if (error) *error = ErrorContextToIPC(ectx);
					throw Exception();
				} else if (error) *error = ErrorSuccess;
			}
			virtual ~SystemSharedMemory(void) override {}
			virtual string GetSegmentName(void) override { return _mem->GetSegmentName().GetData(); }
			virtual uint GetLength(void) noexcept override { return _mem->GetLength(); }
			virtual bool Map(void ** pdata, uint map_flags) noexcept override
			{
				ESSE::ErrorContext ectx; ESSE::ErrorClear(ectx);
				uint mode = 0;
				if (map_flags & SharedMemoryMapRead) mode |= ESSE::Memory::VirtualMemoryMapRead;
				if (map_flags & SharedMemoryMapWrite) mode |= ESSE::Memory::VirtualMemoryMapWrite;
				auto result = _mem->Map(mode, ectx);
				if (ESSE::ErrorTest(ectx)) return false;
				if (pdata) *pdata = result;
				return true;
			}
			virtual void Unmap(void) noexcept override { _mem->Unmap(); }
		};
		class SystemSharedLock : public ISharedLock
		{
			ESSE::oref<ESSE::IPC::ISharedLock> _lock;
		public:
			SystemSharedLock(const string & lock_name, uint * error)
			{
				ESSE::ErrorContext ectx; ESSE::ErrorClear(ectx);
				_lock = ESSE::IPC::CreateSharedLock(static_cast<const ESSE::unichar32 *>(lock_name), ectx);
				if (ESSE::ErrorTest(ectx)) {
					if (error) *error = ErrorContextToIPC(ectx);
					throw Exception();
				} else if (error) *error = ErrorSuccess;
			}
			virtual ~SystemSharedLock(void) override {}
			virtual string GetLockName(void) override { return _lock->GetLockName().GetData(); }
			virtual bool TryWait(void) noexcept override { return _lock->TryLock(); }
			virtual void Open(void) noexcept override { _lock->Unlock(); }
		};

		IConnection * Connect(const string & listener_name, uint flags, uint * error) noexcept { try { if (error) *error = ErrorAllocation; return new SystemConnection(listener_name, flags, error); } catch (...) { return 0; } }
		IConnectionListener * CreateConnectionListener(const string & listener_name, uint flags, uint * error) noexcept { try { if (error) *error = ErrorAllocation; return new SystemConnectionListener(listener_name, flags, error); } catch (...) { return 0; } }
		ISharedMemory * CreateSharedMemory(const string & segment_name, uint length, uint flags, uint * error) noexcept { try { if (error) *error = ErrorAllocation; return new SystemSharedMemory(segment_name, length, flags, error); } catch (...) { return 0; } }
		ISharedLock * CreateSharedLock(const string & lock_name, uint * error) noexcept { try { if (error) *error = ErrorAllocation; return new SystemSharedLock(lock_name, error); } catch (...) { return 0; } }

		void DestroyConnectionListener(const string & name) noexcept { ESSE::IPC::PurifyConnectionListener(static_cast<const ESSE::unichar32 *>(name)); }
		void DestroySharedMemory(const string & name) noexcept { ESSE::IPC::PurifySharedMemory(static_cast<const ESSE::unichar32 *>(name)); }
		void DestroySharedLock(const string & name) noexcept { ESSE::IPC::PurifySharedLock(static_cast<const ESSE::unichar32 *>(name)); }
	}
}