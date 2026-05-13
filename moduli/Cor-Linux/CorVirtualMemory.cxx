#include <Cor/CorVirtualMemory.h>
#include <Cor/CorErrores.h>

#include <sys/mman.h>
#include <errno.h>

namespace ESSE
{
	uint InterlockedIncrement(uint & value) noexcept { return __sync_add_and_fetch(&value, 1); }
	uint InterlockedDecrement(uint & value) noexcept { return __sync_sub_and_fetch(&value, 1); }
	uintptr InterlockedIncrement(uintptr & value) noexcept { return __sync_add_and_fetch(&value, 1); }
	uintptr InterlockedDecrement(uintptr & value) noexcept { return __sync_sub_and_fetch(&value, 1); }
	namespace Memory
	{
		void * VirtualAllocate(uintptr size, uint attributes, ErrorContext & ectx) noexcept
		{
			int protection = PROT_NONE;
			int flags = MAP_ANONYMOUS;
			if (attributes & VirtualMemoryMapRead) protection |= PROT_READ;
			if (attributes & VirtualMemoryMapWrite) protection |= PROT_WRITE;
			if (attributes & VirtualMemoryMapExecute) protection |= PROT_EXEC;
			if (attributes & VirtualMemoryCopyOnWrite) flags |= MAP_PRIVATE; else flags |= MAP_SHARED;
			auto address = mmap(0, size, protection, flags, -1, 0);
			if (address == MAP_FAILED) {
				if (errno == ENOMEM || errno == EAGAIN || errno == EMFILE) ErrorSet(ectx, Errores::ErrorOutOfMemory);
				else if (errno == EINVAL) ErrorSet(ectx, Errores::ErrorInvalidArgument);
				else if (errno == ENOTSUP) ErrorSet(ectx, Errores::ErrorNotImplemented);
				else ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::Unknown);
				return 0;
			}
			return address;
		}
		void VirtualDeallocate(void * pmem, uintptr size) noexcept { munmap(pmem, size); }
		void SetMemoryProtection(void * pmem, uintptr size, uint attributes, ErrorContext & ectx) noexcept
		{
			int protection = PROT_NONE;
			if (attributes & VirtualMemoryMapRead) protection |= PROT_READ;
			if (attributes & VirtualMemoryMapWrite) protection |= PROT_WRITE;
			if (attributes & VirtualMemoryMapExecute) protection |= PROT_EXEC;
			if (mprotect(pmem, size, protection) < 0) {
				if (errno == EACCES) ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::AccessDenied);
				else if (errno == EAGAIN) ErrorSet(ectx, Errores::ErrorOutOfMemory);
				else if (errno == EINVAL || errno == ENOMEM) ErrorSet(ectx, Errores::ErrorInvalidArgument);
				else if (errno == ENOTSUP) ErrorSet(ectx, Errores::ErrorNotImplemented);
				else ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::Unknown);
			}
		}
		void EnablePerThreadJITExecution(bool enable) noexcept {}
		void ClearVirtualMemoryCache(void * pmem, uintptr size) noexcept
		{
			auto base = const_cast<char *>(reinterpret_cast<const char *>(pmem));
			__builtin___clear_cache(base, base + size);
		}
	}
}