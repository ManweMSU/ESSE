#pragma once

#include <Cor/IO/CorStreams.h>
#include <Compressio/ChainCompression.h>

namespace ESSE
{
	namespace Formationes
	{
		namespace ArchiveFlags
		{
			constexpr uint Create32bit	= 0x001;
			constexpr uint Create64bit	= 0x000;
			constexpr uint UseMetadata	= 0x002;
			constexpr uint NoMetadata	= 0x000;
		}
		enum class ArchiveStream : uint { Native = 0, Decompressed = 1, Default = 2 };
		class Archive : public Object
		{
		public:
			virtual bool HasMetadata(void) noexcept = 0;
			virtual uint GetFileCount(void) noexcept = 0;
			virtual uint FindArchiveFile(const string & name) = 0;
			virtual uint FindArchiveFile(const unichar8 * type, uint32 file_id) = 0;
			virtual oref<Stream> QueryFileStream(uint file_no) = 0;
			virtual oref<Stream> QueryFileStream(uint file_no, ArchiveStream scls) = 0;
			virtual ucs4_string GetFileName(uint file_no) = 0;
			virtual ucs1_string GetFileType(uint file_no) = 0;
			virtual uint GetFileID(uint file_no) = 0;
			virtual uint GetFileUserData(uint file_no) = 0;
			virtual Time GetFileCreationTime(uint file_no) = 0;
			virtual Time GetFileAccessTime(uint file_no) = 0;
			virtual Time GetFileAlternationTime(uint file_no) = 0;
			virtual void GetFilePermissions(uint file_no, uint * access_user, uint * access_group, uint * access_world) = 0;
			virtual bool IsFileCompressed(uint file_no) = 0;
			virtual oref<array<string>> EnumerateFileAttributes(uint file_no) = 0;
			virtual string GetFileAttribute(uint file_no, const string & key) = 0;
			virtual oref<DataBlock> GetFileHash(uint file_no) = 0;

			static oref<Archive> Open(Stream * stream);
			static oref<Archive> Open(Stream * stream, uint metadata_mode);
		};
		class NewArchive : public Object
		{
		public:
			virtual bool IsLongFormat(void) noexcept = 0;
			virtual bool HasMetadata(void) noexcept = 0;
			virtual uint GetFileCount(void) noexcept = 0;
			virtual void SetFileName(uint file_no, const ucs4_string & name) = 0;
			virtual void SetFileType(uint file_no, const unichar8 * type) = 0;
			virtual void SetFileID(uint file_no, uint id) = 0;
			virtual void SetFileUserData(uint file_no, uint ud) = 0;
			virtual void SetFileCreationTime(uint file_no, Time time) = 0;
			virtual void SetFileAccessTime(uint file_no, Time time) = 0;
			virtual void SetFileAlternationTime(uint file_no, Time time) = 0;
			virtual void SetFilePermissions(uint file_no, uint access_user, uint access_group, uint access_world) = 0;
			virtual void SetFileCompressionFlag(uint file_no, bool compressed) = 0;
			virtual void SetFileAttribute(uint file_no, const string & key, const string & value) = 0;
			virtual void SetFileHash(uint file_no, const void * data, uintptr length) = 0;
			virtual void SetFileData(uint file_no, const void * data, uintptr length) = 0;
			virtual void SetFileData(uint file_no, Stream * source) = 0;
			virtual void SetFileData(uint file_no, Stream * source, const Compression::MethodChain * chains, uintptr nchains, Compression::Quality quality, ThreadPool * pool = 0) = 0;
			virtual void SetFileData(uint file_no, Stream * source, const Compression::MethodChain * chains, uintptr nchains, Compression::Quality quality, ThreadPool * pool, uint32 block_size) = 0;
			virtual void Finalize(void) = 0;

			static oref<NewArchive> Create(Stream * stream, uint number_of_files, uint format_flags = ArchiveFlags::Create64bit | ArchiveFlags::UseMetadata);
		};
	}
}