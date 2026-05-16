#pragma once

#include "../CorBasis.h"
#include "../CorUnicode.h"
#include "../CorEncodingConverter.h"
#include "CorException.h"

namespace ESSE
{
	constexpr const unichar32 * DecimalBase = U"0123456789";
	constexpr const unichar32 * HexadecimalBase = U"0123456789ABCDEF";
	constexpr const unichar32 * HexadecimalBaseLowerCase = U"0123456789abcdef";
	constexpr const unichar32 * OctalBase = U"01234567";
	constexpr const unichar32 * BinaryBase = U"01";

	class ucs1_string;
	class ucs2_string;
	class ucs4_string;

	class ucs1_string
	{
		handle _hmem;
	public:
		ucs1_string(void) noexcept;
		ucs1_string(const ucs1_string & src);
		ucs1_string(const ucs2_string & src);
		ucs1_string(const ucs4_string & src);
		ucs1_string(ucs1_string && src) noexcept;
		ucs1_string(const unichar8 * src);
		ucs1_string(const unichar16 * src);
		ucs1_string(const unichar32 * src);
		ucs1_string(const unichar8 * src, uintptr length);
		ucs1_string(const unichar16 * src, uintptr length);
		ucs1_string(const unichar32 * src, uintptr length);
		ucs1_string(const void * pdata, intptr data_length, Unicode::Encoding enc);
		ucs1_string(const void * pdata, intptr data_length, const Unicode::DecodingCodepage & cp);
		~ucs1_string(void);

		ucs1_string & operator = (const ucs1_string & src);
		ucs1_string & operator = (const unichar8 * src);
		ucs1_string & operator = (ucs1_string && src) noexcept;
		operator const unichar8 * (void) const noexcept;
		uintptr GetLength(void) const noexcept;
		const unichar8 * GetData(void) const noexcept;

		bool friend operator == (const ucs1_string & a, const ucs1_string & b) noexcept;
		bool friend operator == (const unichar8 * a, const ucs1_string & b) noexcept;
		bool friend operator == (const ucs1_string & a, const unichar8 * b) noexcept;
		bool friend operator != (const ucs1_string & a, const ucs1_string & b) noexcept;
		bool friend operator != (const unichar8 * a, const ucs1_string & b) noexcept;
		bool friend operator != (const ucs1_string & a, const unichar8 * b) noexcept;

		bool friend operator <= (const ucs1_string & a, const ucs1_string & b) noexcept;
		bool friend operator >= (const ucs1_string & a, const ucs1_string & b) noexcept;
		bool friend operator < (const ucs1_string & a, const ucs1_string & b) noexcept;
		bool friend operator > (const ucs1_string & a, const ucs1_string & b) noexcept;

		static int Compare(const ucs1_string & a, const ucs1_string & b) noexcept;
		static int CompareCaseInsensitively(const ucs1_string & a, const ucs1_string & b);

		unichar8 operator [] (uintptr index) const noexcept;
		unichar8 GetCharacterAt(uintptr index) const noexcept;

		static ucs1_string Concatenate(const ucs1_string ** strings, uintptr count);
		static ucs1_string Concatenate(const unichar8 ** strings, uintptr * lengths, uintptr count);

		ucs1_string friend operator + (const ucs1_string & a, const ucs1_string & b);
		ucs1_string friend operator + (const unichar8 * a, const ucs1_string & b);
		ucs1_string friend operator + (const ucs1_string & a, const unichar8 * b);
		ucs1_string & operator += (const ucs1_string & str);

		uintptr GetUnitLengthInEncoding(Unicode::Encoding enc, bool include_terminator) const noexcept;
		uintptr GetByteLengthInEncoding(Unicode::Encoding enc, bool include_terminator) const noexcept;
		uintptr GetUnitLengthInEncoding(const Unicode::EncodingCodepage & enc_cp, bool include_terminator) const noexcept;
		uintptr GetByteLengthInEncoding(const Unicode::EncodingCodepage & enc_cp, bool include_terminator) const noexcept;

