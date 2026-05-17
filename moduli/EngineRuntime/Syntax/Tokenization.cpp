#include "Tokenization.h"

#include "../Miscellaneous/DynamicString.h"

namespace Engine
{
	namespace Syntax
	{
		ParserSpellingException::ParserSpellingException(int At, const string & About) : Position(At), Comments(About) {}
		string ParserSpellingException::ToString(void) const { return U"ParserSpellingException: " + Comments + U" at position " + string(Position); }

		Token::Token(void) {}
		Token Token::EndOfStreamToken(void) { Token result; result.Class = TokenClass::EndOfStream; return result; }
		Token Token::KeywordToken(const string & word) { Token result; result.Class = TokenClass::Keyword; result.Content = word; return result; }
		Token Token::IdentifierToken(const string & ident) { Token result; result.Class = TokenClass::Identifier; result.Content = ident; return result; }
		Token Token::IdentifierToken(void) { Token result; result.Class = TokenClass::Identifier; return result; }
		Token Token::ConstantToken(const string & content, TokenConstantClass type) { Token result; result.Class = TokenClass::Constant; result.ValueClass = type; result.Content = content; return result; }
		Token Token::ConstantToken(TokenConstantClass type) { Token result; result.Class = TokenClass::Constant; result.ValueClass = type; return result; }
		Token Token::ConstantToken(void) { Token result; result.Class = TokenClass::Constant; return result; }
		Token Token::CharacterToken(const string & combo) { Token result; result.Class = TokenClass::CharCombo; result.Content = combo; return result; }
		Token Token::VoidToken(void) { Token result; result.Class = TokenClass::Void; return result; }

		uint64 Token::AsInteger(void) const
		{
			try {
				if (Content.Length() > 2) {
					if (Content[1] == U'x' || Content[1] == U'X') {
						return Content.Fragment(2, -1).ToUInt64(U"0123456789ABCDEF");
					} else if (Content[1] == U'o' || Content[1] == U'O') {
						return Content.Fragment(2, -1).ToUInt64(U"01234567");
					} else if (Content[1] == U'b' || Content[1] == U'B') {
						return Content.Fragment(2, -1).ToUInt64(U"01");
					} else return Content.ToUInt64();
				} else {
					return Content.ToUInt64();
				}
			}
			catch (...) {
				throw ParserSpellingException(SourcePosition, U"Integer value is out of range");
			}
			return 0;
		}
		double Token::AsDouble(void) const
		{
			double zero = 0.0;
			if (Content == U"inf") {
				return 1.0 / zero;
			} else if (Content == U"nan") {
				return zero / zero;
			}
			if (Content[Content.Length() - 1] == U'f' || Content[Content.Length() - 1] == U'F') return Content.Fragment(0, Content.Length() - 1).ToDouble();
			return Content.ToDouble();
		}
		float Token::AsFloat(void) const
		{
			float zero = 0.0f;
			if (Content == U"inf") {
				return 1.0f / zero;
			} else if (Content == U"nan") {
				return zero / zero;
			}
			if (Content[Content.Length() - 1] == U'f' || Content[Content.Length() - 1] == U'F') return Content.Fragment(0, Content.Length() - 1).ToFloat();
			return Content.ToFloat();
		}
		bool Token::AsBoolean(void) const { return Content.Length() > 0; }
		NumericTokenClass Token::NumericClass(void) const
		{
			if (Content == U"inf" || Content == U"nan") return NumericTokenClass::Double;
			for (int i = 0; i < Content.Length(); i++) {
				if (Content[i] == U'.') {
					return (Content[Content.Length() - 1] == U'f' || Content[Content.Length() - 1] == U'F') ? NumericTokenClass::Float : NumericTokenClass::Double;
				}
			}
			return NumericTokenClass::Integer;
		}
		bool Token::IsSimilar(const Token & Template) const
		{
			if (Class != Template.Class) return false;
			if (Class == TokenClass::Constant && (ValueClass == TokenConstantClass::Unknown || Template.ValueClass == TokenConstantClass::Unknown)) return true;
			return ValueClass == Template.ValueClass && ((Class != TokenClass::Keyword && Class != TokenClass::CharCombo) || Content == Template.Content);
		}
		bool Token::IsVoid(void) const { return Class == TokenClass::Void; }

