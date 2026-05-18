#include <Cor/IO/CorIPC.h>
#include <Cor/CorVirtualMemory.h>
#include "CorIOEx.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/un.h>

namespace ESSE
{
	namespace IPC
	{
		class LinuxConnection : public IConnection
		{
			int _socket;
		public:
			LinuxConnection(const string & listener_name, uint mode)
			{
				ucs1_string path(U"/tmp/pipe_" + listener_name);
				sockaddr_un addr;
				Memory::ZeroMemory(&addr, sizeof(addr));
				addr.sun_family = PF_LOCAL;
				if (path.GetLength() >= sizeof(addr.sun_path)) throw InputOutputException(Errores::SuberrorIO::FileNameTooLong);
				Memory::MemoryCopy(&addr.sun_path, path.GetData(), path.GetLength() + 1);
				_socket = socket(PF_LOCAL, SOCK_STREAM, 0);
				if (_socket < 0) { ErrorContext ectx; Linux::ErrorSetPosix(ectx); throw CustomException(ectx); }
				while (true) {
					auto status = connect(_socket, reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
					if (status < 0 && errno != EINTR) {
						ErrorContext ectx; Linux::ErrorSetPosix(ectx);
						close(_socket);
						throw CustomException(ectx);
					} else if (status >= 0) break;
				}
			}
			LinuxConnection(int io) : _socket(io) {}
			virtual ~LinuxConnection(void) override { shutdown(_socket, SHUT_RDWR); close(_socket); }
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"IPC Connection"; ESSE_TRY_OUTRO(string()) }
			virtual uintptr SendData(const void * data, uintptr size, ErrorContext & ectx) noexcept override { return IO::WriteFile(reinterpret_cast<handle>(intptr(_socket)), data, size, ectx); }
			virtual uintptr ReceiveData(void * data, uintptr size, ErrorContext & ectx) noexcept override { return IO::ReadFile(reinterpret_cast<handle>(intptr(_socket)), data, size, ectx); }
			virtual void SendHandle(handle file, ErrorContext & ectx) noexcept override
			{
				uint8 ctl_data[CMSG_SPACE(sizeof(int))];
				Memory::ZeroMemory(&ctl_data, sizeof(ctl_data));
				char null = 0;
				struct iovec io;
				io.iov_base = &null;
				io.iov_len = 1;
				struct cmsghdr & ctl = *reinterpret_cast<struct cmsghdr *>(&ctl_data);
				ctl.cmsg_len = CMSG_LEN(sizeof(int));
				ctl.cmsg_level = SOL_SOCKET;
				ctl.cmsg_type = SCM_RIGHTS;
				*reinterpret_cast<int *>(&CMSG_DATA(&ctl)) = reinterpret_cast<intptr>(file);
				struct msghdr hdr;
				hdr.msg_name = 0;
				hdr.msg_namelen = 0;
				hdr.msg_iov = &io;
				hdr.msg_iovlen = 1;
				hdr.msg_control = &ctl_data;
				hdr.msg_controllen = sizeof(ctl_data);
				hdr.msg_flags = 0;
				while (true) {
					auto status = sendmsg(_socket, &hdr, 0);
					if (status < 0 && errno != EINTR) { Linux::ErrorSetPosix(ectx); return; }
					else if (status >= 0) break;
				}
			}
			virtual handle ReceiveHandle(ErrorContext & ectx) noexcept override
			{
				uint8 ctl_data[0x100];
				char null;
				struct iovec io;
				io.iov_base = &null;
				io.iov_len = 1;
				struct msghdr hdr;
				hdr.msg_name = 0;
				hdr.msg_namelen = 0;
				hdr.msg_iov = &io;
				hdr.msg_iovlen = 1;
				hdr.msg_control = &ctl_data;
				hdr.msg_controllen = sizeof(ctl_data);
				hdr.msg_flags = 0;
				while (true) {
					auto status = recvmsg(_socket, &hdr, 0);
					if (status < 0 && errno != EINTR) { Linux::ErrorSetPosix(ectx); return; }
					else if (status >= 0) break;
				}
				if (hdr.msg_flags & (MSG_TRUNC | MSG_CTRUNC)) { ErrorSet(ectx, Errores::ErrorInvalidFormat); return; }
				auto ctl_hdr = CMSG_FIRSTHDR(&hdr);
				if (!ctl_hdr) { ErrorSet(ectx, Errores::ErrorInvalidFormat); return; }
				if (ctl_hdr->cmsg_len < sizeof(struct cmsghdr) + sizeof(int)) { ErrorSet(ectx, Errores::ErrorInvalidFormat); return; }
				if (ctl_hdr->cmsg_level != SOL_SOCKET || ctl_hdr->cmsg_type != SCM_RIGHTS) { ErrorSet(ectx, Errores::ErrorInvalidFormat); return; }
				int fd = *reinterpret_cast<int *>(&CMSG_DATA(ctl_hdr));
				return reinterpret_cast<handle>(intptr(fd));
			}
			virtual void SendMachPort(uint32 port, ErrorContext & ectx) noexcept override { ErrorSet(ectx, Errores::ErrorNotImplemented); }
			virtual uint32 ReceiveMachPort(ErrorContext & ectx) noexcept override { ErrorSet(ectx, Errores::ErrorNotImplemented); return 0; }
			virtual handle GetIOHandle(void) noexcept override { return reinterpret_cast<handle>(intptr(_socket)); }
		};
		class LinuxConnectionListener : public IConnectionListener
		{
			string _name, _public_path;
			ucs1_string _path;
			int _socket;
		public:
			LinuxConnectionListener(const string & listener_name, uint mode) : _name(listener_name)
			{
				_path = _public_path = U"/tmp/pipe_" + _name;
				sockaddr_un addr;
				Memory::ZeroMemory(&addr, sizeof(addr));
				addr.sun_family = PF_LOCAL;
				if (_path.GetLength() >= sizeof(addr.sun_path)) throw InputOutputException(Errores::SuberrorIO::FileNameTooLong);
				Memory::MemoryCopy(&addr.sun_path, _path.GetData(), _path.GetLength() + 1);
				_socket = socket(PF_LOCAL, SOCK_STREAM, 0);
				if (_socket < 0) { ErrorContext ectx; Linux::ErrorSetPosix(ectx); throw CustomException(ectx); }
				if (bind(_socket, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0) {
					ErrorContext ectx; Linux::ErrorSetPosix(ectx);
					close(_socket);
					throw CustomException(ectx);
				}
				if (listen(_socket, SOMAXCONN) < 0) {
					ErrorContext ectx; Linux::ErrorSetPosix(ectx);
					unlink(_path); close(_socket);
					throw CustomException(ectx);
				}
			}
			virtual ~LinuxConnectionListener(void) override { unlink(_path); shutdown(_socket, SHUT_RDWR); close(_socket); }
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"IPC Connection Listener"; ESSE_TRY_OUTRO(string()) }
			virtual oref<IConnection> Accept(ErrorContext & ectx) noexcept override
			{
				int client;
				oref<IConnection> result;
				while (true) {
					client = accept(_socket, 0, 0);
					if (client < 0 && errno != EINTR) { Linux::ErrorSetPosix(ectx); return 0; }
					else if (client >= 0) break;
				}
				try { result.SetOwned(new LinuxConnection(client)); } catch (...) {
					close(client);
					ErrorSet(ectx, Errores::ErrorOutOfMemory);
					return 0;
				}
				return result;
			}
			virtual string GetOSPath(ErrorContext & ectx) noexcept override { ESSE_TRY_INTRO return _public_path; ESSE_TRY_OUTRO(string()) }
			virtual string GetListenerName(ErrorContext & ectx) noexcept override { ESSE_TRY_INTRO return _name; ESSE_TRY_OUTRO(string()) }
		};
		class LinuxSharedMemory : public ISharedMemory
		{
			string _name;
			ucs1_string _path;
			int _file;
			uintptr _length;
			bool _unlink;
			void * _pdata;
		public:
			LinuxSharedMemory(const string & segment_name, uintptr size, FileCreationMode mode) : _name(segment_name), _pdata(MAP_FAILED)
			{
				if (!size) throw InvalidArgumentException();
				_path = _name;
				int flags = O_RDWR;
				if (mode == FileCreationMode::CreateNew) {
					_unlink = true;
					flags |= O_CREAT | O_EXCL;
				} else if (mode == FileCreationMode::OpenExisting) {
					_unlink = false;
				} else throw InvalidArgumentException();
				while (true) {
					_file = shm_open(_path, flags, 0666);
					if (_file < 0 && errno != EINTR) {
						ErrorContext ectx; Linux::ErrorSetPosix(ectx);
						throw CustomException(ectx);
					} else if (_file >= 0) break;
				}
				if (_unlink) {
					_length = size;
					while (true) {
						auto status = ftruncate(_file, _length);
						if (status < 0 && errno != EINTR) {
							ErrorContext ectx; Linux::ErrorSetPosix(ectx);
							close(_file); shm_unlink(_path);
							throw CustomException(ectx);
						} else if (status >= 0) break;
					}
				} else {
					struct stat fs;
					if (fstat(_file, &fs) < 0) {
						ErrorContext ectx; Linux::ErrorSetPosix(ectx);
						close(_file);
						throw CustomException(ectx);
					}
					_length = fs.st_size;
				}
			}
			virtual ~LinuxSharedMemory(void) override { Unmap(); close(_file); if (_unlink) shm_unlink(_path); }
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"IPC Shared Memory"; ESSE_TRY_OUTRO(string()) }
			virtual string GetSegmentName(ErrorContext & ectx) noexcept override { ESSE_TRY_INTRO return _name; ESSE_TRY_OUTRO(string()) }
			virtual uintptr GetLength(void) noexcept override { return _length; }
			virtual void * Map(uint vmem_attributes, ErrorContext & ectx) noexcept override
			{
				if (_pdata != MAP_FAILED) { ErrorSet(ectx, Errores::ErrorInvalidState); return 0; }
				int protection = PROT_NONE;
				int flags = 0;
				if (vmem_attributes & Memory::VirtualMemoryMapRead) protection |= PROT_READ;
				if (vmem_attributes & Memory::VirtualMemoryMapWrite) protection |= PROT_WRITE;
				if (vmem_attributes & Memory::VirtualMemoryMapExecute) protection |= PROT_EXEC;
				if (vmem_attributes & Memory::VirtualMemoryCopyOnWrite) flags |= MAP_PRIVATE; else flags |= MAP_SHARED;
				_pdata = mmap(0, _length, protection, flags, _file, 0);
				if (_pdata == MAP_FAILED) {
					if (errno == EACCES) ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::AccessDenied);
					else if (errno == ENOMEM || errno == EAGAIN || errno == EMFILE) ErrorSet(ectx, Errores::ErrorOutOfMemory);
					else if (errno == EINVAL) ErrorSet(ectx, Errores::ErrorInvalidArgument);
					else if (errno == ENOTSUP) ErrorSet(ectx, Errores::ErrorNotImplemented);
					else if (errno == ENXIO || errno == EOVERFLOW) ErrorSet(ectx, Errores::ErrorInvalidState);
					else ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::Unknown);
					return 0;
				}
				return _pdata;
			}
			virtual void Unmap(void) noexcept override { if (_pdata != MAP_FAILED) { munmap(_pdata, _length); _pdata = MAP_FAILED; } }
		};
		class LinuxSharedLock : public ISharedLock
		{
			string _name;
			ucs1_string _path;
			int _file;
			bool _locked;
		public:
			LinuxSharedLock(const string & lock_name) : _locked(false), _name(lock_name)
			{
				_path = U"/tmp/lock_" + lock_name;
				while (true) {
					_file = open(_path, O_RDONLY | O_CREAT, 0666);
					if (_file < 0 && errno != EINTR) {
						ErrorContext ectx; Linux::ErrorSetPosix(ectx);
						throw CustomException(ectx);
					} else if (_file >= 0) break;
				}
			}
			virtual ~LinuxSharedLock(void) override { if (_locked) flock(_file, LOCK_UN); close(_file); }
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"IPC Shared Lock"; ESSE_TRY_OUTRO(string()) }
			virtual string GetLockName(ErrorContext & ectx) noexcept override { ESSE_TRY_INTRO return _name; ESSE_TRY_OUTRO(string()) }
			virtual bool TryLock(void) noexcept override { if (_locked) return false; if (flock(_file, LOCK_EX | LOCK_NB) < 0) return false; _locked = true; return true; }
			virtual void Unlock(void) noexcept override { if (_locked) { flock(_file, LOCK_UN); _locked = false; } }
		};

		oref<IConnection> Connect(const string & listener_name, uint mode, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
			oref<IConnection> result;
			result.SetOwned(new LinuxConnection(listener_name, mode));
			return result;
			ESSE_TRY_OUTRO(0)
		}
		oref<IConnectionListener> CreateConnectionListener(const string & listener_name, uint mode, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
			oref<IConnectionListener> result;
			result.SetOwned(new LinuxConnectionListener(listener_name, mode));
			return result;
			ESSE_TRY_OUTRO(0)
		}
		oref<ISharedMemory> CreateSharedMemory(const string & segment_name, uintptr size, FileCreationMode mode, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
			oref<ISharedMemory> result;
			result.SetOwned(new LinuxSharedMemory(segment_name, size, mode));
			return result;
			ESSE_TRY_OUTRO(0)
		}
		oref<ISharedLock> CreateSharedLock(const string & lock_name, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
			oref<ISharedLock> result;
			result.SetOwned(new LinuxSharedLock(lock_name));
			return result;
			ESSE_TRY_OUTRO(0)
		}
		void PurifyConnectionListener(const string & name) noexcept { try { unlink(ucs1_string(U"/tmp/pipe_" + name)); } catch (...) {} }
		void PurifySharedMemory(const string & name) noexcept { try { shm_unlink(ucs1_string(name)); } catch (...) {} }
		void PurifySharedLock(const string & name) noexcept { try { unlink(ucs1_string(U"/tmp/lock_" + name)); } catch (...) {} }
	}
}