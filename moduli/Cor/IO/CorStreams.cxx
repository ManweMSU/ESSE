#include "CorStreams.h"

namespace ESSE
{
	uintptr Stream::Read(void * data, uintptr size) { ErrorContext ectx; ErrorClear(ectx); auto result = ReadE(data, size, ectx); ErrorThrow(ectx); return result; }
	uintptr Stream::Write(const void * data, uintptr size) { ErrorContext ectx; ErrorClear(ectx); auto result = WriteE(data, size, ectx); ErrorThrow(ectx); return result; }
	int64 Stream::Seek(uint64 position, SeekOrigin org) { ErrorContext ectx; ErrorClear(ectx); auto result = SeekE(position, org, ectx); ErrorThrow(ectx); return result; }
	uint64 Stream::GetLength(void) { ErrorContext ectx; ErrorClear(ectx); auto result = GetLengthE(ectx); ErrorThrow(ectx); return result; }
	void Stream::SetLength(uint64 length) { ErrorContext ectx; ErrorClear(ectx); SetLengthE(length, ectx); ErrorThrow(ectx); }
	void Stream::Flush(void) { ErrorContext ectx; ErrorClear(ectx); FlushE(ectx); ErrorThrow(ectx); }
	void Stream::CopyTo(Stream * to, uint64 length)
	{
		constexpr uint64 buflen = 0x100000;
		array<uint8> buffer(1);
		buffer.SetLength(buflen);
		uint64 pending = length;
		while (pending) {
			uintptr amount = uintptr(min(buflen, pending));
			auto read = Read(buffer, amount);
			if (read != amount) throw InvalidFormatException();
			auto written = to->Write(buffer, amount);
			if (written != amount) throw OutOfMemoryException();
			pending -= amount;
		}
	}
	void Stream::CopyToUntilEof(Stream * to)
	{
		constexpr uint64 buflen = 0x100000;
		array<uint8> buffer(1);
		buffer.SetLength(buflen);
		while (true) {
			auto read = Read(buffer, buflen);
			if (read) {
				auto written = to->Write(buffer, read);
				if (written != read) throw OutOfMemoryException();
			} else break;
		}
	}
	oref<DataBlock> Stream::ReadAll(void)
	{
		auto current = Seek(0, SeekOrigin::Current);
		auto length = GetLength();
		constexpr uintptr max_read = uintptr(intptr(-1)) >> 1;
		auto length_read = length - current;
		if (length_read > max_read) throw OutOfMemoryException();
		auto result = owrap(new DataBlock(length_read));
		result->SetLength(length_read);
		auto read = Read(result->GetBuffer(), length_read);
		if (read != length_read) throw InvalidStateException();
		return result;
	}
	oref<DataBlock> Stream::ReadBlock(uintptr length)
	{
		auto result = owrap(new DataBlock(length));
		result->SetLength(length);
		auto read = Read(result->GetBuffer(), length);
		if (read != length) result->SetLength(read);
		return result;
	}
	void Stream::WriteBlock(const DataBlock * data) { if (data) Write(data->GetBuffer(), data->GetLength()); }
	void Stream::RelocateData(uint64 offset_from, uint64 offset_to, uint64 length)
	{
		if (offset_to > offset_from) {
			DataBlock block(0x10000);
			block.SetLength(0x100000);
			Memory::ZeroMemory(block.GetBuffer(), block.GetLength());
			uint64 data_left = length;
			while (data_left) {
				uint32 current = uint32(min(data_left, uint64(block.GetLength())));
				Seek(offset_from + data_left - current, SeekOrigin::Begin);
				Read(block.GetBuffer(), current);
				Seek(offset_to + data_left - current, SeekOrigin::Begin);
				Write(block.GetBuffer(), current);
				data_left -= current;
			}
		} else if (offset_to < offset_from) {
			DataBlock block(0x10000);
			block.SetLength(0x100000);
			Memory::ZeroMemory(block.GetBuffer(), block.GetLength());
			uint64 data_left = length;
			while (data_left) {
				uint32 current = uint32(min(data_left, uint64(block.GetLength())));
				Seek(offset_from + length - data_left, SeekOrigin::Begin);
				Read(block.GetBuffer(), current);
				Seek(offset_to + length - data_left, SeekOrigin::Begin);
				Write(block.GetBuffer(), current);
				data_left -= current;
			}
		}
	}

