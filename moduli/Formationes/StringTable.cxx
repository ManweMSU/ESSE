#include "StringTable.h"
#include "Language.h"

namespace ESSE
{
	namespace Formationes
	{
		namespace Format
		{
			ESSE_PACKED_STRUCTURE(StringTableHeader)
				char signature[8];		// "ecs.1.0\0"
				uint32 signature_ex;	// 0x80000007
				uint32 version;			// 0
				uint32 data_offset;
				uint32 data_size;
				uint32 string_count;
			ESSE_END_PACKED_STRUCTURE
			ESSE_PACKED_STRUCTURE(StringTableEntity)
				uint32 id;
				uint32 offset;
			ESSE_END_PACKED_STRUCTURE
		}
		StringTable::StringTable(void) noexcept {}
		void StringTable::ValidateMemoryString(const uint8 * data, uintptr size, uintptr data_at)
		{
			if (data_at > size || size - data_at < 2) throw InvalidFormatException();
			auto pos = data_at;
			while (size - pos >= 2 && *reinterpret_cast<const uint16 *>(data + pos)) pos += 2;
			if (size - pos < 2) throw InvalidFormatException();
		}
		StringTable::~StringTable(void) {}
		string StringTable::ToStringE(ErrorContext & ectx) const noexcept { ESSE_TRY_INTRO return U"String table"; ESSE_TRY_OUTRO(string()) }
		const string & StringTable::GetString(int id) const noexcept { auto e = _strings[id]; return e ? *e : _stub; }
		oref<Set<int>> StringTable::GetIndex(void) const
		{
			auto result = owrap(new Set<int>);
			for (auto & s : _strings) result->AddElement(s.key);
			return result;
		}
		void StringTable::AddString(const string & text, int id) { _strings.Update(id, text); }
		void StringTable::RemoveString(int id) noexcept { _strings.Remove(id); }
		void StringTable::Save(Stream * stream) const
		{
			if (!stream) throw InvalidArgumentException();
			uintptr value = _strings.Count();
			Format::StringTableHeader hdr;
			array<Format::StringTableEntity> ehdr(1);
			ehdr.SetLength(value);
			array<unichar16> buffer(0x1000);
			value = 0;
			for (auto & s : _strings) {
				ehdr[value].id = s.key;
				ucs2_string entry = s.value;
				intptr collision = -1;
				if (entry.GetLength() < buffer.GetLength()) {
					for (intptr j = buffer.GetLength() - entry.GetLength() - 1; j >= 0; j--) {
						if (Memory::MemoryCompare(buffer.GetBuffer() + j, entry.GetData(), (entry.GetLength() + 1U) << 1U) == 0) { collision = j; break; }
					}
				}
				if (collision < 0) {
					ehdr[value].offset = buffer.GetLength() << 1U;
					buffer.Append(entry.GetData(), entry.GetLength() + 1);
				} else ehdr[value].offset = collision << 1U;
				value++;
			}
			Memory::MemoryCopy(hdr.signature, "ecs.1.0\0", 8);
			hdr.signature_ex = 0x80000007;
			hdr.version = 0;
			hdr.data_offset = sizeof(hdr) + sizeof(Format::StringTableEntity) * ehdr.GetLength();
			hdr.data_size = buffer.GetLength() << 1U;
			hdr.string_count = ehdr.GetLength();
			stream->Write(&hdr, sizeof(hdr));
			stream->Write(ehdr.GetBuffer(), sizeof(Format::StringTableEntity) * ehdr.GetLength());
			stream->Write(buffer.GetBuffer(), hdr.data_size);
		}
		void StringTable::SaveToText(Stream * stream) const
		{
			if (!stream) throw InvalidArgumentException();
			auto encoder = owrap(new TextEncoder(stream, Unicode::Encoding::UTF16));
			encoder->WriteEncodingSignature();
			SaveToText(encoder, false);
		}
		void StringTable::SaveToText(Stream * stream, Unicode::Encoding enc) const
		{
			if (!stream) throw InvalidArgumentException();
			auto encoder = owrap(new TextEncoder(stream, enc));
			encoder->WriteEncodingSignature();
			SaveToText(encoder, enc == Unicode::Encoding::ASCII);
		}
		void StringTable::SaveToText(Stream * stream, const Unicode::EncodingCodepage & cp) const
		{
			if (!stream) throw InvalidArgumentException();
			auto encoder = owrap(new TextEncoder(stream, cp));
			encoder->WriteEncodingSignature();
			SaveToText(encoder, true);
		}
		void StringTable::SaveToText(ITextEncoder * enc, bool escape_unicode) const
		{
			if (!enc) throw InvalidArgumentException();
			enc->Write(SaveToText(escape_unicode));
		}
		string StringTable::SaveToText(void) const { return SaveToText(false); }
		string StringTable::SaveToText(bool escape_unicode) const
		{
			dynamic_string_ucs4 result;
			for (auto & s : _strings) result << CreateToken(uint64(s.key)) << U" " << CreateToken(s.value, escape_unicode ? Unicode::Encoding::ASCII : Unicode::Encoding::UTF32) << IO::LineFeedSequence;
			return result;
		}
		oref<StringTable> StringTable::Create(void) { return owrap(new StringTable); }
		oref<StringTable> StringTable::Load(Stream * source)
		{
			auto result = owrap(new StringTable);
			Format::StringTableHeader hdr;
			source->Seek(0, SeekOrigin::Begin);
			if (source->Read(&hdr, sizeof(hdr)) != sizeof(hdr)) throw InvalidFormatException();
			if (Memory::MemoryCompare(hdr.signature, "ecs.1.0\0", 8) || hdr.signature_ex != 0x80000007 || hdr.version) throw InvalidFormatException();
			array<Format::StringTableEntity> ehdr(1);
			ehdr.SetLength(hdr.string_count);
			if (source->Read(ehdr.GetBuffer(), ehdr.GetLength() * sizeof(*ehdr)) != ehdr.GetLength() * sizeof(*ehdr)) throw InvalidFormatException();
			source->Seek(hdr.data_offset, SeekOrigin::Begin);
			auto data = source->ReadBlock(hdr.data_size);
			for (auto & e : ehdr) {
				ValidateMemoryString(data->GetBuffer(), data->GetLength(), e.offset);
				result->AddString(string(reinterpret_cast<const unichar16 *>(data->GetBuffer() + e.offset)), e.id);
			}
			return result;
		}
		oref<StringTable> StringTable::LoadFromText(const string & data)
		{
			Token token;
			TokenStream stream(data.GetData(), data.GetLength());
			if (!stream.ReadToken(token)) throw InvalidFormatException();
			auto result = StringTable::Create();
			while (TokenIsIntegerLiteral(token)) {
				auto id = GetTokenInteger(token);
				if (!stream.ReadToken(token)) throw InvalidFormatException();
				if (!TokenIsStringLiteral(token)) throw InvalidFormatException();
				auto value = GetTokenString(token);
				if (!stream.ReadToken(token)) throw InvalidFormatException();
				result->AddString(value, id);
			}
			if (!TokenIsEOS(token)) throw InvalidFormatException();
			return result;
		}
		oref<StringTable> StringTable::LoadFromText(Stream * source)
		{
			if (!source) throw InvalidArgumentException();
			auto decoder = owrap(new TextDecoder(source, Unicode::Encoding::Unknown));
			return LoadFromText(decoder);
		}
		oref<StringTable> StringTable::LoadFromText(Stream * source, Unicode::Encoding enc)
		{
			if (!source) throw InvalidArgumentException();
			auto decoder = owrap(new TextDecoder(source, enc));
			return LoadFromText(decoder);
		}
		oref<StringTable> StringTable::LoadFromText(Stream * source, const Unicode::DecodingCodepage & cp)
		{
			if (!source) throw InvalidArgumentException();
			auto decoder = owrap(new TextDecoder(source, cp));
			return LoadFromText(decoder);
		}
		oref<StringTable> StringTable::LoadFromText(ITextDecoder * dec)
		{
			if (!dec) throw InvalidArgumentException();
			return LoadFromText(dec->ReadAll());
		}
		oref<StringTable> StringTable::LoadGeneric(Stream * source)
		{
			if (!source) throw InvalidArgumentException();
			try { return Load(source); }
			catch (Exception & e) { if (e.GetError().error_code != Errores::ErrorInvalidFormat) throw e; }
			source->Seek(0, SeekOrigin::Begin);
			auto buffer = MemoryStream::Create(0x10000);
			source->CopyToUntilEof(buffer);
			buffer->Seek(0, SeekOrigin::Begin);
			return LoadFromText(buffer);
		}
	}
}