		void Encode(void * to, uintptr maxlen, Unicode::Encoding enc, bool include_terminator) const;
		void Encode(void * to, uintptr maxlen, const Unicode::EncodingCodepage & enc_cp, bool include_terminator) const;
	};
	class ucs2_string
	{
		handle _hmem;
	public:
		ucs2_string(void) noexcept;
		ucs2_string(const ucs1_string & src);
		ucs2_string(const ucs2_string & src);
		ucs2_string(const ucs4_string & src);
		ucs2_string(ucs2_string && src) noexcept;
		ucs2_string(const unichar8 * src);
		ucs2_string(const unichar16 * src);
		ucs2_string(const unichar32 * src);
		ucs2_string(const unichar8 * src, uintptr length);
		ucs2_string(const unichar16 * src, uintptr length);
		ucs2_string(const unichar32 * src, uintptr length);
		ucs2_string(const void * pdata, intptr data_length, Unicode::Encoding enc);
		ucs2_string(const void * pdata, intptr data_length, const Unicode::DecodingCodepage & cp);
		~ucs2_string(void);

		ucs2_string & operator = (const ucs2_string & src);
		ucs2_string & operator = (const unichar16 * src);
		ucs2_string & operator = (ucs2_string && src) noexcept;
		operator const unichar16 * (void) const noexcept;
		uintptr GetLength(void) const noexcept;
		const unichar16 * GetData(void) const noexcept;

		bool friend operator == (const ucs2_string & a, const ucs2_string & b) noexcept;
		bool friend operator == (const unichar16 * a, const ucs2_string & b) noexcept;
		bool friend operator == (const ucs2_string & a, const unichar16 * b) noexcept;
		bool friend operator != (const ucs2_string & a, const ucs2_string & b) noexcept;
		bool friend operator != (const unichar16 * a, const ucs2_string & b) noexcept;
		bool friend operator != (const ucs2_string & a, const unichar16 * b) noexcept;

		bool friend operator <= (const ucs2_string & a, const ucs2_string & b) noexcept;
		bool friend operator >= (const ucs2_string & a, const ucs2_string & b) noexcept;
		bool friend operator < (const ucs2_string & a, const ucs2_string & b) noexcept;
		bool friend operator > (const ucs2_string & a, const ucs2_string & b) noexcept;

		static int Compare(const ucs2_string & a, const ucs2_string & b) noexcept;
		static int CompareCaseInsensitively(const ucs2_string & a, const ucs2_string & b);

		unichar16 operator [] (uintptr index) const noexcept;
		unichar16 GetCharacterAt(uintptr index) const noexcept;

		static ucs2_string Concatenate(const ucs2_string ** strings, uintptr count);
		static ucs2_string Concatenate(const unichar16 ** strings, uintptr * lengths, uintptr count);

		ucs2_string friend operator + (const ucs2_string & a, const ucs2_string & b);
		ucs2_string friend operator + (const unichar16 * a, const ucs2_string & b);
		ucs2_string friend operator + (const ucs2_string & a, const unichar16 * b);
		ucs2_string & operator += (const ucs2_string & str);

		uintptr GetUnitLengthInEncoding(Unicode::Encoding enc, bool include_terminator) const noexcept;
		uintptr GetByteLengthInEncoding(Unicode::Encoding enc, bool include_terminator) const noexcept;
		uintptr GetUnitLengthInEncoding(const Unicode::EncodingCodepage & enc_cp, bool include_terminator) const noexcept;
		uintptr GetByteLengthInEncoding(const Unicode::EncodingCodepage & enc_cp, bool include_terminator) const noexcept;

