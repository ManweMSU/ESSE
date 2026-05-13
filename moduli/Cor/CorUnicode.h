#pragma once

#include "CorBasis.h"

namespace ESSE
{
	namespace Unicode
	{
		enum class NormalizationForm { C = 0, D = 1, KC = 2, KD = 3 };
		enum class CharacterClass { Letter = 0, Mark = 1, Number = 2, Punctuation = 3, Symbol = 4, Separator = 5, Control = 6 };

		unichar32 ConvertToLowerCase(unichar32 chr) noexcept;
		unichar32 ConvertToUpperCase(unichar32 chr) noexcept;

		void ConvertToLowerCase(const unichar8 * src, uintptr src_length, unichar8 ** presult, uintptr * presult_length, ErrorContext & ectx) noexcept;
		void ConvertToLowerCase(const unichar16 * src, uintptr src_length, unichar16 ** presult, uintptr * presult_length, ErrorContext & ectx) noexcept;
		void ConvertToLowerCase(const unichar32 * src, uintptr src_length, unichar32 ** presult, uintptr * presult_length, ErrorContext & ectx) noexcept;

		void ConvertToUpperCase(const unichar8 * src, uintptr src_length, unichar8 ** presult, uintptr * presult_length, ErrorContext & ectx) noexcept;
		void ConvertToUpperCase(const unichar16 * src, uintptr src_length, unichar16 ** presult, uintptr * presult_length, ErrorContext & ectx) noexcept;
		void ConvertToUpperCase(const unichar32 * src, uintptr src_length, unichar32 ** presult, uintptr * presult_length, ErrorContext & ectx) noexcept;

		int CaseInsensitiveCompare(const unichar8 * a, uintptr a_length, const unichar8 * b, uintptr b_length, ErrorContext & ectx) noexcept;
		int CaseInsensitiveCompare(const unichar16 * a, uintptr a_length, const unichar16 * b, uintptr b_length, ErrorContext & ectx) noexcept;
		int CaseInsensitiveCompare(const unichar32 * a, uintptr a_length, const unichar32 * b, uintptr b_length, ErrorContext & ectx) noexcept;

		void Normalize(const unichar8 * src, uintptr src_length, NormalizationForm form, unichar8 ** presult, uintptr * presult_length, ErrorContext & ectx) noexcept;
		void Normalize(const unichar16 * src, uintptr src_length, NormalizationForm form, unichar16 ** presult, uintptr * presult_length, ErrorContext & ectx) noexcept;
		void Normalize(const unichar32 * src, uintptr src_length, NormalizationForm form, unichar32 ** presult, uintptr * presult_length, ErrorContext & ectx) noexcept;

		bool BelongsToClass(unichar32 chr, CharacterClass cls) noexcept;
	}
}