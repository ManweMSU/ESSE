#pragma once

#include <Cor/Classes/CorObject.h>

namespace ESSE
{
	namespace Formationes
	{
		enum class TokenClass : uint { EOS = 0, Word = 1, Punctuation = 2, LiteralInteger = 3, LiteralFloat = 4, LiteralString = 5 };
		struct Token
		{
			TokenClass cls;
			uintptr source_range_from, source_length;
			const unichar32 * content_ptr;
			union {
				uint64 content_i;
				double content_f;
			};
		};
		struct RangeDesc
		{
			string line;
			uintptr line_number, line_position_from, line_range_length;
		};
		class TokenStream : public Object
		{
			const unichar32 * _data;
			uintptr _position, _length;
		public:
			TokenStream(const unichar32 * data, uintptr length) noexcept;
			virtual ~TokenStream(void) override;
			bool ExtractRange(RangeDesc & desc, const Token & token_for) const noexcept;
			bool ReadToken(Token & token) noexcept;
			uintptr GetCurrentPosition(void) const noexcept;
		};

		bool TokenIsEOS(const Token & t) noexcept;
		bool TokenIsWord(const Token & t) noexcept;
		bool TokenIsWord(const Token & t, const unichar32 * word) noexcept;
		bool TokenIsPunctuation(const Token & t, const unichar32 * word) noexcept;
		bool TokenIsIntegerLiteral(const Token & t) noexcept;
		bool TokenIsFloatLiteral(const Token & t) noexcept;
		bool TokenIsStringLiteral(const Token & t) noexcept;
		uint64 GetTokenInteger(const Token & t) noexcept;
		double GetTokenFloat(const Token & t) noexcept;
		string GetTokenString(const Token & t);

		string CreateToken(uint64 from);
		string CreateToken(double from);
		string CreateToken(const string & from, Unicode::Encoding enc);
	}
}