		Spelling::Spelling(void) : Keywords(0x10), IsolatedChars(0x20), CombinableChars(0x20), AllowNonLatinNames(false) {}
		bool Spelling::IsKeyword(const string & word) const
		{
			for (int i = 0; i < Keywords.Length(); i++) if (Keywords[i] == word) return true;
			return false;
		}
		bool Spelling::IsIsolatedChars(widechar letter) const
		{
			for (int i = 0; i < IsolatedChars.Length(); i++) if (IsolatedChars[i] == letter) return true;
			return false;
		}
		bool Spelling::IsBooleanFalseLiteral(const string & word) const { return word == BooleanFalseLiteral; }
		bool Spelling::IsBooleanTrueLiteral(const string & word) const { return word == BooleanTrueLiteral; }
		bool Spelling::IsFloatInfinityLiteral(const string & word) const { return word == InfinityLiteral; }
		bool Spelling::IsFloatNonNumberLiteral(const string & word) const { return word == NonNumberLiteral; }

		bool operator==(const Token & a, const Token & b) { return a.Content == b.Content && a.Class == b.Class && a.ValueClass == b.ValueClass; }
		bool operator!=(const Token & a, const Token & b) { return a.Content != b.Content || a.Class != b.Class || a.ValueClass != b.ValueClass; }

		bool IsAllowedAlphabetical(widechar c, const Spelling & spelling)
		{
			if (spelling.AllowNonLatinNames) return IsAlphabetical(c);
			else return (c >= U'A' && c <= U'Z') || (c >= U'a' && c <= U'z');
		}

