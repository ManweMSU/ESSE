#include <Auxilia/Auxilia.h>

namespace ESSE
{
	extern unsigned int __rescnt;
	extern const char ** __resnames;
	extern const char ** __reslocales;
	extern const void ** __resdata;
	extern unsigned int * __reslengths;

	bool QueryResource(const void ** pmem, uintptr * plength, const char * resname, const char * locale) noexcept
	{
		for (uint i = 0; i < __rescnt; i++) {
			if (Memory::StringCompare(__resnames[i], resname) == 0 && Memory::StringCompare(__reslocales[i], locale) == 0) {
				*pmem = __resdata[i];
				*plength = __reslengths[i];
				return true;
			}
		}
		return false;
	}
}