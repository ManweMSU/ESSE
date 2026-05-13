#pragma once

#include <stdlib.h>

#ifdef ESSE_SYSTEMA_WINDOWS
#define ESSE_PACKED_STRUCTURE(NAME) __pragma(pack(push, 1)) struct NAME {
#define ESSE_END_PACKED_STRUCTURE }; __pragma(pack(pop))
#endif
#ifdef ESSE_SYSTEMA_UNIX
#define ESSE_PACKED_STRUCTURE(NAME) struct NAME {
#define ESSE_END_PACKED_STRUCTURE } __attribute__((packed));
#endif

#define ESSE_MAIN_ROUTINE int Main(void) noexcept
#define ESSE_LIBRARY_INITIALIZE_ROUTINE int LibraryInitialize(void) noexcept
#define ESSE_LIBRARY_SHUTDOWN_ROUTINE int LibraryShutdown(void) noexcept

namespace ESSE
{
	typedef unsigned int		uint;
	typedef signed long long	int64;
	typedef unsigned long long	uint64;
	typedef signed int			int32;
	typedef unsigned int		uint32;
	typedef signed short		int16;
	typedef unsigned short		uint16;
	typedef signed char			int8;
	typedef unsigned char		uint8;

	typedef char				unichar8;
	typedef char16_t			unichar16;
	typedef char32_t			unichar32;
	typedef unichar32			unichar;

	#ifdef ESSE_MACHINA_32
		typedef int32			intptr;
		typedef uint32			uintptr;
		typedef int32			sintptr;
	#endif
	#ifdef ESSE_MACHINA_64
		typedef int64			intptr;
		typedef uint64			uintptr;
		typedef int64			sintptr;
	#endif
	
	typedef void * handle;

	namespace Memory
	{
		void ZeroMemory(void * pmem, uintptr size) noexcept;
		void * MemoryCopy(void * pdest, const void * psrc, uintptr size) noexcept;
		int MemoryCompare(const void * pmem1, const void * pmem2, uintptr size) noexcept;

		intptr StringLength(const unichar8 * str) noexcept;
		int StringCompare(const unichar8 * str1, const unichar8 * str2) noexcept;
		unichar8 * StringCopy(unichar8 * dest, const unichar8 * src) noexcept;
		unichar8 * StringConcatenate(unichar8 * dest, const unichar8 * src) noexcept;

		intptr StringLength(const unichar16 * str) noexcept;
		int StringCompare(const unichar16 * str1, const unichar16 * str2) noexcept;
		unichar16 * StringCopy(unichar16 * dest, const unichar16 * src) noexcept;
		unichar16 * StringConcatenate(unichar16 * dest, const unichar16 * src) noexcept;

		intptr StringLength(const unichar32 * str) noexcept;
		int StringCompare(const unichar32 * str1, const unichar32 * str2) noexcept;
		unichar32 * StringCopy(unichar32 * dest, const unichar32 * src) noexcept;
		unichar32 * StringConcatenate(unichar32 * dest, const unichar32 * src) noexcept;

		void AcquireRootLock(void) noexcept;
		void ReleaseRootLock(void) noexcept;
	}

	struct ErrorContext { uintptr error_code; uintptr error_subcode; };
	void ErrorClear(ErrorContext & ctx) noexcept;
	void ErrorSet(ErrorContext & ctx, uintptr code) noexcept;
	void ErrorSet(ErrorContext & ctx, uintptr code, uintptr subcode) noexcept;
	bool ErrorTest(const ErrorContext & ctx) noexcept;
	ErrorContext ErrorMake(uintptr code) noexcept;
	ErrorContext ErrorMake(uintptr code, uintptr subcode) noexcept;

	namespace Math {
		constexpr double pi	= 3.14159265358979323846;
		constexpr double e	= 2.71828182845904523536;
	}
}