#include "CorString.h"

namespace ESSE
{
	handle ucs1_string_handle_allocate(uintptr length)
	{
		if (length) {
			handle result = reinterpret_cast<uint8 *>(malloc(sizeof(uintptr) + sizeof(unichar8) * (length + 1)));
			if (!result) throw OutOfMemoryException();
			*reinterpret_cast<uintptr *>(result) = length;
			return result;
		} else return 0;
	}
	void ucs1_string_handle_deallocate(handle data) noexcept { free(data); }
	void ucs1_string_handle_unpack(handle data, const unichar8 ** pchr, uintptr * plen) noexcept
	{
		if (data) {
			*pchr = reinterpret_cast<const unichar8 *>(reinterpret_cast<uint8 *>(data) + sizeof(uintptr));
			*plen = *reinterpret_cast<uintptr *>(data);
		} else { *pchr = ""; *plen = 0; }
	}
	void ucs1_string_handle_unpack_rw(handle data, unichar8 ** pchr, uintptr * plen) noexcept
	{
		if (data) {
			*pchr = reinterpret_cast<unichar8 *>(reinterpret_cast<uint8 *>(data) + sizeof(uintptr));
			*plen = *reinterpret_cast<uintptr *>(data);
		} else { *pchr = 0; *plen = 0; }
	}
	handle ucs2_string_handle_allocate(uintptr length)
	{
		if (length) {
			handle result = reinterpret_cast<uint8 *>(malloc(sizeof(uintptr) + sizeof(unichar16) * (length + 1)));
			if (!result) throw OutOfMemoryException();
			*reinterpret_cast<uintptr *>(result) = length;
			return result;
		} else return 0;
	}
	void ucs2_string_handle_deallocate(handle data) noexcept { free(data); }
	void ucs2_string_handle_unpack(handle data, const unichar16 ** pchr, uintptr * plen) noexcept
	{
		if (data) {
			*pchr = reinterpret_cast<const unichar16 *>(reinterpret_cast<uint8 *>(data) + sizeof(uintptr));
			*plen = *reinterpret_cast<uintptr *>(data);
		} else { *pchr = u""; *plen = 0; }
	}
	void ucs2_string_handle_unpack_rw(handle data, unichar16 ** pchr, uintptr * plen) noexcept
	{
		if (data) {
			*pchr = reinterpret_cast<unichar16 *>(reinterpret_cast<uint8 *>(data) + sizeof(uintptr));
			*plen = *reinterpret_cast<uintptr *>(data);
		} else { *pchr = 0; *plen = 0; }
	}
	handle ucs4_string_handle_allocate(uintptr length)
	{
		if (length) {
			handle result = reinterpret_cast<uint8 *>(malloc(sizeof(uintptr) + sizeof(unichar32) * (length + 1)));
			if (!result) throw OutOfMemoryException();
			*reinterpret_cast<uintptr *>(result) = length;
			return result;
		} else return 0;
	}
	void ucs4_string_handle_deallocate(handle data) noexcept { free(data); }
	void ucs4_string_handle_unpack(handle data, const unichar32 ** pchr, uintptr * plen) noexcept
	{
		if (data) {
			*pchr = reinterpret_cast<const unichar32 *>(reinterpret_cast<uint8 *>(data) + sizeof(uintptr));
			*plen = *reinterpret_cast<uintptr *>(data);
		} else { *pchr = U""; *plen = 0; }
	}
	void ucs4_string_handle_unpack_rw(handle data, unichar32 ** pchr, uintptr * plen) noexcept
	{
		if (data) {
			*pchr = reinterpret_cast<unichar32 *>(reinterpret_cast<uint8 *>(data) + sizeof(uintptr));
			*plen = *reinterpret_cast<uintptr *>(data);
		} else { *pchr = 0; *plen = 0; }
	}

	#define U1_UP(S, P) const unichar8 * P##_chr; uintptr P##_len; ucs1_string_handle_unpack(S, &P##_chr, &P##_len);
	#define U1_UP_RW(S, P) unichar8 * P##_chr; uintptr P##_len; ucs1_string_handle_unpack_rw(S, &P##_chr, &P##_len);
	#define U2_UP(S, P) const unichar16 * P##_chr; uintptr P##_len; ucs2_string_handle_unpack(S, &P##_chr, &P##_len);
	#define U2_UP_RW(S, P) unichar16 * P##_chr; uintptr P##_len; ucs2_string_handle_unpack_rw(S, &P##_chr, &P##_len);
	#define U4_UP(S, P) const unichar32 * P##_chr; uintptr P##_len; ucs4_string_handle_unpack(S, &P##_chr, &P##_len);
	#define U4_UP_RW(S, P) unichar32 * P##_chr; uintptr P##_len; ucs4_string_handle_unpack_rw(S, &P##_chr, &P##_len);

	uintptr ucs_measure_transcoded_units(const void * srcdata, uintptr srclen, Unicode::Encoding dest, Unicode::Encoding src)
	{
		ErrorContext ectx; ErrorClear(ectx);
		uintptr position = 0, length = 0;
		while (true) {
			auto chr = Unicode::ReadCharacter(srcdata, srclen, position, src, ectx);
			if (ectx.error_code == Errores::ErrorInvalidState) break;
			ErrorThrow(ectx);
			if (chr) length += Unicode::GetBytesPerCharacter(chr, dest); else break;
		}
		return length / Unicode::GetBytesPerCodeUnit(dest);
	}
	uintptr ucs_measure_transcoded_units(const void * srcdata, uintptr srclen, Unicode::Encoding dest, const Unicode::DecodingCodepage & cp)
	{
		ErrorContext ectx; ErrorClear(ectx);
		uintptr position = 0, length = 0;
		while (true) {
			auto chr = Unicode::ReadCharacter(srcdata, srclen, position, cp, ectx);
			if (ectx.error_code == Errores::ErrorInvalidState) break;
			ErrorThrow(ectx);
			if (chr) length += Unicode::GetBytesPerCharacter(chr, dest); else break;
		}
		return length / Unicode::GetBytesPerCodeUnit(dest);
	}
	void ucs_transcode(void * destdata, uintptr destlen, const void * srcdata, uintptr srclen, Unicode::Encoding dest, Unicode::Encoding src)
	{
		ErrorContext ectx; ErrorClear(ectx);
		uintptr rpos = 0, wpos = 0;
		while (true) {
			auto chr = Unicode::ReadCharacter(srcdata, srclen, rpos, src, ectx);
			if (ectx.error_code == Errores::ErrorInvalidState) { chr = 0; ErrorClear(ectx); }
			ErrorThrow(ectx);
			Unicode::WriteCharacter(destdata, destlen, wpos, chr, dest, ectx);
			ErrorThrow(ectx);
			if (!chr) break;
		}
	}
	void ucs_transcode(void * destdata, uintptr destlen, const void * srcdata, uintptr srclen, Unicode::Encoding dest, const Unicode::DecodingCodepage & cp)
	{
		ErrorContext ectx; ErrorClear(ectx);
		uintptr rpos = 0, wpos = 0;
		while (true) {
			auto chr = Unicode::ReadCharacter(srcdata, srclen, rpos, cp, ectx);
			if (ectx.error_code == Errores::ErrorInvalidState) { chr = 0; ErrorClear(ectx); }
			ErrorThrow(ectx);
			Unicode::WriteCharacter(destdata, destlen, wpos, chr, dest, ectx);
			ErrorThrow(ectx);
			if (!chr) break;
		}
	}

