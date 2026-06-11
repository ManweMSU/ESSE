#pragma once

#include <Cor/Classes/CorVolume.hxx>
#include <Cor/IO/CorStreams.h>

namespace ESSE
{
	namespace Formationes
	{
		class StringTable : public Object
		{
			string _stub;
			Dictionary<int, string> _strings;
		private:
			StringTable(void) noexcept;
			static void ValidateMemoryString(const uint8 * data, uintptr size, uintptr data_at);
		public:
			virtual ~StringTable(void) override;
			virtual string ToStringE(ErrorContext & ectx) const noexcept override;

			const string & GetString(int id) const noexcept;
			oref<Set<int>> GetIndex(void) const;
			void AddString(const string & text, int id);
			void RemoveString(int id) noexcept;

			void Save(Stream * stream) const;
			void SaveToText(Stream * stream) const;
			void SaveToText(Stream * stream, Unicode::Encoding enc) const;
			void SaveToText(Stream * stream, const Unicode::EncodingCodepage & cp) const;
			void SaveToText(ITextEncoder * enc, bool escape_unicode) const;
			string SaveToText(void) const;
			string SaveToText(bool escape_unicode) const;

			static oref<StringTable> Create(void);
			static oref<StringTable> Load(Stream * source);
			static oref<StringTable> LoadFromText(const string & data);
			static oref<StringTable> LoadFromText(Stream * source);
			static oref<StringTable> LoadFromText(Stream * source, Unicode::Encoding enc);
			static oref<StringTable> LoadFromText(Stream * source, const Unicode::DecodingCodepage & cp);
			static oref<StringTable> LoadFromText(ITextDecoder * dec);
			static oref<StringTable> LoadGeneric(Stream * source);
		};
	}
}