	FileStream::FileStream(handle file, bool owned) : _file(file), _owned(owned) {}
	oref<FileStream> FileStream::Create(const string & path, uint access, FileCreationMode mode)
	{
		ErrorContext ectx; ErrorClear(ectx);
		auto file = IO::CreateFile(path, access, mode, ectx);
		ErrorThrow(ectx);
		try { return owrap(new FileStream(file, true)); } catch (...) { IO::CloseHandle(file); throw; }
	}
	oref<FileStream> FileStream::CreateWrapper(handle file, bool take_ownership) { return owrap(new FileStream(file, take_ownership)); }
	oref<FileStream> FileStream::CreateFromDuplicate(handle file)
	{
		ErrorContext ectx; ErrorClear(ectx);
		auto copy = IO::DuplicateHandle(file, ectx);
		ErrorThrow(ectx);
		try { return owrap(new FileStream(copy, true)); } catch (...) { IO::CloseHandle(copy); throw; }
	}
	FileStream::~FileStream(void) { if (_owned) IO::CloseHandle(_file); }
	string FileStream::ToStringE(ErrorContext & ectx) const noexcept { ESSE_TRY_INTRO return U"File Stream"; ESSE_TRY_OUTRO(string()) }
	uintptr FileStream::ReadE(void * data, uintptr size, ErrorContext & ectx) noexcept { return IO::ReadFile(_file, data, size, ectx); }
	uintptr FileStream::WriteE(const void * data, uintptr size, ErrorContext & ectx) noexcept { return IO::WriteFile(_file, data, size, ectx); }
	uint64 FileStream::SeekE(int64 position, SeekOrigin org, ErrorContext & ectx) noexcept { return IO::SeekFile(_file, position, org, ectx); }
	uint64 FileStream::GetLengthE(ErrorContext & ectx) noexcept { return IO::GetFileSize(_file, ectx); }
	void FileStream::SetLengthE(const uint64 & length, ErrorContext & ectx) noexcept { IO::SetFileSize(_file, length, ectx); }
	void FileStream::FlushE(ErrorContext & ectx) noexcept { IO::Flush(_file, ectx); }
	handle FileStream::GetIOHandle(void) const noexcept { return _file; }

	MemoryStream::MemoryStream(DataBlock * data) : _data(data), _pointer(0) {}
	oref<MemoryStream> MemoryStream::Create(uintptr block_size)
	{
		auto data = owrap(new DataBlock(block_size));
		return owrap(new MemoryStream(data));
	}
	oref<MemoryStream> MemoryStream::Create(const void * memory, uintptr size)
	{
		auto data = owrap(new DataBlock(0x10000));
		data->Append(reinterpret_cast<const uint8 *>(memory), size);
		return owrap(new MemoryStream(data));
	}
	oref<MemoryStream> MemoryStream::Create(const void * memory, uintptr size, uintptr block_size)
	{
		auto data = owrap(new DataBlock(block_size));
		data->Append(reinterpret_cast<const uint8 *>(memory), size);
		return owrap(new MemoryStream(data));
	}
	oref<MemoryStream> MemoryStream::CreateFromData(DataBlock * data)
	{
		if (!data) throw InvalidArgumentException();
		return owrap(new MemoryStream(data));
	}
	MemoryStream::~MemoryStream(void) {}
	string MemoryStream::ToStringE(ErrorContext & ectx) const noexcept { ESSE_TRY_INTRO return U"Memory Stream"; ESSE_TRY_OUTRO(string()) }
	uintptr MemoryStream::ReadE(void * data, uintptr size, ErrorContext & ectx) noexcept
	{
		uintptr rem = _data->GetLength() - _pointer;
		uintptr read = min(size, rem);
		Memory::MemoryCopy(data, _data->GetBuffer() + _pointer, read);
		_pointer += read;
		return read;
	}
	uintptr MemoryStream::WriteE(const void * data, uintptr size, ErrorContext & ectx) noexcept
	{
		uintptr rem = _data->GetLength() - _pointer;
		if (size > rem) {
			constexpr uintptr max_intptr = uintptr(intptr(-1)) >> 1;
			if (size > max_intptr - _pointer) throw InputOutputException(Errores::SuberrorIO::FileTooLarge);
			_data->SetLength(_pointer + size);
		}
		Memory::MemoryCopy(_data->GetBuffer() + _pointer, data, size);
		_pointer += size;
		return size;
	}
	uint64 MemoryStream::SeekE(int64 position, SeekOrigin org, ErrorContext & ectx) noexcept
	{
		int64 newpos = position;
		if (org == SeekOrigin::Current) newpos += int64(_pointer);
		else if (org == SeekOrigin::End) newpos += int64(_data->GetLength());
		else if (org != SeekOrigin::Begin) throw InvalidArgumentException();
		if (newpos < 0 || newpos > int64(_data->GetLength())) throw InvalidArgumentException();
		_pointer = newpos;
		return _pointer;
	}
	uint64 MemoryStream::GetLengthE(ErrorContext & ectx) noexcept { return _data->GetLength(); }
	void MemoryStream::SetLengthE(const uint64 & length, ErrorContext & ectx) noexcept
	{
		constexpr uint64 max_intptr = uintptr(intptr(-1)) >> 1;
		if (length > max_intptr) throw InvalidArgumentException();
		_data->SetLength(length);
		if (_pointer > length) _pointer = length;
	}
	void MemoryStream::FlushE(ErrorContext & ectx) noexcept {}
	DataBlock * MemoryStream::GetStorage(void) noexcept { return _data; }
	const DataBlock * MemoryStream::GetStorage(void) const noexcept { return _data; }
	void * MemoryStream::GetData(void) noexcept { return _data->GetBuffer(); }
	const void * MemoryStream::GetData(void) const noexcept { return _data->GetBuffer(); }