	ucs1_string::ucs1_string(void) noexcept : _hmem(0) {}
	ucs1_string::ucs1_string(const ucs1_string & src)
	{
		_hmem = ucs1_string_handle_allocate(src.GetLength());
		U1_UP_RW(_hmem, dest);
		if (dest_chr) Memory::MemoryCopy(dest_chr, src.GetData(), (dest_len + 1) * sizeof(*dest_chr));
	}
	ucs1_string::ucs1_string(const ucs2_string & src) : ucs1_string(src.GetData(), src.GetLength()) {}
	ucs1_string::ucs1_string(const ucs4_string & src) : ucs1_string(src.GetData(), src.GetLength()) {}
	ucs1_string::ucs1_string(ucs1_string && src) noexcept { _hmem = src._hmem; src._hmem = 0; }
	ucs1_string::ucs1_string(const unichar8 * src)
	{
		_hmem = ucs1_string_handle_allocate(Memory::StringLength(src));
		U1_UP_RW(_hmem, dest);
		if (dest_chr) Memory::MemoryCopy(dest_chr, src, (dest_len + 1) * sizeof(*dest_chr));
	}
	ucs1_string::ucs1_string(const unichar16 * src) : ucs1_string(src, Memory::StringLength(src)) {}
	ucs1_string::ucs1_string(const unichar32 * src) : ucs1_string(src, Memory::StringLength(src)) {}
	ucs1_string::ucs1_string(const unichar8 * src, uintptr length)
	{
		_hmem = ucs1_string_handle_allocate(length);
		U1_UP_RW(_hmem, dest);
		if (dest_chr) { Memory::MemoryCopy(dest_chr, src, dest_len * sizeof(*dest_chr)); dest_chr[dest_len] = 0; }
	}
	ucs1_string::ucs1_string(const unichar16 * src, uintptr length)
	{
		_hmem = ucs1_string_handle_allocate(ucs_measure_transcoded_units(src, length * sizeof(*src), Unicode::Encoding::UTF8, Unicode::Encoding::UTF16));
		U1_UP_RW(_hmem, dest);
		if (dest_chr) {
			try { ucs_transcode(dest_chr, (dest_len + 1) * sizeof(*dest_chr), src, length * sizeof(*src), Unicode::Encoding::UTF8, Unicode::Encoding::UTF16); }
			catch (...) { ucs1_string_handle_deallocate(_hmem); throw; }
		}
	}
	ucs1_string::ucs1_string(const unichar32 * src, uintptr length)
	{
		_hmem = ucs1_string_handle_allocate(ucs_measure_transcoded_units(src, length * sizeof(*src), Unicode::Encoding::UTF8, Unicode::Encoding::UTF32));
		U1_UP_RW(_hmem, dest);
		if (dest_chr) {
			try { ucs_transcode(dest_chr, (dest_len + 1) * sizeof(*dest_chr), src, length * sizeof(*src), Unicode::Encoding::UTF8, Unicode::Encoding::UTF32); }
			catch (...) { ucs1_string_handle_deallocate(_hmem); throw; }
		}
	}
	ucs1_string::ucs1_string(const void * pdata, intptr data_length, Unicode::Encoding enc)
	{
		_hmem = ucs1_string_handle_allocate(ucs_measure_transcoded_units(pdata, data_length, Unicode::Encoding::UTF8, enc));
		U1_UP_RW(_hmem, dest);
		if (dest_chr) {
			try { ucs_transcode(dest_chr, (dest_len + 1) * sizeof(*dest_chr), pdata, data_length, Unicode::Encoding::UTF8, enc); }
			catch (...) { ucs1_string_handle_deallocate(_hmem); throw; }
		}
	}
	ucs1_string::ucs1_string(const void * pdata, intptr data_length, const Unicode::DecodingCodepage & cp)
	{
		_hmem = ucs1_string_handle_allocate(ucs_measure_transcoded_units(pdata, data_length, Unicode::Encoding::UTF8, cp));
		U1_UP_RW(_hmem, dest);
		if (dest_chr) {
			try { ucs_transcode(dest_chr, (dest_len + 1) * sizeof(*dest_chr), pdata, data_length, Unicode::Encoding::UTF8, cp); }
			catch (...) { ucs1_string_handle_deallocate(_hmem); throw; }
		}
	}
	ucs1_string::~ucs1_string(void) { ucs1_string_handle_deallocate(_hmem); }
	ucs1_string & ucs1_string::operator = (const ucs1_string & src)
	{
		if (&src == this) return *this;
		auto hmem_new = ucs1_string_handle_allocate(src.GetLength());
		U1_UP_RW(hmem_new, dest);
		if (dest_chr) Memory::MemoryCopy(dest_chr, src.GetData(), (dest_len + 1) * sizeof(*dest_chr));
		ucs1_string_handle_deallocate(_hmem); _hmem = hmem_new;
		return *this;
	}
	ucs1_string & ucs1_string::operator = (const unichar8 * src)
	{
		if (src == GetData()) return *this;
		auto hmem_new = ucs1_string_handle_allocate(Memory::StringLength(src));
		U1_UP_RW(hmem_new, dest);
		if (dest_chr) Memory::MemoryCopy(dest_chr, src, (dest_len + 1) * sizeof(*dest_chr));
		ucs1_string_handle_deallocate(_hmem); _hmem = hmem_new;
		return *this;
	}
	ucs1_string & ucs1_string::operator = (ucs1_string && src) noexcept { ucs1_string_handle_deallocate(_hmem); _hmem = src._hmem; src._hmem = 0; return *this; }
	ucs1_string::operator const unichar8 * (void) const noexcept { U1_UP(_hmem, dest); return dest_chr; }
	uintptr ucs1_string::GetLength(void) const noexcept { U1_UP(_hmem, dest); return dest_len; }
	const unichar8 * ucs1_string::GetData(void) const noexcept { U1_UP(_hmem, dest); return dest_chr; }
	int ucs1_string::Compare(const ucs1_string & a, const ucs1_string & b) noexcept { return Memory::StringCompare(a, b); }
	int ucs1_string::CompareCaseInsensitively(const ucs1_string & a, const ucs1_string & b)
	{
		ErrorContext ectx; ErrorClear(ectx);
		auto result = Unicode::CaseInsensitiveCompare(a.GetData(), a.GetLength(), b.GetData(), b.GetLength(), ectx);
		ErrorThrow(ectx);
		return result;
	}
	unichar8 ucs1_string::operator [] (uintptr index) const noexcept { U1_UP(_hmem, dest); return dest_chr[index]; }
	unichar8 ucs1_string::GetCharacterAt(uintptr index) const noexcept { U1_UP(_hmem, dest); return dest_chr[index]; }
	ucs1_string ucs1_string::Concatenate(const ucs1_string ** strings, uintptr count)
	{
		uintptr length = 0;
		for (uintptr i = 0; i < count; i++) length += strings[i]->GetLength();
		ucs1_string result;
		result._hmem = ucs1_string_handle_allocate(length);
		U1_UP_RW(result._hmem, dest);
		if (dest_chr) {
			length = 0;
			for (uintptr i = 0; i < count; i++) {
				Memory::MemoryCopy(dest_chr + length, strings[i]->GetData(), sizeof(*dest_chr) * strings[i]->GetLength());
				length += strings[i]->GetLength();
			}
			dest_chr[length] = 0;
		}
		return result;
	}
	ucs1_string ucs1_string::Concatenate(const unichar8 ** strings, uintptr * lengths, uintptr count)
	{
		uintptr length = 0;
		for (uintptr i = 0; i < count; i++) length += lengths[i];
		ucs1_string result;
		result._hmem = ucs1_string_handle_allocate(length);
		U1_UP_RW(result._hmem, dest);
		if (dest_chr) {
			length = 0;
			for (uintptr i = 0; i < count; i++) {
				Memory::MemoryCopy(dest_chr + length, strings[i], sizeof(*dest_chr) * lengths[i]);
				length += lengths[i];
			}
			dest_chr[length] = 0;
		}
		return result;
	}
	ucs1_string & ucs1_string::operator += (const ucs1_string & str) { const ucs1_string * args[] = { this, &str }; return *this = Concatenate(args, 2); }
	uintptr ucs1_string::GetUnitLengthInEncoding(Unicode::Encoding enc, bool include_terminator) const noexcept { return ucs_measure_transcoded_units(GetData(), GetLength() * sizeof(*GetData()), enc, Unicode::Encoding::UTF8) + (include_terminator ? 1 : 0); }
	uintptr ucs1_string::GetByteLengthInEncoding(Unicode::Encoding enc, bool include_terminator) const noexcept { return GetUnitLengthInEncoding(enc, include_terminator) * Unicode::GetBytesPerCodeUnit(enc); }
	uintptr ucs1_string::GetUnitLengthInEncoding(const Unicode::EncodingCodepage & enc_cp, bool include_terminator) const noexcept { return ucs_measure_transcoded_units(GetData(), GetLength() * sizeof(*GetData()), Unicode::Encoding::ASCII, Unicode::Encoding::UTF8) + (include_terminator ? 1 : 0); }
	uintptr ucs1_string::GetByteLengthInEncoding(const Unicode::EncodingCodepage & enc_cp, bool include_terminator) const noexcept { return GetUnitLengthInEncoding(enc_cp, include_terminator); }
	void ucs1_string::Encode(void * to, uintptr maxlen, Unicode::Encoding enc, bool include_terminator) const
	{
		ErrorContext ectx; ErrorClear(ectx);
		uintptr srclen = (GetLength() + (include_terminator ? 1 : 0)) * sizeof(*GetData());
		uintptr rpos = 0, wpos = 0;
		while (true) {
			auto chr = Unicode::ReadCharacter(GetData(), srclen, rpos, Unicode::Encoding::UTF8, ectx);
			if (ectx.error_code == Errores::ErrorInvalidState) return;
			ErrorThrow(ectx);
			Unicode::WriteCharacter(to, maxlen, wpos, chr, enc, ectx);
			ErrorThrow(ectx);
		}
	}
	void ucs1_string::Encode(void * to, uintptr maxlen, const Unicode::EncodingCodepage & enc_cp, bool include_terminator) const
	{
		ErrorContext ectx; ErrorClear(ectx);
		uintptr srclen = (GetLength() + (include_terminator ? 1 : 0)) * sizeof(*GetData());
		uintptr rpos = 0, wpos = 0;
		while (true) {
			auto chr = Unicode::ReadCharacter(GetData(), srclen, rpos, Unicode::Encoding::UTF8, ectx);
			if (ectx.error_code == Errores::ErrorInvalidState) return;
			ErrorThrow(ectx);
			Unicode::WriteCharacter(to, maxlen, wpos, chr, enc_cp, ectx);
			ErrorThrow(ectx);
		}
	}

