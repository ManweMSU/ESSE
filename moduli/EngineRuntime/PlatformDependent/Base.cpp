#include "../Interfaces/Base.h"

namespace Engine
{
	uint InterlockedIncrement(uint & Value) { return ESSE::InterlockedIncrement(Value); }
	uint InterlockedDecrement(uint & Value) { return ESSE::InterlockedDecrement(Value); }
	void ZeroMemory(void * Memory, intptr Size) { ESSE::Memory::ZeroMemory(Memory, Size); }
	uint32 GetTimerValue(void) { return ESSE::System::GetMonotonicTime(); }
	uint64 GetNativeTime(void) { return ESSE::System::GetSystemTime(); }
	uint64 TimeUniversalToLocal(uint64 time) { return ESSE::System::TimeConvertToLocal(time); }
	uint64 TimeLocalToUniversal(uint64 time) { return ESSE::System::TimeConvertToUniversal(time); }
	void * MemoryCopy(void * Dest, const void * Source, intptr Length) { return ESSE::Memory::MemoryCopy(Dest, Source, Length); }
	widechar * StringCopy(widechar * Dest, const widechar * Source) { return ESSE::Memory::StringCopy(Dest, Source); }
	int StringCompare(const widechar * A, const widechar * B) { return ESSE::Memory::StringCompare(A, B); }
	int SequenceCompare(const widechar * A, const widechar * B, int Length)
	{
		int i = 0;
		while (A[i] == B[i] && i < Length) i++;
		if (i == Length) return 0;
		if (A[i] < B[i]) return -1;
		return 1;
	}
	int MemoryCompare(const void * A, const void * B, intptr Length) { return ESSE::Memory::MemoryCompare(A, B, Length); }
	int StringCompareCaseInsensitive(const widechar * A, const widechar * B)
	{
		ESSE::ErrorContext ectx;
		ESSE::ErrorClear(ectx);
		auto result = ESSE::Unicode::CaseInsensitiveCompare(A, ESSE::Memory::StringLength(A), B, ESSE::Memory::StringLength(B), ectx);
		if (ESSE::ErrorTest(ectx)) return 0;
		return result;
	}
	int StringLength(const widechar * str) { return ESSE::Memory::StringLength(str); }
	void UnicodeNormalize(const widechar * source, widechar ** dest, NormalizeForm form)
	{
		ESSE::ErrorContext ectx;
		ESSE::ErrorClear(ectx);
		widechar * result;
		uintptr length;
		ESSE::Unicode::Normalize(source, ESSE::Memory::StringLength(source) + 1, form, &result, &length, ectx);
		if (ESSE::ErrorTest(ectx)) { *dest = 0; return; }
		*dest = result;
	}
	void StringAppend(widechar * str, widechar letter) { auto len = ESSE::Memory::StringLength(str); str[len + 1] = 0; str[len] = letter; }
	void StringLower(widechar * str, int length) { for (int i = 0; i < length; i++) str[i] = ESSE::Unicode::ConvertToLowerCase(str[i]); }
	void StringUpper(widechar * str, int length) { for (int i = 0; i < length; i++) str[i] = ESSE::Unicode::ConvertToUpperCase(str[i]); }
	bool IsAlphabetical(uint32 letter) { return ESSE::Unicode::BelongsToClass(letter, ESSE::Unicode::CharacterClass::Letter); }
	bool IsPlatformAvailable(Platform platform) { return ESSE::System::IsArchitectureEmulationEnabled(platform); }
	Platform GetSystemPlatform(void) { return ESSE::System::GetSystemArchitecture(); }
	int GetProcessorsNumber(void) { return ESSE::System::GetProcessorCores(); }
	uint64 GetInstalledMemory(void) { return ESSE::System::GetPhysicalMemory(); }
	bool GetSystemInformation(SystemDesc & desc)
	{
		ZeroMemory(&desc, sizeof(desc));
		desc.Architecture = ESSE::System::GetSystemArchitecture();
		desc.PhysicalCores = ESSE::System::GetProcessorCores(true);
		desc.VirtualCores = ESSE::System::GetProcessorCores(false);
		desc.ClockFrequency = ESSE::System::GetProcessorFrequency();
		desc.PhysicalMemory = ESSE::System::GetPhysicalMemory();
		ESSE::System::GetSystemVersion(&desc.SystemVersionMajor, &desc.SystemVersionMinor);
		ESSE::System::GetProcessorName(desc.ProcessorName, 0x80);
		return true;
	}
}