	StaticMemoryStream::StaticMemoryStream(const uint8 * data, uintptr size) : _data(data), _size(size), _pointer(0) {}
	oref<StaticMemoryStream> StaticMemoryStream::Create(const void * memory, uintptr size) { return owrap(new StaticMemoryStream(reinterpret_cast<const uint8 *>(memory), size)); }
	StaticMemoryStream::~StaticMemoryStream(void) {}
	string StaticMemoryStream::ToStringE(ErrorContext & ectx) const noexcept { ESSE_TRY_INTRO return U"Static Memory Stream"; ESSE_TRY_OUTRO(string()) }
	uintptr StaticMemoryStream::ReadE(void * data, uintptr size, ErrorContext & ectx) noexcept
	{
		uintptr rem = _size - _pointer;
		uintptr read = min(size, rem);
		Memory::MemoryCopy(data, _data + _pointer, read);
		_pointer += read;
		return read;
	}
	uintptr StaticMemoryStream::WriteE(const void * data, uintptr size, ErrorContext & ectx) noexcept { ErrorSet(ectx, Errores::ErrorNotImplemented); return 0; }
	uint64 StaticMemoryStream::SeekE(int64 position, SeekOrigin org, ErrorContext & ectx) noexcept
	{
		int64 newpos = position;
		if (org == SeekOrigin::Current) newpos += int64(_pointer);
		else if (org == SeekOrigin::End) newpos += int64(_size);
		else if (org != SeekOrigin::Begin) throw InvalidArgumentException();
		if (newpos < 0 || newpos > int64(_size)) throw InvalidArgumentException();
		_pointer = newpos;
		return _pointer;
	}
	uint64 StaticMemoryStream::GetLengthE(ErrorContext & ectx) noexcept { return _size; }
	void StaticMemoryStream::SetLengthE(const uint64 & length, ErrorContext & ectx) noexcept { ErrorSet(ectx, Errores::ErrorNotImplemented); }
	void StaticMemoryStream::FlushE(ErrorContext & ectx) noexcept {}
	const void * StaticMemoryStream::GetData(void) const { return _data; }

