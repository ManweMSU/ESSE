#include "CorIOEx.h"
#include "CorBoot.h"
#include <Cor/IO/CorStreams.h>

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <limits.h>
#include <dirent.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/mount.h>
#include <sys/param.h>
#include <sys/vfs.h>
#include <sys/types.h>

using namespace ESSE::Linux;

namespace ESSE
{
	namespace Linux
	{
		void ErrorSetPosix(ErrorContext & ectx) noexcept { ErrorSetPosix(ectx, errno); }
		void ErrorSetPosix(ErrorContext & ectx, int error_number) noexcept
		{
			if (error_number == 0) ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::Success);
			else if (error_number == EACCES) ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::AccessDenied);
			else if (error_number == EDQUOT) ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::NoDiskSpace);
			else if (error_number == EEXIST) ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::FileExists);
			else if (error_number == EISDIR) ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::FileExists);
			else if (error_number == EMFILE) ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::TooManyOpenFiles);
			else if (error_number == ENAMETOOLONG) ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::FileNameTooLong);
			else if (error_number == ENFILE) ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::NoDiskSpace);
			else if (error_number == ENOENT || errno == ECONNREFUSED) ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::FileNotFound);
			else if (error_number == ENOSPC) ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::NoDiskSpace);
			else if (error_number == ENOTDIR) ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::PathNotFound);
			else if (error_number == EOPNOTSUPP) ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::NotImplemented);
			else if (error_number == EROFS) ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::IsReadOnly);
			else if (error_number == ETXTBSY) ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::AccessDenied);
			else if (error_number == EILSEQ || errno == EADDRNOTAVAIL || errno == ELOOP) ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::BadPathName);
			else if (error_number == EBADF) ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::InvalidHandle);
			else if (error_number == EINVAL) ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::NotImplemented);
			else if (error_number == ENOTEMPTY) ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::DirectoryNotEmpty);
			else if (error_number == ENOTDIR) ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::PathNotFound);
			else if (error_number == ENOTSUP) ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::NotImplemented);
			else if (error_number == EPERM) ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::AccessDenied);
			else if (error_number == EXDEV) ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::NotSameDevice);
			else if (error_number == ENOBUFS) ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::NotEnoughMemory);
			else if (error_number == ENOMEM || error_number == EAGAIN) ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::NotEnoughMemory);
			else if (error_number == ENXIO) ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::InvalidDevice);
			else if (error_number == EFBIG) ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::FileTooLarge);
			else if (error_number == EBUSY) ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::AccessDenied);
			else ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::Unknown);
		}
		handle HandleWrap(int file) noexcept { return reinterpret_cast<handle>(intptr(file)); }
		int HandleUnwrap(handle file) noexcept { return int(reinterpret_cast<intptr>(file)); }
		string UnescapeString(const string & text)
		{
			dynamic_string_ucs4 buffer;
			uintptr pos = 0;
			do {
				if (text[pos] == U'\\') {
					pos++;
					uint32 ucsdec = Unicode::CharacterInvalid;
					auto escc = text[pos];
					if ((escc == U'\\') || (escc == U'\'') || (escc == U'\"') || (escc == U'?') || (escc == U'/')) {
						buffer += escc; pos++;
					} else if (escc == U'a' || escc == U'A') {
						buffer += U'\a'; pos++;
					} else if (escc == U'b' || escc == U'B') {
						buffer += U'\b'; pos++;
					} else if (escc == U'e' || escc == U'E') {
						buffer += U'\33'; pos++;
					} else if (escc == U'f' || escc == U'F') {
						buffer += U'\f'; pos++;
					} else if (escc == U'n' || escc == U'N') {
						buffer += U'\n'; pos++;
					} else if (escc == U'r' || escc == U'R') {
						buffer += U'\r'; pos++;
					} else if (escc == U't' || escc == U'T') {
						buffer += U'\t'; pos++;
					} else if (escc == U'v' || escc == U'V') {
						buffer += U'\v'; pos++;
					} else if (escc == U'x' || escc == U'X' || escc == U'U') {
						pos++;
						ucsdec = 0;
						int count = 0;
						while ((count < 2) && ((text[pos] >= U'0' && text[pos] <= U'9') || (text[pos] >= U'A' && text[pos] <= U'F') || (text[pos] >= U'a' && text[pos] <= U'f'))) {
							int rec = 0;
							if ((text[pos] >= U'0') && (text[pos] <= U'9')) rec = text[pos] - U'0';
							else if ((text[pos] >= U'A') && (text[pos] <= U'F')) rec = text[pos] - U'A' + 10;
							else rec = text[pos] - U'a' + 10;
							ucsdec <<= 4;
							ucsdec |= rec;
							count++;
							pos++;
						}
					} else if (escc == U'u') {
						pos++;
						ucsdec = 0;
						int count = 0;
						while ((count < 2) && ((text[pos] >= U'0' && text[pos] <= U'9') || (text[pos] >= U'A' && text[pos] <= U'F') || (text[pos] >= U'a' && text[pos] <= U'f'))) {
							int rec = 0;
							if ((text[pos] >= U'0') && (text[pos] <= U'9')) rec = text[pos] - U'0';
							else if ((text[pos] >= U'A') && (text[pos] <= U'F')) rec = text[pos] - U'A' + 10;
							else rec = text[pos] - U'a' + 10;
							ucsdec <<= 4;
							ucsdec |= rec;
							count++;
							pos++;
						}
					} else if ((escc >= U'0') && (escc <= U'7')) {
						int count = 1;
						ucsdec = 0;
						while ((count < 4) && (escc >= U'0') && (escc <= U'7')) {
							int rec = text[pos] - U'0';
							ucsdec <<= 3;
							ucsdec |= rec;
							count++;
							pos++;
						}
					} else return L"";
					if (ucsdec != Unicode::CharacterInvalid) buffer += ucsdec;
				} else if (text[pos]) {
					buffer += text[pos]; pos++;
				}
			} while (text[pos]);
			return buffer;
		}
	}
	namespace IO
	{
		handle DuplicateHandle(handle file, ErrorContext & ectx) noexcept
		{
			if (file == InvalidHandle) return InvalidHandle;
			auto new_file = dup(HandleUnwrap(file));
			if (new_file < 0) { ErrorSetPosix(ectx); return 0; }
			return HandleWrap(new_file);
		}
		handle CreateFile(const string & path, uint access, FileCreationMode mode, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
			ucs1_string path_utf8(Path::Normalize(path));
			int flags = 0, lf = 0;
			switch (access & (FileAccess::AccessReadWrite))
			{
			case FileAccess::AccessNo:
			case FileAccess::AccessRead:
				flags = O_RDONLY;
				lf = LOCK_SH | LOCK_NB;
				break;
			case FileAccess::AccessWrite:
				flags = O_WRONLY;
				lf = LOCK_EX | LOCK_NB;
				break;
			case FileAccess::AccessReadWrite:
				flags = O_RDWR;
				lf = LOCK_EX | LOCK_NB;
				break;
			default: throw InvalidArgumentException();
			}
			switch (mode)
			{
			case FileCreationMode::CreateAlways:
				flags |= O_CREAT | O_TRUNC;
				break;
			case FileCreationMode::CreateNew:
				flags |= O_CREAT | O_EXCL;
				break;
			case FileCreationMode::OpenAlways:
				flags |= O_CREAT;
				break;
			case FileCreationMode::OpenExisting:
				break;
			case FileCreationMode::TruncateExisting:
				flags |= O_TRUNC;
				break;
			default: throw InvalidArgumentException();
			}
			int result = -1;
			do {
				result = open(path_utf8, flags, 0666);
				if (result < 0 && errno != EINTR) { ErrorSetPosix(ectx); return InvalidHandle; }
			} while (result < 0);
			struct stat fs;
			if (fstat(result, &fs) < 0) { ErrorSetPosix(ectx); close(result); return InvalidHandle; }
			if ((fs.st_mode & S_IFMT) == S_IFDIR) { ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::AccessDenied); close(result); return InvalidHandle; }
			if (flock(result, lf) < 0) { ErrorSet(ectx, Errores::ErrorIO, Errores::SuberrorIO::AccessDenied); close(result); return InvalidHandle; }
			return HandleWrap(result);
			ESSE_TRY_OUTRO(InvalidHandle)
		}
		handle GetStandardHandle(StandardHandleType type) noexcept
		{
			int fno;
			if (type == StandardHandleType::Input) fno = 0;
			else if (type == StandardHandleType::Output) fno = 1;
			else if (type == StandardHandleType::Error) fno = 2;
			else return InvalidHandle;
			struct stat fs;
			if (fstat(1, &fs) < 0) return InvalidHandle;
			return HandleWrap(fno);
		}
		void SetStandardHandle(StandardHandleType type, handle file, ErrorContext & ectx) noexcept
		{
			int fno;
			if (type == StandardHandleType::Input) fno = 0;
			else if (type == StandardHandleType::Output) fno = 1;
			else if (type == StandardHandleType::Error) fno = 2;
			else { ErrorSet(ectx, Errores::ErrorInvalidArgument); return; }
			if (file != InvalidHandle) {
				if (fno == HandleUnwrap(file)) return;
				while (true) {
					auto status = dup2(HandleUnwrap(file), fno);
					if (status >= 0) break;
					else if (errno != EINTR) { ErrorSetPosix(ectx); return; }
				}
			} else {
				int ndev = -1;
				do {
					ndev = open("/dev/null", O_RDWR);
					if (ndev < 0 && errno != EINTR) { ErrorSetPosix(ectx); return; }
				} while (ndev < 0);
				while (true) {
					auto status = dup2(ndev, fno);
					if (status >= 0) break;
					else if (errno != EINTR) { ErrorSetPosix(ectx); close(ndev); return; }
				}
				close(ndev);
			}
		}
		void CreatePipe(handle & pipe_in, handle & pipe_out, ErrorContext & ectx) noexcept
		{
			int result[2];
			if (pipe(result) < 0) { ErrorSetPosix(ectx); return; }
			pipe_in = HandleWrap(result[1]);
			pipe_out = HandleWrap(result[0]);
		}
		void CloseHandle(handle file) noexcept { if (file == InvalidHandle) return; close(HandleUnwrap(file)); }
		uintptr ReadFile(handle file, void * data, uintptr size, ErrorContext & ectx) noexcept
		{
			auto pdata = reinterpret_cast<uint8 *>(data);
			uintptr transferred = 0, pending = size;
			do {
				auto status = read(HandleUnwrap(file), pdata + transferred, pending);
				if (status == -1 && errno != EINTR) { ErrorSetPosix(ectx); return 0; } else if (status >= 0) {
					pending -= status; transferred += status;
					if (!status || !pending) return transferred;
				}
			} while (true);
		}
		uintptr WriteFile(handle file, const void * data, uintptr size, ErrorContext & ectx) noexcept
		{
			auto pdata = reinterpret_cast<const uint8 *>(data);
			uintptr transferred = 0, pending = size;
			do {
				auto status = write(HandleUnwrap(file), pdata + transferred, pending);
				if (status == -1 && errno != EINTR) { ErrorSetPosix(ectx); return 0; } else if (status >= 0) {
					pending -= status; transferred += status;
					if (!status || !pending) return transferred;
				}
			} while (true);
		}
		uint64 SeekFile(handle file, int64 position, SeekOrigin org, ErrorContext & ectx) noexcept
		{
			int seek_org;
			if (org == SeekOrigin::Begin) seek_org = SEEK_SET;
			else if (org == SeekOrigin::Current) seek_org = SEEK_CUR;
			else if (org == SeekOrigin::End) seek_org = SEEK_END;
			else { ErrorSet(ectx, Errores::ErrorInvalidArgument); return 0; }
			auto status = lseek(HandleUnwrap(file), position, seek_org);
			if (status < 0) { ErrorSetPosix(ectx); return 0; }
			return status;
		}
		uint64 GetFileSize(handle file, ErrorContext & ectx) noexcept
		{
			struct stat fs;
			if (fstat(HandleUnwrap(file), &fs) < 0) { ErrorSetPosix(ectx); return 0; }
			return fs.st_size;
		}
		void SetFileSize(handle file, uint64 size, ErrorContext & ectx) noexcept
		{
			do {
				auto status = ftruncate(HandleUnwrap(file), size);
				if (status == -1 && errno != EINTR) { ErrorSetPosix(ectx); return; }
				else if (status >= 0) return;
			} while (true);
		}
		void Flush(handle file, ErrorContext & ectx) noexcept
		{
			do {
				auto status = fsync(HandleUnwrap(file));
				if (status == -1 && errno != EINTR) { ErrorSetPosix(ectx); return; }
				else if (status >= 0) return;
			} while (true);
		}
		void SetFilePermissions(handle file, uint access_user, uint access_group, uint access_world, ErrorContext & ectx) noexcept
		{
			uint mode = (access_world & 0x7) | ((access_group & 0x7) << 3) | ((access_user & 0x7) << 6);
			do {
				auto status = fchmod(HandleUnwrap(file), mode);
				if (status == -1 && errno != EINTR) { ErrorSetPosix(ectx); return; }
				else if (status >= 0) return;
			} while (true);
		}
		void GetFilePermissions(handle file, uint * access_user, uint * access_group, uint * access_world, ErrorContext & ectx) noexcept
		{
			struct stat fs;
			if (fstat(HandleUnwrap(file), &fs) < 0) { ErrorSetPosix(ectx); return; }
			if (access_user) *access_user = (fs.st_mode & 0700) >> 6;
			if (access_group) *access_group = (fs.st_mode & 0070) >> 3;
			if (access_world) *access_world = (fs.st_mode & 0007);
		}
		Time GetFileCreationTime(handle file, ErrorContext & ectx) noexcept
		{
			struct stat fs;
			if (fstat(HandleUnwrap(file), &fs) < 0) { ErrorSetPosix(ectx); return 0; }
			return Time::FromUnixTime(fs.st_mtim.tv_sec * 1000ULL + fs.st_mtim.tv_nsec / 1000000ULL);
		}
		Time GetFileAccessTime(handle file, ErrorContext & ectx) noexcept
		{
			struct stat fs;
			if (fstat(HandleUnwrap(file), &fs) < 0) { ErrorSetPosix(ectx); return 0; }
			return Time::FromUnixTime(fs.st_atim.tv_sec * 1000ULL + fs.st_atim.tv_nsec / 1000000ULL);
		}
		Time GetFileAlternationTime(handle file, ErrorContext & ectx) noexcept
		{
			struct stat fs;
			if (fstat(HandleUnwrap(file), &fs) < 0) { ErrorSetPosix(ectx); return 0; }
			return Time::FromUnixTime(fs.st_mtim.tv_sec * 1000ULL + fs.st_mtim.tv_nsec / 1000000ULL);
		}
		void SetFileCreationTime(handle file, Time time, ErrorContext & ectx) noexcept {}
		void SetFileAccessTime(handle file, Time time, ErrorContext & ectx) noexcept
		{
			struct stat fs;
			if (fstat(HandleUnwrap(file), &fs) < 0) { ErrorSetPosix(ectx); return; }
			struct timespec times[2];
			auto t = time.ToUnixTime();
			times[0].tv_sec = t / 1000ULL;
			times[0].tv_nsec = (t % 1000ULL) * 1000000ULL;
			times[1].tv_sec = fs.st_mtim.tv_sec;
			times[1].tv_nsec = fs.st_mtim.tv_nsec;
			if (futimens(HandleUnwrap(file), times) < 0) { ErrorSetPosix(ectx); return; }
		}
		void SetFileAlternationTime(handle file, Time time, ErrorContext & ectx) noexcept
		{
			struct stat fs;
			if (fstat(HandleUnwrap(file), &fs) < 0) { ErrorSetPosix(ectx); return; }
			struct timespec times[2];
			auto t = time.ToUnixTime();
			times[0].tv_sec = fs.st_atim.tv_sec;
			times[0].tv_nsec = fs.st_atim.tv_nsec;
			times[1].tv_sec = t / 1000ULL;
			times[1].tv_nsec = (t % 1000ULL) * 1000000ULL;
			if (futimens(HandleUnwrap(file), times) < 0) { ErrorSetPosix(ectx); return; }
		}
		FileType GetFileTypeFollow(const string & file, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				ucs1_string path_utf8(Path::Normalize(file));
				int fd = -1;
				do {
					fd = open(path_utf8, O_RDONLY | O_PATH);
					if (fd < 0 && errno != EINTR) { ErrorSetPosix(ectx); return FileType::Unknown; }
				} while (fd < 0);
				struct stat fs;
				if (fstat(fd, &fs) < 0) { ErrorSetPosix(ectx); close(fd); return FileType::Unknown; }
				close(fd);
				if ((fs.st_mode & S_IFMT) == S_IFREG) return FileType::Regular;
				else if ((fs.st_mode & S_IFMT) == S_IFDIR) return FileType::Directory;
				else if ((fs.st_mode & S_IFMT) == S_IFLNK) return FileType::SymbolicLink;
				else return FileType::Unknown;
			ESSE_TRY_OUTRO(FileType::Unknown)
		}
		FileType GetFileType(const string & file, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				ucs1_string path_utf8(Path::Normalize(file));
				int fd = -1;
				do {
					fd = open(path_utf8, O_RDONLY | O_NOFOLLOW | O_PATH);
					if (fd < 0 && errno != EINTR) { ErrorSetPosix(ectx); return FileType::Unknown; }
				} while (fd < 0);
				struct stat fs;
				if (fstat(fd, &fs) < 0) { ErrorSetPosix(ectx); close(fd); return FileType::Unknown; }
				close(fd);
				if ((fs.st_mode & S_IFMT) == S_IFREG) return FileType::Regular;
				else if ((fs.st_mode & S_IFMT) == S_IFDIR) return FileType::Directory;
				else if ((fs.st_mode & S_IFMT) == S_IFLNK) return FileType::SymbolicLink;
				else return FileType::Unknown;
			ESSE_TRY_OUTRO(FileType::Unknown)
		}
		bool FileExists(const string & path) noexcept
		{
			ErrorContext ectx; ErrorClear(ectx);
			auto ft = GetFileTypeFollow(path, ectx);
			if (ErrorTest(ectx) || ft == FileType::Directory) return false;
			else return true;
		}
		void MoveFile(const string & from, const string & to, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				ucs1_string from_utf8(Path::Normalize(from)), to_utf8(Path::Normalize(to));
				if (rename(from_utf8, to_utf8) < 0) { ErrorSetPosix(ectx); return; }
			ESSE_TRY_OUTRO()
		}
		void RemoveFile(const string & path, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				ucs1_string path_utf8(Path::Normalize(path));
				if (unlink(path_utf8) < 0) { ErrorSetPosix(ectx); return; }
			ESSE_TRY_OUTRO()
		}
		bool CreateDirectory(const string & path, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				ucs1_string path_utf8(Path::Normalize(path));
				if (mkdir(path_utf8, 0777) < 0) {
					if (errno == EEXIST) return false;
					else { ErrorSetPosix(ectx); return false; }
				} else return true;
			ESSE_TRY_OUTRO(false)
		}
		void RemoveDirectory(const string & path, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				ucs1_string path_utf8(Path::Normalize(path));
				if (rmdir(path_utf8) < 0) { ErrorSetPosix(ectx); return; }
			ESSE_TRY_OUTRO()
		}
		void CreateSymbolicLink(const string & at, const string & to, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				ucs1_string at_utf8(Path::Normalize(at));
				ucs1_string to_utf8(Path::Normalize(to));
				if (symlink(to_utf8, at_utf8) < 0) { ErrorSetPosix(ectx); return; }
			ESSE_TRY_OUTRO()
		}
		void CreateHardLink(const string & at, const string & to, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				ucs1_string at_utf8(Path::Normalize(at));
				ucs1_string to_utf8(Path::Normalize(to));
				if (linkat(AT_FDCWD, to_utf8, AT_FDCWD, at_utf8, AT_SYMLINK_FOLLOW) < 0) { ErrorSetPosix(ectx); return; }
			ESSE_TRY_OUTRO()
		}
		string GetSymbolicLinkDestination(const string & file, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				ucs1_string file_utf8(Path::Normalize(file));
				int fd = -1;
				do {
					fd = open(file_utf8, O_RDONLY | O_NOFOLLOW | O_PATH);
					if (fd < 0 && errno != EINTR) { ErrorSetPosix(ectx); return string(); }
				} while (fd < 0);
				struct stat fs;
				if (fstat(fd, &fs) < 0) { ErrorSetPosix(ectx); close(fd); return string(); }
				close(fd);
				if ((fs.st_mode & S_IFMT) == S_IFLNK) {
					array<unichar8> buffer;
					buffer.SetLength(PATH_MAX);
					auto length = readlink(file_utf8, buffer.GetBuffer(), buffer.GetLength());
					if (length < 0) { ErrorSetPosix(ectx); return string(); }
					ucs1_string base(buffer.GetBuffer(), length); buffer.Clear();
					if (base[0] == '/') return base;
					auto file_expanded = ExpandPath(file, ectx);
					if (ErrorTest(ectx)) return string();
					return ExpandPath(Path::GetDirectory(file_expanded) + U"/" + string(base), ectx);
				} else return ExpandPath(file, ectx);
			ESSE_TRY_OUTRO(string())
		}
		void GetVolumeSpace(const string & volume, uint64 * total_bytes, uint64 * free_bytes, uint64 * user_available_bytes, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				ucs1_string path_utf8(Path::Normalize(volume));
				struct statfs fss;
				while (true) {
					auto status = statfs(path_utf8, &fss);
					if (status < 0 && errno != EINTR) { ErrorSetPosix(ectx); return; }
					else if (status >= 0) break;
				}
				if (total_bytes) *total_bytes = fss.f_bsize * fss.f_blocks;
				if (free_bytes) *free_bytes = fss.f_bsize * fss.f_bfree;
				if (user_available_bytes) *user_available_bytes = fss.f_bsize * fss.f_bavail;
			ESSE_TRY_OUTRO()
		}
		string ExpandPath(const string & path, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				string full;
				if (path[0] == U'/' || path[0] == U'\\') full = path; else {
					auto wd = GetCurrentDirectory(ectx);
					if (ErrorTest(ectx)) return string();
					full = wd + U"/" + path;
				}
				auto parts = SplitString(Path::Normalize(full), U'/');
				for (uintptr i = 0; i < parts.GetLength(); i++) {
					if (!parts[i].GetLength() || parts[i] == U".") { parts.Remove(i); i--; continue; }
					if (parts[i] == U"..") {
						if (i > 1) { parts.RemoveRange(i - 1, 2); i -= 2; continue; }
						else { parts.Remove(i); i--; continue; }
					}
				}
				dynamic_string_ucs4 result;
				for (auto & p : parts) result << U'/' << p;
				if (!result.GetLength()) result << U'/';
				return result;
			ESSE_TRY_OUTRO(string())
		}
		string GetExecutablePath(ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				pid_t self_pid;
				GetThisProcessPID(&self_pid);
				auto lnf = "/proc/" + ucs1_string(string(self_pid)) + "/exe";
				array<unichar8> buffer;
				buffer.SetLength(PATH_MAX);
				auto length = readlink(lnf, buffer.GetBuffer(), buffer.GetLength());
				if (length < 0) { ErrorSetPosix(ectx); return string(); }
				return string(buffer.GetBuffer(), length);
			ESSE_TRY_OUTRO(string())
		}
		string GetCurrentDirectory(ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				array<unichar8> buffer;
				buffer.SetLength(PATH_MAX);
				do {
					if (getcwd(buffer.GetBuffer(), buffer.GetLength())) break;
					if (errno == ERANGE) buffer.SetLength(buffer.GetLength() * 2);
					else { ErrorSetPosix(ectx); return string(); }
				} while (true);
				return string(buffer.GetBuffer());
			ESSE_TRY_OUTRO(string())
		}
		void SetCurrentDirectory(const string & path, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				ucs1_string path_utf8(Path::Normalize(path));
				array<unichar8> buffer;
				buffer.SetLength(PATH_MAX);
				if (!realpath(path_utf8, buffer.GetBuffer())) { ErrorSetPosix(ectx); return; }
				if (chdir(buffer.GetBuffer()) < 0) { ErrorSetPosix(ectx); return; }
			ESSE_TRY_OUTRO()
		}
		void EnumerateEntities(array<string> & dest, const string & at_path, const string & filter, const string & prefix, uint mode)
		{
			ucs1_string path_utf8(at_path);
			struct dirent ** elements;
			auto count = scandir(path_utf8, &elements, 0, alphasort);
			if (count >= 0) {
				try {
					path_utf8 = ucs1_string();
					if (mode & FileSearch::FileSearchRecursive) {
						for (int i = 0; i < count; i++) if (elements[i]->d_type == DT_DIR) {
							auto & e = *elements[i];
							if (Memory::StringCompare(e.d_name, ".") == 0 || Memory::StringCompare(e.d_name, "..") == 0) continue;
							if (e.d_name[0] == '.' && !(mode & FileSearch::FileSearchAllowHidden)) continue;
							string name(e.d_name);
							try { EnumerateEntities(dest, at_path + U"/" + name, filter, prefix + name + U"/", mode); } catch (...) {}
						}
					}
					for (int i = 0; i < count; i++) {
						auto & e = *elements[i];
						if (e.d_type == DT_DIR && !(mode & FileSearch::FileSearchDirectories)) continue;
						if (e.d_type != DT_DIR && !(mode & FileSearch::FileSearchMainEntries)) continue;
						if (Memory::StringCompare(e.d_name, ".") == 0 || Memory::StringCompare(e.d_name, "..") == 0) continue;
						if (e.d_name[0] == '.' && !(mode & FileSearch::FileSearchAllowHidden)) continue;
						string name(e.d_name);
						if (!filter.GetLength() || Path::MatchFilter(name, filter)) dest.Append(prefix + name);
					}
				} catch (...) { for (int i = 0; i < count; i++) free(elements[i]); free(elements); throw; }
				for (int i = 0; i < count; i++) free(elements[i]); free(elements);
			} else { ErrorContext ectx; ErrorSetPosix(ectx); throw CustomException(ectx); }
		}
		oref<array<string>> EnumerateFiles(const string & at_path, const string & filter, uint mode, ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
				auto result = owrap(new array<string>(0x100));
				auto expanded = ExpandPath(at_path, ectx);
				if (ErrorTest(ectx)) return 0;
				EnumerateEntities(*result, expanded, filter, string(), mode);
				return result;
			ESSE_TRY_OUTRO(0)
		}
		oref<array<VolumeDesc>> EnumerateVolumes(ErrorContext & ectx) noexcept
		{
			ESSE_TRY_INTRO
			auto result = owrap(new array<VolumeDesc>(0x20));
			int fd = -1;
			while (true) {
				fd = open("/proc/mounts", O_RDONLY);
				if (fd >= 0 || errno != EINTR) break;
			}
			if (fd >= 0) {
				try {
					auto dec = owrap(new TextDecoder(FileStream::CreateWrapper(HandleWrap(fd)), Unicode::Encoding::UTF8));
					while (!dec->IsEOF()) {
						auto parts = SplitString(dec->ReadLine(), U' ');
						if (parts.GetLength() < 2) continue;
						if (parts[0].Substring(0, 5) != U"/dev/" && parts[1].Substring(0, 7) != U"/media/" && parts[1].Substring(0, 5) != U"/mnt/") continue;
						VolumeDesc desc;
						desc.root_path = UnescapeString(parts[1]);
						desc.label = UnescapeString(parts[0]);
						result->Append(desc);
					}
				} catch (...) {}
				close(fd);
			}
			array<VolumeDesc> labels(0x20);
			struct dirent ** llnk;
			int num_llnk = scandir("/dev/disk/by-label", &llnk, 0, alphasort);
			if (num_llnk >= 0) {
				for (int i = 0; i < num_llnk; i++) {
					if (llnk[i]->d_type == DT_LNK) try {
						ErrorContext lectx; ErrorClear(lectx);
						string label(llnk[i]->d_name);
						string dev = GetSymbolicLinkDestination(U"/dev/disk/by-label/" + label, lectx);
						ErrorThrow(lectx);
						VolumeDesc v;
						v.label = UnescapeString(label);
						v.root_path = dev;
						labels << v;
					} catch (...) {}
					free(llnk[i]);
				}
				free(llnk);
			}
			num_llnk = scandir("/dev/disk/by-designator", &llnk, 0, alphasort);
			if (num_llnk >= 0) {
				for (int i = 0; i < num_llnk; i++) {
					if (llnk[i]->d_type == DT_LNK) try {
						ErrorContext lectx; ErrorClear(lectx);
						string label(llnk[i]->d_name);
						string dev = GetSymbolicLinkDestination(U"/dev/disk/by-designator/" + label, lectx);
						ErrorThrow(lectx);
						VolumeDesc v;
						v.label = UnescapeString(label);
						v.root_path = dev;
						labels << v;
					} catch (...) {}
					free(llnk[i]);
				}
				free(llnk);
			}
			for (auto & r : *result) try {
				bool found = false;
				for (auto & l : labels) if (l.root_path == r.label) { r.label = l.label; found = true; break; }
				if (!found) r.label = Path::GetFileName(r.label).Uppercased();
			} catch (...) {}
			return result;
			ESSE_TRY_OUTRO(0)
		}
	}
}