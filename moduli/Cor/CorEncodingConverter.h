#pragma once

#include "CorBasis.h"
#include "CorErrores.h"

namespace ESSE
{
	namespace Unicode
	{
		constexpr unichar32 CharacterByteOrderMark = 0xFEFF;
		constexpr unichar32 CharacterInvalid = 0xFFFFFFFF;

		enum class Encoding {
			Unknown = 0, ASCII = 1, UTF8 = 2,
			UTF16_LE = 3, UTF16_BE = 5, UTF16 = UTF16_LE,
			UTF32_LE = 4, UTF32_BE = 6, UTF32 = UTF32_LE
		};
		struct DecodingCodepage
		{
			uint32 codes_used;
			unichar32 charmap[0x100];
		};
		struct EncodingCodepage
		{
			uint32 codes_used, fallback;
			uint32 charmap[0x100]; // HI 24 - UCS code unit, LO 8 - byte code
		};

		void CreateReverseCodepage(EncodingCodepage & rev, const DecodingCodepage & src, uint8 fallback) noexcept;

		uint GetBytesPerCharacter(unichar32 chr, Encoding enc) noexcept;
		uint GetCodeUnitsPerCharacter(unichar32 chr, Encoding enc) noexcept;
		uint GetBytesPerCodeUnit(Encoding enc) noexcept;

		unichar32 ReadCharacter(const void * data, uintptr length, uintptr & position, Encoding enc, ErrorContext & ectx) noexcept;
		unichar32 ReadCharacter(const void * data, uintptr length, uintptr & position, const DecodingCodepage & cp, ErrorContext & ectx) noexcept;

		void WriteCharacter(void * data, uintptr length, uintptr & position, unichar32 chr, Encoding enc, ErrorContext & ectx) noexcept;
		void WriteCharacter(void * data, uintptr length, uintptr & position, unichar32 chr, const EncodingCodepage & rev_cp, ErrorContext & ectx) noexcept;
	}
}