	ucs2_string::ucs2_string(void) noexcept : _hmem(0) {}
	ucs2_string::ucs2_string(const ucs1_string & src) : ucs2_string(src.GetData(), src.GetLength()) {}
	ucs2_string::ucs2_string(const ucs2_string & src)
	{
		_hmem = ucs2_string_handle_allocate(src.GetLength());
		U2_UP_RW(_hmem, dest);
		if (dest_chr) Memory::MemoryCopy(dest_chr, src.GetData(), (dest_len + 1) * sizeof(*dest_chr));
	}
	ucs2_string::ucs2_string(const ucs4_string & src) : ucs2_string(src.GetData(), src.GetLength()) {}
	ucs2_string::ucs2_string(ucs2_string && src) noexcept { _hmem = src._hmem; src._hmem = 0; }
	ucs2_string::ucs2_string(const unichar8 * src) : ucs2_string(src, Memory::StringLength(src)) {}
	ucs2_string::ucs2_string(const unichar16 * src)
	{
		_hmem = ucs2_string_handle_allocate(Memory::StringLength(src));
		U2_UP_RW(_hmem, dest);
		if (dest_chr) Memory::MemoryCopy(dest_chr, src, (dest_len + 1) * sizeof(*dest_chr));
	}
	ucs2_string::ucs2_string(const unichar32 * src) : ucs2_string(src, Memory::StringLength(src)) {}
	ucs2_string::ucs2_string(const unichar8 * src, uintptr length)
	{
		_hmem = ucs2_string_handle_allocate(ucs_measure_transcoded_units(src, length * sizeof(*src), Unicode::Encoding::UTF16, Unicode::Encoding::UTF8));
		U2_UP_RW(_hmem, dest);
		if (dest_chr) {
			try { ucs_transcode(dest_chr, (dest_len + 1) * sizeof(*dest_chr), src, length * sizeof(*src), Unicode::Encoding::UTF16, Unicode::Encoding::UTF8); }
			catch (...) { ucs2_string_handle_deallocate(_hmem); throw; }
		}
	}
	ucs2_string::ucs2_string(const unichar16 * src, uintptr length)
	{
		_hmem = ucs2_string_handle_allocate(length);
		U2_UP_RW(_hmem, dest);
		if (dest_chr) { Memory::MemoryCopy(dest_chr, src, dest_len * sizeof(*dest_chr)); dest_chr[dest_len] = 0; }
	}
	ucs2_string::ucs2_string(const unichar32 * src, uintptr length)
	{
		_hmem = ucs2_string_handle_allocate(ucs_measure_transcoded_units(src, length * sizeof(*src), Unicode::Encoding::UTF16, Unicode::Encoding::UTF32));
		U2_UP_RW(_hmem, dest);
		if (dest_chr) {
			try { ucs_transcode(dest_chr, (dest_len + 1) * sizeof(*dest_chr), src, length * sizeof(*src), Unicode::Encoding::UTF16, Unicode::Encoding::UTF32); }
			catch (...) { ucs2_string_handle_deallocate(_hmem); throw; }
		}
	}
	ucs2_string::ucs2_string(const void * pdata, intptr data_length, Unicode::Encoding enc)
	{
		_hmem = ucs2_string_handle_allocate(ucs_measure_transcoded_units(pdata, data_length, Unicode::Encoding::UTF16, enc));
		U2_UP_RW(_hmem, dest);
		if (dest_chr) {
			try { ucs_transcode(dest_chr, (dest_len + 1) * sizeof(*dest_chr), pdata, data_length, Unicode::Encoding::UTF16, enc); }
			catch (...) { ucs2_string_handle_deallocate(_hmem); throw; }
		}
	}
	ucs2_string::ucs2_string(const void * pdata, intptr data_length, const Unicode::DecodingCodepage & cp)
	{
		_hmem = ucs2_string_handle_allocate(ucs_measure_transcoded_units(pdata, data_length, Unicode::Encoding::UTF16, cp));
		U2_UP_RW(_hmem, dest);
		if (dest_chr) {
			try { ucs_transcode(dest_chr, (dest_len + 1) * sizeof(*dest_chr), pdata, data_length, Unicode::Encoding::UTF16, cp); }
			catch (...) { ucs2_string_handle_deallocate(_hmem); throw; }
		}
	}
	ucs2_string::~ucs2_string(void) { ucs2_string_handle_deallocate(_hmem); }
	ucs2_string & ucs2_string::operator = (const ucs2_string & src)
	{
		if (&src == this) return *this;
		auto hmem_new = ucs2_string_handle_allocate(src.GetLength());
		U2_UP_RW(hmem_new, dest);
		if (dest_chr) Memory::MemoryCopy(dest_chr, src.GetData(), (dest_len + 1) * sizeof(*dest_chr));
		ucs2_string_handle_deallocate(_hmem); _hmem = hmem_new;
		return *this;
	}
	ucs2_string & ucs2_string::operator = (const unichar16 * src)
	{
		if (src == GetData()) return *this;
		auto hmem_new = ucs2_string_handle_allocate(Memory::StringLength(src));
		U2_UP_RW(hmem_new, dest);
		if (dest_chr) Memory::MemoryCopy(dest_chr, src, (dest_len + 1) * sizeof(*dest_chr));
		ucs2_string_handle_deallocate(_hmem); _hmem = hmem_new;
		return *this;
	}
	ucs2_string & ucs2_string::operator = (ucs2_string && src) noexcept { ucs2_string_handle_deallocate(_hmem); _hmem = src._hmem; src._hmem = 0; return *this; }
	ucs2_string::operator const unichar16 * (void) const noexcept { U2_UP(_hmem, dest); return dest_chr; }
	uintptr ucs2_string::GetLength(void) const noexcept { U2_UP(_hmem, dest); return dest_len; }
	const unichar16 * ucs2_string::GetData(void) const noexcept { U2_UP(_hmem, dest); return dest_chr; }
	int ucs2_string::Compare(const ucs2_string & a, const ucs2_string & b) noexcept { return Memory::StringCompare(a, b); }
	int ucs2_string::CompareCaseInsensitively(const ucs2_string & a, const ucs2_string & b)
	{
		ErrorContext ectx; ErrorClear(ectx);
		auto result = Unicode::CaseInsensitiveCompare(a.GetData(), a.GetLength(), b.GetData(), b.GetLength(), ectx);
		ErrorThrow(ectx);
		return result;
	}
	unichar16 ucs2_string::operator [] (uintptr index) const noexcept { U2_UP(_hmem, dest); return dest_chr[index]; }
	unichar16 ucs2_string::GetCharacterAt(uintptr index) const noexcept { U2_UP(_hmem, dest); return dest_chr[index]; }
	ucs2_string ucs2_string::Concatenate(const ucs2_string ** strings, uintptr count)
	{
		uintptr length = 0;
		for (uintptr i = 0; i < count; i++) length += strings[i]->GetLength();
		ucs2_string result;
		result._hmem = ucs2_string_handle_allocate(length);
		U2_UP_RW(result._hmem, dest);
		if (dest_chr) {
			length = 0;
			for (uintptr i = 0; i < count; i++) {
				Memory::MemoryCopy(dest_chr + length, strings[i]->GetData(), sizeof(*dest_chr) * strings[i]->GetLength());
				length += strings[i]->GetLength();
			}
			dest_chr[length] = 0;
		}
		return result;
	}
	ucs2_string ucs2_string::Concatenate(const unichar16 ** strings, uintptr * lengths, uintptr count)
	{
		uintptr length = 0;
		for (uintptr i = 0; i < count; i++) length += lengths[i];
		ucs2_string result;
		result._hmem = ucs2_string_handle_allocate(length);
		U2_UP_RW(result._hmem, dest);
		if (dest_chr) {
			length = 0;
			for (uintptr i = 0; i < count; i++) {
				Memory::MemoryCopy(dest_chr + length, strings[i], sizeof(*dest_chr) * lengths[i]);
				length += lengths[i];
			}
			dest_chr[length] = 0;
		}
		return result;
	}
	ucs2_string & ucs2_string::operator += (const ucs2_string & str) { const ucs2_string * args[] = { this, &str }; return *this = Concatenate(args, 2); }
	uintptr ucs2_string::GetUnitLengthInEncoding(Unicode::Encoding enc, bool include_terminator) const noexcept { return ucs_measure_transcoded_units(GetData(), GetLength() * sizeof(*GetData()), enc, Unicode::Encoding::UTF16) + (include_terminator ? 1 : 0); }
	uintptr ucs2_string::GetByteLengthInEncoding(Unicode::Encoding enc, bool include_terminator) const noexcept { return GetUnitLengthInEncoding(enc, include_terminator) * Unicode::GetBytesPerCodeUnit(enc); }
	uintptr ucs2_string::GetUnitLengthInEncoding(const Unicode::EncodingCodepage & enc_cp, bool include_terminator) const noexcept { return ucs_measure_transcoded_units(GetData(), GetLength() * sizeof(*GetData()), Unicode::Encoding::ASCII, Unicode::Encoding::UTF16) + (include_terminator ? 1 : 0); }
	uintptr ucs2_string::GetByteLengthInEncoding(const Unicode::EncodingCodepage & enc_cp, bool include_terminator) const noexcept { return GetUnitLengthInEncoding(enc_cp, include_terminator); }
	void ucs2_string::Encode(void * to, uintptr maxlen, Unicode::Encoding enc, bool include_terminator) const
	{
		ErrorContext ectx; ErrorClear(ectx);
		uintptr srclen = (GetLength() + (include_terminator ? 1 : 0)) * sizeof(*GetData());
		uintptr rpos = 0, wpos = 0;
		while (true) {
			auto chr = Unicode::ReadCharacter(GetData(), srclen, rpos, Unicode::Encoding::UTF16, ectx);
			if (ectx.error_code == Errores::ErrorInvalidState) return;
			ErrorThrow(ectx);
			Unicode::WriteCharacter(to, maxlen, wpos, chr, enc, ectx);
			ErrorThrow(ectx);
		}
	}
	void ucs2_string::Encode(void * to, uintptr maxlen, const Unicode::EncodingCodepage & enc_cp, bool include_terminator) const
	{
		ErrorContext ectx; ErrorClear(ectx);
		uintptr srclen = (GetLength() + (include_terminator ? 1 : 0)) * sizeof(*GetData());
		uintptr rpos = 0, wpos = 0;
		while (true) {
			auto chr = Unicode::ReadCharacter(GetData(), srclen, rpos, Unicode::Encoding::UTF16, ectx);
			if (ectx.error_code == Errores::ErrorInvalidState) return;
			ErrorThrow(ectx);
			Unicode::WriteCharacter(to, maxlen, wpos, chr, enc_cp, ectx);
			ErrorThrow(ectx);
		}
	}

