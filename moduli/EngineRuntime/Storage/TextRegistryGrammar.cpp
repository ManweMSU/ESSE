#include "TextRegistryGrammar.h"

namespace Engine
{
	namespace Storage
	{
		void CreateTextRegistrySpelling(Syntax::Spelling & spelling)
		{
			spelling.BooleanFalseLiteral = U"false";
			spelling.BooleanTrueLiteral = U"true";
			spelling.InfinityLiteral = U"float_infinity";
			spelling.NonNumberLiteral = U"float_nan";
			spelling.CommentBlockClosingWord = U"*/";
			spelling.CommentBlockOpeningWord = U"/*";
			spelling.CommentEndOfLineWord = U"//";
			spelling.IsolatedChars << U'{';
			spelling.IsolatedChars << U'}';
			spelling.IsolatedChars << U'=';
			spelling.IsolatedChars << U'-';
			spelling.Keywords << U"long";
			spelling.Keywords << U"color";
			spelling.Keywords << U"time";
			spelling.Keywords << U"binary";
		}
		void CreateTextRegistryGrammar(Syntax::Grammar & grammar)
		{
			SafePointer<Syntax::Grammar::GrammarRule> node =
				new Syntax::Grammar::GrammarRule(Syntax::Grammar::GrammarRule::SequenceRule(U"NODE"));
			node->Rules << Syntax::Grammar::GrammarRule::SequenceRule(U"", 0, -1);
			node->Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::VariantRule(U"");
			node->Rules.LastElement().Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::TokenRule(U"NAME", Syntax::Token::ConstantToken());
			node->Rules.LastElement().Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::TokenRule(U"NAME", Syntax::Token::KeywordToken(U"long"));
			node->Rules.LastElement().Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::TokenRule(U"NAME", Syntax::Token::KeywordToken(U"color"));
			node->Rules.LastElement().Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::TokenRule(U"NAME", Syntax::Token::KeywordToken(U"time"));
			node->Rules.LastElement().Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::TokenRule(U"NAME", Syntax::Token::KeywordToken(U"binary"));
			node->Rules.LastElement().Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::TokenRule(U"NAME", Syntax::Token::IdentifierToken());
			node->Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::VariantRule(U"");
			node->Rules.LastElement().Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::SequenceRule(U"NODEDEF");
			node->Rules.LastElement().Rules.LastElement().Rules.LastElement().Rules <<
				Syntax::Grammar::GrammarRule::TokenRule(U"", Syntax::Token::CharacterToken(U"{"));
			node->Rules.LastElement().Rules.LastElement().Rules.LastElement().Rules <<
				Syntax::Grammar::GrammarRule::ReferenceRule(U"", U"NODE");
			node->Rules.LastElement().Rules.LastElement().Rules.LastElement().Rules <<
				Syntax::Grammar::GrammarRule::TokenRule(U"", Syntax::Token::CharacterToken(U"}"));
			node->Rules.LastElement().Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::SequenceRule(U"VALDEF");
			node->Rules.LastElement().Rules.LastElement().Rules.LastElement().Rules <<
				Syntax::Grammar::GrammarRule::TokenRule(U"", Syntax::Token::CharacterToken(U"="));
			node->Rules.LastElement().Rules.LastElement().Rules.LastElement().Rules <<
				Syntax::Grammar::GrammarRule::ReferenceRule(U"", U"VALDEF");
			SafePointer<Syntax::Grammar::GrammarRule> valdef =
				new Syntax::Grammar::GrammarRule(Syntax::Grammar::GrammarRule::VariantRule(U"VALDEF"));
			valdef->Rules << Syntax::Grammar::GrammarRule::ReferenceRule(U"", U"INTDEF");
			valdef->Rules << Syntax::Grammar::GrammarRule::ReferenceRule(U"", U"BOOLDEF");
			valdef->Rules << Syntax::Grammar::GrammarRule::ReferenceRule(U"", U"STRDEF");
			valdef->Rules << Syntax::Grammar::GrammarRule::ReferenceRule(U"", U"LINTDEF");
			valdef->Rules << Syntax::Grammar::GrammarRule::ReferenceRule(U"", U"COLORDEF");
			valdef->Rules << Syntax::Grammar::GrammarRule::ReferenceRule(U"", U"TIMEDEF");
			valdef->Rules << Syntax::Grammar::GrammarRule::ReferenceRule(U"", U"BINDEF");
			SafePointer<Syntax::Grammar::GrammarRule> intdef =
				new Syntax::Grammar::GrammarRule(Syntax::Grammar::GrammarRule::SequenceRule(U"INTDEF"));
			intdef->Rules << Syntax::Grammar::GrammarRule::TokenRule(U"", Syntax::Token::CharacterToken(U"-"), 0, 1);
			intdef->Rules << Syntax::Grammar::GrammarRule::TokenRule(U"VALUE", Syntax::Token::ConstantToken(Syntax::TokenConstantClass::Numeric));
			SafePointer<Syntax::Grammar::GrammarRule> booldef =
				new Syntax::Grammar::GrammarRule(Syntax::Grammar::GrammarRule::SequenceRule(U"BOOLDEF"));
			booldef->Rules << Syntax::Grammar::GrammarRule::TokenRule(U"VALUE", Syntax::Token::ConstantToken(Syntax::TokenConstantClass::Boolean));
			SafePointer<Syntax::Grammar::GrammarRule> strdef =
				new Syntax::Grammar::GrammarRule(Syntax::Grammar::GrammarRule::SequenceRule(U"STRDEF"));
			strdef->Rules << Syntax::Grammar::GrammarRule::TokenRule(U"VALUE", Syntax::Token::ConstantToken(Syntax::TokenConstantClass::String));
			SafePointer<Syntax::Grammar::GrammarRule> lintdef =
				new Syntax::Grammar::GrammarRule(Syntax::Grammar::GrammarRule::SequenceRule(U"LINTDEF"));
			lintdef->Rules << Syntax::Grammar::GrammarRule::TokenRule(U"", Syntax::Token::KeywordToken(U"long"));
			lintdef->Rules << Syntax::Grammar::GrammarRule::TokenRule(U"", Syntax::Token::CharacterToken(U"-"), 0, 1);
			lintdef->Rules << Syntax::Grammar::GrammarRule::TokenRule(U"VALUE", Syntax::Token::ConstantToken(Syntax::TokenConstantClass::Numeric));
			SafePointer<Syntax::Grammar::GrammarRule> colordef =
				new Syntax::Grammar::GrammarRule(Syntax::Grammar::GrammarRule::SequenceRule(U"COLORDEF"));
			colordef->Rules << Syntax::Grammar::GrammarRule::TokenRule(U"", Syntax::Token::KeywordToken(U"color"));
			colordef->Rules << Syntax::Grammar::GrammarRule::TokenRule(U"VALUE", Syntax::Token::ConstantToken(Syntax::TokenConstantClass::Numeric));
			SafePointer<Syntax::Grammar::GrammarRule> timedef =
				new Syntax::Grammar::GrammarRule(Syntax::Grammar::GrammarRule::SequenceRule(U"TIMEDEF"));
			timedef->Rules << Syntax::Grammar::GrammarRule::TokenRule(U"", Syntax::Token::KeywordToken(U"time"));
			timedef->Rules << Syntax::Grammar::GrammarRule::TokenRule(U"", Syntax::Token::CharacterToken(U"{"));
			timedef->Rules << Syntax::Grammar::GrammarRule::SequenceRule(U"", 1, 7);
			timedef->Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::TokenRule(U"VALUE", Syntax::Token::ConstantToken(Syntax::TokenConstantClass::Numeric));
			timedef->Rules << Syntax::Grammar::GrammarRule::TokenRule(U"", Syntax::Token::CharacterToken(U"}"));
			SafePointer<Syntax::Grammar::GrammarRule> bindef =
				new Syntax::Grammar::GrammarRule(Syntax::Grammar::GrammarRule::SequenceRule(U"BINDEF"));
			bindef->Rules << Syntax::Grammar::GrammarRule::TokenRule(U"", Syntax::Token::KeywordToken(U"binary"));
			bindef->Rules << Syntax::Grammar::GrammarRule::TokenRule(U"", Syntax::Token::CharacterToken(U"{"));
			bindef->Rules << Syntax::Grammar::GrammarRule::SequenceRule(U"", 0, -1);
			bindef->Rules.LastElement().Rules << Syntax::Grammar::GrammarRule::TokenRule(U"VALUE", Syntax::Token::ConstantToken(Syntax::TokenConstantClass::Numeric));
			bindef->Rules << Syntax::Grammar::GrammarRule::TokenRule(U"", Syntax::Token::CharacterToken(U"}"));
			grammar.Rules.Append(node->Label, node);
			grammar.Rules.Append(valdef->Label, valdef);
			grammar.Rules.Append(intdef->Label, intdef);
			grammar.Rules.Append(booldef->Label, booldef);
			grammar.Rules.Append(strdef->Label, strdef);
			grammar.Rules.Append(lintdef->Label, lintdef);
			grammar.Rules.Append(colordef->Label, colordef);
			grammar.Rules.Append(timedef->Label, timedef);
			grammar.Rules.Append(bindef->Label, bindef);
			grammar.EntranceRule = node->Label;
		}
	}
}