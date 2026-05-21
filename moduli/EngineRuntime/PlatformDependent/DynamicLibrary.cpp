#include "../Interfaces/DynamicLibrary.h"

namespace Engine
{
	handle LoadLibrary(const string & path) { return ESSE::IO::LoadLibrary(static_cast<const ESSE::unichar32 *>(path)); }
	void ReleaseLibrary(handle library) { ESSE::IO::ReleaseLibrary(library); }
	void * GetLibraryRoutine(handle library, const char * routine_name) { return ESSE::IO::GetLibraryRoutine(library, routine_name); }
}