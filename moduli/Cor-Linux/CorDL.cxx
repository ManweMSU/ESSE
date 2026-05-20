#include <Cor/IO/CorDL.h>
#include "CorIOEx.h"

#include <dlfcn.h>

namespace ESSE
{
	namespace IO
	{
		handle LoadLibrary(const string & path) noexcept
		{
			try {
				ErrorContext ectx; ErrorClear(ectx);
				ucs1_string path_utf8(ExpandPath(path, ectx));
				if (ErrorTest(ectx)) return 0;
				return dlopen(path_utf8, RTLD_NOW);
			} catch (...) { return 0; }
		}
		void ReleaseLibrary(handle library) noexcept { dlclose(library); }
		void * GetLibraryRoutine(handle library, const char * routine_name) noexcept { return dlsym(library, routine_name); }
	}
}