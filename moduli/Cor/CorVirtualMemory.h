#pragma once

#include "CorBasis.h"

namespace ESSE
{
	uint InterlockedIncrement(uint & value) noexcept;
	uint InterlockedDecrement(uint & value) noexcept;
	uintptr InterlockedIncrement(uintptr & value) noexcept;
	uintptr InterlockedDecrement(uintptr & value) noexcept;

	namespace Memory
	{
		constexpr uint VirtualMemoryMapRead		= 0x01;
		constexpr uint VirtualMemoryMapWrite	= 0x02;
		constexpr uint VirtualMemoryMapExecute	= 0x04;
		constexpr uint VirtualMemoryCopyOnWrite	= 0x10;
		constexpr uint VirtualMemoryJIT			= 0x20;

		void * VirtualAllocate(uintptr size, uint attributes, ErrorContext & ectx) noexcept;
		void VirtualDeallocate(void * pmem, uintptr size) noexcept;
		void SetMemoryProtection(void * pmem, uintptr size, uint attributes, ErrorContext & ectx) noexcept;

		void EnablePerThreadJITExecution(bool enable) noexcept;
		void ClearVirtualMemoryCache(void * pmem, uintptr size) noexcept;
	}
}