	Substream::Substream(Stream * stream, uint64 offset, uint64 size) : _inner(stream), _begin(offset), _end(offset + size), _size(size), _pointer(0) {}
	oref<Substream> Substream::Create(Stream * stream, uint64 offset, uint64 size) { return owrap(new Substream(stream, offset, size)); }
	Substream::~Substream(void) {}
	string Substream::ToStringE(ErrorContext & ectx) const noexcept { ESSE_TRY_INTRO return U"Substream"; ESSE_TRY_OUTRO(string()) }
	uintptr Substream::ReadE(void * data, uintptr size, ErrorContext & ectx) noexcept
	{
		ErrorContext lectx; ErrorClear(lectx);
		uint64 rem = _size - _pointer;
		uintptr read = min(uint64(size), rem);
		auto prev = _inner->SeekE(0, SeekOrigin::Current, ectx);
		if (ErrorTest(ectx)) return 0;
		_inner->SeekE(_begin + _pointer, SeekOrigin::Begin, ectx);
		if (ErrorTest(ectx)) return 0;
		auto read_fx = _inner->ReadE(data, read, ectx);
		_inner->SeekE(prev, SeekOrigin::Begin, lectx);
		if (ErrorTest(ectx)) return 0;
		_pointer += read_fx;
		return read_fx;
	}
	uintptr Substream::WriteE(const void * data, uintptr size, ErrorContext & ectx) noexcept { ErrorSet(ectx, Errores::ErrorNotImplemented); }
	uint64 Substream::SeekE(int64 position, SeekOrigin org, ErrorContext & ectx) noexcept
	{
		int64 newpos = position;
		if (org == SeekOrigin::Current) newpos += int64(_pointer);
		else if (org == SeekOrigin::End) newpos += int64(_size);
		else if (org != SeekOrigin::Begin) throw InvalidArgumentException();
		if (newpos < 0 || newpos > int64(_size)) throw InvalidArgumentException();
		_pointer = newpos;
		return _pointer;
	}
	uint64 Substream::GetLengthE(ErrorContext & ectx) noexcept { return _size; }
	void Substream::SetLengthE(const uint64 & length, ErrorContext & ectx) noexcept { ErrorSet(ectx, Errores::ErrorNotImplemented); }
	void Substream::FlushE(ErrorContext & ectx) noexcept {}

	void ITextEncoder::Write(const string & text) { ErrorContext ectx; ErrorClear(ectx); WriteE(text, ectx); ErrorThrow(ectx); }
	void ITextEncoder::WriteLine(const string & text) { ErrorContext ectx; ErrorClear(ectx); WriteLineE(text, ectx); ErrorThrow(ectx); }
	void ITextEncoder::LineFeed(void) { ErrorContext ectx; ErrorClear(ectx); LineFeedE(ectx); ErrorThrow(ectx); }
	void ITextEncoder::WriteEncodingSignature(void) { ErrorContext ectx; ErrorClear(ectx); WriteEncodingSignatureE(ectx); ErrorThrow(ectx); }
	ITextEncoder & ITextEncoder::operator << (const string & text) { Write(text); return *this; }
	unichar32 ITextDecoder::ReadCharacter(void) { ErrorContext ectx; ErrorClear(ectx); auto result = ReadCharacterE(ectx); ErrorThrow(ectx); return result; }
	string ITextDecoder::ReadLine(void) { ErrorContext ectx; ErrorClear(ectx); auto result = ReadLineE(ectx); ErrorThrow(ectx); return result; }
	string ITextDecoder::ReadAll(void) { ErrorContext ectx; ErrorClear(ectx); auto result = ReadAllE(ectx); ErrorThrow(ectx); return result; }
	ITextDecoder & ITextDecoder::operator >> (string & str) { str = ReadLine(); return *this; }

	TextEncoder::TextEncoder(Stream * dest) : _inner(dest), _enc(IO::TextFileEncoding), _enc_cp(0) {}
	TextEncoder::TextEncoder(Stream * dest, Unicode::Encoding enc) : _inner(dest), _enc(enc), _enc_cp(0) {}
	TextEncoder::TextEncoder(Stream * dest, const Unicode::EncodingCodepage & cp) : _inner(dest), _enc(Unicode::Encoding::Unknown), _enc_cp(&cp) {}
	TextEncoder::~TextEncoder(void) {}
	string TextEncoder::ToStringE(ErrorContext & ectx) const noexcept { ESSE_TRY_INTRO return U"Text Encoder"; ESSE_TRY_OUTRO(string()) }
	void TextEncoder::WriteE(const string & text, ErrorContext & ectx) noexcept
	{
		ESSE_TRY_INTRO
			oref<DataBlock> dw;
			if (_enc_cp) dw = EncodeString(text, *_enc_cp, false);
			else dw = EncodeString(text, _enc, false);
			auto written = _inner->WriteE(dw->GetBuffer(), dw->GetLength(), ectx);
			if (ErrorTest(ectx)) return;
			if (written != dw->GetLength()) { ErrorSet(ectx, Errores::ErrorOutOfMemory); return; }
		ESSE_TRY_OUTRO()
	}
	void TextEncoder::WriteLineE(const string & text, ErrorContext & ectx) noexcept{ WriteE(text, ectx); if (ErrorTest(ectx)) return; LineFeedE(ectx); }
	void TextEncoder::LineFeedE(ErrorContext & ectx) noexcept { ESSE_TRY_INTRO WriteE(IO::LineFeedSequence, ectx); ESSE_TRY_OUTRO() }
	void TextEncoder::WriteEncodingSignatureE(ErrorContext & ectx) noexcept
	{
		if (!_enc_cp && _enc != Unicode::Encoding::ASCII && _enc != Unicode::Encoding::Unknown) {
			uint8 data[4];
			uintptr w = 0;
			Unicode::WriteCharacter(&data, 4, w, Unicode::CharacterByteOrderMark, _enc, ectx);
			if (ErrorTest(ectx)) return;
			auto written = _inner->WriteE(&data, w, ectx);
			if (ErrorTest(ectx)) return;
			if (written != w) { ErrorSet(ectx, Errores::ErrorOutOfMemory); return; }
		}
	}

