#include "MathExpression.h"

#include "../Math/MathBase.h"

namespace Engine
{
	namespace Syntax
	{
		namespace Math
		{
			bool lang_initialized = false;
			Spelling lang_spelling;
			Grammar lang_grammar;
			void InitializeLanguage(void)
			{
				if (!lang_initialized) {
					GetLanguageInfo(lang_spelling, lang_grammar);
					lang_initialized = true;
				}
			}

			int64 IVariableProvider::GetInteger(const string & name) { return 0; }
			double IVariableProvider::GetDouble(const string & name) { return 0.0; }
			void GetLanguageInfo(Spelling & spelling, Grammar & grammar)
			{
				spelling.IsolatedChars.Append(U'(');
				spelling.IsolatedChars.Append(U')');
				spelling.IsolatedChars.Append(U'+');
				spelling.IsolatedChars.Append(U'-');
				spelling.IsolatedChars.Append(U'*');
				spelling.IsolatedChars.Append(U'/');
				spelling.IsolatedChars.Append(U'%');
				spelling.Keywords << U"sgn";
				spelling.Keywords << U"abs";
				spelling.Keywords << U"sin";
				spelling.Keywords << U"cos";
				spelling.Keywords << U"tg";
				spelling.Keywords << U"ctg";
				spelling.Keywords << U"arcsin";
				spelling.Keywords << U"arccos";
				spelling.Keywords << U"arctg";
				spelling.Keywords << U"arcctg";
				spelling.Keywords << U"ln";
				spelling.Keywords << U"exp";
				spelling.Keywords << U"sqrt";
				grammar.EntranceRule = U"EXPRESSION";
				SafePointer<Grammar::GrammarRule> Expression = new Grammar::GrammarRule(Grammar::GrammarRule::SequenceRule(U"EXPRESSION"));
				SafePointer<Grammar::GrammarRule> MulArg = new Grammar::GrammarRule(Grammar::GrammarRule::SequenceRule(U"MULARG"));
				SafePointer<Grammar::GrammarRule> Operand = new Grammar::GrammarRule(Grammar::GrammarRule::VariantRule(U"OPERAND"));
				SafePointer<Grammar::GrammarRule> AddOp = new Grammar::GrammarRule(Grammar::GrammarRule::VariantRule(U"ADDOP"));
				SafePointer<Grammar::GrammarRule> MulOp = new Grammar::GrammarRule(Grammar::GrammarRule::VariantRule(U"MULOP"));
				SafePointer<Grammar::GrammarRule> FuncWord = new Grammar::GrammarRule(Grammar::GrammarRule::VariantRule(U"FUNCWORD"));
				Expression->Rules << Grammar::GrammarRule::ReferenceRule(U"", U"MULARG");
				Expression->Rules << Grammar::GrammarRule::SequenceRule(U"", 0, -1);
				Expression->Rules.LastElement().Rules << Grammar::GrammarRule::ReferenceRule(U"", U"ADDOP");
				Expression->Rules.LastElement().Rules << Grammar::GrammarRule::ReferenceRule(U"", U"MULARG");
				MulArg->Rules << Grammar::GrammarRule::ReferenceRule(U"", U"OPERAND");
				MulArg->Rules << Grammar::GrammarRule::SequenceRule(U"", 0, -1);
				MulArg->Rules.LastElement().Rules << Grammar::GrammarRule::ReferenceRule(U"", U"MULOP");
				MulArg->Rules.LastElement().Rules << Grammar::GrammarRule::ReferenceRule(U"", U"OPERAND");
				Operand->Rules << Grammar::GrammarRule::SequenceRule(U"");
				Operand->Rules.LastElement().Rules << Grammar::GrammarRule::TokenRule(U"", Token::CharacterToken(U"("));
				Operand->Rules.LastElement().Rules << Grammar::GrammarRule::ReferenceRule(U"", U"EXPRESSION");
				Operand->Rules.LastElement().Rules << Grammar::GrammarRule::TokenRule(U"", Token::CharacterToken(U")"));
				Operand->Rules << Grammar::GrammarRule::SequenceRule(U"");
				Operand->Rules.LastElement().Rules << Grammar::GrammarRule::ReferenceRule(U"", U"ADDOP", 0, 1);
				Operand->Rules.LastElement().Rules << Grammar::GrammarRule::TokenRule(U"", Token::ConstantToken(TokenConstantClass::Numeric));
				Operand->Rules << Grammar::GrammarRule::TokenRule(U"", Token::IdentifierToken());
				Operand->Rules << Grammar::GrammarRule::SequenceRule(U"");
				Operand->Rules.LastElement().Rules << Grammar::GrammarRule::ReferenceRule(U"", U"FUNCWORD");
				Operand->Rules.LastElement().Rules << Grammar::GrammarRule::TokenRule(U"", Token::CharacterToken(U"("));
				Operand->Rules.LastElement().Rules << Grammar::GrammarRule::ReferenceRule(U"", U"EXPRESSION");
				Operand->Rules.LastElement().Rules << Grammar::GrammarRule::TokenRule(U"", Token::CharacterToken(U")"));
				AddOp->Rules << Grammar::GrammarRule::TokenRule(U"", Token::CharacterToken(U"+"));
				AddOp->Rules << Grammar::GrammarRule::TokenRule(U"", Token::CharacterToken(U"-"));
				MulOp->Rules << Grammar::GrammarRule::TokenRule(U"", Token::CharacterToken(U"*"));
				MulOp->Rules << Grammar::GrammarRule::TokenRule(U"", Token::CharacterToken(U"/"));
				MulOp->Rules << Grammar::GrammarRule::TokenRule(U"", Token::CharacterToken(U"%"));
				FuncWord->Rules << Grammar::GrammarRule::TokenRule(U"", Token::KeywordToken(U"sgn"));
				FuncWord->Rules << Grammar::GrammarRule::TokenRule(U"", Token::KeywordToken(U"abs"));
				FuncWord->Rules << Grammar::GrammarRule::TokenRule(U"", Token::KeywordToken(U"sin"));
				FuncWord->Rules << Grammar::GrammarRule::TokenRule(U"", Token::KeywordToken(U"cos"));
				FuncWord->Rules << Grammar::GrammarRule::TokenRule(U"", Token::KeywordToken(U"tg"));
				FuncWord->Rules << Grammar::GrammarRule::TokenRule(U"", Token::KeywordToken(U"ctg"));
				FuncWord->Rules << Grammar::GrammarRule::TokenRule(U"", Token::KeywordToken(U"arcsin"));
				FuncWord->Rules << Grammar::GrammarRule::TokenRule(U"", Token::KeywordToken(U"arccos"));
				FuncWord->Rules << Grammar::GrammarRule::TokenRule(U"", Token::KeywordToken(U"arctg"));
				FuncWord->Rules << Grammar::GrammarRule::TokenRule(U"", Token::KeywordToken(U"arcctg"));
				FuncWord->Rules << Grammar::GrammarRule::TokenRule(U"", Token::KeywordToken(U"ln"));
				FuncWord->Rules << Grammar::GrammarRule::TokenRule(U"", Token::KeywordToken(U"exp"));
				FuncWord->Rules << Grammar::GrammarRule::TokenRule(U"", Token::KeywordToken(U"sqrt"));
				grammar.Rules.Append(Expression->Label, Expression);
				grammar.Rules.Append(MulArg->Label, MulArg);
				grammar.Rules.Append(Operand->Label, Operand);
				grammar.Rules.Append(AddOp->Label, AddOp);
				grammar.Rules.Append(MulOp->Label, MulOp);
				grammar.Rules.Append(FuncWord->Label, FuncWord);
			}
			int64 CalculateIntegerValue(SyntaxTreeNode & expression, IVariableProvider * variables);
			double CalculateDoubleValue(SyntaxTreeNode & expression, IVariableProvider * variables);
			int64 CalculateIntegerValue(SyntaxTreeNode & expression, IVariableProvider * variables)
			{
				if (expression.Label == U"OPERAND") {
					if (expression.Subnodes[0].Expands.Content == U"(") return CalculateIntegerValue(expression.Subnodes[1], variables);
					else if (expression.Subnodes[0].Expands.Class == TokenClass::Identifier) {
						if (variables) {
							return variables->GetInteger(expression.Subnodes[0].Expands.Content);
						} else return 0;
					} else if (expression.Subnodes[0].Label == U"FUNCWORD") {
						string func = expression.Subnodes[0].Subnodes[0].Expands.Content;
						if (func == U"sgn") {
							int64 value = CalculateIntegerValue(expression.Subnodes[2], variables);
							return (value > 0) ? 1 : ((value < 0) ? -1 : 0);
						} else if (func == U"abs") {
							int64 value = CalculateIntegerValue(expression.Subnodes[2], variables);
							return (value > 0) ? value : -value;
						} else if (func == U"sin") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return int64(Engine::Math::sin(value));
						} else if (func == U"cos") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return int64(Engine::Math::cos(value));
						} else if (func == U"tg") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return int64(Engine::Math::tg(value));
						} else if (func == U"ctg") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return int64(Engine::Math::ctg(value));
						} else if (func == U"arcsin") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return int64(Engine::Math::arcsin(value));
						} else if (func == U"arccos") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return int64(Engine::Math::arccos(value));
						} else if (func == U"arctg") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return int64(Engine::Math::arctg(value));
						} else if (func == U"arcctg") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return int64(Engine::Math::arcctg(value));
						} else if (func == U"ln") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return int64(Engine::Math::ln(value));
						} else if (func == U"exp") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return int64(Engine::Math::exp(value));
						} else if (func == U"sqrt") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return int64(Engine::Math::sqrt(value));
						}
						return 0;
					} else {
						int vi = 0;
						int sgn = 1;
						if (expression.Subnodes[0].Label == U"ADDOP") {
							vi = 1;
							if (expression.Subnodes[0].Subnodes[0].Expands.Content == U"-") sgn = -1;
						}
						if (expression.Subnodes[vi].Expands.NumericClass() == NumericTokenClass::Integer) {
							return int64(expression.Subnodes[vi].Expands.AsInteger()) * sgn;
						} else {
							return int64(expression.Subnodes[vi].Expands.AsDouble()) * sgn;
						}
					}
				} else {
					int64 value = CalculateIntegerValue(expression.Subnodes[0], variables);
					for (int i = 1; i < expression.Subnodes.Length(); i += 2) {
						int64 value2 = CalculateIntegerValue(expression.Subnodes[i + 1], variables);
						if (expression.Subnodes[i].Subnodes[0].Expands.Content == U"+") {
							value += value2;
						} else if (expression.Subnodes[i].Subnodes[0].Expands.Content == U"-") {
							value -= value2;
						} else if (expression.Subnodes[i].Subnodes[0].Expands.Content == U"*") {
							value *= value2;
						} else if (expression.Subnodes[i].Subnodes[0].Expands.Content == U"/") {
							value /= value2;
						} else if (expression.Subnodes[i].Subnodes[0].Expands.Content == U"%") {
							value %= value2;
						}
					}
					return value;
				}
			}
			double CalculateDoubleValue(SyntaxTreeNode & expression, IVariableProvider * variables)
			{
				if (expression.Label == U"OPERAND") {
					if (expression.Subnodes[0].Expands.Content == U"(") return CalculateDoubleValue(expression.Subnodes[1], variables);
					else if (expression.Subnodes[0].Expands.Class == TokenClass::Identifier) {
						if (variables) {
							return variables->GetDouble(expression.Subnodes[0].Expands.Content);
						} else return 0.0;
					} else if (expression.Subnodes[0].Label == U"FUNCWORD") {
						string func = expression.Subnodes[0].Subnodes[0].Expands.Content;
						if (func == U"sgn") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return (value > 0.0) ? 1.0 : ((value < 0) ? -1.0 : 0.0);
						} else if (func == U"abs") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return (value > 0.0) ? value : -value;
						} else if (func == U"sin") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return Engine::Math::sin(value);
						} else if (func == U"cos") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return Engine::Math::cos(value);
						} else if (func == U"tg") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return Engine::Math::tg(value);
						} else if (func == U"ctg") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return Engine::Math::ctg(value);
						} else if (func == U"arcsin") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return Engine::Math::arcsin(value);
						} else if (func == U"arccos") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return Engine::Math::arccos(value);
						} else if (func == U"arctg") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return Engine::Math::arctg(value);
						} else if (func == U"arcctg") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return Engine::Math::arcctg(value);
						} else if (func == U"ln") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return Engine::Math::ln(value);
						} else if (func == U"exp") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return Engine::Math::exp(value);
						} else if (func == U"sqrt") {
							double value = CalculateDoubleValue(expression.Subnodes[2], variables);
							return Engine::Math::sqrt(value);
						}
						return 0.0;
					} else {
						int vi = 0;
						double sgn = 1.0;
						if (expression.Subnodes[0].Label == U"ADDOP") {
							vi = 1;
							if (expression.Subnodes[0].Subnodes[0].Expands.Content == U"-") sgn = -1.0;
						}
						if (expression.Subnodes[vi].Expands.NumericClass() == NumericTokenClass::Integer) {
							return double(expression.Subnodes[vi].Expands.AsInteger()) * sgn;
						} else {
							return expression.Subnodes[vi].Expands.AsDouble() * sgn;
						}
					}
				} else {
					double value = CalculateDoubleValue(expression.Subnodes[0], variables);
					for (int i = 1; i < expression.Subnodes.Length(); i += 2) {
						double value2 = CalculateDoubleValue(expression.Subnodes[i + 1], variables);
						if (expression.Subnodes[i].Subnodes[0].Expands.Content == U"+") {
							value += value2;
						} else if (expression.Subnodes[i].Subnodes[0].Expands.Content == U"-") {
							value -= value2;
						} else if (expression.Subnodes[i].Subnodes[0].Expands.Content == U"*") {
							value *= value2;
						} else if (expression.Subnodes[i].Subnodes[0].Expands.Content == U"/") {
							value /= value2;
						}
					}
					return value;
				}
			}
			int64 CalculateExpressionInteger(const string & expression, IVariableProvider * variables)
			{
				InitializeLanguage();
				SafePointer< Array<Token> > Stream = ParseText(expression, lang_spelling);
				SafePointer<SyntaxTree> Tree = new SyntaxTree(*Stream, lang_grammar);
				Tree->Root.OptimizeNode();
				return CalculateIntegerValue(Tree->Root, variables);
			}
			double CalculateExpressionDouble(const string & expression, IVariableProvider * variables)
			{
				InitializeLanguage();
				SafePointer< Array<Token> > Stream = ParseText(expression, lang_spelling);
				SafePointer<SyntaxTree> Tree = new SyntaxTree(*Stream, lang_grammar);
				Tree->Root.OptimizeNode();
				return CalculateDoubleValue(Tree->Root, variables);
			}
		}
	}
}