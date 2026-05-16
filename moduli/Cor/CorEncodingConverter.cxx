#include "CorEncodingConverter.h"

namespace ESSE
{
	namespace Unicode
	{
		void CreateReverseCodepage(EncodingCodepage & rev, const DecodingCodepage & src, uint8 fallback) noexcept
		{
			rev.codes_used = src.codes_used;
			rev.fallback = fallback;
			for (uintptr i = 0; i < rev.codes_used; i++) rev.charmap[i] = (uint32(src.charmap[i]) << 8) | (i & 0xFF);
			qsort_r(&rev.charmap, rev.codes_used, sizeof(uint32), [](const void * a, const void * b, void * c) -> int{
				auto & ar = *reinterpret_cast<const uint32 *>(a);
				auto & br = *reinterpret_cast<const uint32 *>(b);
				if (ar > br) return 1; else if (ar < br) return -1; else return 0;
			}, 0);
		}
		uint GetBytesPerCharacter(unichar32 chr, Encoding enc) noexcept { return GetCodeUnitsPerCharacter(chr, enc) * GetBytesPerCodeUnit(enc); }
		uint GetCodeUnitsPerCharacter(unichar32 chr, Encoding enc) noexcept
		{
			if (enc == Encoding::UTF32_LE || enc == Encoding::UTF32_BE || enc == Encoding::ASCII) return 1;
			else if (enc == Encoding::UTF16_LE || enc == Encoding::UTF16_BE) { if (chr >= 0x10000) return 2; else return 1; }
			else if (enc == Encoding::UTF8) {
				if (chr >= 0x10000) return 4;
				else if (chr >= 0x800) return 3;
				else if (chr >= 0x80) return 2;
				else return 1;
			} else return 0;
		}
		uint GetBytesPerCodeUnit(Encoding enc) noexcept
		{
			if (enc == Encoding::UTF32_LE || enc == Encoding::UTF32_BE) return 4;
			else if (enc == Encoding::UTF16_LE || enc == Encoding::UTF16_BE) return 2;
			else if (enc == Encoding::ASCII || enc == Encoding::UTF8) return 1;
			else return 0;
		}
		unichar32 ReadCharacter(const void * data, uintptr length, uintptr & position, Encoding enc, ErrorContext & ectx) noexcept
		{
			if (enc == Encoding::UTF32_LE || enc == Encoding::UTF32_BE) {
				auto base = reinterpret_cast<const unichar32 *>(reinterpret_cast<const uint8 *>(data) + position);
				if (position + 3 >= length) { ErrorSet(ectx, position == length ? Errores::ErrorInvalidState : Errores::ErrorInvalidFormat); return 0; }
				auto chr = *base;
				if (enc == Encoding::UTF32_BE) chr = Memory::ReverseByteOrder(uint32(chr));
				if (chr >= 0x110000 || (chr >= 0xD800 && chr < 0xE000)) { ErrorSet(ectx, Errores::ErrorInvalidFormat); return 0; }
				position += 4;
				return chr;
			} else if (enc == Encoding::UTF16_LE || enc == Encoding::UTF16_BE) {
				auto base = reinterpret_cast<const unichar16 *>(reinterpret_cast<const uint8 *>(data) + position);
				if (position + 1 >= length) { ErrorSet(ectx, position == length ? Errores::ErrorInvalidState : Errores::ErrorInvalidFormat); return 0; }
				uint32 cp1 = base[0];
				if (enc == Encoding::UTF16_BE) cp1 = Memory::ReverseByteOrder(uint16(cp1));
				if (cp1 >= 0xD800 && cp1 < 0xE000) {
					if (position + 3 >= length) { ErrorSet(ectx, Errores::ErrorInvalidFormat); return 0; }
					uint32 cp2 = base[1];
					if (enc == Encoding::UTF16_BE) cp2 = Memory::ReverseByteOrder(uint16(cp2));
					if (cp1 >= 0xDC00 || cp2 < 0xDC00 || cp2 >= 0xE000) { ErrorSet(ectx, Errores::ErrorInvalidFormat); return 0; }
					position += 4;
					return ((cp1 - 0xD800) << 10) | (cp2 - 0xDC00) + 0x10000;
				} else {
					position += 2;
					return cp1;
				}
			} else if (enc == Encoding::UTF8) {
				auto base = reinterpret_cast<const uint8 *>(data) + position;
				if (position >= length) { ErrorSet(ectx, Errores::ErrorInvalidState); return 0; }
				uint32 cp1 = base[0];
				if (cp1 >= 0x80) {
					if ((cp1 & 0xE0) == 0xC0) {
						if (position + 1 >= length) { ErrorSet(ectx, Errores::ErrorInvalidFormat); return 0; }
						uint32 cp2 = base[1];
						if ((cp2 & 0xC0) != 0x80) { ErrorSet(ectx, Errores::ErrorInvalidFormat); return 0; }
						position += 2;
						return ((cp1 & 0x1F) << 6) | (cp2 & 0x3F);
					} else if ((cp1 & 0xF0) == 0xE0) {
						if (position + 2 >= length) { ErrorSet(ectx, Errores::ErrorInvalidFormat); return 0; }
						uint32 cp2 = base[1];
						uint32 cp3 = base[2];
						if ((cp2 & 0xC0) != 0x80 || (cp3 & 0xC0) != 0x80) { ErrorSet(ectx, Errores::ErrorInvalidFormat); return 0; }
						position += 3;
						return ((cp1 & 0x0F) << 12) | ((cp2 & 0x3F) << 6) | (cp3 & 0x3F);
					} else if ((cp1 & 0xF8) == 0xF0) {
						if (position + 3 >= length) { ErrorSet(ectx, Errores::ErrorInvalidFormat); return 0; }
						uint32 cp2 = base[1];
						uint32 cp3 = base[2];
						uint32 cp4 = base[3];
						if ((cp2 & 0xC0) != 0x80 || (cp3 & 0xC0) != 0x80 || (cp4 & 0xC0) != 0x80) { ErrorSet(ectx, Errores::ErrorInvalidFormat); return 0; }
						position += 4;
						return ((cp1 & 0x07) << 18) | ((cp2 & 0x3F) << 12) | ((cp3 & 0x3F) << 6) | (cp4 & 0x3F);
					} else { ErrorSet(ectx, Errores::ErrorInvalidFormat); return 0; }
				} else {
					position++;
					return uint32(cp1);
				}
			} else if (enc == Encoding::ASCII) {
				auto base = reinterpret_cast<const uint8 *>(data) + position;
				if (position >= length) { ErrorSet(ectx, Errores::ErrorInvalidState); return 0; }
				auto chr = *base;
				if (chr >= 0x80) { ErrorSet(ectx, Errores::ErrorInvalidFormat); return 0; }
				position++;
				return chr;
			} else { ErrorSet(ectx, Errores::ErrorInvalidArgument); return 0; }
		}
		unichar32 ReadCharacter(const void * data, uintptr length, uintptr & position, const DecodingCodepage & cp, ErrorContext & ectx) noexcept
		{
			auto base = reinterpret_cast<const uint8 *>(data) + position;
			if (position >= length) { ErrorSet(ectx, Errores::ErrorInvalidState); return 0; }
			auto chr = *base;
			if (chr >= cp.codes_used || cp.charmap[chr] == CharacterInvalid) { ErrorSet(ectx, Errores::ErrorInvalidFormat); return 0; }
			position++;
			return cp.charmap[chr];
		}
		void WriteCharacter(void * data, uintptr length, uintptr & position, unichar32 chr, Encoding enc, ErrorContext & ectx) noexcept
		{
			if (position + GetBytesPerCharacter(chr, enc) > length) { ErrorSet(ectx, Errores::ErrorOutOfMemory); return; }
			if (enc == Encoding::UTF32_LE || enc == Encoding::UTF32_BE) {
				auto base = reinterpret_cast<unichar32 *>(reinterpret_cast<uint8 *>(data) + position);
				*base = (enc == Encoding::UTF32_BE ? Memory::ReverseByteOrder(uint32(chr)) : chr);
				position += 4;
			} else if (enc == Encoding::UTF16_LE || enc == Encoding::UTF16_BE) {
				auto base = reinterpret_cast<unichar16 *>(reinterpret_cast<uint8 *>(data) + position);
				if (chr >= 0x10000) {
					uint32 chr0 = chr - 0x10000;
					uint16 cp1 = ((chr0 >> 10) & 0x3FF) + 0xD800;
					uint16 cp2 = (chr0 & 0x3FF) + 0xDC00;
					if (enc == Encoding::UTF16_BE) { base[0] = Memory::ReverseByteOrder(cp1); base[1] = Memory::ReverseByteOrder(cp2); }
					else { base[0] = cp1; base[1] = cp2; }
					position += 4;
				} else {
					*base = (enc == Encoding::UTF16_BE ? Memory::ReverseByteOrder(uint16(chr)) : uint16(chr));
					position += 2;
				}
			} else if (enc == Encoding::UTF8) {
				auto base = reinterpret_cast<uint8 *>(data) + position;
				if (chr >= 0x10000) {
					base[0] = (uint8(chr >> 18) & 0x07) | 0xF0;
					base[1] = (uint8(chr >> 12) & 0x3F) | 0x80;
					base[2] = (uint8(chr >> 6) & 0x3F) | 0x80;
					base[3] = (uint8(chr) & 0x3F) | 0x80;
					position += 4;
				} else if (chr >= 0x800) {
					base[0] = (uint8(chr >> 12) & 0x0F) | 0xE0;
					base[1] = (uint8(chr >> 6) & 0x3F) | 0x80;
					base[2] = (uint8(chr) & 0x3F) | 0x80;
					position += 3;
				} else if (chr >= 0x80) {
					base[0] = (uint8(chr >> 6) & 0x1F) | 0xC0;
					base[1] = (uint8(chr) & 0x3F) | 0x80;
					position += 2;
				} else {
					*base = uint8(chr);
					position++;
				}
			} else if (enc == Encoding::ASCII) {
				auto base = reinterpret_cast<uint8 *>(data) + position;
				if (chr >= 0x80) *base = '?'; else *base = chr;
				position++;
			} else { ErrorSet(ectx, Errores::ErrorInvalidArgument); return; }
		}
		void WriteCharacter(void * data, uintptr length, uintptr & position, unichar32 chr, const EncodingCodepage & rev_cp, ErrorContext & ectx) noexcept
		{
			if (position >= length) { ErrorSet(ectx, Errores::ErrorOutOfMemory); return; }
			auto base = reinterpret_cast<uint8 *>(data) + position;
			if (!rev_cp.codes_used) {
				*base = rev_cp.fallback;
				position++;
			} else {
				uint imn = 0, imx = rev_cp.codes_used - 1;
				uint vmn = rev_cp.charmap[imn] >> 8, vmx = rev_cp.charmap[imx] >> 8;
				while (imn != imx) {
					if (imn + 1 == imx) {
						if (vmn == chr) { imx = imn; vmx = vmn; } else { imn = imx; vmn = vmx; }
					} else {
						uint imm = (imn + imx) >> 1;
						uint vmm = rev_cp.charmap[imm] >> 8;
						if (vmm > chr) { imx = imm - 1; vmx = rev_cp.charmap[imx] >> 8; }
						else if (vmm < chr) { imn = imm + 1; vmn = rev_cp.charmap[imn] >> 8; }
						else { imn = imx = imm; vmn = vmx = vmm; }
					}
				}
				if (vmn == chr) *base = rev_cp.charmap[imn] & 0xFF; else *base = rev_cp.fallback & 0xFF;
				position++;
			}
		}
	}
}