	TextDecoder::TextDecoder(Stream * src) : _inner(src), _enc(IO::TextFileEncoding), _dec_cp(0), _eof(false) {}
	TextDecoder::TextDecoder(Stream * src, Unicode::Encoding enc) : _inner(src), _enc(enc), _dec_cp(0), _eof(false) {}
	TextDecoder::TextDecoder(Stream * src, const Unicode::DecodingCodepage & cp) : _inner(src), _enc(Unicode::Encoding::Unknown), _dec_cp(&cp), _eof(false) {}
	TextDecoder::~TextDecoder(void) {}
	string TextDecoder::ToStringE(ErrorContext & ectx) const noexcept { ESSE_TRY_INTRO return U"Text Decoder"; ESSE_TRY_OUTRO(string()) }
	unichar32 TextDecoder::ReadCharacterE(ErrorContext & ectx) noexcept
	{
		if (_dec_cp) {
			uint8 byte;
			auto read = _inner->ReadE(&byte, 1, ectx);
			if (ErrorTest(ectx)) return Unicode::CharacterInvalid;
			if (!read) { _eof = true; return Unicode::CharacterInvalid; }
			uintptr p = 0;
			unichar32 result = Unicode::ReadCharacter(&byte, 1, p, *_dec_cp, ectx);
			if (ErrorTest(ectx)) return Unicode::CharacterInvalid;
			return result;
		} else {
			if (_enc == Unicode::Encoding::Unknown) {
				uint8 byte;
				auto read = _inner->ReadE(&byte, 1, ectx);
				if (ErrorTest(ectx)) return Unicode::CharacterInvalid;
				if (!read) { _eof = true; return Unicode::CharacterInvalid; }
				if (byte == 0xEF) {
					read = _inner->ReadE(&byte, 1, ectx);
					if (ErrorTest(ectx)) return Unicode::CharacterInvalid;
					if (!read) { _eof = true; return Unicode::CharacterInvalid; }
					if (byte == 0xFF) _enc = Unicode::Encoding::UTF16_BE;
					else if (byte == 0xBB) {
						read = _inner->ReadE(&byte, 1, ectx);
						if (ErrorTest(ectx)) return Unicode::CharacterInvalid;
						if (!read) { _eof = true; return Unicode::CharacterInvalid; }
						if (byte == 0xBF) _enc = Unicode::Encoding::UTF8;
						else { ErrorSet(ectx, Errores::ErrorInvalidFormat); return Unicode::CharacterInvalid; }
					} else { ErrorSet(ectx, Errores::ErrorInvalidFormat); return Unicode::CharacterInvalid; }
				} else if (byte == 0xFF) {
					read = _inner->ReadE(&byte, 1, ectx);
					if (ErrorTest(ectx)) return Unicode::CharacterInvalid;
					if (!read) { _eof = true; return Unicode::CharacterInvalid; }
					if (byte == 0xFE) _enc = Unicode::Encoding::UTF16_LE;
					else { ErrorSet(ectx, Errores::ErrorInvalidFormat); return Unicode::CharacterInvalid; }
				} else {
					_enc = Unicode::Encoding::ASCII;
					return byte;
				}
			}
			uintptr s = 0, r = 0;
			uint8 data[4];
			unichar32 result = Unicode::CharacterInvalid;
			while (s < 4) {
				auto read = _inner->ReadE(data + s, 1, ectx);
				if (ErrorTest(ectx)) return Unicode::CharacterInvalid;
				if (!read) { _eof = true; return Unicode::CharacterInvalid; }
				s += read;
				ErrorContext lectx; ErrorClear(lectx);
				unichar32 lresult = Unicode::ReadCharacter(data, s, r, _enc, lectx);
				if (!ErrorTest(lectx)) { result = lresult; break; }
			}
			if (result == Unicode::CharacterInvalid) { ErrorSet(ectx, Errores::ErrorInvalidFormat); return Unicode::CharacterInvalid; }
			return result;
		}
	}
	string TextDecoder::ReadLineE(ErrorContext & ectx) noexcept
	{
		ESSE_TRY_INTRO
			ErrorContext ectx; ErrorClear(ectx);
			dynamic_string_ucs4 result;
			unichar32 chr;
			do {
				chr = TextDecoder::ReadCharacterE(ectx);
				ErrorThrow(ectx);
				if (chr != Unicode::CharacterInvalid && (chr >= 0x20 || chr == U'\t')) result += chr;
			} while (chr != Unicode::CharacterInvalid && chr != U'\n');
			return result;
		ESSE_TRY_OUTRO(string());
	}
	string TextDecoder::ReadAllE(ErrorContext & ectx) noexcept
	{
		ESSE_TRY_INTRO
			ErrorContext ectx; ErrorClear(ectx);
			dynamic_string_ucs4 result;
			unichar32 chr;
			do {
				chr = TextDecoder::ReadCharacterE(ectx);
				ErrorThrow(ectx);
				if (chr != Unicode::CharacterInvalid) result += chr;
			} while (chr != Unicode::CharacterInvalid);
			return result;
		ESSE_TRY_OUTRO(string());
	}
	bool TextDecoder::IsEOF(void) noexcept { return _eof; }