	ucs4_string::ucs4_string(void) noexcept : _hmem(0) {}
	ucs4_string::ucs4_string(const ucs1_string & src) : ucs4_string(src.GetData(), src.GetLength()) {}
	ucs4_string::ucs4_string(const ucs2_string & src) : ucs4_string(src.GetData(), src.GetLength()) {}
	ucs4_string::ucs4_string(const ucs4_string & src)
	{
		_hmem = ucs4_string_handle_allocate(src.GetLength());
		U4_UP_RW(_hmem, dest);
		if (dest_chr) Memory::MemoryCopy(dest_chr, src.GetData(), (dest_len + 1) * sizeof(*dest_chr));
	}
	ucs4_string::ucs4_string(ucs4_string && src) noexcept { _hmem = src._hmem; src._hmem = 0; }
	ucs4_string::ucs4_string(const unichar8 * src) : ucs4_string(src, Memory::StringLength(src)) {}
	ucs4_string::ucs4_string(const unichar16 * src) : ucs4_string(src, Memory::StringLength(src)) {}
	ucs4_string::ucs4_string(const unichar32 * src)
	{
		_hmem = ucs4_string_handle_allocate(Memory::StringLength(src));
		U4_UP_RW(_hmem, dest);
		if (dest_chr) Memory::MemoryCopy(dest_chr, src, (dest_len + 1) * sizeof(*dest_chr));
	}
	ucs4_string::ucs4_string(const unichar8 * src, uintptr length)
	{
		_hmem = ucs4_string_handle_allocate(ucs_measure_transcoded_units(src, length * sizeof(*src), Unicode::Encoding::UTF32, Unicode::Encoding::UTF8));
		U4_UP_RW(_hmem, dest);
		if (dest_chr) {
			try { ucs_transcode(dest_chr, (dest_len + 1) * sizeof(*dest_chr), src, length * sizeof(*src), Unicode::Encoding::UTF32, Unicode::Encoding::UTF8); }
			catch (...) { ucs4_string_handle_deallocate(_hmem); throw; }
		}
	}
	ucs4_string::ucs4_string(const unichar16 * src, uintptr length)
	{
		_hmem = ucs4_string_handle_allocate(ucs_measure_transcoded_units(src, length * sizeof(*src), Unicode::Encoding::UTF32, Unicode::Encoding::UTF16));
		U4_UP_RW(_hmem, dest);
		if (dest_chr) {
			try { ucs_transcode(dest_chr, (dest_len + 1) * sizeof(*dest_chr), src, length * sizeof(*src), Unicode::Encoding::UTF32, Unicode::Encoding::UTF16); }
			catch (...) { ucs4_string_handle_deallocate(_hmem); throw; }
		}
	}
	ucs4_string::ucs4_string(const unichar32 * src, uintptr length)
	{
		_hmem = ucs4_string_handle_allocate(length);
		U4_UP_RW(_hmem, dest);
		if (dest_chr) { Memory::MemoryCopy(dest_chr, src, dest_len * sizeof(*dest_chr)); dest_chr[dest_len] = 0; }
	}
	ucs4_string::ucs4_string(const void * pdata, intptr data_length, Unicode::Encoding enc)
	{
		_hmem = ucs4_string_handle_allocate(ucs_measure_transcoded_units(pdata, data_length, Unicode::Encoding::UTF32, enc));
		U4_UP_RW(_hmem, dest);
		if (dest_chr) {
			try { ucs_transcode(dest_chr, (dest_len + 1) * sizeof(*dest_chr), pdata, data_length, Unicode::Encoding::UTF32, enc); }
			catch (...) { ucs4_string_handle_deallocate(_hmem); throw; }
		}
	}
	ucs4_string::ucs4_string(const void * pdata, intptr data_length, const Unicode::DecodingCodepage & cp)
	{
		_hmem = ucs4_string_handle_allocate(ucs_measure_transcoded_units(pdata, data_length, Unicode::Encoding::UTF32, cp));
		U4_UP_RW(_hmem, dest);
		if (dest_chr) {
			try { ucs_transcode(dest_chr, (dest_len + 1) * sizeof(*dest_chr), pdata, data_length, Unicode::Encoding::UTF32, cp); }
			catch (...) { ucs4_string_handle_deallocate(_hmem); throw; }
		}
	}
	ucs4_string::~ucs4_string(void) { ucs4_string_handle_deallocate(_hmem); }
	ucs4_string & ucs4_string::operator = (const ucs4_string & src)
	{
		if (&src == this) return *this;
		auto hmem_new = ucs4_string_handle_allocate(src.GetLength());
		U4_UP_RW(hmem_new, dest);
		if (dest_chr) Memory::MemoryCopy(dest_chr, src.GetData(), (dest_len + 1) * sizeof(*dest_chr));
		ucs4_string_handle_deallocate(_hmem); _hmem = hmem_new;
		return *this;
	}
	ucs4_string & ucs4_string::operator = (const unichar32 * src)
	{
		if (src == GetData()) return *this;
		auto hmem_new = ucs4_string_handle_allocate(Memory::StringLength(src));
		U4_UP_RW(hmem_new, dest);
		if (dest_chr) Memory::MemoryCopy(dest_chr, src, (dest_len + 1) * sizeof(*dest_chr));
		ucs4_string_handle_deallocate(_hmem); _hmem = hmem_new;
		return *this;
	}
	ucs4_string & ucs4_string::operator = (ucs4_string && src) noexcept { ucs4_string_handle_deallocate(_hmem); _hmem = src._hmem; src._hmem = 0; return *this; }
	ucs4_string::operator const unichar32 * (void) const noexcept { U4_UP(_hmem, dest); return dest_chr; }
	uintptr ucs4_string::GetLength(void) const noexcept { U4_UP(_hmem, dest); return dest_len; }
	const unichar32 * ucs4_string::GetData(void) const noexcept { U4_UP(_hmem, dest); return dest_chr; }
	int ucs4_string::Compare(const ucs4_string & a, const ucs4_string & b) noexcept { return Memory::StringCompare(a, b); }
	int ucs4_string::CompareCaseInsensitively(const ucs4_string & a, const ucs4_string & b)
	{
		ErrorContext ectx; ErrorClear(ectx);
		auto result = Unicode::CaseInsensitiveCompare(a.GetData(), a.GetLength(), b.GetData(), b.GetLength(), ectx);
		ErrorThrow(ectx);
		return result;
	}
	unichar32 ucs4_string::operator [] (uintptr index) const noexcept { U4_UP(_hmem, dest); return dest_chr[index]; }
	unichar32 ucs4_string::GetCharacterAt(uintptr index) const noexcept { U4_UP(_hmem, dest); return dest_chr[index]; }
	ucs4_string ucs4_string::Concatenate(const ucs4_string ** strings, uintptr count)
	{
		uintptr length = 0;
		for (uintptr i = 0; i < count; i++) length += strings[i]->GetLength();
		ucs4_string result;
		result._hmem = ucs4_string_handle_allocate(length);
		U4_UP_RW(result._hmem, dest);
		if (dest_chr) {
			length = 0;
			for (uintptr i = 0; i < count; i++) {
				Memory::MemoryCopy(dest_chr + length, strings[i]->GetData(), sizeof(*dest_chr) * strings[i]->GetLength());
				length += strings[i]->GetLength();
			}
			dest_chr[length] = 0;
		}
		return result;
	}
	ucs4_string ucs4_string::Concatenate(const unichar32 ** strings, const uintptr * lengths, uintptr count)
	{
		uintptr length = 0;
		for (uintptr i = 0; i < count; i++) length += lengths[i];
		ucs4_string result;
		result._hmem = ucs4_string_handle_allocate(length);
		U4_UP_RW(result._hmem, dest);
		if (dest_chr) {
			length = 0;
			for (uintptr i = 0; i < count; i++) {
				Memory::MemoryCopy(dest_chr + length, strings[i], sizeof(*dest_chr) * lengths[i]);
				length += lengths[i];
			}
			dest_chr[length] = 0;
		}
		return result;
	}
	ucs4_string & ucs4_string::operator += (const ucs4_string & str) { const ucs4_string * args[] = { this, &str }; return *this = Concatenate(args, 2); }
	uintptr ucs4_string::GetUnitLengthInEncoding(Unicode::Encoding enc, bool include_terminator) const noexcept { return ucs_measure_transcoded_units(GetData(), GetLength() * sizeof(*GetData()), enc, Unicode::Encoding::UTF32) + (include_terminator ? 1 : 0); }
	uintptr ucs4_string::GetByteLengthInEncoding(Unicode::Encoding enc, bool include_terminator) const noexcept { return GetUnitLengthInEncoding(enc, include_terminator) * Unicode::GetBytesPerCodeUnit(enc); }
	uintptr ucs4_string::GetUnitLengthInEncoding(const Unicode::EncodingCodepage & enc_cp, bool include_terminator) const noexcept { return ucs_measure_transcoded_units(GetData(), GetLength() * sizeof(*GetData()), Unicode::Encoding::ASCII, Unicode::Encoding::UTF32) + (include_terminator ? 1 : 0); }
	uintptr ucs4_string::GetByteLengthInEncoding(const Unicode::EncodingCodepage & enc_cp, bool include_terminator) const noexcept { return GetUnitLengthInEncoding(enc_cp, include_terminator); }
	void ucs4_string::Encode(void * to, uintptr maxlen, Unicode::Encoding enc, bool include_terminator) const
	{
		ErrorContext ectx; ErrorClear(ectx);
		uintptr srclen = (GetLength() + (include_terminator ? 1 : 0)) * sizeof(*GetData());
		uintptr rpos = 0, wpos = 0;
		while (true) {
			auto chr = Unicode::ReadCharacter(GetData(), srclen, rpos, Unicode::Encoding::UTF32, ectx);
			if (ectx.error_code == Errores::ErrorInvalidState) return;
			ErrorThrow(ectx);
			Unicode::WriteCharacter(to, maxlen, wpos, chr, enc, ectx);
			ErrorThrow(ectx);
		}
	}
	void ucs4_string::Encode(void * to, uintptr maxlen, const Unicode::EncodingCodepage & enc_cp, bool include_terminator) const
	{
		ErrorContext ectx; ErrorClear(ectx);
		uintptr srclen = (GetLength() + (include_terminator ? 1 : 0)) * sizeof(*GetData());
		uintptr rpos = 0, wpos = 0;
		while (true) {
			auto chr = Unicode::ReadCharacter(GetData(), srclen, rpos, Unicode::Encoding::UTF32, ectx);
			if (ectx.error_code == Errores::ErrorInvalidState) return;
			ErrorThrow(ectx);
			Unicode::WriteCharacter(to, maxlen, wpos, chr, enc_cp, ectx);
			ErrorThrow(ectx);
		}
	}

	bool operator == (const ucs1_string & a, const ucs1_string & b) noexcept { return a.Compare(a, b) == 0; }
	bool operator == (const unichar8 * a, const ucs1_string & b) noexcept { return b.Compare(a, b) == 0; }
	bool operator == (const ucs1_string & a, const unichar8 * b) noexcept { return a.Compare(a, b) == 0; }
	bool operator != (const ucs1_string & a, const ucs1_string & b) noexcept { return a.Compare(a, b) != 0; }
	bool operator != (const unichar8 * a, const ucs1_string & b) noexcept { return b.Compare(a, b) != 0; }
	bool operator != (const ucs1_string & a, const unichar8 * b) noexcept { return a.Compare(a, b) != 0; }
	bool operator <= (const ucs1_string & a, const ucs1_string & b) noexcept { return a.Compare(a, b) <= 0; }
	bool operator >= (const ucs1_string & a, const ucs1_string & b) noexcept { return a.Compare(a, b) >= 0; }
	bool operator < (const ucs1_string & a, const ucs1_string & b) noexcept { return a.Compare(a, b) < 0; }
	bool operator > (const ucs1_string & a, const ucs1_string & b) noexcept { return a.Compare(a, b) > 0; }
	ucs1_string operator + (const ucs1_string & a, const ucs1_string & b)
	{
		const unichar8 * sa[] = { a.GetData(), b.GetData() };
		uintptr la[] = { a.GetLength(), b.GetLength() };
		return a.Concatenate(sa, la, 2);
	}
	ucs1_string operator + (const unichar8 * a, const ucs1_string & b)
	{
		const unichar8 * sa[] = { a, b.GetData() };
		uintptr la[] = { Memory::StringLength(a), b.GetLength() };
		return b.Concatenate(sa, la, 2);
	}
	ucs1_string operator + (const ucs1_string & a, const unichar8 * b)
	{
		const unichar8 * sa[] = { a.GetData(), b };
		uintptr la[] = { a.GetLength(), Memory::StringLength(b) };
		return a.Concatenate(sa, la, 2);
	}

	bool operator == (const ucs2_string & a, const ucs2_string & b) noexcept { return a.Compare(a, b) == 0; }
	bool operator == (const unichar16 * a, const ucs2_string & b) noexcept { return b.Compare(a, b) == 0; }
	bool operator == (const ucs2_string & a, const unichar16 * b) noexcept { return a.Compare(a, b) == 0; }
	bool operator != (const ucs2_string & a, const ucs2_string & b) noexcept { return a.Compare(a, b) != 0; }
	bool operator != (const unichar16 * a, const ucs2_string & b) noexcept { return b.Compare(a, b) != 0; }
	bool operator != (const ucs2_string & a, const unichar16 * b) noexcept { return a.Compare(a, b) != 0; }
	bool operator <= (const ucs2_string & a, const ucs2_string & b) noexcept { return a.Compare(a, b) <= 0; }
	bool operator >= (const ucs2_string & a, const ucs2_string & b) noexcept { return a.Compare(a, b) >= 0; }
	bool operator < (const ucs2_string & a, const ucs2_string & b) noexcept { return a.Compare(a, b) < 0; }
	bool operator > (const ucs2_string & a, const ucs2_string & b) noexcept { return a.Compare(a, b) > 0; }
	ucs2_string operator + (const ucs2_string & a, const ucs2_string & b)
	{
		const unichar16 * sa[] = { a.GetData(), b.GetData() };
		uintptr la[] = { a.GetLength(), b.GetLength() };
		return a.Concatenate(sa, la, 2);
	}
	ucs2_string operator + (const unichar16 * a, const ucs2_string & b)
	{
		const unichar16 * sa[] = { a, b.GetData() };
		uintptr la[] = { Memory::StringLength(a), b.GetLength() };
		return b.Concatenate(sa, la, 2);
	}
	ucs2_string operator + (const ucs2_string & a, const unichar16 * b)
	{
		const unichar16 * sa[] = { a.GetData(), b };
		uintptr la[] = { a.GetLength(), Memory::StringLength(b) };
		return a.Concatenate(sa, la, 2);
	}