		Array<Token>* ParseText(const string & text, const Spelling & spelling)
		{
			int pos = 0;
			SafePointer< Array<Token> > Result = new Array<Token>(0x1000);
			while (true) {
				Token token;
				while (text[pos] == U' ' || text[pos] == U'\t' || text[pos] == U'\r' || text[pos] == U'\n') pos++;
				if (!text[pos]) {
					token.Class = TokenClass::EndOfStream;
					token.ValueClass = TokenConstantClass::Unknown;
					token.SourcePosition = pos;
					Result->Append(token);
					break;
				} else if (IsAllowedAlphabetical(text[pos], spelling) || text[pos] == U'_') {
					token.SourcePosition = pos;
					int sp = pos;
					while (IsAllowedAlphabetical(text[pos], spelling) || (text[pos] >= U'0' && text[pos] <= U'9') || text[pos] == U'_') pos++;
					token.Content = text.Fragment(sp, pos - sp);
					if (spelling.IsBooleanFalseLiteral(token.Content)) {
						token.Content = U"";
						token.Class = TokenClass::Constant;
						token.ValueClass = TokenConstantClass::Boolean;
					} else if (spelling.IsBooleanTrueLiteral(token.Content)) {
						token.Content = U"1";
						token.Class = TokenClass::Constant;
						token.ValueClass = TokenConstantClass::Boolean;
					} else if (spelling.IsFloatInfinityLiteral(token.Content)) {
						token.Content = U"inf";
						token.Class = TokenClass::Constant;
						token.ValueClass = TokenConstantClass::Numeric;
					} else if (spelling.IsFloatNonNumberLiteral(token.Content)) {
						token.Content = U"nan";
						token.Class = TokenClass::Constant;
						token.ValueClass = TokenConstantClass::Numeric;
					} else if (spelling.IsKeyword(token.Content)) {
						token.Class = TokenClass::Keyword;
						token.ValueClass = TokenConstantClass::Unknown;
					} else {
						token.Class = TokenClass::Identifier;
						token.ValueClass = TokenConstantClass::Unknown;
					}
					Result->Append(token);
				} else if (text[pos] >= U'0' && text[pos] <= U'9') {
					token.SourcePosition = pos;
					token.Class = TokenClass::Constant;
					token.ValueClass = TokenConstantClass::Numeric;
					int sp = pos;
					while ((text[pos] >= U'A' && text[pos] <= U'Z') || (text[pos] >= U'a' && text[pos] <= U'z') || (text[pos] >= U'0' && text[pos] <= U'9')) pos++;
					if (text[pos] == U'.') {
						pos++;
						while ((text[pos] >= U'A' && text[pos] <= U'Z') || (text[pos] >= U'a' && text[pos] <= U'z') || (text[pos] >= U'0' && text[pos] <= U'9')) pos++;
					}
					token.Content = text.Fragment(sp, pos - sp);
					Result->Append(token);
				} else if (spelling.CommentEndOfLineWord.Length() && text.Length() - pos >= spelling.CommentEndOfLineWord.Length() && text.Fragment(pos, spelling.CommentEndOfLineWord.Length()) == spelling.CommentEndOfLineWord) {
					while (text[pos] != U'\n' && text[pos] != 0) pos++;
				} else if (spelling.CommentBlockOpeningWord.Length() && text.Length() - pos >= spelling.CommentBlockOpeningWord.Length() && text.Fragment(pos, spelling.CommentBlockOpeningWord.Length()) == spelling.CommentBlockOpeningWord) {
					pos += spelling.CommentBlockOpeningWord.Length();
					while (text[pos] && !(spelling.CommentBlockClosingWord.Length() && text.Length() - pos >= spelling.CommentBlockClosingWord.Length() && text.Fragment(pos, spelling.CommentBlockClosingWord.Length()) == spelling.CommentBlockClosingWord)) pos++;
					if (text[pos]) pos += spelling.CommentBlockClosingWord.Length();
				} else if (text[pos] == U'\"' || text[pos] == U'\'') {
					widechar opening = text[pos];
					token.SourcePosition = pos;
					token.Class = TokenClass::Constant;
					DynamicString Text;
					pos++;
					do {
						if (text[pos] == U'\n') throw ParserSpellingException(pos, U"Unexpected caret return inside string constant");
						else if (text[pos] == 0) throw ParserSpellingException(pos, U"Unexpected end-of-stream inside string constant");
						else if (text[pos] != opening) {
							if (text[pos] == U'\\') {
								pos++;
								uint32 ucsdec = 0xFFFFFFFF;
								widechar escc = text[pos];
								if ((escc == U'\\') || (escc == U'\'') || (escc == U'\"') || (escc == U'?') || (escc == U'/')) {
									Text += escc;
									pos++;
								} else if (escc == U'a' || escc == U'A') {
									Text += U'\a';
									pos++;
								} else if (escc == U'b' || escc == U'B') {
									Text += U'\b';
									pos++;
								} else if (escc == U'e' || escc == U'E') {
									Text += U'\33';
									pos++;
								} else if (escc == U'f' || escc == U'F') {
									Text += U'\f';
									pos++;
								} else if (escc == U'n' || escc == U'N') {
									Text += U'\n';
									pos++;
								} else if (escc == U'r' || escc == U'R') {
									Text += U'\r';
									pos++;
								} else if (escc == U't' || escc == U'T') {
									Text += U'\t';
									pos++;
								} else if (escc == U'v' || escc == U'V') {
									Text += U'\v';
									pos++;
								} else if (escc == U'x' || escc == U'X' || escc == U'U') {
									pos++;
									ucsdec = 0;
									int count = 0;
									while ((count < 8) && ((text[pos] >= U'0' && text[pos] <= U'9') || (text[pos] >= U'A' && text[pos] <= U'F') || (text[pos] >= U'a' && text[pos] <= U'f'))) {
										int rec = 0;
										if ((text[pos] >= U'0') && (text[pos] <= U'9')) rec = text[pos] - U'0';
										else if ((text[pos] >= U'A') && (text[pos] <= U'F')) rec = text[pos] - U'A' + 10;
										else rec = text[pos] - U'a' + 10;
										ucsdec <<= 4;
										ucsdec |= rec;
										count++;
										pos++;
									}
								} else if (escc == U'u') {
									pos++;
									ucsdec = 0;
									int count = 0;
									while ((count < 4) && ((text[pos] >= U'0' && text[pos] <= U'9') || (text[pos] >= U'A' && text[pos] <= U'F') || (text[pos] >= U'a' && text[pos] <= U'f'))) {
										int rec = 0;
										if ((text[pos] >= U'0') && (text[pos] <= U'9')) rec = text[pos] - U'0';
										else if ((text[pos] >= U'A') && (text[pos] <= U'F')) rec = text[pos] - U'A' + 10;
										else rec = text[pos] - U'a' + 10;
										ucsdec <<= 4;
										ucsdec |= rec;
										count++;
										pos++;
									}
								} else if ((escc >= U'0') && (escc <= U'7')) {
									int count = 1;
									ucsdec = 0;
									while ((count < 4) && (escc >= U'0') && (escc <= U'7')) {
										int rec = text[pos] - U'0';
										ucsdec <<= 3;
										ucsdec |= rec;
										count++;
										pos++;
									}
								} else throw ParserSpellingException(pos, U"Unknown escape sequence");
								if (ucsdec != 0xFFFFFFFF) {
									Text += string(&ucsdec, 1, Encoding::UTF32);
								}
							} else {
								Text += text[pos];
								pos++;
							}
						}
					} while (text[pos] != opening && text[pos]);
					if (text[pos] == opening) pos++;
					if (opening == U'\"') {
						token.Content = Text.ToString();
						token.ValueClass = TokenConstantClass::String;
					} else {
						string str = Text.ToString();
						if (str.GetEncodedLength(Encoding::UTF32) != 1) throw ParserSpellingException(pos, U"Invalid character constant");
						uint32 chr;
						str.Encode(&chr, Encoding::UTF32, false);
						token.Content = string(chr);
						token.ValueClass = TokenConstantClass::Numeric;
					}
					Result->Append(token);
				} else if (spelling.IsIsolatedChars(text[pos])) {
					token.SourcePosition = pos;
					token.Class = TokenClass::CharCombo;
					token.ValueClass = TokenConstantClass::Unknown;
					token.Content = text[pos];
					pos++;
					Result->Append(token);
				} else {
					bool found = false;
					for (int i = 0; i < spelling.CombinableChars.Length(); i++) {
						if (text[pos] == spelling.CombinableChars[i]) {
							found = true;
							break;
						}
					}
					if (!found) throw ParserSpellingException(pos, U"Illegal character input");
					token.SourcePosition = pos;
					int sp = pos;
					do {
						pos++;
						found = false;
						for (int i = 0; i < spelling.CombinableChars.Length(); i++) {
							if (text[pos] == spelling.CombinableChars[i]) {
								found = true;
								break;
							}
						}
					} while (found);
					token.Content = text.Fragment(sp, pos - sp);
					token.Class = TokenClass::CharCombo;
					token.ValueClass = TokenConstantClass::Unknown;
					Result->Append(token);		
				}
			}
			Result->Retain();
			return Result;
		}
		string FormatStringToken(const string & input)
		{
			int ucslen = input.GetEncodedLength(Encoding::UTF32);
			Array<uint32> ucs;
			ucs.SetLength(ucslen);
			input.Encode(ucs, Encoding::UTF32, false);
			DynamicString result;
			for (int i = 0; i < ucs.Length(); i++) {
				if (ucs[i] < 0x20 || ucs[i] > 0xFFFF || ucs[i] == U'\\' || ucs[i] == U'\"') {
					if (ucs[i] > 0xFFFF) {
						result += U"\\x" + string(ucs[i], U"0123456789ABCDEF", 8);
					} else if (ucs[i] == U'\\') {
						result += U"\\\\";
					} else if (ucs[i] == U'\"') {
						result += U"\\\"";
					} else if (ucs[i] == U'\n') {
						result += U"\\n";
					} else if (ucs[i] == U'\r') {
						result += U"\\r";
					} else if (ucs[i] == U'\t') {
						result += U"\\t";
					} else if (ucs[i] == U'\33') {
						result += U"\\e";
					} else {
						result += U"\\" + string(ucs[i], U"01234567", 3);
					}
				} else {
					result += widechar(ucs[i]);
				}
			}
			return result.ToString();
		}
		string FormatFloatToken(float input, const Spelling & spelling)
		{
			auto src = input;
			uint32 & value = reinterpret_cast<uint32&>(input);
			bool negative = (value & 0x80000000) != 0;
			int exp = (value & 0x7F800000) >> 23;
			value &= 0x007FFFFF;
			if (exp == 0xFF) {
				if (value == 0) return negative ? (U"-" + spelling.InfinityLiteral) : spelling.InfinityLiteral;
				else return spelling.NonNumberLiteral;
			} else {
				string notation(src, U'.');
				if (notation.FindFirst(U'.') == -1) notation += U".0";
				return notation + U"f";
			}
		}
		string FormatDoubleToken(double input, const Spelling & spelling)
		{
			auto src = input;
			uint64 & value = reinterpret_cast<uint64&>(input);
			bool negative = (value & 0x8000000000000000) != 0;
			int exp = (value & 0x7FF0000000000000) >> 52;
			value &= 0x000FFFFFFFFFFFFF;
			if (exp == 0x7FF) {
				if (value == 0) return negative ? (U"-" + spelling.InfinityLiteral) : spelling.InfinityLiteral;
				else return spelling.NonNumberLiteral;
			} else {
				string notation(src, U'.');
				if (notation.FindFirst(U'.') == -1) notation += U".0";
				return notation;
			}
		}
	}
}