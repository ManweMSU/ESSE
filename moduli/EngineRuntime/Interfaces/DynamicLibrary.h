#pragma once

#include "../EngineBase.h"

#define ENGINE_EXPORT_API ESSE_EXPORT_API
#define ENGINE_LIBRARY_EXTENSION ESSE_LIBRARY_EXTENSION

namespace Engine
{
	handle LoadLibrary(const string & path);
	void ReleaseLibrary(handle library);
	void * GetLibraryRoutine(handle library, const char * routine_name);
}