	bool operator == (const ucs4_string & a, const ucs4_string & b) noexcept { return a.Compare(a, b) == 0; }
	bool operator == (const unichar32 * a, const ucs4_string & b) noexcept { return b.Compare(a, b) == 0; }
	bool operator == (const ucs4_string & a, const unichar32 * b) noexcept { return a.Compare(a, b) == 0; }
	bool operator != (const ucs4_string & a, const ucs4_string & b) noexcept { return a.Compare(a, b) != 0; }
	bool operator != (const unichar32 * a, const ucs4_string & b) noexcept { return b.Compare(a, b) != 0; }
	bool operator != (const ucs4_string & a, const unichar32 * b) noexcept { return a.Compare(a, b) != 0; }
	bool operator <= (const ucs4_string & a, const ucs4_string & b) noexcept { return a.Compare(a, b) <= 0; }
	bool operator >= (const ucs4_string & a, const ucs4_string & b) noexcept { return a.Compare(a, b) >= 0; }
	bool operator < (const ucs4_string & a, const ucs4_string & b) noexcept { return a.Compare(a, b) < 0; }
	bool operator > (const ucs4_string & a, const ucs4_string & b) noexcept { return a.Compare(a, b) > 0; }
	ucs4_string operator + (const ucs4_string & a, const ucs4_string & b)
	{
		const unichar32 * sa[] = { a.GetData(), b.GetData() };
		uintptr la[] = { a.GetLength(), b.GetLength() };
		return a.Concatenate(sa, la, 2);
	}
	ucs4_string operator + (const unichar32 * a, const ucs4_string & b)
	{
		const unichar32 * sa[] = { a, b.GetData() };
		uintptr la[] = { Memory::StringLength(a), b.GetLength() };
		return b.Concatenate(sa, la, 2);
	}
	ucs4_string operator + (const ucs4_string & a, const unichar32 * b)
	{
		const unichar32 * sa[] = { a.GetData(), b };
		uintptr la[] = { a.GetLength(), Memory::StringLength(b) };
		return a.Concatenate(sa, la, 2);
	}

	uint64 ucs4_convert_to_uint64(const unichar32 * data, uintptr length)
	{
		if (!length) throw InvalidFormatException();
		uint64 value = 0;
		for (uintptr i = 0; i < length; i++) {
			if (data[i] < U'0' || data[i] > U'9') throw InvalidFormatException();
			uint64 digit = data[i] - U'0';
			if (value >= 0x199999999999999A) throw InvalidFormatException();
			value *= 10;
			if (value > 0xFFFFFFFFFFFFFFFF - digit) throw InvalidFormatException();
			value += digit;
		}
		return value;
	}
	uint64 ucs4_convert_to_uint64(const unichar32 * data, uintptr length, const unichar32 * digits, bool case_sensitive)
	{
		if (!length) throw InvalidFormatException();
		uint64 value = 0;
		uintptr radix = Memory::StringLength(digits);
		if (radix < 2) throw InvalidArgumentException();
		uint64 max_prem = 0xFFFFFFFFFFFFFFFF / radix + 1;
		for (uintptr i = 0; i < length; i++) {
			intptr dn = -1;
			for (intptr j = 0; j < radix; j++) {
				if (case_sensitive) { if (digits[j] == data[i]) { dn = j; break; } }
				else { if (Unicode::ConvertToUpperCase(digits[j]) == Unicode::ConvertToUpperCase(data[i])) { dn = j; break; } }
			}
			if (dn == -1) throw InvalidFormatException();
			uint64 digit = dn;
			if (value >= max_prem) throw InvalidFormatException();
			value *= radix;
			if (value > 0xFFFFFFFFFFFFFFFF - digit) throw InvalidFormatException();
			value += digit;
		}
		return value;
	}
	template<class T> T ucs4_convert_to_float(const unichar32 * data, uintptr length, const unichar32 * fps)
	{
		bool negative = false;
		uintptr from = 0, nfps = Memory::StringLength(fps);
		if (data[0] == L'-') { from = 1; negative = true; } else if (data[0] == L'+') { from = 1; }
		uintptr pdel = from;
		while (pdel < length) {
			bool is_del = false;
			for (uintptr j = 0; j < nfps; j++) if (fps[j] == data[pdel]) { is_del = true; break; }
			if (is_del) break;
			pdel++;
		}
		T value = 0.0;
		for (uintptr i = from; i < pdel; i++) {
			if (data[i] < U'0' || data[i] > U'9') throw InvalidFormatException();
			uintptr digit = data[i] - U'0';
			value *= T(10.0);
			value += T(digit);
		}
		T e = 1.0;
		for (uintptr i = pdel + 1; i < length; i++) {
			if (data[i] < U'0' || data[i] > U'9') throw InvalidFormatException();
			uintptr digit = data[i] - U'0';
			e /= T(10.0);
			value += e * T(digit);
		}
		return negative ? -value : value;
	}
	struct ucs4_decimal { uintptr num_int_digits, num_digits; char * digits; };
	bool ucs4_decimal_allocate(ucs4_decimal & dec, uintptr num_digits) noexcept
	{
		if (num_digits) {
			dec.digits = reinterpret_cast<char *>(malloc(num_digits));
			if (!dec.digits) return false;
		} else dec.digits = 0;
		dec.num_digits = num_digits;
		dec.num_int_digits = 0;
		return true;
	}
	void ucs4_decimal_deallocate(ucs4_decimal & dec) noexcept { free(dec.digits); }
	bool ucs4_decimal_add(ucs4_decimal & dest, const ucs4_decimal & a, const ucs4_decimal & b) noexcept
	{
		uintptr a_frac = a.num_digits - a.num_int_digits, b_frac = b.num_digits - b.num_int_digits;
		uintptr d_int = a.num_int_digits > b.num_int_digits ? a.num_int_digits : b.num_int_digits;
		uintptr d_frac = a_frac > b_frac ? a_frac : b_frac;
		if (!ucs4_decimal_allocate(dest, 1 + d_int + d_frac)) return false;
		dest.num_int_digits = 1 + d_int;
		char carry = 0;
		for (intptr e = -intptr(d_frac); e < intptr(d_int); e++) {
			char da = a.num_int_digits - uintptr(e + 1) < a.num_digits ? a.digits[a.num_int_digits - uintptr(e + 1)] : 0;
			char db = b.num_int_digits - uintptr(e + 1) < b.num_digits ? b.digits[b.num_int_digits - uintptr(e + 1)] : 0;
			char s = da + db + carry;
			dest.digits[dest.num_int_digits - uintptr(e + 1)] = s % 10;
			carry = s / 10;
		}
		dest.digits[0] = carry;
		return true;
	}
	bool ucs4_decimal_divide_2(ucs4_decimal & dest, const ucs4_decimal & value) noexcept
	{
		if (value.num_digits != value.num_int_digits || (value.num_digits && (value.digits[value.num_digits - 1] & 1))) {
			if (!ucs4_decimal_allocate(dest, value.num_digits + 1)) return false;
		} else {
			if (!ucs4_decimal_allocate(dest, value.num_digits)) return false;
		}
		dest.num_int_digits = value.num_int_digits;
		char carry = 0;
		for (uintptr i = 0; i < dest.num_digits; i++) {
			char num = i < value.num_digits ? 10 * carry + value.digits[i] : 10 * carry;
			dest.digits[i] = num / 2;
			carry = num % 2;
		}
		return true;
	}
	bool ucs4_decimal_power_2(ucs4_decimal & dest, int power) noexcept
	{
		if (power > 0) {
			uintptr alloc = power / 2 + 5;
			if (!ucs4_decimal_allocate(dest, alloc)) return false;
			dest.num_int_digits = dest.num_digits;
			for (uintptr j = 0; j < alloc - 1; j++) dest.digits[j] = 0;
			dest.digits[alloc - 1] = 1;
			uintptr carry = 0, nsig = 1;
			for (int j = 0; j < power; j++) {
				carry = 0;
				for (uintptr i = alloc - 1; i >= alloc - nsig; i--) {
					char v = dest.digits[i] * 2 + carry;
					dest.digits[i] = v % 10;
					carry = v / 10;
				}
				if (carry) {
					dest.digits[alloc - nsig - 1] = carry;
					nsig++;
				}
			}
			uintptr dd = alloc - nsig;
			for (uintptr i = 0; i < nsig; i++) dest.digits[i] = dest.digits[i + dd];
			dest.num_digits = dest.num_int_digits = dest.num_digits - dd;
			return true;
		} else if (power < 0) {
			if (!ucs4_decimal_allocate(dest, 1)) return false;
			dest.digits[0] = 1;
			dest.num_int_digits = 1;
			for (int i = 0; i < -power; i++) {
				ucs4_decimal t;
				if (!ucs4_decimal_divide_2(t, dest)) { ucs4_decimal_deallocate(dest); return false; }
				ucs4_decimal_deallocate(dest); dest = t;
			}
			return true;
		} else {
			if (!ucs4_decimal_allocate(dest, 1)) return false;
			dest.digits[0] = 1;
			dest.num_int_digits = 1;
			return true;
		}
	}
	void ucs4_decimal_round(ucs4_decimal & dec, int precision) noexcept
	{
		uintptr fnz = 0;
		while (fnz < dec.num_digits && !dec.digits[fnz]) fnz++;
		uintptr ft = fnz;
		for (int i = 0; i < precision; i++) { ft++; if (ft >= dec.num_digits) break; }
		if (ft < dec.num_digits) {
			int carry = dec.digits[ft] >= 5 ? 1 : 0;
			for (uintptr i = ft; i > 0; i--) {
				int value = dec.digits[i - 1] + carry;
				dec.digits[i] = value % 10;
				carry = carry / 10;
			}
			dec.digits[0] = carry;
			dec.num_int_digits++;
			dec.num_digits = ft + 1;
			if (!carry) fnz++;
		}
		uintptr lnz = dec.num_digits - 1;
		while (lnz && !dec.digits[lnz]) lnz--;
		if (lnz < dec.num_int_digits) dec.num_digits = dec.num_int_digits; else dec.num_digits = lnz + 1;
		if (fnz >= dec.num_int_digits && dec.num_int_digits) fnz = dec.num_int_digits - 1;
		for (uintptr i = 0; i < dec.num_digits - fnz; i++) dec.digits[i] = dec.digits[i + fnz];
		dec.num_digits -= fnz; dec.num_int_digits -= fnz;
	}
	bool ucs4_decimal_to_handle_and_free(handle & hmem, ucs4_decimal & dec, bool neg, unichar32 sep) noexcept
	{
		uintptr length = dec.num_digits;
		if (neg) length++;
		if (dec.num_digits != dec.num_int_digits) length++;
		try { hmem = ucs4_string_handle_allocate(length); } catch (...) { ucs4_decimal_deallocate(dec); return false; }
		U4_UP_RW(hmem, dest);
		uintptr wpos = 0;
		if (neg) { dest_chr[wpos] = U'-'; wpos++; }
		for (uintptr i = 0; i < dec.num_int_digits; i++) { dest_chr[wpos] = U'0' + dec.digits[i]; wpos++; }
		if (dec.num_digits != dec.num_int_digits) {
			dest_chr[wpos] = sep; wpos++; 
			for (uintptr i = dec.num_int_digits; i < dec.num_digits; i++) { dest_chr[wpos] = U'0' + dec.digits[i]; wpos++; }
		}
		dest_chr[wpos] = 0;
		ucs4_decimal_deallocate(dec);
		return true;
	}

