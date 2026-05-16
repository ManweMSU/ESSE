#pragma once

#include <Cor/Cor.h>
#include "../Miscellaneous/Encoding.h"

#include <stdlib.h>
#include <math.h>

#define ENGINE_RUNTIME_VERSION_MAJOR ESSE_VERSIO_CORDIS_MAJOR
#define ENGINE_RUNTIME_VERSION_MINOR ESSE_VERSIO_CORDIS_MINOR
#ifdef ESSE_SYSTEMA_WINDOWS
	#define ENGINE_WINDOWS
#endif
#ifdef ESSE_SYSTEMA_UNIX
	#define ENGINE_UNIX
#endif
#ifdef ESSE_SYSTEMA_MACOSX
	#define ENGINE_MACOSX
#endif
#ifdef ESSE_SYSTEMA_LINUX
	#define ENGINE_LINUX
	#define ENGINE_LINUX_FULL
#endif
#ifdef ESSE_MACHINA_X86_32
#endif
#ifdef ESSE_MACHINA_X86_64
	#define ENGINE_X64
#endif
#ifdef ESSE_MACHINA_ARM_V7
	#define ENGINE_ARM
#endif
#ifdef ESSE_MACHINA_ARM_V8
	#define ENGINE_ARM
	#define ENGINE_X64
#endif
#ifdef ESSE_DEBUG
	#define ENGINE_DEBUG
#endif
#ifdef ESSE_SUBSYSTEMA_CONSOLE
	#define ENGINE_SUBSYSTEM_CONSOLE
#endif
#ifdef ESSE_SUBSYSTEMA_GUI
	#define ENGINE_SUBSYSTEM_GUI
#endif
#ifdef ESSE_SUBSYSTEMA_LIBRARY
	#define ENGINE_SUBSYSTEM_LIBRARY
#endif
#ifdef ESSE_SUBSYSTEMA_SILENT
	#define ENGINE_SUBSYSTEM_SILENT
#endif
#ifdef ESSE_META_NOMEN_APPLICATIONIS
	#define ENGINE_VI_APPNAME ESSE_META_NOMEN_APPLICATIONIS
#endif
#ifdef ESSE_META_AUTHOR_APPLICATIONIS
	#define ENGINE_VI_COMPANY ESSE_META_AUTHOR_APPLICATIONIS
#endif
#ifdef ESSE_META_JURA_EXEMPLI
	#define ENGINE_VI_COPYRIGHT ESSE_META_JURA_EXEMPLI
#endif
#ifdef ESSE_META_NOMEN_INTERNUM
	#define ENGINE_VI_APPSYSNAME ESSE_META_NOMEN_INTERNUM
#endif
#ifdef ESSE_META_VERSIO_APPLICATIONIS
	#define ENGINE_VI_APPVERSION ESSE_META_VERSIO_APPLICATIONIS
#endif
#ifdef ESSE_META_VERSIO_BREVIS
	#define ENGINE_VI_APPSHORTVERSION ESSE_META_VERSIO_BREVIS
#endif
#ifdef ESSE_META_VERSIO_MAJOR
	#define ENGINE_VI_VERSIONMAJOR ESSE_META_VERSIO_MAJOR
#endif
#ifdef ESSE_META_VERSIO_MINOR
	#define ENGINE_VI_VERSIONMINOR ESSE_META_VERSIO_MINOR
#endif
#ifdef ESSE_META_VERSIO_MICRO
	#define ENGINE_VI_SUBVERSION ESSE_META_VERSIO_MICRO
#endif
#ifdef ESSE_META_VERSIO_ITERATIO
	#define ENGINE_VI_BUILD ESSE_META_VERSIO_ITERATIO
#endif
#ifdef ESSE_META_INDENTITAS_APPLICATIONIS
	#define ENGINE_VI_APPIDENT ESSE_META_INDENTITAS_APPLICATIONIS
#endif
#ifdef ESSE_META_INDENTITAS_AUTHORIS
	#define ENGINE_VI_COMPANYIDENT ESSE_META_INDENTITAS_AUTHORIS