	namespace IO {
		void CreateDirectoryTree(const string & path, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				auto full = ExpandPath(path, ectx); if (ErrorTest(ectx)) return;
				for (uintptr i = 0; i < full.GetLength(); i++) {
					if (full[i] == U'/' || full[i] == U'\\') {
						auto subpath = full.Substring(0, i);
						CreateDirectory(subpath, ectx);
						if (ErrorTest(ectx)) return;
					}
				}
			ESSE_TRY_OUTRO()
		}
		void RemoveEntireDirectory(const string & path, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				auto full = ExpandPath(path, ectx); if (ErrorTest(ectx)) return;
				auto files = EnumerateFiles(full, U"", FileSearch::FileSearchMainEntries | FileSearch::FileSearchAllowHidden, ectx); if (ErrorTest(ectx)) return;
				for (auto & f : *files) { RemoveFile(full + U"/" + f, ectx); if (ErrorTest(ectx)) return; }
				files = EnumerateFiles(full, U"", FileSearch::FileSearchDirectories | FileSearch::FileSearchAllowHidden, ectx); if (ErrorTest(ectx)) return;
				for (auto & f : *files) { RemoveEntireDirectory(full + U"/" + f, ectx); if (ErrorTest(ectx)) return; }
				RemoveDirectory(full, ectx);
			ESSE_TRY_OUTRO()
		}
		
		#define ESSE_WRAP_NOTHROW(INV) ErrorContext ectx; ErrorClear(ectx); auto result = INV; ErrorThrow(ectx); return result;
		#define ESSE_WRAP_NOTHROW_NORET(INV) ErrorContext ectx; ErrorClear(ectx); INV; ErrorThrow(ectx);

