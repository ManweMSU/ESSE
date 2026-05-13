#pragma once

#include "CorBasis.h"

namespace ESSE
{
	namespace System
	{
		enum class System		{ Unknown = 0x00, Windows = 0x01, MacOSX = 0x02, Linux = 0x12, Unix = 0x22, EFI = 0x11, ESSE = 0x03 };
		enum class Architecture	{ Unknown = 0x00, X86_32 = 0x01, X86_64 = 0x02, ARMv7_T32 = 0x03, ARMv8_A64 = 0x04 };

		enum class ProcessorFeatureStatus	{ Unavailable = 0, Present = 1, Unknown = 2 };
		enum class ProcessorFeature			{ CPUID = 0, RNG = 1, AES = 2, SHA256 = 3, SHA256SW = 4, SHA512 = 5, SHA512SW = 6 };

		#if defined(ESSE_SYSTEMA_WINDOWS)
		constexpr System ThisApplicationSystem = System::Windows;
		#elif defined(ESSE_SYSTEMA_UNIX)
		#if defined(ESSE_SYSTEMA_MACOSX)
		constexpr System ThisApplicationSystem = System::MacOSX;
		#elif defined(ESSE_SYSTEMA_LINUX)
		constexpr System ThisApplicationSystem = System::Linux;
		#else
		constexpr System ThisApplicationSystem = System::Unix;
		#endif
		#else
		constexpr System ThisApplicationSystem = System::Unknown;
		#endif

		#if defined(ESSE_MACHINA_X86_32)
		constexpr Architecture ThisApplicationArchitecture = Architecture::X86_32;
		#elif defined(ESSE_MACHINA_X86_64)
		constexpr Architecture ThisApplicationArchitecture = Architecture::X86_64;
		#elif defined(ESSE_MACHINA_ARM_V7)
		constexpr Architecture ThisApplicationArchitecture = Architecture::ARMv7_T32;
		#elif defined(ESSE_MACHINA_ARM_V8)
		constexpr Architecture ThisApplicationArchitecture = Architecture::ARMv8_A64;
		#else
		constexpr Architecture ThisApplicationArchitecture = Architecture::Unknown;
		#endif

		intptr GetUserLocale(unichar8 * plocale, intptr buf_size) noexcept;
		intptr GetUserLocale(unichar16 * plocale, intptr buf_size) noexcept;
		intptr GetUserLocale(unichar32 * plocale, intptr buf_size) noexcept;

		uint32 GetMonotonicTime(void) noexcept;
		uint64 GetSystemTime(void) noexcept;
		uint64 TimeConvertToUniversal(uint64 time) noexcept;
		uint64 TimeConvertToLocal(uint64 time) noexcept;

		Architecture GetSystemArchitecture(void) noexcept;
		bool IsArchitectureEmulationEnabled(Architecture arch) noexcept;

		ProcessorFeatureStatus GetProcessorFeatureStatus(ProcessorFeature feat) noexcept;
		intptr GetProcessorName(unichar8 * pname, intptr buf_size) noexcept;
		intptr GetProcessorName(unichar16 * pname, intptr buf_size) noexcept;
		intptr GetProcessorName(unichar32 * pname, intptr buf_size) noexcept;
		uint GetProcessorCores(bool physical = false) noexcept;
		uint64 GetProcessorFrequency(void) noexcept;
		uint64 GetPhysicalMemory(void) noexcept;
		uint64 GetVirtualMemoryPageSize(void) noexcept;

		intptr GetSystemName(unichar8 * pname, intptr buf_size) noexcept;
		intptr GetSystemName(unichar16 * pname, intptr buf_size) noexcept;
		intptr GetSystemName(unichar32 * pname, intptr buf_size) noexcept;
		void GetSystemVersion(uint * major, uint * minor) noexcept;
	}
}