#endif
#ifdef ESSE_META_DESCRIPTIO
	#define ENGINE_VI_DESCRIPTION ESSE_META_DESCRIPTIO
#endif

#define ENGINE_PACKED_STRUCTURE(NAME) ESSE_PACKED_STRUCTURE(NAME)
#define ENGINE_END_PACKED_STRUCTURE ESSE_END_PACKED_STRUCTURE

#define ENGINE_PI 3.14159265358979323846

namespace Engine
{
	typedef ESSE::uint uint;
	typedef ESSE::int32 int32;
	typedef ESSE::uint32 uint32;
	typedef ESSE::int64 int64;
	typedef ESSE::uint64 uint64;
	typedef ESSE::int16 int16;
	typedef ESSE::uint16 uint16;
	typedef ESSE::int8 int8;
	typedef ESSE::uint8 uint8;

	typedef ESSE::uintptr intptr;
	typedef ESSE::uintptr uintptr;
	typedef ESSE::sintptr sintptr;
	typedef ESSE::uintptr eint;
	typedef ESSE::handle handle;

	typedef wchar_t legacy_widechar;
	typedef ESSE::unichar widechar;
	typedef ESSE::Unicode::NormalizationForm NormalizeForm;
	typedef ESSE::System::Architecture Platform;

	constexpr Encoding SystemEncoding = Encoding::UTF32;
	constexpr Platform ApplicationPlatform = ESSE::System::ThisApplicationArchitecture;

	#ifdef ENGINE_WINDOWS
		constexpr Encoding LegacySystemEncoding = Encoding::UTF16;
		constexpr const widechar * OperatingSystemName = U"Windows";
	#endif
	#ifdef ENGINE_UNIX
		constexpr Encoding LegacySystemEncoding = Encoding::UTF32;
		#ifdef ENGINE_MACOSX
			constexpr const widechar * OperatingSystemName = U"Mac OS";
		#elif defined(ENGINE_LINUX)
			constexpr const widechar * OperatingSystemName = U"Linux";
		#else
			constexpr const widechar * OperatingSystemName = U"Unix";
		#endif
	#endif

	struct SystemDesc {
		Platform Architecture;
		widechar ProcessorName[0x80];
		uint PhysicalCores;
		uint VirtualCores;
		uint64 ClockFrequency;
		uint64 PhysicalMemory;
		uint SystemVersionMajor;
		uint SystemVersionMinor;
	};

	// Atomic increment and decrement; memory initialization
	uint InterlockedIncrement(uint & Value);
	uint InterlockedDecrement(uint & Value);
	void ZeroMemory(void * Memory, intptr Size);

	// System timer's value in milliseconds. The beginning of this time axis is not important
	uint32 GetTimerValue(void);
	// OS Native Time functions (both in OS dependent currency: Windows or Unix)
	uint64 GetNativeTime(void);
	uint64 TimeUniversalToLocal(uint64 time);
	uint64 TimeLocalToUniversal(uint64 time);

	// Some C standard library and language dependent case insensitive comparation
	void * MemoryCopy(void * Dest, const void * Source, intptr Length);
	widechar * StringCopy(widechar * Dest, const widechar * Source);
	int StringCompare(const widechar * A, const widechar * B);
	int SequenceCompare(const widechar * A, const widechar * B, int Length);
	int MemoryCompare(const void * A, const void * B, intptr Length);
	int StringCompareCaseInsensitive(const widechar * A, const widechar * B);
	int StringLength(const widechar * str);
	void UnicodeNormalize(const widechar * source, widechar ** dest, NormalizeForm form = NormalizeForm::C);
	void StringAppend(widechar * str, widechar letter);

	// Case converters for fixed-length strings - should work with any language chars
	void StringLower(widechar * str, int length);
	void StringUpper(widechar * str, int length);
	bool IsAlphabetical(uint32 letter);

	// Query system information
	bool IsPlatformAvailable(Platform platform);
	Platform GetSystemPlatform(void);
	int GetProcessorsNumber(void);
	uint64 GetInstalledMemory(void);
	bool GetSystemInformation(SystemDesc & desc);
}