		void CreateDirectoryTree(const string & path) { ESSE_WRAP_NOTHROW_NORET(CreateDirectoryTree(path, ectx)) }
		void RemoveEntireDirectory(const string & path) { ESSE_WRAP_NOTHROW_NORET(RemoveEntireDirectory(path, ectx)) }
		handle DuplicateHandle(handle file) { ESSE_WRAP_NOTHROW(DuplicateHandle(file, ectx)) }
		handle CreateFile(const string & path, uint access, FileCreationMode mode) { ESSE_WRAP_NOTHROW(CreateFile(path, access, mode, ectx)) }
		void SetStandardHandle(StandardHandleType type, handle file) { ESSE_WRAP_NOTHROW_NORET(SetStandardHandle(type, file, ectx)) }
		void CreatePipe(handle & pipe_in, handle & pipe_out) { ESSE_WRAP_NOTHROW_NORET(CreatePipe(pipe_in, pipe_out, ectx)) }
		uintptr ReadFile(handle file, void * data, uintptr size) { ESSE_WRAP_NOTHROW(ReadFile(file, data, size, ectx)) }
		uintptr WriteFile(handle file, const void * data, uintptr size) { ESSE_WRAP_NOTHROW(WriteFile(file, data, size, ectx)) }
		uint64 SeekFile(handle file, int64 position, SeekOrigin org) { ESSE_WRAP_NOTHROW(SeekFile(file, position, org, ectx)) }
		uint64 GetFileSize(handle file) { ESSE_WRAP_NOTHROW(GetFileSize(file, ectx)) }
		void SetFileSize(handle file, uint64 size) { ESSE_WRAP_NOTHROW_NORET(SetFileSize(file, size, ectx)) }
		void Flush(handle file) { ESSE_WRAP_NOTHROW_NORET(Flush(file, ectx)) }
		void SetFilePermissions(handle file, uint access_user, uint access_group, uint access_world) { ESSE_WRAP_NOTHROW_NORET(SetFilePermissions(file, access_user, access_group, access_world, ectx)) }
		void GetFilePermissions(handle file, uint * access_user, uint * access_group, uint * access_world) { ESSE_WRAP_NOTHROW_NORET(GetFilePermissions(file, access_user, access_group, access_world, ectx)) }
		Time GetFileCreationTime(handle file) { ESSE_WRAP_NOTHROW(GetFileCreationTime(file, ectx)) }
		Time GetFileAccessTime(handle file) { ESSE_WRAP_NOTHROW(GetFileAccessTime(file, ectx)) }
		Time GetFileAlternationTime(handle file) { ESSE_WRAP_NOTHROW(GetFileAlternationTime(file, ectx)) }
		void SetFileCreationTime(handle file, Time time) { ESSE_WRAP_NOTHROW_NORET(SetFileCreationTime(file, time, ectx)) }
		void SetFileAccessTime(handle file, Time time) { ESSE_WRAP_NOTHROW_NORET(SetFileAccessTime(file, time, ectx)) }
		void SetFileAlternationTime(handle file, Time time) { ESSE_WRAP_NOTHROW_NORET(SetFileAlternationTime(file, time, ectx)) }
		FileType GetFileType(const string & file) { ESSE_WRAP_NOTHROW(GetFileType(file, ectx)) }
		void MoveFile(const string & from, const string & to) { ESSE_WRAP_NOTHROW_NORET(MoveFile(from, to, ectx)) }
		void RemoveFile(const string & path) { ESSE_WRAP_NOTHROW_NORET(RemoveFile(path, ectx)) }
		bool CreateDirectory(const string & path) { ESSE_WRAP_NOTHROW(CreateDirectory(path, ectx)) }
		void RemoveDirectory(const string & path) { ESSE_WRAP_NOTHROW_NORET(RemoveDirectory(path, ectx)) }
		void CreateSymbolicLink(const string & at, const string & to) { ESSE_WRAP_NOTHROW_NORET(CreateSymbolicLink(at, to, ectx)) }
		void CreateHardLink(const string & at, const string & to) { ESSE_WRAP_NOTHROW_NORET(CreateHardLink(at, to, ectx)) }
		string GetSymbolicLinkDestination(const string & file) { ESSE_WRAP_NOTHROW(GetSymbolicLinkDestination(file, ectx)) }
		void GetVolumeSpace(const string & volume, uint64 * total_bytes, uint64 * free_bytes, uint64 * user_available_bytes) { ESSE_WRAP_NOTHROW_NORET(GetVolumeSpace(volume, total_bytes, free_bytes, user_available_bytes, ectx)) }
		string ExpandPath(const string & path) { ESSE_WRAP_NOTHROW(ExpandPath(path, ectx)) }
		string GetExecutablePath(void) { ESSE_WRAP_NOTHROW(GetExecutablePath(ectx)) }
		string GetCurrentDirectory(void) { ESSE_WRAP_NOTHROW(GetCurrentDirectory(ectx)) }
		void SetCurrentDirectory(const string & path) { ESSE_WRAP_NOTHROW_NORET(SetCurrentDirectory(path, ectx)) }
		oref<array<string>> EnumerateFiles(const string & at_path, const string & filter, uint mode) { ESSE_WRAP_NOTHROW(EnumerateFiles(at_path, filter, mode, ectx)) }
		oref<array<VolumeDesc>> EnumerateVolumes(void) { ESSE_WRAP_NOTHROW(EnumerateVolumes(ectx)) }
	}
}