		void Encode(void * to, uintptr maxlen, Unicode::Encoding enc, bool include_terminator) const;
		void Encode(void * to, uintptr maxlen, const Unicode::EncodingCodepage & enc_cp, bool include_terminator) const;
	};
	class ucs4_string
	{
		handle _hmem;
	public:
		ucs4_string(void) noexcept;
		ucs4_string(const ucs1_string & src);
		ucs4_string(const ucs2_string & src);
		ucs4_string(const ucs4_string & src);
		ucs4_string(ucs4_string && src) noexcept;
		ucs4_string(const unichar8 * src);
		ucs4_string(const unichar16 * src);
		ucs4_string(const unichar32 * src);
		ucs4_string(const unichar8 * src, uintptr length);
		ucs4_string(const unichar16 * src, uintptr length);
		ucs4_string(const unichar32 * src, uintptr length);
		ucs4_string(const void * pdata, intptr data_length, Unicode::Encoding enc);
		ucs4_string(const void * pdata, intptr data_length, const Unicode::DecodingCodepage & cp);
		ucs4_string(int8 value);
		ucs4_string(int16 value);
		ucs4_string(int32 value);
		ucs4_string(int64 value);
		ucs4_string(uint8 value, const unichar32 * digits = DecimalBase, int minimal_length = 0);
		ucs4_string(uint16 value, const unichar32 * digits = DecimalBase, int minimal_length = 0);
		ucs4_string(uint32 value, const unichar32 * digits = DecimalBase, int minimal_length = 0);
		ucs4_string(uint64 value, const unichar32 * digits = DecimalBase, int minimal_length = 0);
		ucs4_string(float value, unichar32 separator = U'.', int digits = 7);
		ucs4_string(double value, unichar32 separator = U'.', int digits = 16);
		ucs4_string(bool value);
		ucs4_string(unichar32 value);
		ucs4_string(unichar32 value, uintptr repeats);
		ucs4_string(const void * pointer);
		~ucs4_string(void);

		ucs4_string & operator = (const ucs4_string & src);
		ucs4_string & operator = (const unichar32 * src);
		ucs4_string & operator = (ucs4_string && src) noexcept;
		operator const unichar32 * (void) const noexcept;
		uintptr GetLength(void) const noexcept;
		const unichar32 * GetData(void) const noexcept;

		bool friend operator == (const ucs4_string & a, const ucs4_string & b) noexcept;
		bool friend operator == (const unichar32 * a, const ucs4_string & b) noexcept;
		bool friend operator == (const ucs4_string & a, const unichar32 * b) noexcept;
		bool friend operator != (const ucs4_string & a, const ucs4_string & b) noexcept;
		bool friend operator != (const unichar32 * a, const ucs4_string & b) noexcept;
		bool friend operator != (const ucs4_string & a, const unichar32 * b) noexcept;

		bool friend operator <= (const ucs4_string & a, const ucs4_string & b) noexcept;
		bool friend operator >= (const ucs4_string & a, const ucs4_string & b) noexcept;
		bool friend operator < (const ucs4_string & a, const ucs4_string & b) noexcept;
		bool friend operator > (const ucs4_string & a, const ucs4_string & b) noexcept;

		static int Compare(const ucs4_string & a, const ucs4_string & b) noexcept;
		static int CompareCaseInsensitively(const ucs4_string & a, const ucs4_string & b);

		unichar32 operator [] (uintptr index) const noexcept;
		unichar32 GetCharacterAt(uintptr index) const noexcept;

		static ucs4_string Concatenate(const ucs4_string ** strings, uintptr count);
		static ucs4_string Concatenate(const unichar32 ** strings, const uintptr * lengths, uintptr count);

		ucs4_string friend operator + (const ucs4_string & a, const ucs4_string & b);
		ucs4_string friend operator + (const unichar32 * a, const ucs4_string & b);
		ucs4_string friend operator + (const ucs4_string & a, const unichar32 * b);
		ucs4_string & operator += (const ucs4_string & str);

