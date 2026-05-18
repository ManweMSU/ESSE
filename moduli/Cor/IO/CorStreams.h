#pragma once

#include "CorIO.h"

namespace ESSE
{
	class Stream : public Object
	{
	public:
		virtual uintptr ReadE(void * data, uintptr size, ErrorContext & ectx) noexcept = 0;
		virtual uintptr WriteE(const void * data, uintptr size, ErrorContext & ectx) noexcept = 0;
		virtual uint64 SeekE(int64 position, SeekOrigin org, ErrorContext & ectx) noexcept = 0;
		virtual uint64 GetLengthE(ErrorContext & ectx) noexcept = 0;
		virtual void SetLengthE(const uint64 & length, ErrorContext & ectx) noexcept = 0;
		virtual void FlushE(ErrorContext & ectx) noexcept = 0;

		uintptr Read(void * data, uintptr size);
		uintptr Write(const void * data, uintptr size);
		int64 Seek(uint64 position, SeekOrigin org);
		uint64 GetLength(void);
		void SetLength(uint64 length);
		void Flush(void);

		void CopyTo(Stream * to, uint64 length);
		void CopyToUntilEof(Stream * to);
		oref<DataBlock> ReadAll(void);
		oref<DataBlock> ReadBlock(uintptr length);
		void WriteBlock(const DataBlock * data);
		void RelocateData(uint64 offset_from, uint64 offset_to, uint64 length);
	};
	class FileStream final : public Stream
	{
		handle _file;
		bool _owned;
	private:
		FileStream(handle file, bool owned);
	public:
		static oref<FileStream> Create(const string & path, uint access, FileCreationMode mode);
		static oref<FileStream> CreateWrapper(handle file, bool take_ownership = false);
		static oref<FileStream> CreateFromDuplicate(handle file);
		virtual ~FileStream(void) override;
		virtual string ToStringE(ErrorContext & ectx) const noexcept override;
		virtual uintptr ReadE(void * data, uintptr size, ErrorContext & ectx) noexcept override;
		virtual uintptr WriteE(const void * data, uintptr size, ErrorContext & ectx) noexcept override;
		virtual uint64 SeekE(int64 position, SeekOrigin org, ErrorContext & ectx) noexcept override;
		virtual uint64 GetLengthE(ErrorContext & ectx) noexcept override;
		virtual void SetLengthE(const uint64 & length, ErrorContext & ectx) noexcept override;
		virtual void FlushE(ErrorContext & ectx) noexcept override;
		handle GetIOHandle(void) const noexcept;
	};
	class MemoryStream final : public Stream
	{
		oref<DataBlock> _data;
		uintptr _pointer;
	private:
		MemoryStream(DataBlock * data);
	public:
		static oref<MemoryStream> Create(uintptr block_size);
		static oref<MemoryStream> Create(const void * memory, uintptr size);
		static oref<MemoryStream> Create(const void * memory, uintptr size, uintptr block_size);
		static oref<MemoryStream> CreateFromData(DataBlock * data);
		virtual ~MemoryStream(void) override;
		virtual string ToStringE(ErrorContext & ectx) const noexcept override;
		virtual uintptr ReadE(void * data, uintptr size, ErrorContext & ectx) noexcept override;
		virtual uintptr WriteE(const void * data, uintptr size, ErrorContext & ectx) noexcept override;
		virtual uint64 SeekE(int64 position, SeekOrigin org, ErrorContext & ectx) noexcept override;
		virtual uint64 GetLengthE(ErrorContext & ectx) noexcept override;
		virtual void SetLengthE(const uint64 & length, ErrorContext & ectx) noexcept override;
		virtual void FlushE(ErrorContext & ectx) noexcept override;
		DataBlock * GetStorage(void) noexcept;
		const DataBlock * GetStorage(void) const noexcept;
		void * GetData(void) noexcept;
		const void * GetData(void) const noexcept;
	};
	class StaticMemoryStream final : public Stream
	{
		const uint8 * _data;
		uintptr _size, _pointer;
	private:
		StaticMemoryStream(const uint8 * data, uintptr size);
	public:
		static oref<StaticMemoryStream> Create(const void * memory, uintptr size);
		virtual ~StaticMemoryStream(void) override;
		virtual string ToStringE(ErrorContext & ectx) const noexcept override;
		virtual uintptr ReadE(void * data, uintptr size, ErrorContext & ectx) noexcept override;
		virtual uintptr WriteE(const void * data, uintptr size, ErrorContext & ectx) noexcept override;
		virtual uint64 SeekE(int64 position, SeekOrigin org, ErrorContext & ectx) noexcept override;
		virtual uint64 GetLengthE(ErrorContext & ectx) noexcept override;
		virtual void SetLengthE(const uint64 & length, ErrorContext & ectx) noexcept override;
		virtual void FlushE(ErrorContext & ectx) noexcept override;
		const void * GetData(void) const noexcept;
	};
	class Substream final : public Stream
	{
		oref<Stream> _inner;
		uint64 _begin, _end, _size, _pointer;
	private:
		Substream(Stream * stream, uint64 offset, uint64 size);
	public:
		static oref<Substream> Create(Stream * stream, uint64 offset, uint64 size);
		virtual ~Substream(void) override;
		virtual string ToStringE(ErrorContext & ectx) const noexcept override;
		virtual uintptr ReadE(void * data, uintptr size, ErrorContext & ectx) noexcept override;
		virtual uintptr WriteE(const void * data, uintptr size, ErrorContext & ectx) noexcept override;
		virtual uint64 SeekE(int64 position, SeekOrigin org, ErrorContext & ectx) noexcept override;
		virtual uint64 GetLengthE(ErrorContext & ectx) noexcept override;
		virtual void SetLengthE(const uint64 & length, ErrorContext & ectx) noexcept override;
		virtual void FlushE(ErrorContext & ectx) noexcept override;
	};

