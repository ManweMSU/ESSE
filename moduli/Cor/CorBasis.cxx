#include "CorBasis.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <atomic>

namespace ESSE
{
	namespace Memory
	{
		std::atomic_flag _esse_global_lock = ATOMIC_FLAG_INIT;

		void ZeroMemory(void * pmem, uintptr size) noexcept { memset(pmem, 0, size); }
		void * MemoryCopy(void * pdest, const void * psrc, uintptr size) noexcept { return memcpy(pdest, psrc, size); }
		int MemoryCompare(const void * pmem1, const void * pmem2, uintptr size) noexcept { return memcmp(pmem1, pmem2, size); }
		intptr StringLength(const unichar8 * str) noexcept { intptr i = 0; while (str[i]) i++; return i; }
		int StringCompare(const unichar8 * str1, const unichar8 * str2) noexcept
		{
			intptr i = 0;
			while (str1[i] && str1[i] == str2[i]) i++;
			if (str1[i] == 0 && str2[i] == 0) return 0;
			else if (str1[i] < str2[i]) return -1; else return 1;
		}
		unichar8 * StringCopy(unichar8 * dest, const unichar8 * src) noexcept { intptr i = -1; do { i++; dest[i] = src[i]; } while (src[i]); return dest; }
		unichar8 * StringConcatenate(unichar8 * dest, const unichar8 * src) noexcept
		{
			intptr d = 0, i = 0; while (dest[d]) d++;
			while (src[i]) { dest[d + i] = src[i]; i++; }
			dest[d + i] = 0; return dest;
		}
		intptr StringLength(const unichar16 * str) noexcept { intptr i = 0; while (str[i]) i++; return i; }
		int StringCompare(const unichar16 * str1, const unichar16 * str2) noexcept
		{
			intptr i = 0;
			while (str1[i] && str1[i] == str2[i]) i++;
			if (str1[i] == 0 && str2[i] == 0) return 0;
			else if (str1[i] < str2[i]) return -1; else return 1;
		}
		unichar16 * StringCopy(unichar16 * dest, const unichar16 * src) noexcept { intptr i = -1; do { i++; dest[i] = src[i]; } while (src[i]); return dest; }
		unichar16 * StringConcatenate(unichar16 * dest, const unichar16 * src) noexcept
		{
			intptr d = 0, i = 0; while (dest[d]) d++;
			while (src[i]) { dest[d + i] = src[i]; i++; }
			dest[d + i] = 0; return dest;
		}
		intptr StringLength(const unichar32 * str) noexcept { intptr i = 0; while (str[i]) i++; return i; }
		int StringCompare(const unichar32 * str1, const unichar32 * str2) noexcept
		{
			intptr i = 0;
			while (str1[i] && str1[i] == str2[i]) i++;
			if (str1[i] == 0 && str2[i] == 0) return 0;
			else if (str1[i] < str2[i]) return -1; else return 1;
		}
		unichar32 * StringCopy(unichar32 * dest, const unichar32 * src) noexcept { intptr i = -1; do { i++; dest[i] = src[i]; } while (src[i]); return dest; }
		unichar32 * StringConcatenate(unichar32 * dest, const unichar32 * src) noexcept
		{
			intptr d = 0, i = 0; while (dest[d]) d++;
			while (src[i]) { dest[d + i] = src[i]; i++; }
			dest[d + i] = 0; return dest;
		}
		void AcquireRootLock(void) noexcept { while (_esse_global_lock.test_and_set(std::memory_order_acquire)); }
		void ReleaseRootLock(void) noexcept { _esse_global_lock.clear(std::memory_order_release); }
	}
	void ErrorClear(ErrorContext & ctx) noexcept { ctx.error_code = ctx.error_subcode = 0; }
	void ErrorSet(ErrorContext & ctx, uintptr code) noexcept { ctx.error_code = code; ctx.error_subcode = 0; }
	void ErrorSet(ErrorContext & ctx, uintptr code, uintptr subcode) noexcept { ctx.error_code = code; ctx.error_subcode = subcode; }
	bool ErrorTest(const ErrorContext & ctx) noexcept { return ctx.error_code != 0; }
	ErrorContext ErrorMake(uintptr code) noexcept { ErrorContext e; ErrorSet(e, code); return e; }
	ErrorContext ErrorMake(uintptr code, uintptr subcode) noexcept { ErrorContext e; ErrorSet(e, code, subcode); return e; }
}