	ucs4_string::ucs4_string(int8 value) : ucs4_string(int64(value)) {}
	ucs4_string::ucs4_string(int16 value) : ucs4_string(int64(value)) {}
	ucs4_string::ucs4_string(int32 value) : ucs4_string(int64(value)) {}
	ucs4_string::ucs4_string(int64 value) : _hmem(0)
	{
		if (value == 0x8000000000000000) {
			*this = U"-9223372036854775808";
		} else {
			unichar32 buffer[0x20];
			bool neg = false;
			int wpos = 0;
			if (value < 0) { value = -value; neg = true; }
			do {
				auto res = value % 10;
				value /= 10;
				buffer[wpos] = U"0123456789"[res];
				wpos++;
			} while (value);
			if (neg) { buffer[wpos] = U'-'; wpos++; }
			_hmem = ucs4_string_handle_allocate(wpos);
			U4_UP_RW(_hmem, dest);
			if (dest_chr) {
				for (int i = 0; i < wpos; i++) dest_chr[i] = buffer[wpos - i - 1];
				dest_chr[wpos] = 0;
			}
		}
	}
	ucs4_string::ucs4_string(uint8 value, const unichar32 * digits, int minimal_length) : ucs4_string(uint64(value), digits, minimal_length) {}
	ucs4_string::ucs4_string(uint16 value, const unichar32 * digits, int minimal_length) : ucs4_string(uint64(value), digits, minimal_length) {}
	ucs4_string::ucs4_string(uint32 value, const unichar32 * digits, int minimal_length) : ucs4_string(uint64(value), digits, minimal_length) {}
	ucs4_string::ucs4_string(uint64 value, const unichar32 * digits, int minimal_length)
	{
		if (minimal_length < 0 || minimal_length > 0x40) throw InvalidArgumentException();
		auto radix = Memory::StringLength(digits);
		if (radix <= 1) throw InvalidArgumentException();
		unichar32 buffer[0x40];
		int wpos = 0;
		do {
			auto res = value % radix;
			value /= radix;
			buffer[wpos] = digits[res];
			wpos++;
		} while (value);
		while (wpos < minimal_length) { buffer[wpos] = digits[0]; wpos++; }
		_hmem = ucs4_string_handle_allocate(wpos);
		U4_UP_RW(_hmem, dest);
		if (dest_chr) {
			for (int i = 0; i < wpos; i++) dest_chr[i] = buffer[wpos - i - 1];
			dest_chr[wpos] = 0;
		}	
	}
	ucs4_string::ucs4_string(float value, unichar32 separator, int digits) : _hmem(0)
	{
		uint32 & v = reinterpret_cast<uint32 & >(value);
		bool negative = (v & 0x80000000) != 0;
		int exp = (v & 0x7F800000) >> 23;
		v &= 0x007FFFFF;
		if (exp == 0) {
			if (v == 0) *this = U"0"; else {
				ucs4_decimal power, base;
				if (!ucs4_decimal_power_2(power, -126)) throw OutOfMemoryException();
				if (!ucs4_decimal_allocate(base, 0)) { ucs4_decimal_deallocate(power); throw OutOfMemoryException(); }
				for (int i = 22; i >= 0; i--) {
					ucs4_decimal t;
					if (!ucs4_decimal_divide_2(t, power)) { ucs4_decimal_deallocate(power); ucs4_decimal_deallocate(base); throw OutOfMemoryException(); }
					ucs4_decimal_deallocate(power); power = t;
					if ((v >> i) & 1) {
						if (!ucs4_decimal_add(t, base, power)) { ucs4_decimal_deallocate(power); ucs4_decimal_deallocate(base); throw OutOfMemoryException(); }
						ucs4_decimal_deallocate(base); base = t;
					}
				}
				ucs4_decimal_deallocate(power);
				ucs4_decimal_round(base, digits);
				if (!ucs4_decimal_to_handle_and_free(_hmem, base, negative, separator)) throw OutOfMemoryException();
			}
		} else if (exp == 0xFF) {
			if (v == 0) *this = (negative) ? U"-\x221E" : U"+\x221E"; else *this = U"NaN";
		} else {
			exp -= 127;
			ucs4_decimal power, base;
			if (!ucs4_decimal_power_2(power, exp)) throw OutOfMemoryException();
			if (!ucs4_decimal_allocate(base, power.num_digits)) { ucs4_decimal_deallocate(power); throw OutOfMemoryException(); }
			base.num_int_digits = power.num_int_digits;
			Memory::MemoryCopy(base.digits, power.digits, power.num_digits);
			for (int i = 22; i >= 0; i--) {
				ucs4_decimal t;
				if (!ucs4_decimal_divide_2(t, power)) { ucs4_decimal_deallocate(power); ucs4_decimal_deallocate(base); throw OutOfMemoryException(); }
				ucs4_decimal_deallocate(power); power = t;
				if ((v >> i) & 1) {
					if (!ucs4_decimal_add(t, base, power)) { ucs4_decimal_deallocate(power); ucs4_decimal_deallocate(base); throw OutOfMemoryException(); }
					ucs4_decimal_deallocate(base); base = t;
				}
			}
			ucs4_decimal_deallocate(power);
			ucs4_decimal_round(base, digits);
			if (!ucs4_decimal_to_handle_and_free(_hmem, base, negative, separator)) throw OutOfMemoryException();
		}
	}
	ucs4_string::ucs4_string(double value, unichar32 separator, int digits) : _hmem(0)
	{
		uint64 & v = reinterpret_cast<uint64 &>(value);
		bool negative = (v & 0x8000000000000000) != 0;
		int exp = (v & 0x7FF0000000000000) >> 52;
		v &= 0x000FFFFFFFFFFFFF;
		if (exp == 0) {
			if (v == 0) *this = U"0"; else {
				ucs4_decimal power, base;
				if (!ucs4_decimal_power_2(power, -1022)) throw OutOfMemoryException();
				if (!ucs4_decimal_allocate(base, 0)) { ucs4_decimal_deallocate(power); throw OutOfMemoryException(); }
				for (int i = 51; i >= 0; i--) {
					ucs4_decimal t;
					if (!ucs4_decimal_divide_2(t, power)) { ucs4_decimal_deallocate(power); ucs4_decimal_deallocate(base); throw OutOfMemoryException(); }
					ucs4_decimal_deallocate(power); power = t;
					if ((v >> i) & 1) {
						if (!ucs4_decimal_add(t, base, power)) { ucs4_decimal_deallocate(power); ucs4_decimal_deallocate(base); throw OutOfMemoryException(); }
						ucs4_decimal_deallocate(base); base = t;
					}
				}
				ucs4_decimal_deallocate(power);
				ucs4_decimal_round(base, digits);
				if (!ucs4_decimal_to_handle_and_free(_hmem, base, negative, separator)) throw OutOfMemoryException();
			}
		} else if (exp == 0x7FF) {
			if (v == 0) *this = (negative) ? U"-\x221E" : U"+\x221E"; else *this = U"NaN";
		} else {
			exp -= 1023;
			ucs4_decimal power, base;
			if (!ucs4_decimal_power_2(power, exp)) throw OutOfMemoryException();
			if (!ucs4_decimal_allocate(base, power.num_digits)) { ucs4_decimal_deallocate(power); throw OutOfMemoryException(); }
			base.num_int_digits = power.num_int_digits;
			Memory::MemoryCopy(base.digits, power.digits, power.num_digits);
			for (int i = 51; i >= 0; i--) {
				ucs4_decimal t;
				if (!ucs4_decimal_divide_2(t, power)) { ucs4_decimal_deallocate(power); ucs4_decimal_deallocate(base); throw OutOfMemoryException(); }
				ucs4_decimal_deallocate(power); power = t;
				if ((v >> i) & 1) {
					if (!ucs4_decimal_add(t, base, power)) { ucs4_decimal_deallocate(power); ucs4_decimal_deallocate(base); throw OutOfMemoryException(); }
					ucs4_decimal_deallocate(base); base = t;
				}
			}
			ucs4_decimal_deallocate(power);
			ucs4_decimal_round(base, digits);
			if (!ucs4_decimal_to_handle_and_free(_hmem, base, negative, separator)) throw OutOfMemoryException();
		}
	}
	ucs4_string::ucs4_string(bool value) : _hmem(0) { if (value) *this = U"Sic"; else *this = U"Non"; }
	ucs4_string::ucs4_string(unichar32 value)
	{
		_hmem = ucs4_string_handle_allocate(1);
		U4_UP_RW(_hmem, dest);
		if (dest_chr) dest_chr[0] = value;
	}
	ucs4_string::ucs4_string(unichar32 value, uintptr repeats)
	{
		_hmem = ucs4_string_handle_allocate(repeats);
		U4_UP_RW(_hmem, dest);
		if (dest_chr) for (uintptr i = 0; i < repeats; i++) dest_chr[i] = value;
	}
	ucs4_string::ucs4_string(const void * pointer) : ucs4_string(reinterpret_cast<uintptr>(pointer), HexadecimalBase, 2 * sizeof(pointer)) {}
	intptr ucs4_string::FindFirst(unichar32 chr, uintptr not_before) const noexcept
	{
		U4_UP(_hmem, data);
		if (!data_len) return -1;
		for (uintptr i = not_before; i < data_len; i++) if (data_chr[i] == chr) return i;
		return -1;
	}
	intptr ucs4_string::FindFirst(const ucs4_string & str, uintptr not_before) const noexcept
	{
		U4_UP(_hmem, data);
		U4_UP(str._hmem, search);
		if (!search_len) return not_before <= data_len ? not_before : data_len;
		if (search_len > data_len) return -1;
		for (uintptr i = not_before; i <= data_len - search_len; i++) if (Memory::MemoryCompare(data_chr + i, search_chr, search_len * sizeof(*search_chr)) == 0) return i;
		return -1;
	}
	intptr ucs4_string::FindFirst(const unichar32 * str, uintptr len, uintptr not_before) const noexcept
	{
		U4_UP(_hmem, data);
		if (!len) return not_before <= data_len ? not_before : data_len;
		if (len > data_len) return -1;
		for (uintptr i = not_before; i <= data_len - len; i++) if (Memory::MemoryCompare(data_chr + i, str, len * sizeof(*str)) == 0) return i;
		return -1;
	}
	intptr ucs4_string::FindLast(unichar32 chr, uintptr not_after) const noexcept
	{
		U4_UP(_hmem, data);
		if (not_after < data_len) data_len = not_after + 1;
		if (!data_len) return -1;
		for (uintptr i = data_len - 1; i != intptr(-1); i--) if (data_chr[i] == chr) return i;
		return -1;
	}
	intptr ucs4_string::FindLast(const ucs4_string & str, uintptr not_after) const noexcept
	{
		U4_UP(_hmem, data);
		U4_UP(str._hmem, search);
		if (!search_len) return not_after <= data_len ? not_after : data_len;
		if (search_len > data_len) return -1;
		if (not_after <= data_len - search_len) data_len = not_after + search_len;
		for (uintptr i = data_len - search_len; i != intptr(-1); i--) if (Memory::MemoryCompare(data_chr + i, search_chr, search_len * sizeof(*search_chr)) == 0) return i;
		return -1;
	}
	intptr ucs4_string::FindLast(const unichar32 * str, uintptr len, uintptr not_after) const noexcept
	{
		U4_UP(_hmem, data);
		if (!len) return not_after <= data_len ? not_after : data_len;
		if (len > data_len) return -1;
		if (not_after <= data_len - len) data_len = not_after + len;
		for (uintptr i = data_len - len; i != intptr(-1); i--) if (Memory::MemoryCompare(data_chr + i, str, len * sizeof(*str)) == 0) return i;
		return -1;
	}
	ucs4_string ucs4_string::Substring(intptr from, intptr length) const
	{
		U4_UP(_hmem, data);
		if (from < 0) { if (length >= 0) length += from; from = 0; }
		if (uintptr(from) > data_len) return ucs4_string();
		if (length < 0 || data_len - uintptr(from) < uintptr(length)) length = data_len - from;
		return ucs4_string(GetData() + from, length);
	}
	ucs4_string ucs4_string::Replace(const unichar32 ** s, const uintptr * sl, const unichar32 ** w, const uintptr * wl, uintptr count) const
	{
		uintptr nr = 0, lastr = 0;
		while (true) {
			intptr f = -1; uintptr j = 0, l = 0;
			while (j < count) {
				auto ll = sl[j];
				if (!ll) throw InvalidArgumentException();
				auto lf = FindFirst(s[j], sl[j], lastr);
				if (lf >= 0 && (f < 0 || lf < f)) { f = lf; l = ll; }
				j++;
			}
			if (f >= 0) { nr++; lastr = f + l; } else break;
		}
		if (!nr) return *this;
		uintptr fda_len = 2 * nr + 1;
		uintptr * fda = reinterpret_cast<uintptr *>(malloc(sizeof(uintptr) * 2 * fda_len));
		if (!fda) throw OutOfMemoryException();
		const unichar32 ** pstr = reinterpret_cast<const unichar32 **>(fda);
		uintptr * plen = reinterpret_cast<uintptr *>(fda + fda_len);
		nr = lastr = 0;
		while (true) {
			intptr f = -1; uintptr j = 0, l = 0, oj = 0;
			while (j < count) {
				auto ll = sl[j];
				auto lf = FindFirst(s[j], sl[j], lastr);
				if (lf >= 0 && (f < 0 || lf < f)) { f = lf; l = ll; oj = j; }
				j++;
			}
			if (f >= 0) {
				pstr[2 * nr] = GetData() + lastr;
				plen[2 * nr] = f - lastr;
				pstr[2 * nr + 1] = w[oj];
				plen[2 * nr + 1] = wl[oj];
				nr++; lastr = f + l;
			} else break;
		}
		pstr[2 * nr] = GetData() + lastr;
		plen[2 * nr] = GetLength() - lastr;
		ucs4_string result;
		try { result = result.Concatenate(pstr, plen, fda_len); } catch (...) { free(fda); throw; }
		free(fda);
		return result;
	}
	ucs4_string ucs4_string::Replace(const ucs4_string ** substrings, const ucs4_string ** with, uintptr count) const
	{
		uintptr nr = 0, lastr = 0;
		while (true) {
			intptr f = -1; uintptr j = 0, l = 0;
			while (j < count) {
				auto ll = substrings[j]->GetLength();
				if (!ll) throw InvalidArgumentException();
				auto lf = FindFirst(*substrings[j], lastr);
				if (lf >= 0 && (f < 0 || lf < f)) { f = lf; l = ll; }
				j++;
			}
			if (f >= 0) { nr++; lastr = f + l; } else break;
		}
		if (!nr) return *this;
		uintptr fda_len = 2 * nr + 1;
		uintptr * fda = reinterpret_cast<uintptr *>(malloc(sizeof(uintptr) * 2 * fda_len));
		if (!fda) throw OutOfMemoryException();
		const unichar32 ** pstr = reinterpret_cast<const unichar32 **>(fda);
		uintptr * plen = reinterpret_cast<uintptr *>(fda + fda_len);
		nr = lastr = 0;
		while (true) {
			intptr f = -1; uintptr j = 0, l = 0, oj = 0;
			while (j < count) {
				auto ll = substrings[j]->GetLength();
				auto lf = FindFirst(*substrings[j], lastr);
				if (lf >= 0 && (f < 0 || lf < f)) { f = lf; l = ll; oj = j; }
				j++;
			}
			if (f >= 0) {
				pstr[2 * nr] = GetData() + lastr;
				plen[2 * nr] = f - lastr;
				pstr[2 * nr + 1] = with[oj]->GetData();
				plen[2 * nr + 1] = with[oj]->GetLength();
				nr++; lastr = f + l;
			} else break;
		}
		pstr[2 * nr] = GetData() + lastr;
		plen[2 * nr] = GetLength() - lastr;
		ucs4_string result;
		try { result = result.Concatenate(pstr, plen, fda_len); } catch (...) { free(fda); throw; }
		free(fda);
		return result;
	}
	ucs4_string ucs4_string::Replace(const ucs4_string & substring, const ucs4_string & with) const
	{
		uintptr nr = 0, lastr = 0, l = substring.GetLength();
		if (!l) throw InvalidArgumentException();
		while (true) {
			auto f = FindFirst(substring, lastr);
			if (f >= 0) { nr++; lastr = f + l; } else break;
		}
		if (!nr) return *this;
		uintptr fda_len = 2 * nr + 1;
		uintptr * fda = reinterpret_cast<uintptr *>(malloc(sizeof(uintptr) * 2 * fda_len));
		if (!fda) throw OutOfMemoryException();
		const unichar32 ** pstr = reinterpret_cast<const unichar32 **>(fda);
		uintptr * plen = reinterpret_cast<uintptr *>(fda + fda_len);
		nr = lastr = 0;
		while (true) {
			auto f = FindFirst(substring, lastr);
			if (f >= 0) {
				pstr[2 * nr] = GetData() + lastr;
				plen[2 * nr] = f - lastr;
				pstr[2 * nr + 1] = with.GetData();
				plen[2 * nr + 1] = with.GetLength();
				nr++; lastr = f + l;
			} else break;
		}
		pstr[2 * nr] = GetData() + lastr;
		plen[2 * nr] = GetLength() - lastr;
		ucs4_string result;
		try { result = result.Concatenate(pstr, plen, fda_len); } catch (...) { free(fda); throw; }
		free(fda);
		return result;
	}
	ucs4_string ucs4_string::Replace(unichar32 substring, const ucs4_string & with) const
	{
		uintptr nr = 0, lastr = 0, l = 1;
		if (!l) throw InvalidArgumentException();
		while (true) {
			auto f = FindFirst(substring, lastr);
			if (f >= 0) { nr++; lastr = f + l; } else break;
		}
		if (!nr) return *this;
		uintptr fda_len = 2 * nr + 1;
		uintptr * fda = reinterpret_cast<uintptr *>(malloc(sizeof(uintptr) * 2 * fda_len));
		if (!fda) throw OutOfMemoryException();
		const unichar32 ** pstr = reinterpret_cast<const unichar32 **>(fda);
		uintptr * plen = reinterpret_cast<uintptr *>(fda + fda_len);
		nr = lastr = 0;
		while (true) {
			auto f = FindFirst(substring, lastr);
			if (f >= 0) {
				pstr[2 * nr] = GetData() + lastr;
				plen[2 * nr] = f - lastr;
				pstr[2 * nr + 1] = with.GetData();
				plen[2 * nr + 1] = with.GetLength();
				nr++; lastr = f + l;
			} else break;
		}
		pstr[2 * nr] = GetData() + lastr;
		plen[2 * nr] = GetLength() - lastr;
		ucs4_string result;
		try { result = result.Concatenate(pstr, plen, fda_len); } catch (...) { free(fda); throw; }
		free(fda);
		return result;
	}
	ucs4_string ucs4_string::Replace(const ucs4_string & substring, unichar32 with) const
	{
		uintptr nr = 0, lastr = 0, l = substring.GetLength();
		if (!l) throw InvalidArgumentException();
		while (true) {
			auto f = FindFirst(substring, lastr);
			if (f >= 0) { nr++; lastr = f + l; } else break;
		}
		if (!nr) return *this;
		uintptr fda_len = 2 * nr + 1;
		uintptr * fda = reinterpret_cast<uintptr *>(malloc(sizeof(uintptr) * 2 * fda_len));
		if (!fda) throw OutOfMemoryException();
		const unichar32 ** pstr = reinterpret_cast<const unichar32 **>(fda);
		uintptr * plen = reinterpret_cast<uintptr *>(fda + fda_len);
		nr = lastr = 0;
		while (true) {
			auto f = FindFirst(substring, lastr);
			if (f >= 0) {
				pstr[2 * nr] = GetData() + lastr;
				plen[2 * nr] = f - lastr;
				pstr[2 * nr + 1] = &with;
				plen[2 * nr + 1] = 1;
				nr++; lastr = f + l;
			} else break;
		}
		pstr[2 * nr] = GetData() + lastr;
		plen[2 * nr] = GetLength() - lastr;
		ucs4_string result;
		try { result = result.Concatenate(pstr, plen, fda_len); } catch (...) { free(fda); throw; }
		free(fda);
		return result;
	}
	ucs4_string ucs4_string::Replace(unichar32 substring, unichar32 with) const
	{
		ucs4_string result;
		result._hmem = ucs4_string_handle_allocate(GetLength());
		U4_UP_RW(result._hmem, dest);
		U4_UP(_hmem, src);
		if (dest_chr) {
			for (uint i = 0; i < dest_len; i++) dest_chr[i] = src_chr[i] == substring ? with : src_chr[i];
			dest_chr[dest_len] = 0;
		}
		return result;
	}
	ucs4_string ucs4_string::Uppercased(void) const
	{
		ErrorContext ectx; ErrorClear(ectx);
		ucs4_string result;
		unichar32 * buffer;
		uintptr length;
		Unicode::ConvertToUpperCase(GetData(), GetLength(), &buffer, &length, ectx);
		ErrorThrow(ectx);
		try { result._hmem = ucs4_string_handle_allocate(length); } catch (...) { free(buffer); throw; }
		U4_UP_RW(result._hmem, dest);
		if (dest_chr) {
			Memory::MemoryCopy(dest_chr, buffer, dest_len * sizeof(*dest_chr));
			dest_chr[dest_len] = 0;
		}
		free(buffer);
		return result;
	}
	ucs4_string ucs4_string::Lowercased(void) const
	{
		ErrorContext ectx; ErrorClear(ectx);
		ucs4_string result;
		unichar32 * buffer;
		uintptr length;
		Unicode::ConvertToLowerCase(GetData(), GetLength(), &buffer, &length, ectx);
		ErrorThrow(ectx);
		try { result._hmem = ucs4_string_handle_allocate(length); } catch (...) { free(buffer); throw; }
		U4_UP_RW(result._hmem, dest);
		if (dest_chr) {
			Memory::MemoryCopy(dest_chr, buffer, dest_len * sizeof(*dest_chr));
			dest_chr[dest_len] = 0;
		}
		free(buffer);
		return result;
	}
	ucs4_string ucs4_string::Normalize(Unicode::NormalizationForm nf) const
	{
		ErrorContext ectx; ErrorClear(ectx);
		ucs4_string result;
		unichar32 * buffer;
		uintptr length;
		Unicode::Normalize(GetData(), GetLength(), nf, &buffer, &length, ectx);
		ErrorThrow(ectx);
		try { result._hmem = ucs4_string_handle_allocate(length); } catch (...) { free(buffer); throw; }
		U4_UP_RW(result._hmem, dest);
		if (dest_chr) {
			Memory::MemoryCopy(dest_chr, buffer, dest_len * sizeof(*dest_chr));
			dest_chr[dest_len] = 0;
		}
		free(buffer);
		return result;
	}
	uint64 ucs4_string::ToUInt64(void) const { return ucs4_convert_to_uint64(GetData(), GetLength()); }
	uint64 ucs4_string::ToUInt64(const unichar32 * digits, bool case_sensitive) const { return ucs4_convert_to_uint64(GetData(), GetLength(), digits, case_sensitive); }
	int64 ucs4_string::ToInt64(void) const
	{
		uintptr from = 0;
		bool negative = false;
		if (GetData()[0] == U'-') { from = 1; negative = true; } else if (GetData()[0] == U'+') from = 1;
		auto value = ucs4_convert_to_uint64(GetData() + from, GetLength() - from);
		if (negative) {
			if (value > 0x8000000000000000) throw InvalidFormatException();
			else return -int64(value);
		} else {
			if (value > 0x7FFFFFFFFFFFFFFF) throw InvalidFormatException();
			return value;
		}
	}
	int64 ucs4_string::ToInt64(const unichar32 * digits, bool case_sensitive) const
	{
		uintptr from = 0;
		bool negative = false;
		if (GetData()[0] == U'-') { from = 1; negative = true; } else if (GetData()[0] == U'+') from = 1;
		auto value = ucs4_convert_to_uint64(GetData() + from, GetLength() - from, digits, case_sensitive);
		if (negative) {
			if (value > 0x8000000000000000) throw InvalidFormatException();
			else return -int64(value);
		} else {
			if (value > 0x7FFFFFFFFFFFFFFF) throw InvalidFormatException();
			return value;
		}
	}
	uint32 ucs4_string::ToUInt32(void) const
	{
		auto value = ucs4_convert_to_uint64(GetData(), GetLength());
		if (value > 0xFFFFFFFF) throw InvalidFormatException();
		return value;
	}
	uint32 ucs4_string::ToUInt32(const unichar32 * digits, bool case_sensitive) const
	{
		auto value = ucs4_convert_to_uint64(GetData(), GetLength(), digits, case_sensitive);
		if (value > 0xFFFFFFFF) throw InvalidFormatException();
		return value;
	}
	int32 ucs4_string::ToInt32(void) const
	{
		uintptr from = 0;
		bool negative = false;
		if (GetData()[0] == U'-') { from = 1; negative = true; } else if (GetData()[0] == U'+') from = 1;
		auto value = ucs4_convert_to_uint64(GetData() + from, GetLength() - from);
		if (negative) {
			if (value > 0x80000000) throw InvalidFormatException();
			else return -int64(value);
		} else {
			if (value > 0x7FFFFFFF) throw InvalidFormatException();
			return value;
		}
	}
	int32 ucs4_string::ToInt32(const unichar32 * digits, bool case_sensitive) const
	{
		uintptr from = 0;
		bool negative = false;
		if (GetData()[0] == U'-') { from = 1; negative = true; } else if (GetData()[0] == U'+') from = 1;
		auto value = ucs4_convert_to_uint64(GetData() + from, GetLength() - from, digits, case_sensitive);
		if (negative) {
			if (value > 0x80000000) throw InvalidFormatException();
			else return -int64(value);
		} else {
			if (value > 0x7FFFFFFF) throw InvalidFormatException();
			return value;
		}
	}
	float ucs4_string::ToFloat(void) const { return ucs4_convert_to_float<float>(GetData(), GetLength(), U".,"); }
	float ucs4_string::ToFloat(const unichar32 * separators) const { return ucs4_convert_to_float<float>(GetData(), GetLength(), separators); }
	double ucs4_string::ToDouble(void) const { return ucs4_convert_to_float<double>(GetData(), GetLength(), U".,"); }
	double ucs4_string::ToDouble(const unichar32 * separators) const { return ucs4_convert_to_float<double>(GetData(), GetLength(), separators); }
	bool ucs4_string::ToBoolean(void) const
	{
		if (CompareCaseInsensitively(*this, L"sic") == 0 || CompareCaseInsensitively(*this, L"1") == 0) return true;
		else if (CompareCaseInsensitively(*this, L"non") == 0 || CompareCaseInsensitively(*this, L"0") == 0 || GetLength() == 0) return false;
		else throw InvalidFormatException();
	}