	class ITextEncoder : public Object
	{
	public:
		virtual void WriteE(const string & text, ErrorContext & ectx) noexcept = 0;
		virtual void WriteLineE(const string & text, ErrorContext & ectx) noexcept = 0;
		virtual void LineFeedE(ErrorContext & ectx) noexcept = 0;
		virtual void WriteEncodingSignatureE(ErrorContext & ectx) noexcept = 0;

		void Write(const string & text);
		void WriteLine(const string & text);
		void LineFeed(void);
		void WriteEncodingSignature(void);
		ITextEncoder & operator << (const string & text);
	};
	class ITextDecoder : public Object
	{
	public:
		virtual unichar32 ReadCharacterE(ErrorContext & ectx) noexcept = 0;
		virtual string ReadLineE(ErrorContext & ectx) noexcept = 0;
		virtual string ReadAllE(ErrorContext & ectx) noexcept = 0;
		virtual bool IsEOF(void) noexcept = 0;

		unichar32 ReadCharacter(void);
		string ReadLine(void);
		string ReadAll(void);
		ITextDecoder & operator >> (string & str);
	};
	class TextEncoder final : public ITextEncoder
	{
		oref<Stream> _inner;
		Unicode::Encoding _enc;
		const Unicode::EncodingCodepage * _enc_cp;
	public:
		TextEncoder(Stream * dest);
		TextEncoder(Stream * dest, Unicode::Encoding enc);
		TextEncoder(Stream * dest, const Unicode::EncodingCodepage & cp);
		virtual ~TextEncoder(void) override;
		virtual string ToStringE(ErrorContext & ectx) const noexcept override;
		virtual void WriteE(const string & text, ErrorContext & ectx) noexcept override;
		virtual void WriteLineE(const string & text, ErrorContext & ectx) noexcept override;
		virtual void LineFeedE(ErrorContext & ectx) noexcept override;
		virtual void WriteEncodingSignatureE(ErrorContext & ectx) noexcept override;
	};
	class TextDecoder final : public ITextDecoder
	{
		oref<Stream> _inner;
		Unicode::Encoding _enc;
		const Unicode::DecodingCodepage * _dec_cp;
		bool _eof;
	public:
		TextDecoder(Stream * src);
		TextDecoder(Stream * src, Unicode::Encoding enc);
		TextDecoder(Stream * src, const Unicode::DecodingCodepage & cp);
		virtual ~TextDecoder(void) override;
		virtual string ToStringE(ErrorContext & ectx) const noexcept override;
		virtual unichar32 ReadCharacterE(ErrorContext & ectx) noexcept override;
		virtual string ReadLineE(ErrorContext & ectx) noexcept override;
		virtual string ReadAllE(ErrorContext & ectx) noexcept override;
		virtual bool IsEOF(void) noexcept override;
	};

	namespace IO {
		void CreateDirectoryTree(const string & path, ErrorContext & ectx) noexcept;
		void RemoveEntireDirectory(const string & path, ErrorContext & ectx) noexcept;
		void CreateDirectoryTree(const string & path);
		void RemoveEntireDirectory(const string & path);

		handle DuplicateHandle(handle file);
		handle CreateFile(const string & path, uint access, FileCreationMode mode);
		void SetStandardHandle(StandardHandleType type, handle file);
		void CreatePipe(handle & pipe_in, handle & pipe_out);

		uintptr ReadFile(handle file, void * data, uintptr size);
		uintptr WriteFile(handle file, const void * data, uintptr size);
		uint64 SeekFile(handle file, int64 position, SeekOrigin org);
		uint64 GetFileSize(handle file);
		void SetFileSize(handle file, uint64 size);
		void Flush(handle file);

		void SetFilePermissions(handle file, uint access_user, uint access_group, uint access_world);
		void GetFilePermissions(handle file, uint * access_user, uint * access_group, uint * access_world);
		Time GetFileCreationTime(handle file);
		Time GetFileAccessTime(handle file);
		Time GetFileAlternationTime(handle file);
		void SetFileCreationTime(handle file, Time time);
		void SetFileAccessTime(handle file, Time time);
		void SetFileAlternationTime(handle file, Time time);

		FileType GetFileType(const string & file);
		void MoveFile(const string & from, const string & to);
		void RemoveFile(const string & path);
		bool CreateDirectory(const string & path);
		void RemoveDirectory(const string & path);
		void CreateSymbolicLink(const string & at, const string & to);
		void CreateHardLink(const string & at, const string & to);
		string GetSymbolicLinkDestination(const string & file);
		void GetVolumeSpace(const string & volume, uint64 * total_bytes, uint64 * free_bytes, uint64 * user_available_bytes);

		string ExpandPath(const string & path);
		string GetExecutablePath(void);
		string GetCurrentDirectory(void);
		void SetCurrentDirectory(const string & path);

		oref<array<string>> EnumerateFiles(const string & at_path, const string & filter, uint mode);
		oref<array<VolumeDesc>> EnumerateVolumes(void);
	}
}