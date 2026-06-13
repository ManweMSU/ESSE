#include "Language.h"
#include <Cor/Classes/CorArray.hxx>

namespace ESSE
{
	namespace Formationes
	{
		bool IsPunctuation(unichar32 chr) noexcept
		{
			return chr == U'@' || chr == U'^' || chr == U'(' || chr == U')' || chr == U'[' || chr == U']' ||
				chr == U'{' || chr == U'}' || chr == U';' || chr == U':' || chr == U'~' || chr == U'.' ||
				chr == U',' || chr == U'?' || chr == U'#' || chr == U'%' || chr == U'&' || chr == U'*' ||
				chr == U'+' || chr == U'-' || chr == U'=' || chr == U'|' || chr == U'/' || chr == U'<' ||
				chr == U'>' || chr == U'!' || chr == U'$' || chr == U'\'' || chr == U'\"' || chr == U'\\';
		}
		bool IsWhitespace(unichar32 chr) noexcept { return chr == U' ' || chr == U'\t' || chr == U'\n' || chr == U'\r'; }
		bool ReadEscapeCode(const unichar32 * data, uintptr & current, uintptr length, uintptr max_len, const unichar32 * allowed, uintptr & relength) noexcept
		{
			relength = 0;
			uintptr n = 0, la = Memory::StringLength(allowed);
			while (n < max_len && current < length) {
				bool found = false;
				for (uintptr i = 0; i < la; i++) if (data[current] == allowed[i]) { found = true; break; }
				if (!found) return true;
				relength++; current++; n++;
			}
			return true;
		}
		bool ReadCharacter(const unichar32 * data, uintptr & current, uintptr length, unichar32 & result) noexcept
		{
			if (current >= length) return false;
			if (data[current] == U'\\') {
				current++;
				if (current >= length) return false;
				auto escc = data[current];
				if (escc == U'\\' || escc == U'\'' || escc == U'\"' || escc == U'?' || escc == U'/') {
					result = escc; current++;
				} else if (escc == U'a' || escc == U'A') {
					result = U'\a'; current++;
				} else if (escc == U'b' || escc == U'B') {
					result = U'\b'; current++;
				} else if (escc == U'e' || escc == U'E') {
					result = U'\e'; current++;
				} else if (escc == U'f' || escc == U'F') {
					result = U'\f'; current++;
				} else if (escc == U'n' || escc == U'N') {
					result = U'\n'; current++;
				} else if (escc == U'r' || escc == U'R') {
					result = U'\r'; current++;
				} else if (escc == U't' || escc == U'T') {
					result = U'\t'; current++;
				} else if (escc == U'v' || escc == U'V') {
					result = U'\v'; current++;
				} else if (escc == U'x' || escc == U'X' || escc == U'U') {
					current++;
					uintptr elen, es = current;
					if (!ReadEscapeCode(data, current, length, 8, U"0123456789ABCDEFabcdef", elen)) return false;
					try { result = string(data + es, elen).ToUInt32(HexadecimalBase); } catch (...) { return false; }
				} else if (escc == U'u') {
					current++;
					uintptr elen, es = current;
					if (!ReadEscapeCode(data, current, length, 4, U"0123456789ABCDEFabcdef", elen)) return false;
					try { result = string(data + es, elen).ToUInt32(HexadecimalBase); } catch (...) { return false; }
				} else if (escc >= U'0' && escc <= U'7') {
					uintptr elen, es = current;
					if (!ReadEscapeCode(data, current, length, 3, U"01234567", elen)) return false;
					try { result = string(data + es, elen).ToUInt32(OctalBase); } catch (...) { return false; }
				} else return false;
			} else {
				if (data[current] == U'\"' || data[current] < 32) return false;
				result = data[current];
				current++;
			}
			return true;
		}

