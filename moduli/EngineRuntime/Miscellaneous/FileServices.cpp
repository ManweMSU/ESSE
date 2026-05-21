#include "FileServices.h"

namespace Engine
{
	namespace IO
	{
		const handle InvalidHandle = handle(sintptr(-1));

		FileReadEndOfFileException::FileReadEndOfFileException(uint32 data_read) : DataRead(data_read) {}
		ESSE::ErrorContext FileReadEndOfFileException::GetError(void) const noexcept { return ESSE::ErrorMake(ESSE::Errores::ErrorIO, ESSE::Errores::SuberrorIO::ReadFailure); }
		ESSE::ErrorContext DirectoryAlreadyExistsException::GetError(void) const noexcept { return ESSE::ErrorMake(ESSE::Errores::ErrorIO, ESSE::Errores::SuberrorIO::FileExists); }

		namespace Path
		{
			string NormalizePath(const string & path)
			{
				if (PathDirectorySeparator == U'\\') return path.Replace(U'/', U'\\');
				else if (PathDirectorySeparator == U'/') return path.Replace(U'\\', U'/');
				return U"";
			}
			string GetExtension(const string & path)
			{
				int s = path.Length() - 1;
				while (s >= 0 && (path[s] == U'\\' || path[s] == U'/')) s--;
				for (int i = s; i >= 0; i--) {
					if (path[i] == U'.') {
						if (i != 0 && path[i - 1] != U'\\' && path[i - 1] != U'/') return path.Fragment(i + 1, s - i);
					}
					if (path[i] == U'/' || path[i] == U'\\') return U"";
				}
				return U"";
			}
			string GetFileName(const string & path)
			{
				int s = path.Length() - 1;
				while (s >= 0 && (path[s] == U'\\' || path[s] == U'/')) s--;
				for (int i = s; i >= 0; i--) {
					if (path[i] == U'\\' || path[i] == U'/') return path.Fragment(i + 1, s - i);
				}
				return path.Fragment(0, s + 1);
			}
			string GetDirectory(const string & path)
			{
				int s = path.Length() - 1;
				while (s >= 0 && (path[s] == U'\\' || path[s] == U'/')) s--;
				for (int i = s; i >= 0; i--) {
					if (path[i] == U'\\' || path[i] == U'/') return path.Fragment(0, i);
				}
				return U"";
			}
			string GetFileNameWithoutExtension(const string & path)
			{
				string name = GetFileName(path);
				for (int i = name.Length() - 1; i > 0; i--) {
					if (name[i] == U'.') return name.Fragment(0, i);
				}
				return name;
			}
		}
	}
}