		intptr FindFirst(unichar32 chr, uintptr not_before = 0) const noexcept;
		intptr FindFirst(const ucs4_string & str, uintptr not_before = 0) const noexcept;
		intptr FindFirst(const unichar32 * str, uintptr len, uintptr not_before) const noexcept;
		intptr FindLast(unichar32 chr, uintptr not_after = intptr(-1)) const noexcept;
		intptr FindLast(const ucs4_string & str, uintptr not_after = intptr(-1)) const noexcept;
		intptr FindLast(const unichar32 * str, uintptr len, uintptr not_after) const noexcept;

		ucs4_string Substring(intptr from, intptr length) const;
		ucs4_string Replace(const unichar32 ** s, const uintptr * sl, const unichar32 ** w, const uintptr * wl, uintptr count) const;
		ucs4_string Replace(const ucs4_string ** substrings, const ucs4_string ** with, uintptr count) const;
		ucs4_string Replace(const ucs4_string & substring, const ucs4_string & with) const;
		ucs4_string Replace(unichar32 substring, const ucs4_string & with) const;
		ucs4_string Replace(const ucs4_string & substring, unichar32 with) const;
		ucs4_string Replace(unichar32 substring, unichar32 with) const;

		ucs4_string Uppercased(void) const;
		ucs4_string Lowercased(void) const;
		ucs4_string Normalize(Unicode::NormalizationForm nf = Unicode::NormalizationForm::C) const;

		uintptr GetUnitLengthInEncoding(Unicode::Encoding enc, bool include_terminator) const noexcept;
		uintptr GetByteLengthInEncoding(Unicode::Encoding enc, bool include_terminator) const noexcept;
		uintptr GetUnitLengthInEncoding(const Unicode::EncodingCodepage & enc_cp, bool include_terminator) const noexcept;
		uintptr GetByteLengthInEncoding(const Unicode::EncodingCodepage & enc_cp, bool include_terminator) const noexcept;

		void Encode(void * to, uintptr maxlen, Unicode::Encoding enc, bool include_terminator) const;
		void Encode(void * to, uintptr maxlen, const Unicode::EncodingCodepage & enc_cp, bool include_terminator) const;

		uint64 ToUInt64(void) const;
		uint64 ToUInt64(const unichar32 * digits, bool case_sensitive = false) const;
		int64 ToInt64(void) const;
		int64 ToInt64(const unichar32 * digits, bool case_sensitive = false) const;
		uint32 ToUInt32(void) const;
		uint32 ToUInt32(const unichar32 * digits, bool case_sensitive = false) const;
		int32 ToInt32(void) const;
		int32 ToInt32(const unichar32 * digits, bool case_sensitive = false) const;
		float ToFloat(void) const;
		float ToFloat(const unichar32 * separators) const;
		double ToDouble(void) const;
		double ToDouble(const unichar32 * separators) const;
		bool ToBoolean(void) const;
	};

	typedef ucs4_string string;

	string FormatString(const string & format, const string & a0);
	string FormatString(const string & format, const string & a0, const string & a1);
	string FormatString(const string & format, const string & a0, const string & a1, const string & a2);
	string FormatString(const string & format, const string & a0, const string & a1, const string & a2, const string & a3);
	string FormatString(const string & format, const string & a0, const string & a1, const string & a2, const string & a3, const string & a4);
	string FormatString(const string & format, const string & a0, const string & a1, const string & a2, const string & a3, const string & a4, const string & a5);
	string FormatString(const string & format, const string & a0, const string & a1, const string & a2, const string & a3, const string & a4, const string & a5, const string & a6);
	string FormatString(const string & format, const string & a0, const string & a1, const string & a2, const string & a3, const string & a4, const string & a5, const string & a6, const string & a7);
	string FormatString(const string & format, const string & a0, const string & a1, const string & a2, const string & a3, const string & a4, const string & a5, const string & a6, const string & a7, const string & a8);
	string FormatString(const string & format, const string & a0, const string & a1, const string & a2, const string & a3, const string & a4, const string & a5, const string & a6, const string & a7, const string & a8, const string & a9);
}