		TokenStream::TokenStream(const unichar32 * data, uintptr length) noexcept : _data(data), _position(0), _length(length) {}
		TokenStream::~TokenStream(void) {}
		bool TokenStream::ExtractRange(RangeDesc & desc, const Token & token) const noexcept
		{
			try {
				desc.line_number = 1;
				uintptr from = token.source_range_from;
				uintptr length = token.source_length;
				for (uintptr i = 0; i < token.source_range_from; i++) if (_data[i] == U'\n') desc.line_number++;
				if (from >= _length) from = _length - 1;
				while (from >= 0 && _data[from] == U'\n') from--;
				if (from < 0) {
					desc.line = U"";
					desc.line_position_from = desc.line_range_length = 0;
					return true;
				}
				uintptr line_s = from, line_e = from;
				while (line_s && _data[line_s] != U'\n') line_s--;
				while (line_s < _length && (_data[line_s] == U'\n' || _data[line_s] == U'\r' || _data[line_s] == U'\t' || _data[line_s] == U' ')) line_s++;
				while (line_e < _length && _data[line_e] != U'\n' && _data[line_e] != U'\r') line_e++;
				if (from + length > _length) length = _length - from;
				desc.line = string(_data + line_s, (line_e - line_s));
				if (from > line_s) desc.line_position_from = from - line_s;
				else desc.line_position_from = 0;
				if (length > 0) desc.line_range_length = length;
				else desc.line_range_length = 0;
				return true;
			} catch (...) { return false; }
		}
		bool TokenStream::ReadToken(Token & token) noexcept
		{
			begin_read_token:
			while (_position < _length && IsWhitespace(_data[_position])) _position++;
			if (_position == _length || _data[_position] == 0) {
				token.cls = TokenClass::EOS;
				token.source_range_from = _position;
				token.source_length = 0;
				token.content_ptr = 0;
				token.content_i = 0;
			} else {
				if (_data[_position] >= U'0' && _data[_position] <= U'9') {
					if (_position < _length - 1 && (_data[_position + 1] == U'x' || _data[_position + 1] == U'X')) {
						_position += 2;
						auto from = _position;
						while (_position < _length && ((_data[_position] >= U'0' && _data[_position] <= U'9') ||
							(_data[_position] >= U'a' && _data[_position] <= U'f') ||
							(_data[_position] >= U'A' && _data[_position] <= U'F'))) _position++;
						try {
							token.cls = TokenClass::LiteralInteger;
							token.source_range_from = from - 2;
							token.source_length = _position - token.source_range_from;
							token.content_ptr = 0;
							token.content_i = string(_data + from, _position - from).ToUInt64(HexadecimalBase);
						} catch (...) { _position = from - 2; return false; }
					} else if (_position < _length - 1 && (_data[_position + 1] == U'o' || _data[_position + 1] == U'O')) {
						_position += 2;
						auto from = _position;
						while (_position < _length && (_data[_position] >= U'0' && _data[_position] <= U'7')) _position++;
						try {
							token.cls = TokenClass::LiteralInteger;
							token.source_range_from = from - 2;
							token.source_length = _position - token.source_range_from;
							token.content_ptr = 0;
							token.content_i = string(_data + from, _position - from).ToUInt64(OctalBase);
						} catch (...) { _position = from - 2; return false; }
					} else if (_position < _length - 1 && (_data[_position + 1] == U'b' || _data[_position + 1] == U'B')) {
						_position += 2;
						auto from = _position;
						while (_position < _length && (_data[_position] >= U'0' && _data[_position] <= U'1')) _position++;
						try {
							token.cls = TokenClass::LiteralInteger;
							token.source_range_from = from - 2;
							token.source_length = _position - token.source_range_from;
							token.content_ptr = 0;
							token.content_i = string(_data + from, _position - from).ToUInt64(BinaryBase);
						} catch (...) { _position = from - 2; return false; }
					} else {
						auto from = _position;
						while (_position < _length && ((_data[_position] >= U'0' && _data[_position] <= U'9') ||
							(_data[_position] == U'e') || (_data[_position] == U'E') || (_data[_position] == U'.') ||
							(_data[_position] == U'+' && _data[_position - 1] == U'E') ||
							(_data[_position] == U'+' && _data[_position - 1] == U'e') ||
							(_data[_position] == U'-' && _data[_position - 1] == U'E') ||
							(_data[_position] == U'-' && _data[_position - 1] == U'e'))) _position++;
						token.source_range_from = from;
						token.source_length = _position - token.source_range_from;
						token.content_ptr = 0;
						try {
							auto contents = string(_data + from, _position - from);
							if (contents.FindFirst(U'.') >= 0 || contents.FindFirst(U'e') >= 0 || contents.FindFirst(U'E') >= 0) {
								auto e_first = contents.FindFirst(U'e');
								auto e_last = contents.FindLast(U'e');
								auto E_first = contents.FindFirst(U'E');
								auto E_last = contents.FindLast(U'E');
								int e;
								if (e_first == e_last && e_first >= 0 && E_first < 0) e = e_first;
								else if (E_first == E_last && E_first >= 0 && e_first < 0) e = E_first;
								else if (E_first < 0 && e_first < 0) e = -1;
								else { _position = from; return false; }
								auto contents_f = contents.Substring(0, e);
								auto contents_e = e >= 0 ? contents.Substring(e + 1, -1) : U"0";
								auto frac = contents_f.ToDouble();
								auto exp = contents_e.ToInt32();
								while (exp > 0) { frac *= 10.0; exp--; }
								while (exp < 0) { frac /= 10.0; exp++; }
								token.cls = TokenClass::LiteralFloat;
								token.content_f = frac;
							} else {
								token.cls = TokenClass::LiteralInteger;
								token.content_i = contents.ToUInt64();
							}
						} catch (...) { _position = from; return false; }
					}
				} else if (_data[_position] == U'\"') {
					token.cls = TokenClass::LiteralString;
					token.source_range_from = _position;
					_position++;
					auto from = _position;
					while (true) {
						if (_position < _length) {
							if (_data[_position] == U'\"') break; else {
								unichar32 code;
								if (!ReadCharacter(_data, _position, _length, code)) return false;
							}
						} else return false;
					}
					token.content_ptr = _data + from;
					token.content_i = _position - from;
					_position++;
					token.source_length = _position - token.source_range_from;
				} else if (_position < _length - 1 && _data[_position] == U'/' && _data[_position + 1] == U'/') {
					auto com_begin = _position;
					_position += 2;
					while (_position < _length && _data[_position] != U'\n') _position++;
					goto begin_read_token;
				} else if (_position < _length - 1 && _data[_position] == U'/' && _data[_position + 1] == U'*') {
					auto com_begin = _position;
					_position += 2;
					while (_position < _length - 1 && (_data[_position] != U'*' || _data[_position + 1] != U'/')) _position++;
					_position += 2;
					if (_position > _length) _position = _length;
					goto begin_read_token;
				} else if (IsPunctuation(_data[_position])) {
					auto from = _position;
					_position++;
					token.cls = TokenClass::Punctuation;
					token.source_range_from = from;
					token.source_length = _position - token.source_range_from;
					token.content_ptr = _data + from;
					token.content_i = _position - from;
				} else {
					auto from = _position;
					while (_position < _length && !IsWhitespace(_data[_position]) && !IsPunctuation(_data[_position])) _position++;
					token.cls = TokenClass::Word;
					token.source_range_from = from;
					token.source_length = _position - token.source_range_from;
					token.content_ptr = _data + from;
					token.content_i = _position - from;
				}
			}
			return true;
		}
		uintptr TokenStream::GetCurrentPosition(void) const noexcept { return _position; }

