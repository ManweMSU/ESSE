#include "CUPSDL.h"
#include <dlfcn.h>

namespace ESSE
{
	namespace CUPS
	{
		CUPSAPI::CUPSAPI(void)
		{
			_library = dlopen("libcups.so", RTLD_NOW);
			if (!_library) throw NotImplementedException();
			try {
				DEFINE_FUNCTION_IMPORT(cupsEnumDests)
				DEFINE_FUNCTION_IMPORT(cupsCopyDest)
				DEFINE_FUNCTION_IMPORT(cupsFreeDests)
				DEFINE_FUNCTION_IMPORT(cupsCopyDestInfo)
				DEFINE_FUNCTION_IMPORT(cupsFreeDestInfo)
				DEFINE_FUNCTION_IMPORT(cupsAddOption)
				DEFINE_FUNCTION_IMPORT(cupsFreeOptions)
				DEFINE_FUNCTION_IMPORT(cupsFindDestDefault)
				DEFINE_FUNCTION_IMPORT(cupsGetOption)
				DEFINE_FUNCTION_IMPORT(ippGetCount)
				DEFINE_FUNCTION_IMPORT(ippGetInteger)
				DEFINE_FUNCTION_IMPORT(ippGetString)
				DEFINE_FUNCTION_IMPORT(ippGetResolution)
				DEFINE_FUNCTION_IMPORT(ippGetBoolean)
				DEFINE_FUNCTION_IMPORT(cupsGetDestMediaByIndex)
				DEFINE_FUNCTION_IMPORT(cupsGetDestMediaCount)
				DEFINE_FUNCTION_IMPORT(cupsGetDestMediaDefault)
				DEFINE_FUNCTION_IMPORT(cupsCheckDestSupported)
				DEFINE_FUNCTION_IMPORT(cupsCreateDestJob)
				DEFINE_FUNCTION_IMPORT(cupsCancelDestJob)
				DEFINE_FUNCTION_IMPORT(cupsStartDestDocument)
				DEFINE_FUNCTION_IMPORT(cupsWriteRequestData)
				DEFINE_FUNCTION_IMPORT(cupsFinishDestDocument)
			} catch (...) { dlclose(_library); throw; }
		}
		CUPSAPI::~CUPSAPI(void) { dlclose(_library); }
	}
}