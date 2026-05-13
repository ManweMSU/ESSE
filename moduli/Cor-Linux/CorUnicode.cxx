#include <Cor/CorUnicode.h>
#include <Cor/CorErrores.h>
#include "CorUnicodeEx.h"

#include <ctype.h>
#include <dlfcn.h>

namespace ESSE
{
	namespace Unicode
	{
		namespace libunistring
		{
			#define DEFINE_FUNCTION_POINTER(NAME, RV, ARGS) typedef RV (* func_##NAME)ARGS noexcept; func_##NAME NAME;
			#define DEFINE_FUNCTION_IMPORT(NAME) libunistring::NAME = reinterpret_cast<libunistring::func_##NAME>(dlsym(libunistring::library, #NAME));

			constexpr uint32 CATEGORY_L = 0x0000001F;
			constexpr uint32 CATEGORY_M = 0x000000E0;
			constexpr uint32 CATEGORY_N = 0x00000700;
			constexpr uint32 CATEGORY_P = 0x0003F800;
			constexpr uint32 CATEGORY_S = 0x003C0000;
			constexpr uint32 CATEGORY_Z = 0x01C00000;
			constexpr uint32 CATEGORY_C = 0x3E000000;

			handle library;
			handle uninorm_nfd, uninorm_nfc, uninorm_nfkd, uninorm_nfkc;

			DEFINE_FUNCTION_POINTER(uc_toupper, unichar32, (unichar32))
			DEFINE_FUNCTION_POINTER(uc_tolower, unichar32, (unichar32))
			DEFINE_FUNCTION_POINTER(uc_locale_language, const char *, (void))
			DEFINE_FUNCTION_POINTER(u8_toupper, unichar8 *, (const unichar8 *, uintptr, const char *, handle, unichar8 *, uintptr *))
			DEFINE_FUNCTION_POINTER(u16_toupper, unichar16 *, (const unichar16 *, uintptr, const char *, handle, unichar16 *, uintptr *))
			DEFINE_FUNCTION_POINTER(u32_toupper, unichar32 *, (const unichar32 *, uintptr, const char *, handle, unichar32 *, uintptr *))
			DEFINE_FUNCTION_POINTER(u8_tolower, unichar8 *, (const unichar8 *, uintptr, const char *, handle, unichar8 *, uintptr *))
			DEFINE_FUNCTION_POINTER(u16_tolower, unichar16 *, (const unichar16 *, uintptr, const char *, handle, unichar16 *, uintptr *))
			DEFINE_FUNCTION_POINTER(u32_tolower, unichar32 *, (const unichar32 *, uintptr, const char *, handle, unichar32 *, uintptr *))
			DEFINE_FUNCTION_POINTER(u8_casecmp, int, (const unichar8 *, uintptr, const unichar8 *, uintptr, const char *, handle, int *))
			DEFINE_FUNCTION_POINTER(u16_casecmp, int, (const unichar16 *, uintptr, const unichar16 *, uintptr, const char *, handle, int *))
			DEFINE_FUNCTION_POINTER(u32_casecmp, int, (const unichar32 *, uintptr, const unichar32 *, uintptr, const char *, handle, int *))
			DEFINE_FUNCTION_POINTER(u8_normalize, unichar8 *, (handle, const unichar8 *, uintptr, unichar8 *, uintptr *))
			DEFINE_FUNCTION_POINTER(u16_normalize, unichar16 *, (handle, const unichar16 *, uintptr, unichar16 *, uintptr *))
			DEFINE_FUNCTION_POINTER(u32_normalize, unichar32 *, (handle, const unichar32 *, uintptr, unichar32 *, uintptr *))
			DEFINE_FUNCTION_POINTER(uc_is_general_category_withtable, bool, (unichar32, uint32))
		};

