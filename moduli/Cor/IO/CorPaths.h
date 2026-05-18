#pragma once

#include "../Classes/CorString.h"

namespace ESSE
{
	namespace FileAccess { enum FileAccessFlags : uint {
		AccessNo		= 0,
		AccessExecute	= 1,
		AccessWrite		= 2,
		AccessRead		= 4,
		AccessReadWrite	= AccessRead | AccessWrite,
		AccessReadOnly	= AccessRead | AccessExecute,
		AccessWriteExec	= AccessWrite | AccessExecute,
		AccessAll		= AccessRead | AccessWrite | AccessExecute
	}; };
	enum class FileCreationMode : uint { CreateAlways = 1, CreateNew = 2, OpenAlways = 3, OpenExisting = 4, TruncateExisting = 5 };
	enum class SeekOrigin : uint { Begin = 0, Current = 1, End = 2 };

	namespace IO
	{
		extern const handle InvalidHandle;

		#if defined(ESSE_SYSTEMA_WINDOWS)
			constexpr unichar32 PathSeparator = U'\\';
			constexpr const unichar32 * LineFeedSequence = U"\r\n";
			constexpr Unicode::Encoding TextFileEncoding = Unicode::Encoding::UTF8;
		#elif defined(ESSE_SYSTEMA_UNIX)
			constexpr unichar32 PathSeparator = U'/';
			constexpr const unichar32 * LineFeedSequence = U"\n";
			constexpr Unicode::Encoding TextFileEncoding = Unicode::Encoding::UTF8;
		#else
			constexpr unichar32 PathSeparator = U'/';
			constexpr const unichar32 * LineFeedSequence = U"\n";
			constexpr Unicode::Encoding TextFileEncoding = Unicode::Encoding::ASCII;
		#endif

		namespace Path
		{
			string Normalize(const string & path);
			string GetExtension(const string & path);
			string GetFileName(const string & path);
			string GetDirectory(const string & path);
			string GetPureFileName(const string & path);
			bool MatchFilter(const string & path, const string & filter);
		}
	}
}