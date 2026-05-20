#pragma once

#include "../Classes/CorString.h"

#if defined(ESSE_SYSTEMA_WINDOWS)
	#define ESSE_EXPORT_API extern "C" __declspec(dllexport)
	#define ESSE_LIBRARY_EXTENSION U"dll"
#elif defined(ESSE_SYSTEMA_MACOSX)
	#define ESSE_EXPORT_API extern "C" __attribute__((visibility("default")))
	#define ESSE_LIBRARY_EXTENSION U"dylib"
#elif defined(ESSE_SYSTEMA_LINUX)
	#define ESSE_EXPORT_API extern "C" __attribute__((visibility("default")))
	#define ESSE_LIBRARY_EXTENSION U"so"
#else
	#define ESSE_EXPORT_API
	#define ESSE_LIBRARY_EXTENSION U""
#endif

namespace ESSE
{
	namespace IO
	{
		handle LoadLibrary(const string & path) noexcept;
		void ReleaseLibrary(handle library) noexcept;
		void * GetLibraryRoutine(handle library, const char * routine_name) noexcept;
	}
}