	string FormatString(const string & format, const string & a0)
	{
		const unichar32 * sa[] = { U"%%", U"%0" };
		uintptr sal[] = { 2, 2 };
		const unichar32 * ia[] = { U"%", a0.GetData() };
		uintptr ial[] = { 1, a0.GetLength() };
		return format.Replace(sa, sal, ia, ial, 2);
	}
	string FormatString(const string & format, const string & a0, const string & a1)
	{
		const unichar32 * sa[] = { U"%%", U"%0", U"%1" };
		uintptr sal[] = { 2, 2, 2 };
		const unichar32 * ia[] = { U"%", a0.GetData(), a1.GetData() };
		uintptr ial[] = { 1, a0.GetLength(), a1.GetLength() };
		return format.Replace(sa, sal, ia, ial, 3);
	}
	string FormatString(const string & format, const string & a0, const string & a1, const string & a2)
	{
		const unichar32 * sa[] = { U"%%", U"%0", U"%1", U"%2" };
		uintptr sal[] = { 2, 2, 2, 2 };
		const unichar32 * ia[] = { U"%", a0.GetData(), a1.GetData(), a2.GetData() };
		uintptr ial[] = { 1, a0.GetLength(), a1.GetLength(), a2.GetLength() };
		return format.Replace(sa, sal, ia, ial, 4);
	}
	string FormatString(const string & format, const string & a0, const string & a1, const string & a2, const string & a3)
	{
		const unichar32 * sa[] = { U"%%", U"%0", U"%1", U"%2", U"%3" };
		uintptr sal[] = { 2, 2, 2, 2, 2 };
		const unichar32 * ia[] = { U"%", a0.GetData(), a1.GetData(), a2.GetData(), a3.GetData() };
		uintptr ial[] = { 1, a0.GetLength(), a1.GetLength(), a2.GetLength(), a3.GetLength() };
		return format.Replace(sa, sal, ia, ial, 5);
	}
	string FormatString(const string & format, const string & a0, const string & a1, const string & a2, const string & a3, const string & a4)
	{
		const unichar32 * sa[] = { U"%%", U"%0", U"%1", U"%2", U"%3", U"%4" };
		uintptr sal[] = { 2, 2, 2, 2, 2, 2 };
		const unichar32 * ia[] = { U"%", a0.GetData(), a1.GetData(), a2.GetData(), a3.GetData(), a4.GetData() };
		uintptr ial[] = { 1, a0.GetLength(), a1.GetLength(), a2.GetLength(), a3.GetLength(), a4.GetLength() };
		return format.Replace(sa, sal, ia, ial, 6);
	}
	string FormatString(const string & format, const string & a0, const string & a1, const string & a2, const string & a3, const string & a4, const string & a5)
	{
		const unichar32 * sa[] = { U"%%", U"%0", U"%1", U"%2", U"%3", U"%4", U"%5" };
		uintptr sal[] = { 2, 2, 2, 2, 2, 2, 2 };
		const unichar32 * ia[] = { U"%", a0.GetData(), a1.GetData(), a2.GetData(), a3.GetData(), a4.GetData(), a5.GetData() };
		uintptr ial[] = { 1, a0.GetLength(), a1.GetLength(), a2.GetLength(), a3.GetLength(), a4.GetLength(), a5.GetLength() };
		return format.Replace(sa, sal, ia, ial, 7);
	}
	string FormatString(const string & format, const string & a0, const string & a1, const string & a2, const string & a3, const string & a4, const string & a5, const string & a6)
	{
		const unichar32 * sa[] = { U"%%", U"%0", U"%1", U"%2", U"%3", U"%4", U"%5", U"%6" };
		uintptr sal[] = { 2, 2, 2, 2, 2, 2, 2, 2 };
		const unichar32 * ia[] = { U"%", a0.GetData(), a1.GetData(), a2.GetData(), a3.GetData(), a4.GetData(), a5.GetData(), a6.GetData() };
		uintptr ial[] = { 1, a0.GetLength(), a1.GetLength(), a2.GetLength(), a3.GetLength(), a4.GetLength(), a5.GetLength(), a6.GetLength() };
		return format.Replace(sa, sal, ia, ial, 8);
	}
	string FormatString(const string & format, const string & a0, const string & a1, const string & a2, const string & a3, const string & a4, const string & a5, const string & a6, const string & a7)
	{
		const unichar32 * sa[] = { U"%%", U"%0", U"%1", U"%2", U"%3", U"%4", U"%5", U"%6", U"%7" };
		uintptr sal[] = { 2, 2, 2, 2, 2, 2, 2, 2, 2 };
		const unichar32 * ia[] = { U"%", a0.GetData(), a1.GetData(), a2.GetData(), a3.GetData(), a4.GetData(), a5.GetData(), a6.GetData(), a7.GetData() };
		uintptr ial[] = { 1, a0.GetLength(), a1.GetLength(), a2.GetLength(), a3.GetLength(), a4.GetLength(), a5.GetLength(), a6.GetLength(), a7.GetLength() };
		return format.Replace(sa, sal, ia, ial, 9);
	}
	string FormatString(const string & format, const string & a0, const string & a1, const string & a2, const string & a3, const string & a4, const string & a5, const string & a6, const string & a7, const string & a8)
	{
		const unichar32 * sa[] = { U"%%", U"%0", U"%1", U"%2", U"%3", U"%4", U"%5", U"%6", U"%7", U"%8" };
		uintptr sal[] = { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
		const unichar32 * ia[] = { U"%", a0.GetData(), a1.GetData(), a2.GetData(), a3.GetData(), a4.GetData(), a5.GetData(), a6.GetData(), a7.GetData(), a8.GetData() };
		uintptr ial[] = { 1, a0.GetLength(), a1.GetLength(), a2.GetLength(), a3.GetLength(), a4.GetLength(), a5.GetLength(), a6.GetLength(), a7.GetLength(), a8.GetLength() };
		return format.Replace(sa, sal, ia, ial, 10);
	}
	string FormatString(const string & format, const string & a0, const string & a1, const string & a2, const string & a3, const string & a4, const string & a5, const string & a6, const string & a7, const string & a8, const string & a9)
	{
		const unichar32 * sa[] = { U"%%", U"%0", U"%1", U"%2", U"%3", U"%4", U"%5", U"%6", U"%7", U"%8", U"%9" };
		uintptr sal[] = { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };
		const unichar32 * ia[] = { U"%", a0.GetData(), a1.GetData(), a2.GetData(), a3.GetData(), a4.GetData(), a5.GetData(), a6.GetData(), a7.GetData(), a8.GetData(), a9.GetData() };
		uintptr ial[] = { 1, a0.GetLength(), a1.GetLength(), a2.GetLength(), a3.GetLength(), a4.GetLength(), a5.GetLength(), a6.GetLength(), a7.GetLength(), a8.GetLength(), a9.GetLength() };
		return format.Replace(sa, sal, ia, ial, 11);
	}
}