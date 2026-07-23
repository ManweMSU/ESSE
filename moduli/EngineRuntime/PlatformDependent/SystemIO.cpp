#include "../Interfaces/SystemIO.h"
#include "../Syntax/Regular.h"
#include "../Miscellaneous/DynamicString.h"
#include "../Streaming.h"

namespace Engine
{
	namespace IO
	{
		handle CreateFile(const string & path, Streaming::FileAccess access, Streaming::FileCreationMode mode)
		{
			ESSE::uint amask;
			ESSE::FileCreationMode cmode;
			if (access == Streaming::AccessRead) amask = ESSE::FileAccess::AccessRead;
			else if (access == Streaming::AccessWrite) amask = ESSE::FileAccess::AccessWrite;
			else if (access == Streaming::AccessReadWrite) amask = ESSE::FileAccess::AccessReadWrite;
			else if (access == Streaming::AccessNo) amask = ESSE::FileAccess::AccessNo;
			else throw InvalidArgumentException();
			int result = -1;
			if (mode == Streaming::CreateNew) cmode = ESSE::FileCreationMode::CreateNew;
			else if (mode == Streaming::CreateAlways) cmode = ESSE::FileCreationMode::CreateAlways;
			else if (mode == Streaming::OpenAlways) cmode = ESSE::FileCreationMode::OpenAlways;
			else if (mode == Streaming::TruncateExisting) cmode = ESSE::FileCreationMode::TruncateExisting;
			else if (mode == Streaming::OpenExisting) cmode = ESSE::FileCreationMode::OpenExisting;
			else throw InvalidArgumentException();
			return ESSE::IO::CreateFile(static_cast<const ESSE::unichar32 *>(path), amask, cmode);
		}
		void CreatePipe(handle * pipe_in, handle * pipe_out) { ESSE::IO::CreatePipe(*pipe_in, *pipe_out); }
		handle CloneHandle(handle file) { return ESSE::IO::DuplicateHandle(file); }
		void CloseHandle(handle file) { return ESSE::IO::CloseHandle(file); }
		void ReadFile(handle file, void * to, uint32 amount)
		{
			auto read = ESSE::IO::ReadFile(file, to, amount);
			if (read != amount) throw FileReadEndOfFileException(read);
		}
		void WriteFile(handle file, const void * data, uint32 amount)
		{
			auto written = ESSE::IO::WriteFile(file, data, amount);
			if (written != amount) throw ESSE::InputOutputException(ESSE::Errores::SuberrorIO::WriteFailure);
		}
		int64 Seek(handle file, int64 position, Streaming::SeekOrigin origin)
		{
			ESSE::SeekOrigin org = ESSE::SeekOrigin::Begin;
			if (origin == Streaming::Current) org = ESSE::SeekOrigin::Current;
			else if (origin == Streaming::End) org = ESSE::SeekOrigin::End;
			return ESSE::IO::SeekFile(file, position, org);
		}
		uint64 GetFileSize(handle file) { return ESSE::IO::GetFileSize(file); }
		void SetFileSize(handle file, uint64 size) { ESSE::IO::SetFileSize(file, size); }
		void Flush(handle file) { ESSE::IO::Flush(file); }
		bool FileExists(const string & path) { return ESSE::IO::FileExists(static_cast<const ESSE::unichar32 *>(path)); }
		void MoveFile(const string & from, const string & to) { ESSE::IO::MoveFile(static_cast<const ESSE::unichar32 *>(from), static_cast<const ESSE::unichar32 *>(to)); }
		void RemoveFile(const string & path) { ESSE::IO::RemoveFile(static_cast<const ESSE::unichar32 *>(path)); }
		void CreateDirectory(const string & path) { if (!ESSE::IO::CreateDirectory(static_cast<const ESSE::unichar32 *>(path))) throw DirectoryAlreadyExistsException(); }
		void RemoveDirectory(const string & path) { ESSE::IO::RemoveDirectory(static_cast<const ESSE::unichar32 *>(path)); }
		void CreateSymbolicLink(const string & at, const string & to) { ESSE::IO::CreateSymbolicLink(static_cast<const ESSE::unichar32 *>(at), static_cast<const ESSE::unichar32 *>(to)); }
		void CreateHardLink(const string & at, const string & to) { ESSE::IO::CreateHardLink(static_cast<const ESSE::unichar32 *>(at), static_cast<const ESSE::unichar32 *>(to)); }
		FileType GetFileType(const string & file)
		{
			auto type = ESSE::IO::GetFileType(static_cast<const ESSE::unichar32 *>(file));
			if (type == ESSE::IO::FileType::Regular) return FileType::Regular;
			else if (type == ESSE::IO::FileType::Directory) return FileType::Directory;
			else if (type == ESSE::IO::FileType::SymbolicLink) return FileType::SymbolicLink;
			else return FileType::Unknown;
		}
		string GetSymbolicLinkDestination(const string & file) { return ESSE::IO::GetSymbolicLinkDestination(static_cast<const ESSE::unichar32 *>(file)).GetData(); }
		void GetVolumeSpace(const string & volume, uint64 * total_bytes, uint64 * free_bytes, uint64 * user_available_bytes) { ESSE::IO::GetVolumeSpace(static_cast<const ESSE::unichar32 *>(volume), total_bytes, free_bytes, user_available_bytes); }
		string ExpandPath(const string & path) { return ESSE::IO::ExpandPath(static_cast<const ESSE::unichar32 *>(path)).GetData(); }
		string GetCurrentDirectory(void) { return ESSE::IO::GetCurrentDirectory().GetData(); }
		void SetCurrentDirectory(const string & path) { ESSE::IO::SetCurrentDirectory(static_cast<const ESSE::unichar32 *>(path)); }
		string GetExecutablePath(void) { return ESSE::IO::GetExecutablePath().GetData(); }
		handle GetStandardOutput(void) { return ESSE::IO::GetStandardHandle(ESSE::IO::StandardHandleType::Output); }
		handle GetStandardInput(void) { return ESSE::IO::GetStandardHandle(ESSE::IO::StandardHandleType::Input); }
		handle GetStandardError(void) { return ESSE::IO::GetStandardHandle(ESSE::IO::StandardHandleType::Error); }
		void SetStandardOutput(handle file) { ESSE::IO::SetStandardHandle(ESSE::IO::StandardHandleType::Output, file); }
		void SetStandardInput(handle file) { ESSE::IO::SetStandardHandle(ESSE::IO::StandardHandleType::Input, file); }
		void SetStandardError(handle file) { ESSE::IO::SetStandardHandle(ESSE::IO::StandardHandleType::Error, file); }
		namespace Search
		{
			Array<string>* GetFiles(const string & path, bool recursive)
			{
				auto expanded = ExpandPath(path);
				auto where = Path::GetDirectory(expanded);
				auto filter = Path::GetFileName(expanded);
				if (!where.Length()) where = U"/";
				uint mode = ESSE::IO::FileSearch::FileSearchMainEntries | ESSE::IO::FileSearch::FileSearchAllowHidden;
				if (recursive) mode |= ESSE::IO::FileSearch::FileSearchRecursive;
				auto list = ESSE::IO::EnumerateFiles(static_cast<const ESSE::unichar32 *>(where), static_cast<const ESSE::unichar32 *>(filter), mode);
				SafePointer< Array<string> > result = new Array<string>(0x10);
				for (auto r : *list) result->Append(r.GetData());
				result->Retain();
				return result;
			}
			Array<string>* GetDirectories(const string & path)
			{
				auto expanded = ExpandPath(path);
				auto where = Path::GetDirectory(expanded);
				auto filter = Path::GetFileName(expanded);
				if (!where.Length()) where = U"/";
				uint mode = ESSE::IO::FileSearch::FileSearchDirectories | ESSE::IO::FileSearch::FileSearchAllowHidden;
				auto list = ESSE::IO::EnumerateFiles(static_cast<const ESSE::unichar32 *>(where), static_cast<const ESSE::unichar32 *>(filter), mode);
				SafePointer< Array<string> > result = new Array<string>(0x10);
				for (auto r : *list) result->Append(r.GetData());
				result->Retain();
				return result;
			}
			Array<Volume>* GetVolumes(void)
			{
				auto list = ESSE::IO::EnumerateVolumes();
				SafePointer< Array<Volume> > result = new Array<Volume>(0x10);
				for (auto & v : *list) {
					Volume vol;
					vol.Label = v.label.GetData();
					vol.Path = v.root_path.GetData();
					result->Append(vol);
				}
				result->Retain();
				return result;
			}
		}
		namespace DateTime
		{
			Time GetFileCreationTime(handle file) { return ESSE::IO::GetFileCreationTime(file).Ticks; }
			Time GetFileAccessTime(handle file) { return ESSE::IO::GetFileAccessTime(file).Ticks; }
			Time GetFileAlterTime(handle file) { return ESSE::IO::GetFileAlternationTime(file).Ticks; }
			void SetFileCreationTime(handle file, Time time) { ESSE::IO::SetFileCreationTime(file, time.Ticks); }
			void SetFileAccessTime(handle file, Time time) { ESSE::IO::SetFileAccessTime(file, time.Ticks); }
			void SetFileAlterTime(handle file, Time time) { ESSE::IO::SetFileAlternationTime(file, time.Ticks); }
		}
	}
}