#pragma once

#include "CorPaths.h"
#include "../Classes/CorArray.hxx"
#include "../Classes/CorTime.h"

namespace ESSE
{
	namespace IO
	{
		enum class StandardHandleType { Input = 0, Output = 1, Error = 2 };
		enum class FileType { Unknown = 0, Regular = 1, Directory = 2, SymbolicLink = 3 };
		struct VolumeDesc { string label, root_path; };
		namespace FileSearch { enum FileSearchFlags : uint {
			FileSearchMainEntries	= 0x01,
			FileSearchDirectories	= 0x02,
			FileSearchRecursive		= 0x04,
			FileSearchAllowHidden	= 0x08,
		}; };
		
		handle DuplicateHandle(handle file, ErrorContext & ectx) noexcept;
		handle CreateFile(const string & path, uint access, FileCreationMode mode, ErrorContext & ectx) noexcept;
		handle GetStandardHandle(StandardHandleType type) noexcept;
		void SetStandardHandle(StandardHandleType type, handle file, ErrorContext & ectx) noexcept;
		void CreatePipe(handle & pipe_in, handle & pipe_out, ErrorContext & ectx) noexcept;
		void CloseHandle(handle file) noexcept;

		uintptr ReadFile(handle file, void * data, uintptr size, ErrorContext & ectx) noexcept;
		uintptr WriteFile(handle file, const void * data, uintptr size, ErrorContext & ectx) noexcept;
		uint64 SeekFile(handle file, int64 position, SeekOrigin org, ErrorContext & ectx) noexcept;
		uint64 GetFileSize(handle file, ErrorContext & ectx) noexcept;
		void SetFileSize(handle file, uint64 size, ErrorContext & ectx) noexcept;
		void Flush(handle file, ErrorContext & ectx) noexcept;

		void SetFilePermissions(handle file, uint access_user, uint access_group, uint access_world, ErrorContext & ectx) noexcept;
		void GetFilePermissions(handle file, uint * access_user, uint * access_group, uint * access_world, ErrorContext & ectx) noexcept;
		Time GetFileCreationTime(handle file, ErrorContext & ectx) noexcept;
		Time GetFileAccessTime(handle file, ErrorContext & ectx) noexcept;
		Time GetFileAlternationTime(handle file, ErrorContext & ectx) noexcept;
		void SetFileCreationTime(handle file, Time time, ErrorContext & ectx) noexcept;
		void SetFileAccessTime(handle file, Time time, ErrorContext & ectx) noexcept;
		void SetFileAlternationTime(handle file, Time time, ErrorContext & ectx) noexcept;

		FileType GetFileType(const string & file, ErrorContext & ectx) noexcept;
		bool FileExists(const string & path) noexcept;
		void MoveFile(const string & from, const string & to, ErrorContext & ectx) noexcept;
		void RemoveFile(const string & path, ErrorContext & ectx) noexcept;
		bool CreateDirectory(const string & path, ErrorContext & ectx) noexcept;
		void RemoveDirectory(const string & path, ErrorContext & ectx) noexcept;
		void CreateSymbolicLink(const string & at, const string & to, ErrorContext & ectx) noexcept;
		void CreateHardLink(const string & at, const string & to, ErrorContext & ectx) noexcept;
		string GetSymbolicLinkDestination(const string & file, ErrorContext & ectx) noexcept;
		void GetVolumeSpace(const string & volume, uint64 * total_bytes, uint64 * free_bytes, uint64 * user_available_bytes, ErrorContext & ectx) noexcept;

		string ExpandPath(const string & path, ErrorContext & ectx) noexcept;
		string GetExecutablePath(ErrorContext & ectx) noexcept;
		string GetCurrentDirectory(ErrorContext & ectx) noexcept;
		void SetCurrentDirectory(const string & path, ErrorContext & ectx) noexcept;

		oref<array<string>> EnumerateFiles(const string & at_path, const string & filter, uint mode, ErrorContext & ectx) noexcept;
		oref<array<VolumeDesc>> EnumerateVolumes(ErrorContext & ectx) noexcept;
	}
}