		void Linux_UnicodeLibraryInitialize(void) noexcept
		{
			libunistring::library = dlopen("/usr/lib/libunistring.so", RTLD_NOW);
			if (libunistring::library) {
				DEFINE_FUNCTION_IMPORT(uc_toupper)
				DEFINE_FUNCTION_IMPORT(uc_tolower)
				DEFINE_FUNCTION_IMPORT(uc_locale_language)
				DEFINE_FUNCTION_IMPORT(u8_toupper)
				DEFINE_FUNCTION_IMPORT(u16_toupper)
				DEFINE_FUNCTION_IMPORT(u32_toupper)
				DEFINE_FUNCTION_IMPORT(u8_tolower)
				DEFINE_FUNCTION_IMPORT(u16_tolower)
				DEFINE_FUNCTION_IMPORT(u32_tolower)
				DEFINE_FUNCTION_IMPORT(u8_casecmp)
				DEFINE_FUNCTION_IMPORT(u16_casecmp)
				DEFINE_FUNCTION_IMPORT(u32_casecmp)
				DEFINE_FUNCTION_IMPORT(u8_normalize)
				DEFINE_FUNCTION_IMPORT(u16_normalize)
				DEFINE_FUNCTION_IMPORT(u32_normalize)
				DEFINE_FUNCTION_IMPORT(uc_is_general_category_withtable)
				libunistring::uninorm_nfd = dlsym(libunistring::library, "uninorm_nfd");
				libunistring::uninorm_nfc = dlsym(libunistring::library, "uninorm_nfc");
				libunistring::uninorm_nfkd = dlsym(libunistring::library, "uninorm_nfkd");
				libunistring::uninorm_nfkc = dlsym(libunistring::library, "uninorm_nfkc");
			}
		}
		unichar32 ConvertToLowerCase(unichar32 chr) noexcept { if (libunistring::uc_tolower) return libunistring::uc_tolower(chr); else return chr < 0x80 ? tolower(chr) : chr; }
		unichar32 ConvertToUpperCase(unichar32 chr) noexcept { if (libunistring::uc_toupper) return libunistring::uc_toupper(chr); else return chr < 0x80 ? toupper(chr) : chr; }
		void ConvertToLowerCase(const unichar8 * src, uintptr src_length, unichar8 ** presult, uintptr * presult_length, ErrorContext & ectx) noexcept
		{
			if (!src_length) { *presult = 0; *presult_length = 0; return; }
			if (libunistring::uc_locale_language && libunistring::u8_tolower) {
				auto mem = libunistring::u8_tolower(src, src_length, libunistring::uc_locale_language(), 0, 0, presult_length);
				if (!mem) { ErrorSet(ectx, Errores::ErrorOutOfMemory); return; }
				*presult = mem;
			} else {
				auto mem = reinterpret_cast<unichar8 *>(malloc(sizeof(unichar8) * src_length));
				if (!mem) { ErrorSet(ectx, Errores::ErrorOutOfMemory); return; }
				for (uintptr i = 0; i < src_length; i++) mem[i] = src[i] < 0x80 ? tolower(src[i]) : src[i];
				*presult_length = src_length;
				*presult = mem;
			}
		}
		void ConvertToLowerCase(const unichar16 * src, uintptr src_length, unichar16 ** presult, uintptr * presult_length, ErrorContext & ectx) noexcept
		{
			if (!src_length) { *presult = 0; *presult_length = 0; return; }
			if (libunistring::uc_locale_language && libunistring::u16_tolower) {
				auto mem = libunistring::u16_tolower(src, src_length, libunistring::uc_locale_language(), 0, 0, presult_length);
				if (!mem) { ErrorSet(ectx, Errores::ErrorOutOfMemory); return; }
				*presult = mem;
			} else {
				auto mem = reinterpret_cast<unichar16 *>(malloc(sizeof(unichar16) * src_length));
				if (!mem) { ErrorSet(ectx, Errores::ErrorOutOfMemory); return; }
				for (uintptr i = 0; i < src_length; i++) mem[i] = src[i] < 0x80 ? tolower(src[i]) : src[i];
				*presult_length = src_length;
				*presult = mem;
			}
		}
		void ConvertToLowerCase(const unichar32 * src, uintptr src_length, unichar32 ** presult, uintptr * presult_length, ErrorContext & ectx) noexcept
		{
			if (!src_length) { *presult = 0; *presult_length = 0; return; }
			if (libunistring::uc_locale_language && libunistring::u32_tolower) {
				auto mem = libunistring::u32_tolower(src, src_length, libunistring::uc_locale_language(), 0, 0, presult_length);
				if (!mem) { ErrorSet(ectx, Errores::ErrorOutOfMemory); return; }
				*presult = mem;
			} else {
				auto mem = reinterpret_cast<unichar32 *>(malloc(sizeof(unichar32) * src_length));
				if (!mem) { ErrorSet(ectx, Errores::ErrorOutOfMemory); return; }
				for (uintptr i = 0; i < src_length; i++) mem[i] = src[i] < 0x80 ? tolower(src[i]) : src[i];
				*presult_length = src_length;
				*presult = mem;
			}
		}
		void ConvertToUpperCase(const unichar8 * src, uintptr src_length, unichar8 ** presult, uintptr * presult_length, ErrorContext & ectx) noexcept
		{
			if (!src_length) { *presult = 0; *presult_length = 0; return; }
			if (libunistring::uc_locale_language && libunistring::u8_toupper) {
				auto mem = libunistring::u8_toupper(src, src_length, libunistring::uc_locale_language(), 0, 0, presult_length);
				if (!mem) { ErrorSet(ectx, Errores::ErrorOutOfMemory); return; }
				*presult = mem;
			} else {
				auto mem = reinterpret_cast<unichar8 *>(malloc(sizeof(unichar8) * src_length));
				if (!mem) { ErrorSet(ectx, Errores::ErrorOutOfMemory); return; }
				for (uintptr i = 0; i < src_length; i++) mem[i] = src[i] < 0x80 ? toupper(src[i]) : src[i];
				*presult_length = src_length;
				*presult = mem;
			}
		}
		void ConvertToUpperCase(const unichar16 * src, uintptr src_length, unichar16 ** presult, uintptr * presult_length, ErrorContext & ectx) noexcept
		{
			if (!src_length) { *presult = 0; *presult_length = 0; return; }
			if (libunistring::uc_locale_language && libunistring::u16_toupper) {
				auto mem = libunistring::u16_toupper(src, src_length, libunistring::uc_locale_language(), 0, 0, presult_length);
				if (!mem) { ErrorSet(ectx, Errores::ErrorOutOfMemory); return; }
				*presult = mem;
			} else {
				auto mem = reinterpret_cast<unichar16 *>(malloc(sizeof(unichar16) * src_length));
				if (!mem) { ErrorSet(ectx, Errores::ErrorOutOfMemory); return; }
				for (uintptr i = 0; i < src_length; i++) mem[i] = src[i] < 0x80 ? toupper(src[i]) : src[i];
				*presult_length = src_length;
				*presult = mem;
			}
		}
		void ConvertToUpperCase(const unichar32 * src, uintptr src_length, unichar32 ** presult, uintptr * presult_length, ErrorContext & ectx) noexcept
		{
			if (!src_length) { *presult = 0; *presult_length = 0; return; }
			if (libunistring::uc_locale_language && libunistring::u32_toupper) {
				auto mem = libunistring::u32_toupper(src, src_length, libunistring::uc_locale_language(), 0, 0, presult_length);
				if (!mem) { ErrorSet(ectx, Errores::ErrorOutOfMemory); return; }
				*presult = mem;
			} else {
				auto mem = reinterpret_cast<unichar32 *>(malloc(sizeof(unichar32) * src_length));
				if (!mem) { ErrorSet(ectx, Errores::ErrorOutOfMemory); return; }
				for (uintptr i = 0; i < src_length; i++) mem[i] = src[i] < 0x80 ? toupper(src[i]) : src[i];
				*presult_length = src_length;
				*presult = mem;
			}
		}
		int CaseInsensitiveCompare(const unichar8 * a, uintptr a_length, const unichar8 * b, uintptr b_length, ErrorContext & ectx) noexcept
		{
			if (libunistring::uc_locale_language && libunistring::u8_casecmp) {
				int result;
				if (libunistring::u8_casecmp(a, a_length, b, b_length, libunistring::uc_locale_language(), 0, &result) < 0) { ErrorSet(ectx, Errores::ErrorOutOfMemory); return 0; }
				return result;
			} else {
				auto com_length = a_length > b_length ? b_length : a_length;
				for (uintptr i = 0; i < com_length; i++) {
					unichar8 auc = a[i] < 0x80 ? toupper(a[i]) : a[i];
					unichar8 buc = b[i] < 0x80 ? toupper(b[i]) : b[i];
					if (auc < buc) return -1;
					else if (auc > buc) return 1;
				}
				if (a_length < b_length) return -1;
				else if (a_length > b_length) return 1;
				else return 0;
			}
		}
		int CaseInsensitiveCompare(const unichar16 * a, uintptr a_length, const unichar16 * b, uintptr b_length, ErrorContext & ectx) noexcept
		{
			if (libunistring::uc_locale_language && libunistring::u16_casecmp) {
				int result;
				if (libunistring::u16_casecmp(a, a_length, b, b_length, libunistring::uc_locale_language(), 0, &result) < 0) { ErrorSet(ectx, Errores::ErrorOutOfMemory); return 0; }
				return result;
			} else {
				auto com_length = a_length > b_length ? b_length : a_length;
				for (uintptr i = 0; i < com_length; i++) {
					unichar16 auc = a[i] < 0x80 ? toupper(a[i]) : a[i];
					unichar16 buc = b[i] < 0x80 ? toupper(b[i]) : b[i];
					if (auc < buc) return -1;
					else if (auc > buc) return 1;
				}
				if (a_length < b_length) return -1;
				else if (a_length > b_length) return 1;
				else return 0;
			}
		}
		int CaseInsensitiveCompare(const unichar32 * a, uintptr a_length, const unichar32 * b, uintptr b_length, ErrorContext & ectx) noexcept
		{
			if (libunistring::uc_locale_language && libunistring::u32_casecmp) {
				int result;
				if (libunistring::u32_casecmp(a, a_length, b, b_length, libunistring::uc_locale_language(), 0, &result) < 0) { ErrorSet(ectx, Errores::ErrorOutOfMemory); return 0; }
				return result;
			} else {
				auto com_length = a_length > b_length ? b_length : a_length;
				for (uintptr i = 0; i < com_length; i++) {
					unichar32 auc = a[i] < 0x80 ? toupper(a[i]) : a[i];
					unichar32 buc = b[i] < 0x80 ? toupper(b[i]) : b[i];
					if (auc < buc) return -1;
					else if (auc > buc) return 1;
				}
				if (a_length < b_length) return -1;
				else if (a_length > b_length) return 1;
				else return 0;
			}
		}
		void Normalize(const unichar8 * src, uintptr src_length, NormalizationForm form, unichar8 ** presult, uintptr * presult_length, ErrorContext & ectx) noexcept
		{
			if (!src_length) { *presult = 0; *presult_length = 0; return; }
			if (libunistring::u8_normalize) {
				handle nf = 0;
				if (form == NormalizationForm::C) nf = libunistring::uninorm_nfc;
				else if (form == NormalizationForm::D) nf = libunistring::uninorm_nfd;
				else if (form == NormalizationForm::KC) nf = libunistring::uninorm_nfkc;
				else if (form == NormalizationForm::KD) nf = libunistring::uninorm_nfkd;
				else { ErrorSet(ectx, Errores::ErrorInvalidArgument); return; }
				if (!nf) { ErrorSet(ectx, Errores::ErrorNotImplemented); return; }
				auto mem = libunistring::u8_normalize(nf, src, src_length, 0, presult_length);
				if (!mem) { ErrorSet(ectx, Errores::ErrorOutOfMemory); return; }
				*presult = mem;
			} else {
				auto mem = reinterpret_cast<unichar8 *>(malloc(sizeof(unichar8) * src_length));
				if (!mem) { ErrorSet(ectx, Errores::ErrorOutOfMemory); return; }
				for (uintptr i = 0; i < src_length; i++) mem[i] = src[i];
				*presult_length = src_length;
				*presult = mem;
			}
		}
		void Normalize(const unichar16 * src, uintptr src_length, NormalizationForm form, unichar16 ** presult, uintptr * presult_length, ErrorContext & ectx) noexcept
		{
			if (!src_length) { *presult = 0; *presult_length = 0; return; }
			if (libunistring::u16_normalize) {
				handle nf = 0;
				if (form == NormalizationForm::C) nf = libunistring::uninorm_nfc;
				else if (form == NormalizationForm::D) nf = libunistring::uninorm_nfd;
				else if (form == NormalizationForm::KC) nf = libunistring::uninorm_nfkc;
				else if (form == NormalizationForm::KD) nf = libunistring::uninorm_nfkd;
				else { ErrorSet(ectx, Errores::ErrorInvalidArgument); return; }
				if (!nf) { ErrorSet(ectx, Errores::ErrorNotImplemented); return; }
				auto mem = libunistring::u16_normalize(nf, src, src_length, 0, presult_length);
				if (!mem) { ErrorSet(ectx, Errores::ErrorOutOfMemory); return; }
				*presult = mem;
			} else {
				auto mem = reinterpret_cast<unichar16 *>(malloc(sizeof(unichar16) * src_length));
				if (!mem) { ErrorSet(ectx, Errores::ErrorOutOfMemory); return; }
				for (uintptr i = 0; i < src_length; i++) mem[i] = src[i];
				*presult_length = src_length;
				*presult = mem;
			}
		}
		void Normalize(const unichar32 * src, uintptr src_length, NormalizationForm form, unichar32 ** presult, uintptr * presult_length, ErrorContext & ectx) noexcept
		{
			if (!src_length) { *presult = 0; *presult_length = 0; return; }
			if (libunistring::u32_normalize) {
				handle nf = 0;
				if (form == NormalizationForm::C) nf = libunistring::uninorm_nfc;
				else if (form == NormalizationForm::D) nf = libunistring::uninorm_nfd;
				else if (form == NormalizationForm::KC) nf = libunistring::uninorm_nfkc;
				else if (form == NormalizationForm::KD) nf = libunistring::uninorm_nfkd;
				else { ErrorSet(ectx, Errores::ErrorInvalidArgument); return; }
				if (!nf) { ErrorSet(ectx, Errores::ErrorNotImplemented); return; }
				auto mem = libunistring::u32_normalize(nf, src, src_length, 0, presult_length);
				if (!mem) { ErrorSet(ectx, Errores::ErrorOutOfMemory); return; }
				*presult = mem;
			} else {
				auto mem = reinterpret_cast<unichar32 *>(malloc(sizeof(unichar32) * src_length));
				if (!mem) { ErrorSet(ectx, Errores::ErrorOutOfMemory); return; }
				for (uintptr i = 0; i < src_length; i++) mem[i] = src[i];
				*presult_length = src_length;
				*presult = mem;
			}
		}
		bool BelongsToClass(unichar32 chr, CharacterClass cls) noexcept
		{
			if (libunistring::uc_is_general_category_withtable) {
				uint32 mask;
				if (cls == CharacterClass::Letter) mask = libunistring::CATEGORY_L;
				else if (cls == CharacterClass::Mark) mask = libunistring::CATEGORY_M;
				else if (cls == CharacterClass::Number) mask = libunistring::CATEGORY_N;
				else if (cls == CharacterClass::Punctuation) mask = libunistring::CATEGORY_P;
				else if (cls == CharacterClass::Symbol) mask = libunistring::CATEGORY_S;
				else if (cls == CharacterClass::Separator) mask = libunistring::CATEGORY_Z;
				else if (cls == CharacterClass::Control) mask = libunistring::CATEGORY_C;
				else return false;
				return libunistring::uc_is_general_category_withtable(chr, mask);
			} else {
				if (chr < 0x80) {
					if (cls == CharacterClass::Letter) return isalpha(chr);
					else if (cls == CharacterClass::Number) return isdigit(chr);
					else if (cls == CharacterClass::Punctuation) return ispunct(chr);
					else if (cls == CharacterClass::Separator) return isspace(chr);
					else if (cls == CharacterClass::Control) return iscntrl(chr);
					else return false;
				} else return false;
			}
		}
	}
}