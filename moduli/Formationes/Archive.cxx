#include "Archive.h"
#include "Registry.h"

namespace ESSE
{
	namespace Formationes
	{
		namespace Format
		{
			ESSE_PACKED_STRUCTURE(ArchiveHeaderBase)
				uint8 signature[8];
				uint32 signature_ex;
				uint32 extra_word;
			ESSE_END_PACKED_STRUCTURE
			ESSE_PACKED_STRUCTURE(ArchiveHeader32)
				uint8 signature[8];		// ecs.1.0
				uint32 signature_ex;	// 0x80000005
				uint32 file_count;
				uint32 base_offset;
				uint32 strings_offset;
				uint32 strings_size;
			ESSE_END_PACKED_STRUCTURE
			ESSE_PACKED_STRUCTURE(ArchiveHeader64)
				uint8 signature[8];		// ecs.1.0
				uint32 signature_ex;	// 0xC0000005
				uint32 reserved;		// 0
				uint64 file_count;
				uint64 base_offset;
				uint64 strings_offset;
				uint64 strings_size;
				uint64 unused_1;		// 0
				uint64 unused_2;		// 0
			ESSE_END_PACKED_STRUCTURE
			ESSE_PACKED_STRUCTURE(ArchiveFileHeader32)
				uint32 file_offset;
				uint32 file_size;
				uint32 file_type;
				uint32 file_id;
				uint32 user_data;
				uint32 name_offset;
			ESSE_END_PACKED_STRUCTURE
			ESSE_PACKED_STRUCTURE(ArchiveFileHeader64)
				uint64 file_offset;
				uint64 file_size;
				uint32 file_type;
				uint32 file_id;
				uint32 user_data;
				uint32 name_offset;
				uint64 unused_1;		// 0
				uint64 unused_2;		// 0
			ESSE_END_PACKED_STRUCTURE
		}
		void ArchiveValidateMemoryString(const uint8 * data, uintptr size, uintptr data_at)
		{
			if (data_at > size || size - data_at < 2) throw InvalidFormatException();
			auto pos = data_at;
			while (size - pos >= 2 && *reinterpret_cast<const uint16 *>(data + pos)) pos += 2;
			if (size - pos < 2) throw InvalidFormatException();
		}
		string ArchiveNormalizeFileName(const string & name)
		{
			dynamic_string_ucs4 result;
			uintptr i = 0;
			while (i < name.GetLength()) {
				while (name[i] == U'\\' || name[i] == U'/') i++;
				while (name[i] != U'\\' && name[i] != U'/' && name[i]) result += name[i++];
				if (name[i]) { result += U'\\'; i++; }
			}
			return result;
		}
		ucs1_string ArchiveReadFileType(uint type)
		{
			auto ptype = reinterpret_cast<const unichar8 *>(&type);
			uintptr length = 0;
			while (length < 4 && ptype[length]) length++;
			return ucs1_string(ptype, length);
		}
		uint ArchiveMakeFileType(const unichar8 * type) noexcept
		{
			if (!type) return 0;
			uintptr length = 0;
			uint result = 0;
			while (length < 4 && type[length] && type[length] != ' ') length++;
			if (length) Memory::MemoryCopy(&result, type, length);
			return result;
		}
		class RegularArchive : public Archive
		{
			struct _file_desc
			{
				uint64 offset, size;
				uint32 type, id, ud;
				string name;
			};
		private:
			oref<Stream> _inner;
			oref<Registry> _metadata;
			array<_file_desc> _files;
		private:
			template <class ArchiveHeader, class ArchiveFileHeader> void _load_archive_data(void)
			{
				ArchiveHeader hdr;
				array<ArchiveFileHeader> fhdr(1);
				DataBlock strings(1);
				if (_inner->Read(&hdr, sizeof(hdr)) != sizeof(hdr)) throw InvalidFormatException();
				fhdr.SetLength(hdr.file_count);
				strings.SetLength(hdr.strings_size);
				if (_inner->Read(fhdr.GetBuffer(), fhdr.GetLength() * sizeof(*fhdr)) != fhdr.GetLength() * sizeof(*fhdr)) throw InvalidFormatException();
				_inner->Seek(hdr.strings_offset, SeekOrigin::Begin);
				if (_inner->Read(strings.GetBuffer(), strings.GetLength()) != strings.GetLength()) throw InvalidFormatException();
				_files.SetLength(fhdr.GetLength());
				for (uintptr i = 0; i < _files.GetLength(); i++) {
					ArchiveValidateMemoryString(strings.GetBuffer(), strings.GetLength(), fhdr[i].name_offset);
					_files[i].offset = fhdr[i].file_offset + hdr.base_offset;
					_files[i].size = fhdr[i].file_size;
					_files[i].type = fhdr[i].file_type;
					_files[i].id = fhdr[i].file_id;
					_files[i].ud = fhdr[i].user_data;
					_files[i].name = string(reinterpret_cast<const unichar16 *>(strings.GetBuffer() + fhdr[i].name_offset));
				}
			}
		public:
			RegularArchive(Stream * stream, uint metadata_mode) : _inner(stream), _files(1)
			{
				if (!stream) throw InvalidArgumentException();
				Format::ArchiveHeaderBase hdr;
				_inner->Seek(0, SeekOrigin::Begin);
				if (_inner->Read(&hdr, sizeof(hdr)) != sizeof(hdr)) throw InvalidFormatException();
				_inner->Seek(0, SeekOrigin::Begin);
				if (Memory::MemoryCompare(&hdr.signature, "ecs.1.0", 8) != 0) throw InvalidFormatException();
				if (hdr.signature_ex == 0x80000005) {
					_load_archive_data<Format::ArchiveHeader32, Format::ArchiveFileHeader32>();
				} else if (hdr.signature_ex == 0xC0000005) {
					if (hdr.extra_word) throw InvalidFormatException();
					_load_archive_data<Format::ArchiveHeader64, Format::ArchiveFileHeader64>();
				} else throw InvalidFormatException();
				if (metadata_mode & ArchiveFlags::UseMetadata && _files.GetLength()) {
					auto & last = _files.LastElement();
					if (last.name.GetLength() == 0 && Memory::MemoryCompare(&last.type, "AMDE", 4) == 0 && last.id == 0xFFFFFFFF && last.ud == 0) {
						auto metadata_stream = Substream::Create(_inner, last.offset, last.size);
						_metadata = Registry::Load(metadata_stream);
					}
				}
			}
			virtual ~RegularArchive(void) override {}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"Archive"; ESSE_TRY_OUTRO(string()) }
			virtual bool HasMetadata(void) noexcept override { return _metadata; }
			virtual uint GetFileCount(void) noexcept override { return _files.GetLength(); }
			virtual uint FindArchiveFile(const string & name) override
			{
				auto normalized = ArchiveNormalizeFileName(name);
				if (!normalized.GetLength()) return 0;
				for (uintptr i = 0; i < _files.GetLength(); i++) {
					ErrorContext ectx; ErrorClear(ectx);
					auto cmp = Unicode::CaseInsensitiveCompare(normalized, normalized.GetLength(), _files[i].name, _files[i].name.GetLength(), ectx);
					if (!ErrorTest(ectx) && cmp == 0) return i + 1;
				}
				return 0;
			}
			virtual uint FindArchiveFile(const unichar8 * type, uint32 file_id) override
			{
				auto file_type = ArchiveMakeFileType(type);
				if (!file_type || !file_id) return 0;
				for (uintptr i = 0; i < _files.GetLength(); i++) if (_files[i].type == file_type && _files[i].id == file_id) return i + 1;
				return 0;
			}
			virtual oref<Stream> QueryFileStream(uint file_no) override { return QueryFileStream(file_no, ArchiveStream::Default); }
			virtual oref<Stream> QueryFileStream(uint file_no, ArchiveStream scls) override
			{
				if (!file_no || file_no > _files.GetLength()) throw InvalidArgumentException();
				if (scls == ArchiveStream::Native) {
					return oref<Stream>(Substream::Create(_inner, _files[file_no - 1].offset, _files[file_no - 1].size));
				} else if (scls == ArchiveStream::Decompressed) {
					auto com = Substream::Create(_inner, _files[file_no - 1].offset, _files[file_no - 1].size);
					return oref<Stream>(Compression::CreateDecompressionStream(com));
				} else if (scls == ArchiveStream::Default) {
					if (IsFileCompressed(file_no)) return QueryFileStream(file_no, ArchiveStream::Decompressed);
					else return QueryFileStream(file_no, ArchiveStream::Native);
				} else throw InvalidArgumentException();
			}
			virtual ucs4_string GetFileName(uint file_no) override
			{
				if (!file_no || file_no > _files.GetLength()) throw InvalidArgumentException();
				return _files[file_no - 1].name;
			}
			virtual ucs1_string GetFileType(uint file_no) override
			{
				if (!file_no || file_no > _files.GetLength()) throw InvalidArgumentException();
				return ArchiveReadFileType(_files[file_no - 1].type);
			}
			virtual uint GetFileID(uint file_no) override
			{
				if (!file_no || file_no > _files.GetLength()) throw InvalidArgumentException();
				return _files[file_no - 1].id;
			}
			virtual uint GetFileUserData(uint file_no) override
			{
				if (!file_no || file_no > _files.GetLength()) throw InvalidArgumentException();
				return _files[file_no - 1].ud;
			}
			virtual Time GetFileCreationTime(uint file_no) override
			{
				if (!file_no || file_no > _files.GetLength()) throw InvalidArgumentException();
				if (_metadata) return _metadata->GetValueTime(string(file_no) + U"/Creation Time"); else return 0;
			}
			virtual Time GetFileAccessTime(uint file_no) override
			{
				if (!file_no || file_no > _files.GetLength()) throw InvalidArgumentException();
				if (_metadata) return _metadata->GetValueTime(string(file_no) + U"/Access Time"); else return 0;
			}
			virtual Time GetFileAlternationTime(uint file_no) override
			{
				if (!file_no || file_no > _files.GetLength()) throw InvalidArgumentException();
				if (_metadata) return _metadata->GetValueTime(string(file_no) + U"/Alter Time"); else return 0;
			}
			virtual void GetFilePermissions(uint file_no, uint * access_user, uint * access_group, uint * access_world) override
			{
				if (!file_no || file_no > _files.GetLength()) throw InvalidArgumentException();
				uint permissions;
				if (_metadata) permissions = _metadata->GetValueInteger(string(file_no) + U"/Permissions"); else permissions = 0;
				if (access_user) *access_user = (permissions >> 6) & 7;
				if (access_group) *access_group = (permissions >> 3) & 7;
				if (access_world) *access_world = permissions & 7;
			}
			virtual bool IsFileCompressed(uint file_no) override
			{
				if (!file_no || file_no > _files.GetLength()) throw InvalidArgumentException();
				if (_metadata) return _metadata->GetValueBoolean(string(file_no) + U"/Compressed"); else return false;
			}
			virtual oref<array<string>> EnumerateFileAttributes(uint file_no) override
			{
				if (!file_no || file_no > _files.GetLength()) throw InvalidArgumentException();
				auto result = owrap(new array<string>(0x10));
				if (_metadata) {
					auto node = _metadata->OpenNode(string(file_no));
					if (node) for (auto & v : node->GetValues()) if (v[0] == U'_') result->Append(v.Substring(1, -1));
				}
				return result;
			}
			virtual string GetFileAttribute(uint file_no, const string & key) override
			{
				if (!file_no || file_no > _files.GetLength()) throw InvalidArgumentException();
				if (_metadata) return _metadata->GetValueString(string(file_no) + U"/_" + key); else return string();
			}
			virtual oref<DataBlock> GetFileHash(uint file_no) override
			{
				if (!file_no || file_no > _files.GetLength()) throw InvalidArgumentException();
				auto result = owrap(new DataBlock(1));
				if (_metadata) {
					auto name = string(file_no) + U"/Hash";
					result->SetLength(_metadata->GetValueBinarySize(name));
					_metadata->GetValueBinary(name, result->GetBuffer());
				}
				return result;
			}
		};
		class RegularNewArchive : public NewArchive
		{
			struct _file_desc
			{
				uint64 data_offset, data_size;
				uint32 name_offset, type, id, ud, mask;
			};
		private:
			oref<Stream> _stream;
			oref<Registry> _metadata;
			array<_file_desc> _files;
			array<unichar16> _strings;
			uint64 _base_offset;
			uint32 _header_size, _file_header_size;
			bool _long_mode;
		private:
			uint32 _encode_string(const string & text)
			{
				ucs2_string ucs2 = text;
				if (ucs2.GetLength() < _strings.GetLength()) {
					for (uintptr i = 0; i < _strings.GetLength() - ucs2.GetLength(); i++) {
						if (Memory::MemoryCompare(ucs2.GetData(), _strings.GetBuffer() + i, (ucs2.GetLength() + 1U) << 1U) == 0) return i << 1U;
					}
				}
				uint32 offset = _strings.GetLength() << 1U;
				_strings.Append(ucs2.GetData(), ucs2.GetLength() + 1);
				return offset;
			}
			void _set_compression_flag(uint file_no, bool compressed)
			{
				if (_metadata) {
					auto nn = string(file_no);
					try { _metadata->CreateNode(nn); } catch (...) {}
					auto node = _metadata->OpenNode(nn);
					if (!node) throw OutOfMemoryException();
					try { node->CreateValue(U"Compressed", RegistryValueType::Boolean); } catch (...) {}
					node->SetValue(U"Compressed", compressed);
				}
			}
			template <class ArchiveHeader, class ArchiveFileHeader, class Size> void _finalize(void)
			{
				ArchiveHeader hdr;
				Memory::ZeroMemory(&hdr, sizeof(hdr));
				Memory::MemoryCopy(&hdr.signature, "ecs.1.0", 8);
				hdr.signature_ex = _long_mode ? 0xC0000005 : 0x80000005;
				hdr.file_count = _files.GetLength();
				hdr.base_offset = Size(_base_offset);
				hdr.strings_offset = Size(_stream->Seek(0, SeekOrigin::Current));
				hdr.strings_size = Size(_strings.GetLength() << 1U);
				_stream->Write(_strings.GetBuffer(), _strings.GetLength() << 1U);
				auto end = _stream->Seek(0, SeekOrigin::Current);
				_stream->Seek(0, SeekOrigin::Begin);
				_stream->Write(&hdr, sizeof(hdr));
				for (auto & f : _files) {
					ArchiveFileHeader fhdr;
					Memory::ZeroMemory(&fhdr, sizeof(fhdr));
					fhdr.file_offset = Size(f.data_offset);
					fhdr.file_size = Size(f.data_size);
					fhdr.file_type = f.type;
					fhdr.file_id = f.id;
					fhdr.user_data = f.ud;
					fhdr.name_offset = f.name_offset;
					_stream->Write(&fhdr, sizeof(fhdr));
				}
				_stream->Seek(end, SeekOrigin::Begin);
			}
		public:
			RegularNewArchive(Stream * stream, uint number_of_files, uint format_flags) : _stream(stream), _files(1), _strings(0x400)
			{
				if (!stream || !number_of_files) throw InvalidArgumentException();
				if (format_flags & ArchiveFlags::UseMetadata) _metadata = Registry::Create();
				if (format_flags & ArchiveFlags::Create32bit) _long_mode = false; else _long_mode = true;
				if (_long_mode) {
					_header_size = sizeof(Format::ArchiveHeader64);
					_file_header_size = sizeof(Format::ArchiveFileHeader64);
				} else {
					_header_size = sizeof(Format::ArchiveHeader32);
					_file_header_size = sizeof(Format::ArchiveFileHeader32);
				}
				if (_metadata) number_of_files++;
				_files.SetLength(number_of_files);
				Memory::ZeroMemory(_files.GetBuffer(), _files.GetLength() * sizeof(*_files));
				_base_offset = _header_size + number_of_files * _file_header_size;
				_stream->SetLength(_base_offset);
				_stream->Seek(_base_offset, SeekOrigin::Begin);
			}
			virtual ~RegularNewArchive(void) override {}
			virtual string ToStringE(ErrorContext & ectx) const noexcept override { ESSE_TRY_INTRO return U"New archive"; ESSE_TRY_OUTRO(string()) }
			virtual bool IsLongFormat(void) noexcept override { return _long_mode; }
			virtual bool HasMetadata(void) noexcept override { return _metadata; }
			virtual uint GetFileCount(void) noexcept override { return _files.GetLength() - (_metadata ? 1 : 0); }
			virtual void SetFileName(uint file_no, const ucs4_string & name) override
			{
				if (!file_no || file_no > GetFileCount()) throw InvalidArgumentException();
				if (!_stream) throw InvalidStateException();
				if (_files[file_no - 1].mask & 2) throw InvalidStateException();
				_files[file_no - 1].name_offset = _encode_string(ArchiveNormalizeFileName(name));
				_files[file_no - 1].mask |= 2;
			}
			virtual void SetFileType(uint file_no, const unichar8 * type) override
			{
				if (!file_no || file_no > GetFileCount()) throw InvalidArgumentException();
				if (!_stream) throw InvalidStateException();
				_files[file_no - 1].type = ArchiveMakeFileType(type);
			}
			virtual void SetFileID(uint file_no, uint id) override
			{
				if (!file_no || file_no > GetFileCount()) throw InvalidArgumentException();
				if (!_stream) throw InvalidStateException();
				_files[file_no - 1].id = id;
			}
			virtual void SetFileUserData(uint file_no, uint ud) override
			{
				if (!file_no || file_no > GetFileCount()) throw InvalidArgumentException();
				if (!_stream) throw InvalidStateException();
				_files[file_no - 1].ud = ud;
			}
			virtual void SetFileCreationTime(uint file_no, Time time) override
			{
				if (!file_no || file_no > GetFileCount()) throw InvalidArgumentException();
				if (!_stream) throw InvalidStateException();
				if (_metadata) {
					auto nn = string(file_no);
					try { _metadata->CreateNode(nn); } catch (...) {}
					auto node = _metadata->OpenNode(nn);
					if (!node) throw OutOfMemoryException();
					try { node->CreateValue(U"Creation Time", RegistryValueType::Time); } catch (...) {}
					node->SetValue(U"Creation Time", time);
				}
			}
			virtual void SetFileAccessTime(uint file_no, Time time) override
			{
				if (!file_no || file_no > GetFileCount()) throw InvalidArgumentException();
				if (!_stream) throw InvalidStateException();
				if (_metadata) {
					auto nn = string(file_no);
					try { _metadata->CreateNode(nn); } catch (...) {}
					auto node = _metadata->OpenNode(nn);
					if (!node) throw OutOfMemoryException();
					try { node->CreateValue(U"Access Time", RegistryValueType::Time); } catch (...) {}
					node->SetValue(U"Access Time", time);
				}
			}
			virtual void SetFileAlternationTime(uint file_no, Time time) override
			{
				if (!file_no || file_no > GetFileCount()) throw InvalidArgumentException();
				if (!_stream) throw InvalidStateException();
				if (_metadata) {
					auto nn = string(file_no);
					try { _metadata->CreateNode(nn); } catch (...) {}
					auto node = _metadata->OpenNode(nn);
					if (!node) throw OutOfMemoryException();
					try { node->CreateValue(U"Alter Time", RegistryValueType::Time); } catch (...) {}
					node->SetValue(U"Alter Time", time);
				}
			}
			virtual void SetFilePermissions(uint file_no, uint access_user, uint access_group, uint access_world) override
			{
				if (!file_no || file_no > GetFileCount()) throw InvalidArgumentException();
				if (!_stream) throw InvalidStateException();
				if (_metadata) {
					auto nn = string(file_no);
					try { _metadata->CreateNode(nn); } catch (...) {}
					auto node = _metadata->OpenNode(nn);
					if (!node) throw OutOfMemoryException();
					int permissions = ((access_user & 7) << 6) | ((access_group & 7) << 3) | (access_world & 7);
					try { node->CreateValue(U"Permissions", RegistryValueType::Integer); } catch (...) {}
					node->SetValue(U"Permissions", permissions);
				}
			}
			virtual void SetFileCompressionFlag(uint file_no, bool compressed) override
			{
				if (!file_no || file_no > GetFileCount()) throw InvalidArgumentException();
				if (!_stream) throw InvalidStateException();
				if (_files[file_no - 1].mask & 1) throw InvalidStateException();
				_set_compression_flag(file_no, compressed);
			}
			virtual void SetFileAttribute(uint file_no, const string & key, const string & value) override
			{
				if (!file_no || file_no > GetFileCount()) throw InvalidArgumentException();
				if (!_stream) throw InvalidStateException();
				if (_metadata) {
					auto nn = string(file_no);
					try { _metadata->CreateNode(nn); } catch (...) {}
					auto node = _metadata->OpenNode(nn);
					if (!node) throw OutOfMemoryException();
					auto path = U"_" + key;
					try { node->CreateValue(path, RegistryValueType::String); } catch (...) {}
					node->SetValue(path, value);
				}
			}
			virtual void SetFileHash(uint file_no, const void * data, uintptr length) override
			{
				if (!file_no || file_no > GetFileCount()) throw InvalidArgumentException();
				if (!_stream) throw InvalidStateException();
				if (_metadata) {
					auto nn = string(file_no);
					try { _metadata->CreateNode(nn); } catch (...) {}
					auto node = _metadata->OpenNode(nn);
					if (!node) throw OutOfMemoryException();
					try { node->CreateValue(U"Hash", RegistryValueType::Binary); } catch (...) {}
					node->SetValue(U"Hash", data, length);
				}
			}
			virtual void SetFileData(uint file_no, const void * data, uintptr length) override
			{
				if (!file_no || file_no > GetFileCount()) throw InvalidArgumentException();
				if (!_stream) throw InvalidStateException();
				if (_files[file_no - 1].mask & 1) throw InvalidStateException();
				_files[file_no - 1].data_offset = _stream->Seek(0, SeekOrigin::Current) - _base_offset;
				_files[file_no - 1].data_size = length;
				_files[file_no - 1].mask |= 1;
				_stream->Write(data, length);
			}
			virtual void SetFileData(uint file_no, Stream * source) override
			{
				if (!file_no || file_no > GetFileCount() || !source) throw InvalidArgumentException();
				if (!_stream) throw InvalidStateException();
				if (_files[file_no - 1].mask & 1) throw InvalidStateException();
				auto position_begin = _stream->Seek(0, SeekOrigin::Current);
				source->CopyToUntilEof(_stream);
				auto position_end = _stream->Seek(0, SeekOrigin::Current);
				_files[file_no - 1].data_offset = position_begin - _base_offset;
				_files[file_no - 1].data_size = position_end - position_begin;
				_files[file_no - 1].mask |= 1;
			}
			virtual void SetFileData(uint file_no, Stream * source, const Compression::MethodChain * chains, uintptr nchains, Compression::Quality quality, ThreadPool * pool) override
			{
				if (!file_no || file_no > GetFileCount() || !source) throw InvalidArgumentException();
				if (!_stream) throw InvalidStateException();
				if (_files[file_no - 1].mask & 1) throw InvalidStateException();
				auto position_begin = _stream->Seek(0, SeekOrigin::Current);
				Compression::ChainCompress(_stream, source, chains, nchains, quality, pool, 0x10000);
				auto position_end = _stream->Seek(0, SeekOrigin::Current);
				_files[file_no - 1].data_offset = position_begin - _base_offset;
				_files[file_no - 1].data_size = position_end - position_begin;
				_files[file_no - 1].mask |= 1;
				_set_compression_flag(file_no, true);
			}
			virtual void SetFileData(uint file_no, Stream * source, const Compression::MethodChain * chains, uintptr nchains, Compression::Quality quality, ThreadPool * pool, uint32 block_size) override
			{
				if (!file_no || file_no > GetFileCount() || !source) throw InvalidArgumentException();
				if (!_stream) throw InvalidStateException();
				if (_files[file_no - 1].mask & 1) throw InvalidStateException();
				auto position_begin = _stream->Seek(0, SeekOrigin::Current);
				Compression::ChainCompress(_stream, source, chains, nchains, quality, pool, block_size);
				auto position_end = _stream->Seek(0, SeekOrigin::Current);
				_files[file_no - 1].data_offset = position_begin - _base_offset;
				_files[file_no - 1].data_size = position_end - position_begin;
				_files[file_no - 1].mask |= 1;
				_set_compression_flag(file_no, true);
			}
			virtual void Finalize(void) override
			{
				if (!_stream) throw InvalidStateException();
				for (uint i = 1; i <= GetFileCount(); i++) {
					if (!(_files[i - 1].mask & 2)) SetFileName(i, L"");
					if (!(_files[i - 1].mask & 1)) { _files[i - 1].data_offset = _files[i - 1].data_size = 0; }
				}
				if (_metadata) {
					auto memory = MemoryStream::Create(0x1000);
					_metadata->Save(memory);
					_files.LastElement().name_offset = _encode_string(string());
					_files.LastElement().type = ArchiveMakeFileType("AMDE");
					_files.LastElement().id = 0xFFFFFFFF;
					_files.LastElement().ud = 0;
					_files.LastElement().data_offset = _stream->Seek(0, SeekOrigin::Current) - _base_offset;
					_files.LastElement().data_size = memory->GetLength();
					_files.LastElement().mask |= 3;
					_stream->WriteBlock(memory->GetStorage());
				}
				auto pos = _stream->Seek(0, SeekOrigin::Current);
				if (!_long_mode && pos + (_strings.GetLength() << 1U) > 0xFFFFFFFF) throw InvalidFormatException();
				if (_long_mode) _finalize<Format::ArchiveHeader64, Format::ArchiveFileHeader64, uint64>();
				else _finalize<Format::ArchiveHeader32, Format::ArchiveFileHeader32, uint32>();
				_stream.Clear();
			}
		};

		oref<Archive> Archive::Open(Stream * stream) { return oref<Archive>::CreateOwned(new RegularArchive(stream, ArchiveFlags::UseMetadata)); }
		oref<Archive> Archive::Open(Stream * stream, uint metadata_mode) { return oref<Archive>::CreateOwned(new RegularArchive(stream, metadata_mode)); }
		oref<NewArchive> NewArchive::Create(Stream * stream, uint number_of_files, uint format_flags) { return oref<NewArchive>::CreateOwned(new RegularNewArchive(stream, number_of_files, format_flags)); }
	}
}