		bool TokenIsEOS(const Token & t) noexcept { return t.cls == TokenClass::EOS; }
		bool TokenIsWord(const Token & t) noexcept { return t.cls == TokenClass::Word; }
		bool TokenIsWord(const Token & t, const unichar32 * word) noexcept
		{
			if (t.cls != TokenClass::Word) return false;
			uintptr p = 0;
			while (p < t.content_i && t.content_ptr[p] == word[p]) p++;
			return p == t.content_i && !word[p];
		}
		bool TokenIsPunctuation(const Token & t, const unichar32 * word) noexcept
		{
			if (t.cls != TokenClass::Punctuation) return false;
			uintptr p = 0;
			while (p < t.content_i && t.content_ptr[p] == word[p]) p++;
			return p == t.content_i && !word[p];
		}
		bool TokenIsIntegerLiteral(const Token & t) noexcept { return t.cls == TokenClass::LiteralInteger; }
		bool TokenIsFloatLiteral(const Token & t) noexcept { return t.cls == TokenClass::LiteralFloat; }
		bool TokenIsStringLiteral(const Token & t) noexcept { return t.cls == TokenClass::LiteralString; }
		uint64 GetTokenInteger(const Token & t) noexcept { return t.content_i; }
		double GetTokenFloat(const Token & t) noexcept { return t.content_f; }
		string GetTokenString(const Token & t)
		{
			if (t.cls == TokenClass::Word || t.cls == TokenClass::Punctuation) {
				return string(t.content_ptr, t.content_i);
			} else if (t.cls == TokenClass::LiteralString) {
				dynamic_string_ucs4 result;
				uintptr pos = 0;
				while (pos < t.content_i) {
					unichar32 w;
					ReadCharacter(t.content_ptr, pos, t.content_i, w);
					result.Append(w);
				}
				return result.ToString();
			} else return U"";
		}

		string CreateToken(uint64 from) { return string(from); }
		string CreateToken(double from)
		{
			auto result = string(from);
			if (result.FindFirst(U'.') < 0) return result + U".0"; else return result;
		}
		string CreateToken(const string & from, Unicode::Encoding enc)
		{
			dynamic_string_ucs4 result;
			result << U'\"';
			for (uintptr i = 0; i < from.GetLength(); i++) {
				auto chr = from[i];
				if (chr < 32 || chr == U'\"' || (chr >= 0x80 && enc == Unicode::Encoding::ASCII)) {
					if (chr == U'\a') result << U"\\a";
					else if (chr == U'\b') result << U"\\b";
					else if (chr == U'\e') result << U"\\e";
					else if (chr == U'\f') result << U"\\f";
					else if (chr == U'\n') result << U"\\n";
					else if (chr == U'\r') result << U"\\r";
					else if (chr == U'\t') result << U"\\t";
					else if (chr == U'\v') result << U"\\v";
					else if (chr == U'\"') result << U"\\\"";
					else if (chr < 0x200) result << U"\\" << string(uint(chr), OctalBase, 3);
					else if (chr < 0x10000) result << U"\\u" << string(uint(chr), HexadecimalBaseLowerCase, 4);
					else result << U"\\U" << string(uint(chr), HexadecimalBaseLowerCase, 8);
				} else result << chr;
			}
			result << U'\"';
			return result;
		}
	}
}